#include "encoder.h"
#include "utils.h"
#include <libavutil/pixfmt.h>

struct XavEncoder {
  struct Encoder *encoder;
  AVFrame *frame;
};
