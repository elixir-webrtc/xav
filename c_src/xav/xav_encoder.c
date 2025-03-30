#include "xav_encoder.h"

ErlNifResourceType *xav_encoder_resource_type;

static ERL_NIF_TERM packets_to_term(ErlNifEnv *, struct Encoder *);
static int get_profile(enum AVCodecID, const char *);
static ERL_NIF_TERM codec_get_profiles(ErlNifEnv *, const AVCodec *);
static ERL_NIF_TERM codec_get_sample_formats(ErlNifEnv *, const AVCodec *);
static ERL_NIF_TERM codec_get_sample_rates(ErlNifEnv *, const AVCodec *);
static ERL_NIF_TERM codec_get_channel_layouts(ErlNifEnv *, const AVCodec *);

ERL_NIF_TERM new (ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 2) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ERL_NIF_TERM ret;
  struct EncoderConfig encoder_config = {0};
  encoder_config.max_b_frames = -1;
  encoder_config.profile = FF_PROFILE_UNKNOWN;

  char *codec_name = NULL, *format = NULL, *profile = NULL;
  char *channel_layout = NULL;
  int codec_id = 0;

  ErlNifMapIterator iter;
  ERL_NIF_TERM key, value;
  char *config_name = NULL;
  int err;

  if (!xav_nif_get_atom(env, argv[0], &codec_name)) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  if (!enif_is_map(env, argv[1])) {
    return xav_nif_raise(env, "failed_to_get_map");
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
      err = xav_nif_get_string(env, value, &profile);
    } else if (strcmp(config_name, "codec_id") == 0) {
      err = enif_get_int(env, value, &codec_id);
    } else if (strcmp(config_name, "sample_rate") == 0) {
      err = enif_get_int(env, value, &encoder_config.sample_rate);
    } else if (strcmp(config_name, "channel_layout") == 0) {
      err = xav_nif_get_string(env, value, &channel_layout);
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

  if (strcmp(codec_name, "nil") == 0) {
    encoder_config.codec = avcodec_find_encoder((enum AVCodecID)codec_id);
  } else {
    encoder_config.codec = avcodec_find_encoder_by_name(codec_name);
  }

  if (!encoder_config.codec) {
    ret = xav_nif_raise(env, "unknown_codec");
    goto clean;
  }

  if (encoder_config.codec->type == AVMEDIA_TYPE_VIDEO) {
    encoder_config.format = av_get_pix_fmt(format);
    if (encoder_config.format == AV_PIX_FMT_NONE) {
      ret = xav_nif_raise(env, "unknown_format");
      goto clean;
    }
  } else {
    encoder_config.sample_format = av_get_sample_fmt(format);
    if (encoder_config.sample_format == AV_SAMPLE_FMT_NONE) {
      ret = xav_nif_raise(env, "unknown_format");
      goto clean;
    }

    if (!xav_get_channel_layout(channel_layout, &encoder_config.channel_layout)) {
      ret = xav_nif_raise(env, "unknown_channel_layout");
      goto clean;
    }
  }

  if (profile) {
    encoder_config.profile = get_profile(encoder_config.codec->id, profile);
    if (encoder_config.profile == FF_PROFILE_UNKNOWN) {
      ret = xav_nif_raise(env, "invalid_profile");
      goto clean;
    }
  }

  struct XavEncoder *xav_encoder =
      enif_alloc_resource(xav_encoder_resource_type, sizeof(struct XavEncoder));

  xav_encoder->encoder = encoder_alloc();
  if (encoder_init(xav_encoder->encoder, &encoder_config) < 0) {
    ret = xav_nif_raise(env, "failed_to_init_encoder");
    goto clean;
  }

  xav_encoder->frame = av_frame_alloc();

  if (encoder_config.codec->type == AVMEDIA_TYPE_AUDIO) {
    xav_encoder->frame->format = encoder_config.format;
    xav_encoder->frame->nb_samples = xav_encoder->encoder->c->frame_size;
    xav_encoder->frame->channel_layout = xav_encoder->encoder->c->channel_layout;
    if (av_frame_get_buffer(xav_encoder->frame, 0) < 0) {
      ret = xav_nif_raise(env, "failed_to_get_buffer");
      goto clean;
    }
  }

  ret = enif_make_resource(env, xav_encoder);
  enif_release_resource(xav_encoder);

clean:
  if (!codec_name)
    XAV_FREE(codec_name);
  if (!format)
    XAV_FREE(format);
  if (!config_name)
    XAV_FREE(config_name);
  if (!profile)
    XAV_FREE(profile);
  if (!channel_layout)
    XAV_FREE(channel_layout);
  enif_map_iterator_destroy(env, &iter);

  return ret;
}

