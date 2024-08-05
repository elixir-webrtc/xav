#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

#include "audio_converter.h"
#include "utils.h"

struct Decoder {
  enum AVMediaType media_type;
  AVFrame *frame;
  AVPacket *pkt;
  const AVCodec *codec;
  AVCodecContext *c;
};

struct Decoder *decoder_alloc();

int decoder_init(struct Decoder *decoder, const char *codec);

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame);

void decoder_free_frame(struct Decoder *decoder);

void decoder_free(struct Decoder **decoder);