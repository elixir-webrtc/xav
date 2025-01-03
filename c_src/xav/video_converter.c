#include "video_converter.h"

int video_converter_convert(AVFrame *src_frame, AVFrame **dst_frame,
                            enum AVPixelFormat out_format) {
  int ret;

  *dst_frame = av_frame_alloc();
  if (!*dst_frame) {
    return -1;
  }

  (*dst_frame)->width = src_frame->width;
  (*dst_frame)->height = src_frame->height;
  (*dst_frame)->format = out_format;
  (*dst_frame)->pts = src_frame->pts;

  ret = av_frame_get_buffer(*dst_frame, 0);
  if (ret < 0) {
    return ret;
  }

  struct SwsContext *sws_ctx =
      sws_getContext(src_frame->width, src_frame->height, src_frame->format, src_frame->width,
                     src_frame->height, out_format, SWS_BILINEAR, NULL, NULL, NULL);

  if (ret < 0) {
    return ret;
  }

  // is this (const uint8_t * const*) cast really correct?
  ret = sws_scale(sws_ctx, (const uint8_t *const *)src_frame->data, src_frame->linesize, 0,
                  src_frame->height, (*dst_frame)->data, (*dst_frame)->linesize);

  if (ret < 0) {
    av_frame_free(dst_frame);
    sws_freeContext(sws_ctx);
    return ret;
  }

  sws_freeContext(sws_ctx);

  return ret;
}