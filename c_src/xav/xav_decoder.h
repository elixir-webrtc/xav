#include "audio_converter.h"
#include "decoder.h"

struct XavDecoder {
  struct Decoder *decoder;
  struct AudioConverter *ac;
  char *out_format;
  int out_sample_rate;
  int out_channels;
};