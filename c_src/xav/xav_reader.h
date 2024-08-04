#include "converter.h"
#include "reader.h"

struct XavReader {
  struct Reader *reader;
  struct Converter *converter;
};