#include "encoder.h"

struct Encoder *encoder_alloc() {
  struct Encoder *encoder = XAV_ALLOC(sizeof(struct Encoder));
  encoder->c = NULL;
  encoder->codec = NULL;
  encoder->num_packets = 0;
  encoder->max_num_packets = 8;
  encoder->packets = XAV_ALLOC(encoder->max_num_packets * sizeof(AVPacket *));

  for (int i = 0; i < encoder->max_num_packets; i++) {
    encoder->packets[i] = av_packet_alloc();
  }

  return encoder;
}

int encoder_init(struct Encoder *encoder, struct EncoderConfig *config) {
  encoder->codec = config->codec;

  encoder->c = avcodec_alloc_context3(encoder->codec);
  if (!encoder->c) {
    return -1;
  }

  if (encoder->codec->type == AVMEDIA_TYPE_VIDEO) {
    encoder->c->width = config->width;
    encoder->c->height = config->height;
    encoder->c->pix_fmt = config->format;
    encoder->c->time_base = config->time_base;

    if (config->gop_size > 0) {
      encoder->c->gop_size = config->gop_size;
    }

    if (config->max_b_frames >= 0) {
      encoder->c->max_b_frames = config->max_b_frames;
    }
  } else {
    encoder->c->sample_fmt = config->sample_format;
    encoder->c->sample_rate = config->sample_rate;
    xav_set_channel_layout(encoder->c, &config->channel_layout);
  }

  if (config->profile != FF_PROFILE_UNKNOWN) {
    encoder->c->profile = config->profile;
  }

  AVDictionary *opts = NULL;
  if (strcmp(encoder->codec->name, "libx265") == 0) {
    char x265_params[256] = "log-level=warning";
    if (config->gop_size > 0) {
      sprintf(x265_params + strlen(x265_params), ":keyint=%d", config->gop_size);
    }

    if (config->max_b_frames >= 0) {
      sprintf(x265_params + strlen(x265_params), ":bframes=%d", config->max_b_frames);
    }

    av_dict_set(&opts, "x265-params", x265_params, 0);
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

    if (++encoder->num_packets >= encoder->max_num_packets) {
      encoder->max_num_packets *= 2;
      encoder->packets =
          XAV_REALLOC(encoder->packets, encoder->max_num_packets * sizeof(AVPacket *));
      for (int i = encoder->num_packets; i < encoder->max_num_packets; i++) {
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

    for (int i = 0; i < e->max_num_packets; i++) {
      av_packet_free(&e->packets[i]);
    }

    XAV_FREE(e);
    *encoder = NULL;
  }
}