ERL_NIF_TERM encode(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  int ret;

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
  if (xav_encoder->encoder->codec->type == AVMEDIA_TYPE_VIDEO) {
    frame->width = xav_encoder->encoder->c->width;
    frame->height = xav_encoder->encoder->c->height;
    frame->format = xav_encoder->encoder->c->pix_fmt;
    frame->pts = pts;

    ret = av_image_fill_arrays(frame->data, frame->linesize, input.data, frame->format,
                               frame->width, frame->height, 1);
    if (ret < 0) {
      return xav_nif_raise(env, "failed_to_fill_arrays");
    }
  } else {
    frame->pts = pts;
    ret = av_samples_fill_arrays(frame->data, frame->linesize, input.data, frame->channels,
                                 frame->nb_samples, xav_encoder->encoder->c->sample_fmt, 1);

    if (ret < 0) {
      return xav_nif_raise(env, "failed_to_fill_arrays");
    }
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

ERL_NIF_TERM list_encoders(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  const AVCodec *codec = NULL;
  void *iter = NULL;

  while ((codec = av_codec_iterate(&iter))) {
    if (av_codec_is_encoder(codec)) {
      ERL_NIF_TERM name = enif_make_atom(env, codec->name);
      ERL_NIF_TERM codec_name = enif_make_atom(env, avcodec_get_name(codec->id));
      ERL_NIF_TERM long_name = codec->long_name
                                   ? enif_make_string(env, codec->long_name, ERL_NIF_LATIN1)
                                   : enif_make_string(env, "", ERL_NIF_LATIN1);
      ERL_NIF_TERM media_type = enif_make_atom(env, av_get_media_type_string(codec->type));
      ERL_NIF_TERM codec_id = enif_make_int64(env, codec->id);
      ERL_NIF_TERM profiles = codec_get_profiles(env, codec);
      ERL_NIF_TERM sample_formats = codec_get_sample_formats(env, codec);
      ERL_NIF_TERM sample_rates = codec_get_sample_rates(env, codec);

      ERL_NIF_TERM desc = enif_make_tuple8(env, codec_name, name, long_name, media_type, codec_id,
                                           profiles, sample_formats, sample_rates);
      result = enif_make_list_cell(env, desc, result);
    }
  }

  return result;
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
  const AVCodecDescriptor *desc = avcodec_descriptor_get(codec);
  const AVProfile *profile = desc->profiles;

  if (profile == NULL) {
    return FF_PROFILE_UNKNOWN;
  }

  while (profile->profile != FF_PROFILE_UNKNOWN) {
    if (strcmp(profile->name, profile_name) == 0) {
      break;
    }

    profile++;
  }

  return profile->profile;
}

static ERL_NIF_TERM codec_get_profiles(ErlNifEnv *env, const AVCodec *codec) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  const AVCodecDescriptor *desc = avcodec_descriptor_get(codec->id);
  const AVProfile *profile = desc->profiles;

  if (profile == NULL) {
    return result;
  }

  while (profile->profile != FF_PROFILE_UNKNOWN) {
    ERL_NIF_TERM profile_name = enif_make_string(env, profile->name, ERL_NIF_LATIN1);
    result = enif_make_list_cell(env, profile_name, result);

    profile++;
  }

  return result;
}

static ERL_NIF_TERM codec_get_sample_formats(ErlNifEnv *env, const AVCodec *codec) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  if (codec->type != AVMEDIA_TYPE_AUDIO) {
    return result;
  }

  const enum AVSampleFormat *sample_format = codec->sample_fmts;
  while (*sample_format != AV_SAMPLE_FMT_NONE) {
    ERL_NIF_TERM format_name = enif_make_atom(env, av_get_sample_fmt_name(*sample_format));
    result = enif_make_list_cell(env, format_name, result);

    sample_format++;
  }

  return result;
}

static ERL_NIF_TERM codec_get_sample_rates(ErlNifEnv *env, const AVCodec *codec) {
  ERL_NIF_TERM result = enif_make_list(env, 0);

  if (codec->type != AVMEDIA_TYPE_AUDIO || codec->supported_samplerates == NULL) {
    return result;
  }

  const int *sample_rate = codec->supported_samplerates;

  while (*sample_rate != 0) {
    result = enif_make_list_cell(env, enif_make_int(env, *sample_rate), result);
    sample_rate++;
  }

  return result;
}

static ErlNifFunc xav_funcs[] = {{"new", 2, new},
                                 {"encode", 3, encode},
                                 {"flush", 1, flush},
                                 {"list_encoders", 0, list_encoders}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  xav_encoder_resource_type =
      enif_open_resource_type(env, NULL, "XavEncoder", free_xav_encoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.Encoder.NIF, xav_funcs, &load, NULL, NULL, NULL);