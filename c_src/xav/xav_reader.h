#include "audio_converter.h"
#include "video_converter.h"
#include "reader.h"

struct XavReader {
  struct Reader *reader;
  struct AudioConverter *ac;
  struct VideoConverter *vc;
  char *out_format;
  int out_sample_rate;
  int out_channels;
};