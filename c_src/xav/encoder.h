#include "channel_layout.h"
#include "utils.h"
#include <libavcodec/avcodec.h>

struct Encoder {
  const AVCodec *codec;
  AVCodecContext *c;
  int num_packets;
  int max_num_packets;
  AVPacket **packets;
};

struct EncoderConfig {
  enum AVMediaType media_type;
  const AVCodec *codec;
  int width;
  int height;
  enum AVPixelFormat format;
  enum AVSampleFormat sample_format;
  AVRational time_base;
  int gop_size;
  int max_b_frames;
  int profile;
  int sample_rate;
  struct ChannelLayout channel_layout;
};

struct Encoder *encoder_alloc();

int encoder_init(struct Encoder *encoder, struct EncoderConfig *encoder_config);

int encoder_encode(struct Encoder *encoder, AVFrame *frame);

void encoder_free(struct Encoder **encoder);