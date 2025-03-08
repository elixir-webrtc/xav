#include "channel_layout.h"

int xav_get_channel_layout(const char *name, struct ChannelLayout *layout) {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  if (av_channel_layout_from_string(&layout->layout, name) < 0) {
    return 0;
  }
#else
  layout->layout = av_get_channel_layout(name);
  if (layout->layout == 0) {
    return 0;
  }
#endif

  return 1;
}