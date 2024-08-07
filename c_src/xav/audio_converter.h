#ifndef CONVERTER_H
#define CONVERTER_H
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <stdint.h>

#include "channel_layout.h"

struct AudioConverter {
  SwrContext *swr_ctx;
  int64_t in_sample_rate;
  int64_t out_sample_rate;
  int64_t out_channels;
  struct ChannelLayout out_chlayout;
  enum AVSampleFormat out_sample_fmt;
};

struct AudioConverter *audio_converter_alloc(void);

int audio_converter_init(struct AudioConverter *c, struct ChannelLayout in_chlayout,
                         int in_sample_rate, enum AVSampleFormat in_sample_fmt,
                         struct ChannelLayout out_chlayout, int out_sample_rate,
                         enum AVSampleFormat out_sample_fmt);

/**
 * Converts AVFrame to the output format.
 *
 * @param c audio converter
 * @param src_frame decoded source frame
 * @param out_data buffer where audio samples are written after convertion.
 * We always convert to the packed format, so only *out_data[0] is set.
 * It will be initialized internally and has to be freed with av_freep(&(*out_data[0])).
 * @param out_samples number of samples per channel in out_data buffer.
 * @param out_size size of out_buffer in bytes.
 * This is the same as *out_samples * bytes_per_sample(out_format) * out_channels
 * @return  0 on success and negative value on error.
 */
int audio_converter_convert(struct AudioConverter *c, AVFrame *src_frame, uint8_t ***out_data,
                            int *out_samples, int *out_size);

void audio_converter_free(struct AudioConverter **converter);
#endif
