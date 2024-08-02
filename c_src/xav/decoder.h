#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

#include "converter.h"

struct Decoder {
  enum AVMediaType media_type;
  const AVCodec *codec;
  AVCodecContext *c;
  SwrContext *swr_ctx;

  const char *out_format_name;

  uint8_t *rgb_dst_data[4];
  int rgb_dst_linesize[4];

  uint8_t **frame_data;
  int *frame_linesize;

  struct Converter *converter;
  // Buffer where audio samples are written after conversion.
  // We always convet to packed format, so only out_data[0] is set.
  uint8_t **out_data;
  // Number of samples in out_data buffer
  int out_samples;
  // Size of out_data buffer.
  // This is the same as out_samples * bytes_per_sample(out_format) * out_channels.
  int out_size;
};

int decoder_init(struct Decoder *decoder, const char *codec);

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame);

void decoder_free(struct Decoder *decoder);