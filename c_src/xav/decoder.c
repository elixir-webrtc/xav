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

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame) {
  int ret;
  ret = avcodec_send_packet(decoder->c, pkt);
  if (ret != 0) {
    return -1;
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
  } else if (decoder->media_type == AVMEDIA_TYPE_AUDIO &&
             av_sample_fmt_is_planar(frame->format) == 1) {
    if (decoder->swr_ctx == NULL) {
      if (init_swr_ctx_from_frame(&decoder->swr_ctx, frame) != 0) {
        return -1;
      }
    }

    if (convert_to_interleaved(decoder->swr_ctx, frame, decoder->rgb_dst_data,
                               decoder->rgb_dst_linesize) != 0) {
      return -1;
    }

    decoder->frame_data = decoder->rgb_dst_data;
    decoder->frame_linesize = decoder->rgb_dst_linesize;
  } else {
    decoder->frame_data = frame->extended_data;
  }

  return 0;
}