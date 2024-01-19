#include <libavcodec/avcodec.h>

struct Decoder {
  const AVCodec *codec;
  AVCodecContext *c;  
};
