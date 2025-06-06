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

int xav_nif_get_atom(ErlNifEnv *env, ERL_NIF_TERM term, char **value) {
  unsigned int atom_len;
  if (!enif_get_atom_length(env, term, &atom_len, ERL_NIF_LATIN1)) {
    return 0;
  }

  char *atom_value = (char *)XAV_ALLOC((atom_len + 1) * sizeof(char *));
  if (!enif_get_atom(env, term, atom_value, atom_len + 1, ERL_NIF_LATIN1)) {
    XAV_FREE(atom_value);
    return 0;
  }

  *value = atom_value;
  return 1;
}

int xav_nif_get_string(ErlNifEnv *env, ERL_NIF_TERM term, char **value) {
  ErlNifBinary bin;
  if (!enif_inspect_binary(env, term, &bin)) {
    return 0;
  }

  char *str_value = (char *)XAV_ALLOC((bin.size + 1) * sizeof(char *));
  memcpy(str_value, bin.data, bin.size);
  str_value[bin.size] = '\0';

  *value = str_value;
  return 1;
}

ERL_NIF_TERM xav_nif_audio_frame_to_term(ErlNifEnv *env, uint8_t **out_data, int out_samples,
                                         int out_size, enum AVSampleFormat out_format, int pts) {
  ERL_NIF_TERM data_term;

  unsigned char *ptr = enif_make_new_binary(env, out_size, &data_term);
  memcpy(ptr, out_data[0], out_size);

  ERL_NIF_TERM samples_term = enif_make_int(env, out_samples);
  ERL_NIF_TERM format_term = enif_make_atom(env, av_get_sample_fmt_name(out_format));
  ERL_NIF_TERM pts_term = enif_make_int(env, pts);

  return enif_make_tuple(env, 4, data_term, format_term, samples_term, pts_term);
}

ERL_NIF_TERM xav_nif_video_frame_to_term(ErlNifEnv *env, AVFrame *frame) {
  ERL_NIF_TERM data_term;

  int payload_size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
  unsigned char *ptr = enif_make_new_binary(env, payload_size, &data_term);

  av_image_copy_to_buffer(ptr, payload_size, (const uint8_t *const *)frame->data,
                          (const int *)frame->linesize, frame->format, frame->width, frame->height,
                          1);

  ERL_NIF_TERM format_term = enif_make_atom(env, av_get_pix_fmt_name(frame->format));
  ERL_NIF_TERM height_term = enif_make_int(env, frame->height);
  ERL_NIF_TERM width_term = enif_make_int(env, frame->width);
  ERL_NIF_TERM pts_term = enif_make_int64(env, frame->pts);
  return enif_make_tuple(env, 5, data_term, format_term, width_term, height_term, pts_term);
}

ERL_NIF_TERM xav_nif_packet_to_term(ErlNifEnv *env, AVPacket *packet) {
  ERL_NIF_TERM data_term;

  unsigned char *ptr = enif_make_new_binary(env, packet->size, &data_term);

  memcpy(ptr, packet->data, packet->size);

  ERL_NIF_TERM dts = enif_make_int(env, packet->dts);
  ERL_NIF_TERM pts = enif_make_int(env, packet->pts);
  ERL_NIF_TERM is_keyframe =
      enif_make_atom(env, packet->flags & AV_PKT_FLAG_KEY ? "true" : "false");
  return enif_make_tuple(env, 4, data_term, dts, pts, is_keyframe);
}

int xav_get_nb_channels(const AVFrame *frame) {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  return frame->ch_layout.nb_channels;
#else
  return frame->channels;
#endif
}