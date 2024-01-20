#include "decoder.h"

int decoder_init(struct Decoder *decoder, const char *codec) {
  decoder->swr_ctx = NULL;

  if (strcmp(codec, "opus") == 0) {
    decoder->media_type = AVMEDIA_TYPE_AUDIO;
    decoder->codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    // we will initialize out_format_name with the first frame
    decoder->out_format_name = NULL;
  } else if (strcmp(codec, "vp8") == 0) {
    decoder->media_type = AVMEDIA_TYPE_VIDEO;
    decoder->codec = avcodec_find_decoder(AV_CODEC_ID_VP8);
    decoder->out_format_name = "rgb";
  } else {
    return -1;
  }

  if (!decoder->codec) {
    return -1;
  }

  decoder->c = avcodec_alloc_context3(decoder->codec);
  if (!decoder->c) {
    return -1;
  }

  if (avcodec_open2(decoder->c, decoder->codec, NULL) < 0) {
    return -1;
  }

  return 0;
}

int decoder_decode(struct Decoder *decoder, AVPacket *pkt) {
  return 0;
}