#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "utils.h"

struct Reader {
  char *path;
  AVFrame *frame;
  AVPacket *pkt;
  const AVCodec *codec;
  AVCodecContext *c;
  AVFormatContext *fmt_ctx;
  int stream_idx;
  const AVInputFormat *input_format;
  AVDictionary *options;
  enum AVMediaType media_type;
  SwrContext *swr_ctx;

  const char *in_format_name;
  const char *out_format_name;

  // used for converting decoded frame
  // to rgb pixel format
  uint8_t *rgb_dst_data[4];
  int rgb_dst_linesize[4];

  // points either to frame->data
  // frame->linesize or rgb_dst_data
  // rgb_dst_linesize depending on
  // whether convertion to rgb was needed
  uint8_t **frame_data;
  int *frame_linesize;

  struct Converter converter;
  // Buffer where audio samples are written after conversion.
  // We always convet to packed format, so only out_data[0] is set.
  uint8_t **out_data;
  // Number of samples in out_data buffer
  int out_samples;
  // Size of out_data buffer.
  // This is the same as out_samples * bytes_per_sample(out_format) * out_channels.
  int out_size;
};

int reader_init(struct Reader *reader, unsigned char *path, size_t path_size, int device_flag,
                enum AVMediaType media_type);

int reader_next_frame(struct Reader *reader);

void reader_free_frame(struct Reader *reader);

void reader_free(struct Reader *reader);
