#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

struct Reader {
  char *path;
  AVFrame *frame;
  AVPacket *pkt;
  const AVCodec *codec;
  AVCodecContext *c;
  AVFormatContext *fmt_ctx;
  int stream_idx;

  // used for converting decoded frame
  // to rgb pixel format
  uint8_t *rgb_dst_data[4];
  int rgb_dst_linesize[4];

  // points either to frame->data
  // frame->linesize or rgb_dst_data
  // rgb_dst_linesize depending on
  // whether convertion to rgb was needed
  uint8_t **frame_data;
  int *frame_linesize;
};

int reader_init(struct Reader *reader, char *path, size_t path_size);

int reader_next_frame(struct Reader *reader);

void reader_unref_frame(struct Reader *reader);

void reader_free(struct Reader *reader);
