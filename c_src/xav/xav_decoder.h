#include "audio_converter.h"
#include "video_converter.h"
#include "decoder.h"

#include <libavutil/pixfmt.h>

struct XavDecoder {
  struct Decoder *decoder;
  // Video params
  struct VideoConverter *vc;
  enum AVPixelFormat out_video_fmt;
  int out_width;
  int out_height;
  // Audio params
  struct AudioConverter *ac;
  enum AVSampleFormat out_audio_fmt;
  int out_sample_rate;
  int out_channels;
};