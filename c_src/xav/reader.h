#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_converter.h"
#include "utils.h"

struct Reader {
  char *path;
  AVFrame *frame;
  AVPacket *pkt;
  const AVCodec *codec;
  AVCodecContext *c;
  AVFormatContext *fmt_ctx;
  int stream_idx;
  const AVInputFormat *input_format;
  AVDictionary *options;
  enum AVMediaType media_type;

  const char *in_format_name;
  const char *out_format_name;
};

struct Reader *reader_alloc();

int reader_init(struct Reader *reader, unsigned char *path, size_t path_size, int device_flag,
                enum AVMediaType media_type);

int reader_next_frame(struct Reader *reader);

void reader_free_frame(struct Reader *reader);

void reader_free(struct Reader **reader);
