#include "utils.h"
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <stdint.h>

void print_supported_pix_fmts(AVCodec *codec) {
  if (codec->pix_fmts == NULL) {
    fprintf(stdout, "unknown\n");
  } else {
    for (int i = 0; codec->pix_fmts[i] != -1; i++) {
      fprintf(stdout, "fmt: %d\n", codec->pix_fmts[i]);
    }
  }
}

int init_swr_ctx_from_frame(SwrContext **swr_ctx, AVFrame *frame) {
  *swr_ctx = swr_alloc();
  enum AVSampleFormat out_sample_fmt = av_get_alt_sample_fmt(frame->format, 0);

#if LIBAVUTIL_VERSION_MAJOR >= 58
  av_opt_set_chlayout(*swr_ctx, "in_chlayout", &frame->ch_layout, 0);
  av_opt_set_chlayout(*swr_ctx, "out_chlayout", &frame->ch_layout, 0);
#else
  av_opt_set_channel_layout(*swr_ctx, "in_channel_layout", frame->channel_layout, 0);
  av_opt_set_channel_layout(*swr_ctx, "out_channel_layout", frame->channel_layout, 0);
#endif

  av_opt_set_int(*swr_ctx, "in_sample_rate", frame->sample_rate, 0);
  av_opt_set_int(*swr_ctx, "out_sample_rate", frame->sample_rate, 0);
  av_opt_set_sample_fmt(*swr_ctx, "in_sample_fmt", frame->format, 0);
  av_opt_set_sample_fmt(*swr_ctx, "out_sample_fmt", out_sample_fmt, 0);

  return swr_init(*swr_ctx);
}

void convert_to_rgb(AVFrame *src_frame, uint8_t *dst_data[], int dst_linesize[]) {
  struct SwsContext *sws_ctx =
      sws_getContext(src_frame->width, src_frame->height, src_frame->format, src_frame->width,
                     src_frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

  av_image_alloc(dst_data, dst_linesize, src_frame->width, src_frame->height, AV_PIX_FMT_RGB24, 1);

  // is this (const uint8_t * const*) cast really correct?
  sws_scale(sws_ctx, (const uint8_t *const *)src_frame->data, src_frame->linesize, 0,
            src_frame->height, dst_data, dst_linesize);
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

ERL_NIF_TERM xav_nif_video_frame_to_term(ErlNifEnv *env, AVFrame *frame, unsigned char *data[],
                                         int *linesize, const char *format_name) {
  ERL_NIF_TERM data_term;
  unsigned char *ptr = enif_make_new_binary(env, linesize[0] * frame->height, &data_term);
  memcpy(ptr, data[0], linesize[0] * frame->height);

  ERL_NIF_TERM format_term = enif_make_atom(env, format_name);
  ERL_NIF_TERM height_term = enif_make_int(env, frame->height);
  ERL_NIF_TERM width_term = enif_make_int(env, frame->width);
  ERL_NIF_TERM pts_term = enif_make_int64(env, frame->pts);
  return enif_make_tuple(env, 5, data_term, format_term, width_term, height_term, pts_term);
}
