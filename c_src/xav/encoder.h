#include "utils.h"
#include <libavcodec/avcodec.h>

struct Encoder {
  const AVCodec *codec;
  AVCodecContext *c;
  int num_packets;
  int size_packets;
  AVPacket **packets;
};

struct EncoderConfig {
  enum AVMediaType media_type;
  char *codec_str;
  enum AVCodecID codec;
  int width;
  int height;
  char *format_str;
  enum AVPixelFormat format;
  AVRational time_base;
};

struct Encoder *encoder_alloc();

int encoder_init(struct Encoder *encoder, struct EncoderConfig *encoder_config);

int encoder_encode(struct Encoder *encoder, AVFrame *frame);

void encoder_free(struct Encoder **encoder);