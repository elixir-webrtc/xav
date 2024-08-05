#include "xav_reader.h"
#include "video_converter.h"

static int init_converter(struct XavReader *xav_reader);
ErlNifResourceType *xav_reader_resource_type;

ERL_NIF_TERM new(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 3) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[0], &bin)) {
    return xav_nif_raise(env, "invalid_path");
  }

  int device_flag;
  if (!enif_get_int(env, argv[1], &device_flag)) {
    return xav_nif_raise(env, "invalid_device_flag");
  }

  int media_type_flag;
  enum AVMediaType media_type;
  if (!enif_get_int(env, argv[2], &media_type_flag)) {
    return xav_nif_raise(env, "invalid_media_type_flag");
  }

  if (media_type_flag == 1) {
    media_type = AVMEDIA_TYPE_VIDEO;
  } else {
    media_type = AVMEDIA_TYPE_AUDIO;
  }

  struct XavReader *xav_reader =
      enif_alloc_resource(xav_reader_resource_type, sizeof(struct XavReader));
  xav_reader->reader = NULL;
  xav_reader->ac = NULL;

  xav_reader->reader = reader_alloc();
  if (xav_reader->reader == NULL) {
    return xav_nif_raise(env, "couldnt_allocate_reader");
  }

  int ret = reader_init(xav_reader->reader, bin.data, bin.size, device_flag, media_type);

  if (ret == -1) {
    return xav_nif_error(env, "couldnt_open_avformat_input");
  } else if (ret == -2) {
    return xav_nif_raise(env, "couldnt_create_new_reader");
  }

  ERL_NIF_TERM in_format_term = enif_make_atom(env, xav_reader->reader->in_format_name);
  ERL_NIF_TERM out_format_term = enif_make_atom(env, xav_reader->reader->out_format_name);

  ERL_NIF_TERM sample_rate_term;
  if (xav_reader->reader->media_type == AVMEDIA_TYPE_AUDIO) {
    sample_rate_term = enif_make_int(env, xav_reader->reader->c->sample_rate);
  }

  ERL_NIF_TERM bit_rate_term = enif_make_int64(env, xav_reader->reader->fmt_ctx->bit_rate);
  ERL_NIF_TERM duration_term =
      enif_make_int64(env, xav_reader->reader->fmt_ctx->duration / AV_TIME_BASE);
  ERL_NIF_TERM codec_term = enif_make_atom(env, xav_reader->reader->codec->name);

  ERL_NIF_TERM xav_term = enif_make_resource(env, xav_reader);
  enif_release_resource(xav_reader);

  ERL_NIF_TERM ok_term = enif_make_atom(env, "ok");

  if (xav_reader->reader->media_type == AVMEDIA_TYPE_AUDIO) {
    return enif_make_tuple(env, 8, ok_term, xav_term, in_format_term, out_format_term,
                           sample_rate_term, bit_rate_term, duration_term, codec_term);
  }

  return enif_make_tuple(env, 7, ok_term, xav_term, in_format_term, out_format_term, bit_rate_term,
                         duration_term, codec_term);
}

ERL_NIF_TERM next_frame(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM frame_term;

  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavReader *xav_reader;
  if (!enif_get_resource(env, argv[0], xav_reader_resource_type, (void **)&xav_reader)) {
    return xav_nif_raise(env, "couldnt_get_reader_resource");
  }

  int ret = reader_next_frame(xav_reader->reader);

  if (ret == AVERROR_EOF) {
    return xav_nif_error(env, "eof");
  } else if (ret != 0) {
    return xav_nif_raise(env, "receive_frame");
  }

  // convert
  if (xav_reader->reader->media_type == AVMEDIA_TYPE_VIDEO) {
    XAV_LOG_DEBUG("Converting video to RGB");

    uint8_t *out_data[4];
    int out_linesize[4];

    ret = video_converter_convert(xav_reader->reader->frame, out_data, out_linesize);
    if (ret <= 0) {
      return xav_nif_raise(env, "failed_to_read");
    }

    frame_term =
        xav_nif_video_frame_to_term(env, xav_reader->reader->frame, out_data, out_linesize, "rgb");

    av_freep(&out_data[0]);
  } else if (xav_reader->reader->media_type == AVMEDIA_TYPE_AUDIO) {
    XAV_LOG_DEBUG("Converting audio to desired out format");

    uint8_t **out_data;
    int out_samples;
    int out_size;

    if (xav_reader->ac == NULL) {
      XAV_LOG_DEBUG("Converter not initialized. Initializing.");
      ret = init_converter(xav_reader);
      if (ret < 0) {
        return ret;
      }
    }

    ret = audio_converter_convert(xav_reader->ac, xav_reader->reader->frame, &out_data,
                                  &out_samples, &out_size);
    if (ret < 0) {
      return xav_nif_raise(env, "failed_to_read");
    }

    const char *out_format = av_get_sample_fmt_name(xav_reader->ac->out_sample_fmt);

    frame_term = xav_nif_audio_frame_to_term(env, out_data, out_samples, out_size, out_format,
                                             xav_reader->reader->frame->pts);
    av_freep(&out_data[0]);
  }

  reader_free_frame(xav_reader->reader);

  return xav_nif_ok(env, frame_term);
}

static int init_converter(struct XavReader *xav_reader) {
  xav_reader->ac = audio_converter_alloc();

  if (xav_reader->ac == NULL) {
    XAV_LOG_DEBUG("Couldn't allocate converter");
    return -1;
  }

  int out_sample_rate = 16000;
  enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLT;

  struct ChannelLayout in_chlayout, out_chlayout;
#if LIBAVUTIL_VERSION_MAJOR >= 58
  in_chlayout.layout = xav_reader->reader->c->ch_layout;
  av_channel_layout_from_mask(&out_chlayout.layout, AV_CH_LAYOUT_MONO);
#else
  in_chlayout.layout = xav_reader->reader->frame->channel_layout;
  out_chlayout.layout = AV_CH_LAYOUT_MONO;

  if (xav_reader->reader->frame->channel_layout == 0 && xav_reader->reader->frame->channels > 0) {
    // In newer FFmpeg versions, 0 means that the order of channels is
    // unspecified but there still might be information about channels number.
    // Let's check againts it and take default channel order for the given channels number.
    // This is also what newer FFmpeg versions do under the hood when passing
    // unspecified channel order.
    XAV_LOG_DEBUG("Channel layout unset. Setting to default for channels number: %d",
                  xav_reader->reader->frame->channels);
    in_chlayout.layout = av_get_default_channel_layout(xav_reader->reader->frame->channels);
  } else if (xav_reader->reader->frame->channel_layout == 0) {
    XAV_LOG_DEBUG("Both channel layout and channels are unset. Cannot init converter.");
    return -1;
  }

#endif

  return audio_converter_init(xav_reader->ac, in_chlayout, xav_reader->reader->c->sample_rate,
                              xav_reader->reader->c->sample_fmt, out_chlayout, out_sample_rate,
                              out_sample_fmt);
}

void free_xav_reader(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing XavReader object");
  struct XavReader *xav_reader = (struct XavReader *)obj;
  if (xav_reader->reader != NULL) {
    reader_free(&xav_reader->reader);
  }

  if (xav_reader->ac != NULL) {
    audio_converter_free(&xav_reader->ac);
  }
}

static ErlNifFunc xav_funcs[] = {{"new", 3, new},
                                 {"next_frame", 1, next_frame, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {

  xav_reader_resource_type =
      enif_open_resource_type(env, NULL, "XavReader", free_xav_reader, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.Reader.NIF, xav_funcs, &load, NULL, NULL, NULL);
