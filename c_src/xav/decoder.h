#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

struct Decoder {
  enum AVMediaType media_type;
  const AVCodec *codec;
  AVCodecContext *c;
  SwrContext *swr_ctx;

  const char *out_format_name;

  uint8_t *rgb_dst_data[4];
  int rgb_dst_linesize[4];

  uint8_t **frame_data;
  int *frame_linesize;
};

int decoder_init(struct Decoder *decoder, const char *codec);

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame);