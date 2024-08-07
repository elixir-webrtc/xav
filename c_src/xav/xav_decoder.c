#include "xav_decoder.h"
#include "audio_converter.h"
#include "video_converter.h"

ErlNifResourceType *xav_decoder_resource_type;

static int init_audio_converter(struct XavDecoder *xav_decoder);

ERL_NIF_TERM new(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 4) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  unsigned int codec_len;
  if (!enif_get_atom_length(env, argv[0], &codec_len, ERL_NIF_LATIN1)) {
    return xav_nif_raise(env, "failed_to_get_atom_length");
  }

  char *codec = (char *)XAV_ALLOC((codec_len + 1) * sizeof(char *));

  if (enif_get_atom(env, argv[0], codec, codec_len + 1, ERL_NIF_LATIN1) == 0) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  unsigned int out_format_len;
  if (!enif_get_atom_length(env, argv[1], &out_format_len, ERL_NIF_LATIN1)) {
    return xav_nif_raise(env, "failed_to_get_atom_length");
  }

  char *out_format = (char *)XAV_ALLOC((out_format_len + 1) * sizeof(char *));

  if (enif_get_atom(env, argv[1], out_format, out_format_len + 1, ERL_NIF_LATIN1) == 0) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  int out_sample_rate;
  if (!enif_get_int(env, argv[2], &out_sample_rate)) {
    return xav_nif_raise(env, "invalid_out_sample_rate");
  }

  int out_channels;
  if (!enif_get_int(env, argv[3], &out_channels)) {
    return xav_nif_raise(env, "invalid_out_channels");
  }

  struct XavDecoder *xav_decoder =
      enif_alloc_resource(xav_decoder_resource_type, sizeof(struct XavDecoder));
  xav_decoder->decoder = NULL;
  xav_decoder->ac = NULL;
  xav_decoder->out_format = out_format;
  xav_decoder->out_sample_rate = out_sample_rate;
  xav_decoder->out_channels = out_channels;

  xav_decoder->decoder = decoder_alloc();
  if (xav_decoder->decoder == NULL) {
    return xav_nif_raise(env, "failed_to_allocate_decoder");
  }

  if (decoder_init(xav_decoder->decoder, codec) != 0) {
    return xav_nif_raise(env, "failed_to_init_decoder");
  }

  ERL_NIF_TERM decoder_term = enif_make_resource(env, xav_decoder);
  enif_release_resource(xav_decoder);

  return decoder_term;
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
  if (!enif_get_int(env, argv[3], &pts)) {
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
  } else if (ret != 0) {
    return xav_nif_raise(env, "failed_to_decode");
  }

  // convert
  if (xav_decoder->decoder->media_type == AVMEDIA_TYPE_VIDEO) {
    XAV_LOG_DEBUG("Converting video to RGB");

    uint8_t *out_data[4];
    int out_linesize[4];

    ret = video_converter_convert(xav_decoder->decoder->frame, out_data, out_linesize);
    if (ret <= 0) {
      return xav_nif_raise(env, "failed_to_decode");
    }

    frame_term = xav_nif_video_frame_to_term(env, xav_decoder->decoder->frame, out_data,
                                             out_linesize, "rgb");

    av_freep(&out_data[0]);
  } else if (xav_decoder->decoder->media_type == AVMEDIA_TYPE_AUDIO) {
    XAV_LOG_DEBUG("Converting audio to desired out format");

    uint8_t **out_data;
    int out_samples;
    int out_size;

    if (xav_decoder->ac == NULL) {
      ret = init_audio_converter(xav_decoder);
      if (ret < 0) {
        return ret;
      }
    }

    ret = audio_converter_convert(xav_decoder->ac, xav_decoder->decoder->frame, &out_data,
                                  &out_samples, &out_size);
    if (ret < 0) {
      return xav_nif_raise(env, "failed_to_decode");
    }

    const char *out_format = av_get_sample_fmt_name(xav_decoder->ac->out_sample_fmt);

    if (strcmp(out_format, "flt") == 0) {
      out_format = "f32";
    } else if (strcmp(out_format, "dbl") == 0) {
      out_format = "f64";
    }

    frame_term = xav_nif_audio_frame_to_term(env, out_data, out_samples, out_size, out_format,
                                             xav_decoder->decoder->frame->pts);

    av_freep(&out_data[0]);
  }

  decoder_free_frame(xav_decoder->decoder);

  return xav_nif_ok(env, frame_term);
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

  enum AVSampleFormat out_sample_fmt;
  if (strcmp(xav_decoder->out_format, "u8") == 0) {
    out_sample_fmt = AV_SAMPLE_FMT_U8;
  } else if (strcmp(xav_decoder->out_format, "s16") == 0) {
    out_sample_fmt = AV_SAMPLE_FMT_S16;
  } else if (strcmp(xav_decoder->out_format, "s32") == 0) {
    out_sample_fmt = AV_SAMPLE_FMT_S32;
  } else if (strcmp(xav_decoder->out_format, "s64") == 0) {
    out_sample_fmt = AV_SAMPLE_FMT_S64;
  } else if (strcmp(xav_decoder->out_format, "f32") == 0) {
    out_sample_fmt = AV_SAMPLE_FMT_FLT;
  } else if (strcmp(xav_decoder->out_format, "f64") == 0) {
    out_sample_fmt = AV_SAMPLE_FMT_DBL;
  } else if (strcmp(xav_decoder->out_format, "nil") == 0) {
    out_sample_fmt = av_get_alt_sample_fmt(xav_decoder->decoder->c->sample_fmt, 0);
  } else {
    return -1;
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
                              out_sample_fmt);
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
}

static ErlNifFunc xav_funcs[] = {{"new", 4, new},
                                 {"decode", 4, decode, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  xav_decoder_resource_type =
      enif_open_resource_type(env, NULL, "XavDecoder", free_xav_decoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.Decoder.NIF, xav_funcs, &load, NULL, NULL, NULL);
