#ifndef XAV_CHANNEL_LAYOUT_H
#define XAV_CHANNEL_LAYOUT_H
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>

struct ChannelLayout {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  AVChannelLayout layout;
#else
  uint64_t layout;
#endif
};

int xav_get_channel_layout(const char *name, struct ChannelLayout *layout);
int xav_get_channel_layout_from_context(struct ChannelLayout *layout, const AVCodecContext *ctx);
int xav_set_channel_layout(AVCodecContext *ctx, struct ChannelLayout *layout);
int xav_set_default_channel_layout(struct ChannelLayout *layout, int channels);
int xav_set_frame_channel_layout(AVFrame *frame, struct ChannelLayout *layout);
#endif