#ifndef CHANNEL_LAYOUT_H
#define CHANNEL_LAYOUT_H
#include <libavutil/channel_layout.h>

struct ChannelLayout {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  AVChannelLayout layout;
#else
  uint64_t layout;
#endif
};
#endif
