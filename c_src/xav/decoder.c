#include "decoder.h"
#include "utils.h"

static int init_converter(struct Decoder *decoder);

struct Decoder *decoder_alloc() {
  struct Decoder *decoder = (struct Decoder *)XAV_ALLOC(sizeof(struct Decoder));

  decoder->codec = NULL;
  decoder->c = NULL;
  decoder->out_format_name = NULL;

  for (int i = 0; i < 4; i++) {
    decoder->rgb_dst_data[i] = NULL;
  }

  decoder->frame_data = NULL;
  decoder->frame_linesize = NULL;
  decoder->converter = NULL;
  decoder->out_data = NULL;

  return decoder;
}

int decoder_init(struct Decoder *decoder, const char *codec) {
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

    if (decoder->converter == NULL) {
      ret = init_converter(decoder);
      if (ret < 0) {
        return ret;
      }
    }

    return converter_convert(decoder->converter, frame, &decoder->out_data, &decoder->out_samples,
                             &decoder->out_size);
  }

  return 0;
}

void decoder_free_frame(struct Decoder *decoder) {
  // TODO revisit this
  av_frame_unref(decoder->frame);
  av_packet_unref(decoder->pkt);

  if (decoder->media_type == AVMEDIA_TYPE_AUDIO && decoder->frame_data == decoder->rgb_dst_data) {
    av_freep(&decoder->frame_data[0]);
  } else if (decoder->media_type == AVMEDIA_TYPE_VIDEO &&
             decoder->frame_data == decoder->rgb_dst_data) {
    av_freep(&decoder->frame_data[0]);
  }

  if (decoder->out_data != NULL) {
    // av_freep sets pointer to NULL
    av_freep(&decoder->out_data);
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

static int init_converter(struct Decoder *decoder) {
  decoder->converter = converter_alloc();

  if (decoder->converter == NULL) {
    XAV_LOG_DEBUG("Couldn't allocate converter");
    return -1;
  }

  int out_sample_rate = decoder->c->sample_rate;
  enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLT;

  struct ChannelLayout in_chlayout, out_chlayout;
#if LIBAVUTIL_VERSION_MAJOR >= 58
  in_chlayout.layout = decoder->c->ch_layout;
  out_chlayout.layout = decoder->c->ch_layout;
#else
  in_chlayout.layout = decoder->c->channel_layout;
  out_chlayout.layout = decoder->c->channel_layout;
  XAV_LOG_DEBUG("in_chlayout %ld", in_chlayout.layout);
  XAV_LOG_DEBUG("in nb_channels %d", av_get_channel_layout_nb_channels(in_chlayout.layout));
#endif

  return converter_init(decoder->converter, in_chlayout, decoder->c->sample_rate,
                        decoder->c->sample_fmt, out_chlayout, out_sample_rate, out_sample_fmt);
}
