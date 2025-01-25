#include "xav_encoder.h"

ErlNifResourceType *xav_encoder_resource_type;

static ERL_NIF_TERM packets_to_term(ErlNifEnv *, struct Encoder *);
static int get_profile(enum AVCodecID, const char *);

ERL_NIF_TERM new (ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 2) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ERL_NIF_TERM ret;
  struct EncoderConfig encoder_config = {0};
  encoder_config.max_b_frames = -1;
  encoder_config.profile = FF_PROFILE_UNKNOWN;

  char *codec = NULL, *format = NULL, *profile = NULL;

  ErlNifMapIterator iter;
  ERL_NIF_TERM key, value;
  char *config_name = NULL;
  int err;

  if (!xav_nif_get_atom(env, argv[0], &codec)) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  if (!enif_is_map(env, argv[1])) {
    return xav_nif_raise(env, "failed_to_get_map");
  }

  if (strcmp(codec, "h264") == 0) {
    encoder_config.media_type = AVMEDIA_TYPE_VIDEO;
    encoder_config.codec = AV_CODEC_ID_H264;
  } else if (strcmp(codec, "h265") == 0 || strcmp(codec, "hevc") == 0) {
    encoder_config.media_type = AVMEDIA_TYPE_VIDEO;
    encoder_config.codec = AV_CODEC_ID_HEVC;
  } else {
    ret = xav_nif_raise(env, "failed_to_resolve_codec");
    goto clean;
  }

  enif_map_iterator_create(env, argv[1], &iter, ERL_NIF_MAP_ITERATOR_FIRST);

  while (enif_map_iterator_get_pair(env, &iter, &key, &value)) {
    if (!xav_nif_get_atom(env, key, &config_name)) {
      ret = xav_nif_raise(env, "failed_to_get_map_key");
      goto clean;
    }

    if (strcmp(config_name, "width") == 0) {
      err = enif_get_int(env, value, &encoder_config.width);
    } else if (strcmp(config_name, "height") == 0) {
      err = enif_get_int(env, value, &encoder_config.height);
    } else if (strcmp(config_name, "format") == 0) {
      err = xav_nif_get_atom(env, value, &format);
    } else if (strcmp(config_name, "time_base_num") == 0) {
      err = enif_get_int(env, value, &encoder_config.time_base.num);
    } else if (strcmp(config_name, "time_base_den") == 0) {
      err = enif_get_int(env, value, &encoder_config.time_base.den);
    } else if (strcmp(config_name, "gop_size") == 0) {
      err = enif_get_int(env, value, &encoder_config.gop_size);
    } else if (strcmp(config_name, "max_b_frames") == 0) {
      err = enif_get_int(env, value, &encoder_config.max_b_frames);
    } else if (strcmp(config_name, "profile") == 0) {
      err = xav_nif_get_atom(env, value, &profile);
    } else {
      ret = xav_nif_raise(env, "unknown_config_key");
      goto clean;
    }

    if (!err) {
      ret = xav_nif_raise(env, "couldnt_read_value");
      goto clean;
    }

    XAV_FREE(config_name);
    enif_map_iterator_next(env, &iter);
  }

  encoder_config.format = av_get_pix_fmt(format);
  if (encoder_config.format == AV_PIX_FMT_NONE) {
    ret = xav_nif_raise(env, "unknown_format");
    goto clean;
  }

  if (profile) {
    encoder_config.profile = get_profile(encoder_config.codec, profile);
    if (encoder_config.profile == FF_PROFILE_UNKNOWN) {
      ret = xav_nif_raise(env, "invalid_profile");
      goto clean;
    }
  }

  struct XavEncoder *xav_encoder =
      enif_alloc_resource(xav_encoder_resource_type, sizeof(struct XavEncoder));

  xav_encoder->frame = av_frame_alloc();
  xav_encoder->encoder = encoder_alloc();
  if (encoder_init(xav_encoder->encoder, &encoder_config) < 0) {
    ret = xav_nif_raise(env, "failed_to_init_encoder");
    goto clean;
  }

  ret = enif_make_resource(env, xav_encoder);
  enif_release_resource(xav_encoder);

clean:
  if (!codec)
    XAV_FREE(codec);
  if (!format)
    XAV_FREE(format);
  if (!config_name)
    XAV_FREE(config_name);
  if (!profile)
    XAV_FREE(profile);
  enif_map_iterator_destroy(env, &iter);

  return ret;
}

