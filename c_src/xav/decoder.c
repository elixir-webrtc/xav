#include "decoder.h"
#include "utils.h"

int decoder_init(struct Decoder *decoder, const char *codec) {
  decoder->swr_ctx = NULL;
  decoder->out_data = NULL;

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

  if (decoder->media_type == AVMEDIA_TYPE_AUDIO) {
    AVChannelLayout out_chlayout = decoder->c->ch_layout;
    int out_sample_rate = decoder->c->sample_rate;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLT;

    int ret = converter_init(&decoder->converter, decoder->c->ch_layout, decoder->c->sample_rate,
                             decoder->c->sample_fmt, out_chlayout, out_sample_rate, out_sample_fmt);

    if (ret < 0) {
      return ret;
    }
  }

  return 0;
}

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame) {
  int ret;
  ret = avcodec_send_packet(decoder->c, pkt);
  if (ret != 0) {
    return -2;
  }

  ret = avcodec_receive_frame(decoder->c, frame);
  if (ret != 0) {
    return -1;
  }

  if (decoder->media_type == AVMEDIA_TYPE_AUDIO && decoder->out_format_name == NULL) {
    enum AVSampleFormat out_sample_fmt = av_get_alt_sample_fmt(frame->format, 0);
    decoder->out_format_name = av_get_sample_fmt_name(out_sample_fmt);
  }

  if (decoder->media_type == AVMEDIA_TYPE_VIDEO) {
    if (frame->format != AV_PIX_FMT_RGB24) {
      convert_to_rgb(frame, decoder->rgb_dst_data, decoder->rgb_dst_linesize);
      decoder->frame_data = decoder->rgb_dst_data;
      decoder->frame_linesize = decoder->rgb_dst_linesize;
    } else {
      decoder->frame_data = frame->data;
      decoder->frame_linesize = frame->linesize;
    }
  } else if (decoder->media_type == AVMEDIA_TYPE_AUDIO) {
    return converter_convert(&decoder->converter, frame, &decoder->out_data, &decoder->out_samples,
                             &decoder->out_size);
  }

  return 0;
}

void decoder_free(struct Decoder *decoder) {
  if (decoder->swr_ctx != NULL) {
    swr_free(&decoder->swr_ctx);
  }

  if (decoder->c != NULL) {
    avcodec_free_context(&decoder->c);
  }
}