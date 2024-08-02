#include "converter.h"
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <stdint.h>

#include "channel_layout.h"
#include "utils.h"

int converter_init(struct Converter *c, struct ChannelLayout in_chlayout, int in_sample_rate,
                   enum AVSampleFormat in_sample_fmt, struct ChannelLayout out_chlayout,
                   int out_sample_rate, enum AVSampleFormat out_sample_fmt) {
  c->swr_ctx = swr_alloc();
  c->in_sample_rate = in_sample_rate;
  c->out_sample_rate = out_sample_rate;
  c->out_chlayout = out_chlayout;
  c->out_sample_fmt = out_sample_fmt;

#if LIBAVUTIL_VERSION_MAJOR >= 58
  av_opt_set_chlayout(c->swr_ctx, "in_chlayout", &in_chlayout.layout, 0);
  av_opt_set_chlayout(c->swr_ctx, "out_chlayout", &out_chlayout.layout, 0);
#else
  av_opt_set_channel_layout(c->swr_ctx, "in_channel_layout", in_chlayout.layout, 0);
  av_opt_set_channel_layout(c->swr_ctx, "out_channel_layout", out_chlayout.layout, 0);
#endif

  av_opt_set_int(c->swr_ctx, "in_sample_rate", in_sample_rate, 0);
  av_opt_set_int(c->swr_ctx, "out_sample_rate", out_sample_rate, 0);

  av_opt_set_sample_fmt(c->swr_ctx, "in_sample_fmt", in_sample_fmt, 0);
  av_opt_set_sample_fmt(c->swr_ctx, "out_sample_fmt", out_sample_fmt, 0);

  return swr_init(c->swr_ctx);
}

int converter_convert(struct Converter *c, AVFrame *src_frame, uint8_t ***out_data,
                      int *out_samples, int *out_size) {

#if LIBAVUTIL_VERSION_MAJOR >= 58
  int out_nb_channels = c->out_chlayout.layout.nb_channels;
#else
  int out_nb_channels = av_get_channel_layout_nb_channels(c->out_chlayout.layout);
#endif

  uint8_t **out_data_tmp = NULL;
  int max_out_nb_samples = swr_get_out_samples(c->swr_ctx, src_frame->nb_samples);
  int out_bytes_per_sample = av_get_bytes_per_sample(c->out_sample_fmt);

  // Some parts of ffmpeg require buffers to by divisible by 32
  // to use fast/aligned SIMD routines - this is what align option is used for.
  // See https://stackoverflow.com/questions/35678041/what-is-linesize-alignment-meaning
  // Because we return the binary straight to the Erlang, we can disable it.
  int ret = av_samples_alloc_array_and_samples(&out_data_tmp, NULL, out_nb_channels,
                                               max_out_nb_samples, c->out_sample_fmt, 1);

  if (ret < 0) {
    XAV_LOG_DEBUG("Couldn't allocate array for out samples.");
    return ret;
  }

  *out_data = out_data_tmp;

  *out_samples = swr_convert(c->swr_ctx, out_data_tmp, max_out_nb_samples,
                             (const uint8_t **)src_frame->data, src_frame->nb_samples);

  if (*out_samples < 0) {
    XAV_LOG_DEBUG("Couldn't convert samples: %d", *out_samples);
    return -1;
  }

  XAV_LOG_DEBUG("Converted %d samples per channel", *out_samples);

  *out_size = *out_samples * out_bytes_per_sample * out_nb_channels;

  return 0;
}

void converter_free(struct Converter *c) { swr_free(&c->swr_ctx); }
