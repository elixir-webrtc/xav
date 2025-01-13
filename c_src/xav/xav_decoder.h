#include "audio_converter.h"
#include "video_converter.h"
#include "decoder.h"

#include <libavutil/pixfmt.h>

struct XavDecoder {
  struct Decoder *decoder;
  struct AudioConverter *ac;
  struct VideoConverter *vc;
  enum AVPixelFormat out_video_fmt;
  enum AVSampleFormat out_audio_fmt;
  int out_sample_rate;
  int out_channels;
};