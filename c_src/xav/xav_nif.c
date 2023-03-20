#include "reader.h"

ErlNifResourceType *reader_resource_type;

ERL_NIF_TERM new_reader(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {

  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[0], &bin)) {
    return xav_nif_raise(env, "invalid_path");
  }

  struct Reader *reader = enif_alloc_resource(reader_resource_type, sizeof(struct Reader));

  int ret = reader_init(reader, bin.data, bin.size);

  if (ret == -1) {
    return xav_nif_raise(env, "couldnt_open_avformat_input");
  } else if (ret == -2) {
    return xav_nif_raise(env, "couldnt_create_new_reader");
  }

  ERL_NIF_TERM ret_term = enif_make_resource(env, reader);
  enif_release_resource(reader);
  return ret_term;
}

ERL_NIF_TERM next_frame(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct Reader *reader;
  if (!enif_get_resource(env, argv[0], reader_resource_type, (void **)&reader)) {
    return xav_nif_raise(env, "couldnt_get_reader_resource");
  }

  int ret = reader_next_frame(reader);

  if (ret == AVERROR_EOF) {
    return xav_nif_error(env, "eof");
  } else if (ret != 0) {
    return xav_nif_raise(env, "receive_frame");
  }

  XAV_LOG_DEBUG("Returning to Erlang");

  ERL_NIF_TERM frame_term =
      xav_nif_frame_to_term(env, reader->frame_data, reader->frame_linesize, reader->frame->width,
                            reader->frame->height, reader->frame->pts);

  reader_unref_frame(reader);

  return xav_nif_ok(env, frame_term);
}

void free_reader(ErlNifEnv *env, void *obj) {
  struct Reader *reader = (struct Reader *)obj;
  reader_free(reader);
}

static ErlNifFunc xav_funcs[] = {{"new_reader", 1, new_reader},
                                 {"next_frame", 1, next_frame, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  reader_resource_type =
      enif_open_resource_type(env, NULL, "Reader", free_reader, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.NIF, xav_funcs, &load, NULL, NULL, NULL);