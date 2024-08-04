#include "xav_reader.h"

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
  xav_reader->converter = NULL;

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

  XAV_LOG_DEBUG("Returning to Erlang");

  ERL_NIF_TERM frame_term;
  if (xav_reader->reader->media_type == AVMEDIA_TYPE_VIDEO) {
    frame_term = xav_nif_video_frame_to_term(
        env, xav_reader->reader->frame, xav_reader->reader->frame_data,
        xav_reader->reader->frame_linesize, xav_reader->reader->out_format_name);
  } else if (xav_reader->reader->media_type == AVMEDIA_TYPE_AUDIO) {
    const char *out_format = av_get_sample_fmt_name(xav_reader->reader->converter->out_sample_fmt);

    frame_term = xav_nif_audio_frame_to_term(
        env, xav_reader->reader->out_data, xav_reader->reader->out_samples,
        xav_reader->reader->out_size, out_format, xav_reader->reader->frame->pts);
  }

  reader_free_frame(xav_reader->reader);

  return xav_nif_ok(env, frame_term);
}

void free_xav_reader(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing XavReader object");
  struct XavReader *xav_reader = (struct XavReader *)obj;
  if (xav_reader->reader != NULL) {
    reader_free(&xav_reader->reader);
  }

  if (xav_reader->converter != NULL) {
    converter_free(&xav_reader->converter);
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
