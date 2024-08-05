#include "audio_converter.h"
#include "reader.h"

struct XavReader {
  struct Reader *reader;
  struct AudioConverter *ac;
};