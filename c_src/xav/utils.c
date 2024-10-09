#include "utils.h"
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <stdint.h>

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

ERL_NIF_TERM xav_nif_audio_frame_to_term(ErlNifEnv *env, uint8_t **out_data, int out_samples,
                                         int out_size, const char *out_format, int pts) {
  ERL_NIF_TERM data_term;

  unsigned char *ptr = enif_make_new_binary(env, out_size, &data_term);
  memcpy(ptr, out_data[0], out_size);

  ERL_NIF_TERM samples_term = enif_make_int(env, out_samples);
  ERL_NIF_TERM format_term = enif_make_atom(env, out_format);
  ERL_NIF_TERM pts_term = enif_make_int(env, pts);

  return enif_make_tuple(env, 4, data_term, format_term, samples_term, pts_term);
}

ERL_NIF_TERM xav_nif_video_frame_to_term(ErlNifEnv *env, AVFrame *frame, uint8_t *data[4],
                                         int linesize[4], const char *format_name) {
  ERL_NIF_TERM data_term;
  unsigned char *ptr = enif_make_new_binary(env, linesize[0] * frame->height, &data_term);
  memcpy(ptr, data[0], linesize[0] * frame->height);

  ERL_NIF_TERM format_term = enif_make_atom(env, format_name);
  ERL_NIF_TERM height_term = enif_make_int(env, frame->height);
  ERL_NIF_TERM width_term = enif_make_int(env, frame->width);
  ERL_NIF_TERM pts_term = enif_make_int64(env, frame->pts);
  return enif_make_tuple(env, 5, data_term, format_term, width_term, height_term, pts_term);
}
