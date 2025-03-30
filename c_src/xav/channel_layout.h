#ifndef CHANNEL_LAYOUT_H
#define CHANNEL_LAYOUT_H
#include <libavutil/channel_layout.h>
#include <libavcodec/avcodec.h>

struct ChannelLayout {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  AVChannelLayout layout;
#else
  uint64_t layout;
#endif
};

int xav_get_channel_layout(const char *name, struct ChannelLayout *layout);
int xav_set_channel_layout(AVCodecContext *ctx, struct ChannelLayout *layout);
int xav_set_frame_channel_layout(AVFrame *frame, struct ChannelLayout *layout);
#endif