ERL_NIF_TERM encode(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 3) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavEncoder *xav_encoder;
  if (!enif_get_resource(env, argv[0], xav_encoder_resource_type, (void **)&xav_encoder)) {
    return xav_nif_raise(env, "invalid_resource");
  }

  ErlNifBinary input;
  if (!enif_inspect_binary(env, argv[1], &input)) {
    return xav_nif_raise(env, "failed_to_inspect_binary");
  }

  int pts;
  if (!enif_get_int(env, argv[2], &pts)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  AVFrame *frame = xav_encoder->frame;
  frame->width = xav_encoder->encoder->c->width;
  frame->height = xav_encoder->encoder->c->height;
  frame->format = xav_encoder->encoder->c->pix_fmt;
  frame->pts = pts;

  int ret = av_image_fill_arrays(frame->data, frame->linesize, input.data, frame->format,
                                 frame->width, frame->height, 1);
  if (ret < 0) {
    return xav_nif_raise(env, "failed_to_fill_arrays");
  }

  ret = encoder_encode(xav_encoder->encoder, frame);
  if (ret < 0) {
    return xav_nif_raise(env, "failed_to_encode");
  }

  return packets_to_term(env, xav_encoder->encoder);
}

ERL_NIF_TERM flush(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavEncoder *xav_encoder;
  if (!enif_get_resource(env, argv[0], xav_encoder_resource_type, (void **)&xav_encoder)) {
    return xav_nif_raise(env, "invalid_resource");
  }

  int ret = encoder_encode(xav_encoder->encoder, NULL);
  if (ret < 0) {
    return xav_nif_raise(env, "failed_to_encode");
  }

  return packets_to_term(env, xav_encoder->encoder);
}

void free_xav_encoder(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing XavEncoder object");
  struct XavEncoder *xav_encoder = (struct XavEncoder *)obj;

  if (xav_encoder->encoder != NULL) {
    encoder_free(&xav_encoder->encoder);
  }

  if (xav_encoder->frame != NULL) {
    av_frame_free(&xav_encoder->frame);
  }
}

static ERL_NIF_TERM packets_to_term(ErlNifEnv *env, struct Encoder *encoder) {
  ERL_NIF_TERM ret;
  ERL_NIF_TERM *packets = XAV_ALLOC(sizeof(ERL_NIF_TERM) * encoder->num_packets);
  for (int i = 0; i < encoder->num_packets; i++) {
    packets[i] = xav_nif_packet_to_term(env, encoder->packets[i]);
  }

  ret = enif_make_list_from_array(env, packets, encoder->num_packets);

  for (int i = 0; i < encoder->num_packets; i++)
    av_packet_unref(encoder->packets[i]);
  XAV_FREE(packets);

  return ret;
}

static int get_profile(enum AVCodecID codec, const char *profile_name) {
  if (codec == AV_CODEC_ID_H264) {
    if (strcmp(profile_name, "constrained_baseline") == 0) {
      return FF_PROFILE_H264_CONSTRAINED_BASELINE;
    } else if (strcmp(profile_name, "baseline") == 0) {
      return FF_PROFILE_H264_BASELINE;
    } else if (strcmp(profile_name, "main") == 0) {
      return FF_PROFILE_H264_MAIN;
    } else if (strcmp(profile_name, "high") == 0) {
      return FF_PROFILE_H264_HIGH;
    }
  }

  if (codec == AV_CODEC_ID_HEVC) {
    if (strcmp(profile_name, "main") == 0) {
      return FF_PROFILE_HEVC_MAIN;
    } else if (strcmp(profile_name, "main_10") == 0) {
      return FF_PROFILE_HEVC_MAIN_10;
    } else if (strcmp(profile_name, "main_still_picture") == 0) {
      return FF_PROFILE_HEVC_MAIN_STILL_PICTURE;
    }
  }

  return FF_PROFILE_UNKNOWN;
}

static ErlNifFunc xav_funcs[] = {{"new", 2, new}, {"encode", 3, encode}, {"flush", 1, flush}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  xav_encoder_resource_type =
      enif_open_resource_type(env, NULL, "XavEncoder", free_xav_encoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.Encoder.NIF, xav_funcs, &load, NULL, NULL, NULL);