#include "converter.h"
#include "decoder.h"

struct XavDecoder {
  struct Decoder *decoder;
  struct Converter *converter;
};