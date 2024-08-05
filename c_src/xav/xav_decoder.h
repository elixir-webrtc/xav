#include "audio_converter.h"
#include "decoder.h"

struct XavDecoder {
  struct Decoder *decoder;
  struct AudioConverter *ac;
};