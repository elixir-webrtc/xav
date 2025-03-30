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

int xav_set_channel_layout(AVCodecContext *ctx, struct ChannelLayout *layout) {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  return av_channel_layout_copy(&ctx->ch_layout, &layout->layout);
#else
  ctx->channel_layout = layout->layout;
  return 0;
#endif
}

int xav_set_frame_channel_layout(AVFrame *frame, struct ChannelLayout *layout) {
#if LIBAVUTIL_VERSION_MAJOR >= 58
  return av_channel_layout_copy(&frame->ch_layout, &layout->layout);
#else
  frame->channel_layout = layout->layout;
  return 0;
#endif
}
