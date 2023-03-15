#include "utils.h"

void print_supported_pix_fmts(AVCodec *codec) {
  if (codec->pix_fmts == NULL) {
    fprintf(stdout, "unknown\n");
  } else {
    for (int i = 0; codec->pix_fmts[i] != -1; i++) {
      fprintf(stdout, "fmt: %d\n", codec->pix_fmts[i]);
    }
  }
}

ERL_NIF_TERM xav_nif_ok(ErlNifEnv *env, ERL_NIF_TERM data_term) {
  ERL_NIF_TERM ok_term = enif_make_atom(env, "ok");
  return enif_make_tuple(env, 2, ok_term, data_term);
}

ERL_NIF_TERM xav_nif_error(ErlNifEnv *env, char *reason) {
  ERL_NIF_TERM error_term = enif_make_atom(env, "error");
  ERL_NIF_TERM reason_term = enif_make_atom(env, reason);
  return enif_make_tuple(env, 2, error_term, reason_term);
}

ERL_NIF_TERM xav_nif_raise(ErlNifEnv *env, char *msg) {
  ERL_NIF_TERM reason = enif_make_atom(env, msg);
  return enif_raise_exception(env, reason);
}

ERL_NIF_TERM xav_nif_frame_to_term(ErlNifEnv *env, unsigned char *data[], int *linesize, int width,
                                   int height, int64_t pts) {
  ERL_NIF_TERM data_term;
  unsigned char *ptr = enif_make_new_binary(env, linesize[0] * height, &data_term);
  memcpy(ptr, data[0], linesize[0] * height);

  ERL_NIF_TERM height_term = enif_make_int(env, height);
  ERL_NIF_TERM width_term = enif_make_int(env, width);
  ERL_NIF_TERM pts_term = enif_make_int64(env, pts);
  return enif_make_tuple(env, 4, data_term, width_term, height_term, pts_term);
}