#include <libavcodec/avcodec.h>

struct Decoder {
  enum AVMediaType media_type;
  const AVCodec *codec;
  AVCodecContext *c;  
};
