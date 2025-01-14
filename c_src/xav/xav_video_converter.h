#include "utils.h"
#include "video_converter.h"

struct XavVideoConverter {
  struct VideoConverter *vc;
  enum AVPixelFormat out_format;
  int out_width;
  int out_height;
  AVFrame *frame;
};