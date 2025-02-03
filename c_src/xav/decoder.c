#include "decoder.h"
#include "utils.h"
#include "video_converter.h"

static int init_converter(struct Decoder *decoder);

struct Decoder *decoder_alloc() {
  struct Decoder *decoder = (struct Decoder *)XAV_ALLOC(sizeof(struct Decoder));

  decoder->codec = NULL;
  decoder->c = NULL;

  return decoder;
}

int decoder_init(struct Decoder *decoder, const AVCodec *codec) {
  decoder->media_type = codec->type;
  decoder->codec = codec;

  decoder->c = avcodec_alloc_context3(decoder->codec);
  if (!decoder->c) {
    return -1;
  }

  decoder->frame = av_frame_alloc();
  if (!decoder->frame) {
    return -1;
  }

  decoder->pkt = av_packet_alloc();
  if (!decoder->pkt) {
    return -1;
  }

  if (avcodec_open2(decoder->c, decoder->codec, NULL) < 0) {
    return -1;
  }

  return 0;
}

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame) {
  int ret;
  ret = avcodec_send_packet(decoder->c, pkt);
  if (ret != 0) {
    return -2;
  }

  return avcodec_receive_frame(decoder->c, frame);
}

int decoder_flush(struct Decoder *decoder, AVFrame **frames, int *frames_count) {
  int ret = avcodec_send_packet(decoder->c, NULL);
  if (ret != 0) {
    return ret;
  }

  while (1) {
    ret = avcodec_receive_frame(decoder->c, frames[*frames_count]);
    if (ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      return ret;
    }

    *frames_count += 1;
  }

  return 0;
}

void decoder_free_frame(struct Decoder *decoder) {
  if (decoder->frame != NULL) {
    av_frame_unref(decoder->frame);
  }
  if (decoder->pkt != NULL) {
    av_packet_unref(decoder->pkt);
  }
}

void decoder_free(struct Decoder **decoder) {
  XAV_LOG_DEBUG("Freeing Decoder object");
  if (*decoder != NULL) {
    struct Decoder *d = *decoder;

    if (d->c != NULL) {
      avcodec_free_context(&d->c);
    }

    if (d->pkt != NULL) {
      av_packet_free(&d->pkt);
    }

    if (d->frame != NULL) {
      av_frame_free(&d->frame);
    }

    XAV_FREE(d);
    *decoder = NULL;
  }
}
