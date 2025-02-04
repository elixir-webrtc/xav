#include "xav_decoder.h"
#include "audio_converter.h"

ErlNifResourceType *xav_decoder_resource_type;

static int init_audio_converter(struct XavDecoder *xav_decoder);
static int init_video_converter(struct XavDecoder *xav_decoder, AVFrame *frame);

void free_frames(AVFrame **frames, int size) {
  for (int i = 0; i < size; i++) {
    av_frame_unref(frames[i]);
  }
}

ERL_NIF_TERM new (ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 6) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ERL_NIF_TERM ret;
  char *codec_name = NULL;
  char *out_format = NULL;

  // resolve codec
  if (!xav_nif_get_atom(env, argv[0], &codec_name)) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  const AVCodec *codec = avcodec_find_decoder_by_name(codec_name);
  if (codec == NULL) {
    ret = xav_nif_raise(env, "unknown_codec");
    goto clean;
  }

  if (codec->type != AVMEDIA_TYPE_VIDEO && codec->type != AVMEDIA_TYPE_AUDIO) {
    ret = xav_nif_raise(env, "unsupported_media_type");
    goto clean;
  }

  // resolve output format
  if (!xav_nif_get_atom(env, argv[1], &out_format)) {
    ret = xav_nif_raise(env, "failed_to_get_atom");
    goto clean;
  }

  enum AVPixelFormat out_video_fmt = AV_PIX_FMT_NONE;
  enum AVSampleFormat out_audo_fmt = AV_SAMPLE_FMT_NONE;
  if (codec->type == AVMEDIA_TYPE_VIDEO && strcmp(out_format, "nil") != 0) {
    out_video_fmt = av_get_pix_fmt(out_format);
    if (out_video_fmt == AV_PIX_FMT_NONE) {
      ret = xav_nif_raise(env, "unknown_out_format");
      goto clean;
    }
  } else if (codec->type == AVMEDIA_TYPE_AUDIO && strcmp(out_format, "nil") != 0) {
    out_audo_fmt = av_get_sample_fmt(out_format);
    if (out_audo_fmt == AV_SAMPLE_FMT_NONE) {
      ret = xav_nif_raise(env, "unknown_out_format");
      goto clean;
    }
  }

  // resolve other params
  int out_sample_rate;
  if (!enif_get_int(env, argv[2], &out_sample_rate)) {
    ret = xav_nif_raise(env, "invalid_out_sample_rate");
    goto clean;
  }

  int out_channels;
  if (!enif_get_int(env, argv[3], &out_channels)) {
    ret = xav_nif_raise(env, "invalid_out_channels");
    goto clean;
  }

  int out_width;
  if (!enif_get_int(env, argv[4], &out_width)) {
    ret = xav_nif_raise(env, "failed_to_get_int");
    goto clean;
  }

  int out_height;
  if (!enif_get_int(env, argv[5], &out_height)) {
    ret = xav_nif_raise(env, "failed_to_get_int");
    goto clean;
  }

  struct XavDecoder *xav_decoder =
      enif_alloc_resource(xav_decoder_resource_type, sizeof(struct XavDecoder));
  xav_decoder->decoder = NULL;
  xav_decoder->ac = NULL;
  xav_decoder->vc = NULL;
  xav_decoder->out_video_fmt = out_video_fmt;
  xav_decoder->out_width = out_width;
  xav_decoder->out_height = out_height;
  xav_decoder->out_audio_fmt = out_audo_fmt;
  xav_decoder->out_sample_rate = out_sample_rate;
  xav_decoder->out_channels = out_channels;

  xav_decoder->decoder = decoder_alloc();
  if (xav_decoder->decoder == NULL) {
    ret = xav_nif_raise(env, "failed_to_allocate_decoder");
    goto clean;
  }

  if (decoder_init(xav_decoder->decoder, codec) != 0) {
    ret = xav_nif_raise(env, "failed_to_init_decoder");
    goto clean;
  }

  ret = enif_make_resource(env, xav_decoder);
  enif_release_resource(xav_decoder);

clean:
  if (codec_name != NULL)
    XAV_FREE(codec_name);
  if (out_format != NULL)
    XAV_FREE(out_format);

  return ret;
}

