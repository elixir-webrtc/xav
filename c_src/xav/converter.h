#ifndef CONVERTER_H
#define CONVERTER_H
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <stdint.h>

#include "channel_layout.h"

struct Converter {
  SwrContext *swr_ctx;
  int64_t in_sample_rate;
  int64_t out_sample_rate;
  struct ChannelLayout out_chlayout;
  enum AVSampleFormat out_sample_fmt;
};

struct Converter *converter_alloc(void);

int converter_init(struct Converter *c, struct ChannelLayout in_chlayout, int in_sample_rate,
                   enum AVSampleFormat in_sample_fmt, struct ChannelLayout out_chlaout,
                   int out_sample_rate, enum AVSampleFormat out_sample_fmt);

int converter_convert(struct Converter *c, AVFrame *src_frame, uint8_t ***out_data,
                      int *out_samples, int *out_size);

void converter_free(struct Converter **converter);
#endif
