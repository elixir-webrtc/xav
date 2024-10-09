#include "video_converter.h"

int video_converter_convert(AVFrame *src_frame, uint8_t *out_data[], int out_linesize[]) {
  int ret;

  struct SwsContext *sws_ctx =
      sws_getContext(src_frame->width, src_frame->height, src_frame->format, src_frame->width,
                     src_frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

  ret = av_image_alloc(out_data, out_linesize, src_frame->width, src_frame->height,
                       AV_PIX_FMT_RGB24, 1);

  if (ret < 0) {
    return ret;
  }

  // is this (const uint8_t * const*) cast really correct?
  ret = sws_scale(sws_ctx, (const uint8_t *const *)src_frame->data, src_frame->linesize, 0,
                  src_frame->height, out_data, out_linesize);

  if (ret < 0) {
    av_freep(&out_data[0]);
    sws_freeContext(sws_ctx);
    return ret;
  }

  sws_freeContext(sws_ctx);

  return ret;
}