ERL_NIF_TERM convert(ErlNifEnv *env, struct XavDecoder *xav_decoder, AVFrame *frame) {
  ERL_NIF_TERM frame_term;
  int ret;

  if (xav_decoder->decoder->media_type == AVMEDIA_TYPE_VIDEO) {
    XAV_LOG_DEBUG("Converting video to RGB");

    // no pixel format conversion and no scaling
    if (xav_decoder->out_video_fmt == AV_PIX_FMT_NONE && xav_decoder->out_width == -1 &&
        xav_decoder->out_height == -1) {
      return xav_nif_video_frame_to_term(env, frame);
    }

    if (xav_decoder->vc == NULL) {
      ret = init_video_converter(xav_decoder, frame);
      if (ret < 0) {
        return xav_nif_raise(env, "failed_to_init_converter");
      }
    }

    ret = video_converter_convert(xav_decoder->vc, frame);
    if (ret < 0) {
      return xav_nif_raise(env, "failed_to_convert");
    }

    frame_term = xav_nif_video_frame_to_term(env, xav_decoder->vc->dst_frame);

  } else if (xav_decoder->decoder->media_type == AVMEDIA_TYPE_AUDIO) {
    XAV_LOG_DEBUG("Converting audio to desired out format");

    uint8_t **out_data;
    int out_samples;
    int out_size;

    if (xav_decoder->ac == NULL) {
      ret = init_audio_converter(xav_decoder);
      if (ret < 0) {
        return xav_nif_raise(env, "failed_to_init_converter");
      }
    }

    ret = audio_converter_convert(xav_decoder->ac, frame, &out_data, &out_samples, &out_size);
    if (ret < 0) {
      return xav_nif_raise(env, "failed_to_decode");
    }

    frame_term = xav_nif_audio_frame_to_term(env, out_data, out_samples, out_size,
                                             xav_decoder->out_audio_fmt, frame->pts);

    av_freep(&out_data[0]);
  }

  return frame_term;
}

ERL_NIF_TERM decode(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM frame_term;

  if (argc != 4) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavDecoder *xav_decoder;
  if (!enif_get_resource(env, argv[0], xav_decoder_resource_type, (void **)&xav_decoder)) {
    return xav_nif_raise(env, "couldnt_get_decoder_resource");
  }

  ErlNifBinary data;
  if (!enif_inspect_binary(env, argv[1], &data)) {
    return xav_nif_raise(env, "couldnt_inspect_binary");
  }

  int pts;
  if (!enif_get_int(env, argv[2], &pts)) {
    return xav_nif_raise(env, "couldnt_get_int");
  }

  int dts;
  if (!enif_get_int(env, argv[3], &dts)) {
    return xav_nif_raise(env, "couldnt_get_int");
  }

  xav_decoder->decoder->pkt->data = data.data;
  xav_decoder->decoder->pkt->size = data.size;
  xav_decoder->decoder->pkt->pts = pts;
  xav_decoder->decoder->pkt->dts = dts;

  int ret =
      decoder_decode(xav_decoder->decoder, xav_decoder->decoder->pkt, xav_decoder->decoder->frame);
  if (ret == -2) {
    return xav_nif_error(env, "no_keyframe");
  } else if (ret == AVERROR(EAGAIN)) {
    // Some frames are meant for decoder only
    // and they don't include actual video samples.
    decoder_free_frame(xav_decoder->decoder);
    return enif_make_atom(env, "ok");
  } else if (ret != 0) {
    return xav_nif_raise(env, "failed_to_decode");
  }

  frame_term = convert(env, xav_decoder, xav_decoder->decoder->frame);

  decoder_free_frame(xav_decoder->decoder);

  return xav_nif_ok(env, frame_term);
}

ERL_NIF_TERM flush(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavDecoder *xav_decoder;
  if (!enif_get_resource(env, argv[0], xav_decoder_resource_type, (void **)&xav_decoder)) {
    return xav_nif_raise(env, "couldnt_get_decoder_resource");
  }

  AVFrame *frames[MAX_FLUSH_BUFFER];
  int frames_count = 0;

  for (int i = 0; i < MAX_FLUSH_BUFFER; i++) {
    frames[i] = av_frame_alloc();
  }

  int ret = decoder_flush(xav_decoder->decoder, frames, &frames_count);
  if (ret < 0) {
    free_frames(frames, MAX_FLUSH_BUFFER);
    return xav_nif_error(env, "failed_to_flush");
  }

  ERL_NIF_TERM frame_terms[frames_count];
  for (int i = 0; i < frames_count; i++) {
    frame_terms[i] = convert(env, xav_decoder, frames[i]);
  }

  free_frames(frames, MAX_FLUSH_BUFFER);

  return xav_nif_ok(env, enif_make_list_from_array(env, frame_terms, frames_count));
}

ERL_NIF_TERM pixel_formats(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  const AVPixFmtDescriptor *desc = NULL;

  while ((desc = av_pix_fmt_desc_next(desc))) {
    ERL_NIF_TERM name = enif_make_atom(env, desc->name);
    ERL_NIF_TERM nb_components = enif_make_int(env, desc->nb_components);
    ERL_NIF_TERM is_hwaccel =
        enif_make_atom(env, desc->flags & AV_PIX_FMT_FLAG_HWACCEL ? "true" : "false");

    result =
        enif_make_list_cell(env, enif_make_tuple3(env, name, nb_components, is_hwaccel), result);
  }

  return result;
}

