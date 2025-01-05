#include "video_converter.h"
#include "utils.h"

static inline unsigned int video_converter_resolution_changed(struct VideoConverter *converter, AVFrame *frame) {
  return converter->in_format != frame->format || 
          converter->in_width != frame->width || 
          converter->in_height != frame->height;
}

struct VideoConverter *video_converter_alloc() {
  struct VideoConverter *converter =
      (struct VideoConverter *)XAV_ALLOC(sizeof(struct VideoConverter));
  if(converter) {
    converter->sws_ctx = NULL;
    converter->dst_frame = av_frame_alloc();
  }
  return converter;
}

int video_converter_init(struct VideoConverter *converter, int in_width, int in_height, 
                          enum AVPixelFormat in_format, enum AVPixelFormat out_format) {                            
  converter->in_width = in_width;
  converter->in_height = in_height;
  converter->in_format = in_format;
  converter->out_format = out_format;

  av_frame_unref(converter->dst_frame);

  converter->dst_frame->width = in_width;
  converter->dst_frame->height = in_height;
  converter->dst_frame->format = out_format;

  int ret = av_frame_get_buffer(converter->dst_frame, 0);
  if (ret < 0)
    return ret;

  converter->sws_ctx = sws_getContext(in_width, in_height, in_format, in_width, in_height, out_format, 
                                  SWS_BILINEAR, NULL, NULL, NULL);

  if (!converter->sws_ctx) {
    XAV_LOG_DEBUG("Couldn't get sws context");
    return -1;
  }

  return 0;
}

int video_converter_convert(struct VideoConverter *converter, AVFrame *src_frame) {
  int ret;

  if (video_converter_resolution_changed(converter, src_frame)) {
    XAV_LOG_DEBUG("Frame resolution changed");
    sws_freeContext(converter->sws_ctx);
    ret = video_converter_init(converter, src_frame->width, src_frame->height, 
                                src_frame->format, converter->out_format);
    if (ret < 0) {
      return ret;
    }
  }

  converter->dst_frame->pts = src_frame->pts;

  // is this (const uint8_t * const*) cast really correct?
  return sws_scale(converter->sws_ctx, (const uint8_t *const *)src_frame->data, src_frame->linesize, 0,
                  src_frame->height, converter->dst_frame->data, converter->dst_frame->linesize);
}

void video_converter_free(struct VideoConverter **converter) {
  if (*converter != NULL) {
    sws_freeContext((*converter)->sws_ctx);
    av_frame_free(&(*converter)->dst_frame);
    XAV_FREE(*converter);
    *converter = NULL;
  }
}