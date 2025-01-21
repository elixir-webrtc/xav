#include "encoder.h"

struct Encoder *encoder_alloc() {
  struct Encoder *encoder = XAV_ALLOC(sizeof(struct Encoder));
  encoder->c = NULL;
  encoder->codec = NULL;
  encoder->num_packets = 0;
  encoder->size_packets = 8;
  encoder->packets = XAV_ALLOC(encoder->size_packets * sizeof(AVPacket *));

  for (int i = 0; i < encoder->size_packets; i++) {
    encoder->packets[i] = av_packet_alloc();
  }

  return encoder;
}

int encoder_init(struct Encoder *encoder, struct EncoderConfig *config) {
  encoder->codec = avcodec_find_encoder(config->codec);
  if (!encoder->codec) {
    return -1;
  }

  encoder->c = avcodec_alloc_context3(encoder->codec);
  if (!encoder->c) {
    return -1;
  }

  encoder->c->width = config->width;
  encoder->c->height = config->height;
  encoder->c->pix_fmt = config->format;
  encoder->c->time_base = config->time_base;

  AVDictionary *opts = NULL;
  if (config->codec == AV_CODEC_ID_HEVC) {
    av_dict_set(&opts, "x265-params", "log-level=warning", 0);
  }

  return avcodec_open2(encoder->c, encoder->codec, &opts);
}

int encoder_encode(struct Encoder *encoder, AVFrame *frame) {
  int ret = avcodec_send_frame(encoder->c, frame);
  if (ret < 0) {
    return ret;
  }

  encoder->num_packets = 0;

  while (1) {
    ret = avcodec_receive_packet(encoder->c, encoder->packets[encoder->num_packets]);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      return ret;
    }

    if (++encoder->num_packets >= encoder->size_packets) {
      encoder->size_packets *= 2;
      encoder->packets = XAV_REALLOC(encoder->packets, encoder->size_packets * sizeof(AVPacket *));
      for (int i = encoder->num_packets; i < encoder->size_packets; i++) {
        encoder->packets[i] = av_packet_alloc();
      }
    }
  }

  return 0;
}

void encoder_free(struct Encoder **encoder) {
  if (*encoder != NULL) {
    struct Encoder *e = *encoder;

    if (e->c != NULL) {
      avcodec_free_context(&e->c);
    }

    for (int i = 0; i < e->size_packets; i++) {
      av_packet_free(&e->packets[i]);
    }

    XAV_FREE(e);
    *encoder = NULL;
  }
}