ERL_NIF_TERM sample_formats(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  for (int fmt = 0; fmt < AV_SAMPLE_FMT_NB; fmt++) {
    enum AVSampleFormat sample_format = (enum AVSampleFormat)fmt;
    const char *name = av_get_sample_fmt_name(sample_format);
    int nb_bytes = av_get_bytes_per_sample(sample_format);

    ERL_NIF_TERM desc =
        enif_make_tuple2(env, enif_make_atom(env, name), enif_make_int(env, nb_bytes));

    result = enif_make_list_cell(env, desc, result);
  }

  return result;
}

ERL_NIF_TERM list_decoders(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  const AVCodec *codec = NULL;
  void *iter = NULL;

  while ((codec = av_codec_iterate(&iter))) {
    if (av_codec_is_decoder(codec)) {
      ERL_NIF_TERM name = enif_make_atom(env, codec->name);
      ERL_NIF_TERM long_name = enif_make_string(env, codec->long_name, ERL_NIF_LATIN1);
      ERL_NIF_TERM media_type = enif_make_atom(env, av_get_media_type_string(codec->type));

      ERL_NIF_TERM desc = enif_make_tuple3(env, name, long_name, media_type);
      result = enif_make_list_cell(env, desc, result);
    }
  }

  return result;
}

static int init_audio_converter(struct XavDecoder *xav_decoder) {
  xav_decoder->ac = audio_converter_alloc();

  if (xav_decoder->ac == NULL) {
    XAV_LOG_DEBUG("Couldn't allocate converter");
    return -1;
  }

  int out_sample_rate;
  if (xav_decoder->out_sample_rate == 0) {
    out_sample_rate = xav_decoder->decoder->c->sample_rate;
  } else {
    out_sample_rate = xav_decoder->out_sample_rate;
  }

  // If user didn't request any specific format,
  // just take the original format but in the packed form.
  // We need to call this function here, as in the decoder_init we don't know
  // what is the sample_fmt yet.
  if (xav_decoder->out_audio_fmt == AV_SAMPLE_FMT_NONE) {
    xav_decoder->out_audio_fmt = av_get_alt_sample_fmt(xav_decoder->decoder->c->sample_fmt, 0);
  }

  struct ChannelLayout in_chlayout, out_chlayout;
#if LIBAVUTIL_VERSION_MAJOR >= 58
  in_chlayout.layout = xav_decoder->decoder->c->ch_layout;
  if (xav_decoder->out_channels == 0) {
    out_chlayout.layout = in_chlayout.layout;
  } else {
    av_channel_layout_default(&out_chlayout.layout, xav_decoder->out_channels);
  }
#else
  in_chlayout.layout = xav_decoder->decoder->c->channel_layout;
  if (xav_decoder->out_channels == 0) {
    out_chlayout.layout = in_chlayout.layout;
  } else {
    out_chlayout.layout = av_get_default_channel_layout(xav_decoder->out_channels);
  }
#endif

  return audio_converter_init(xav_decoder->ac, in_chlayout, xav_decoder->decoder->c->sample_rate,
                              xav_decoder->decoder->c->sample_fmt, out_chlayout, out_sample_rate,
                              xav_decoder->out_audio_fmt);
}

static int init_video_converter(struct XavDecoder *xav_decoder, AVFrame *frame) {
  xav_decoder->vc = video_converter_alloc();
  if (xav_decoder->vc == NULL) {
    XAV_LOG_DEBUG("Couldn't allocate video converter");
    return -1;
  }

  enum AVPixelFormat out_format = xav_decoder->out_video_fmt;
  if (out_format == AV_PIX_FMT_NONE)
    out_format = frame->format;

  return video_converter_init(xav_decoder->vc, frame->width, frame->height, frame->format,
                              xav_decoder->out_width, xav_decoder->out_height, out_format);
}

void free_xav_decoder(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing XavDecoder object");
  struct XavDecoder *xav_decoder = (struct XavDecoder *)obj;
  if (xav_decoder->decoder != NULL) {
    decoder_free(&xav_decoder->decoder);
  }

  if (xav_decoder->ac != NULL) {
    audio_converter_free(&xav_decoder->ac);
  }

  if (xav_decoder->vc != NULL) {
    video_converter_free(&xav_decoder->vc);
  }
}

static ErlNifFunc xav_funcs[] = {{"new", 6, new},
                                 {"decode", 4, decode, ERL_NIF_DIRTY_JOB_CPU_BOUND},
                                 {"flush", 1, flush, ERL_NIF_DIRTY_JOB_CPU_BOUND},
                                 {"pixel_formats", 0, pixel_formats},
                                 {"sample_formats", 0, sample_formats},
                                 {"list_decoders", 0, list_decoders}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  xav_decoder_resource_type =
      enif_open_resource_type(env, NULL, "XavDecoder", free_xav_decoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.Decoder.NIF, xav_funcs, &load, NULL, NULL, NULL);
