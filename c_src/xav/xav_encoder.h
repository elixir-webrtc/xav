#include "encoder.h"
#include "utils.h"
#include <libavutil/pixfmt.h>

/* FFmpeg ≤ 7 used FF_PROFILE_UNKNOWN, FFmpeg ≥ 8 uses AV_PROFILE_UNKNOWN. */
#if defined(AV_PROFILE_UNKNOWN)
#elif defined(FF_PROFILE_UNKNOWN)
#  define AV_PROFILE_UNKNOWN FF_PROFILE_UNKNOWN
#else
#  define AV_PROFILE_UNKNOWN (-99)
#endif

struct XavEncoder {
  struct Encoder *encoder;
  AVFrame *frame;
};
