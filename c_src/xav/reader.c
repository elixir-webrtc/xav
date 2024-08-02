#include "reader.h"
#include "utils.h"
#include <libavutil/samplefmt.h>
#include <libavutil/version.h>

#include <inttypes.h>

static int init_converter(struct Reader *reader);

int reader_init(struct Reader *reader, unsigned char *path, size_t path_size, int device_flag,
                enum AVMediaType media_type) {
  int ret;
  reader->path = XAV_ALLOC(path_size + 1);
  memcpy(reader->path, path, path_size);
  reader->path[path_size] = '\0';

  // TODO which of those are really needed?
  reader->fmt_ctx = NULL;
  reader->codec = NULL;
  reader->c = NULL;
  reader->frame = NULL;
  reader->pkt = NULL;
  reader->input_format = NULL;
  reader->options = NULL;
  reader->swr_ctx = NULL;
  reader->media_type = media_type;
  reader->in_format_name = NULL;
  reader->out_format_name = NULL;
  reader->out_data = NULL;
  reader->converter = NULL;

  if (device_flag == 1) {
    avdevice_register_all();
    reader->input_format = av_find_input_format("v4l2");
    av_dict_set(&reader->options, "framerate", "10", 0);
  }

  XAV_LOG_DEBUG("Trying to open %s", reader->path);

  if (avformat_open_input(&reader->fmt_ctx, reader->path, reader->input_format, NULL) < 0) {
    return -1;
  }

  if (avformat_find_stream_info(reader->fmt_ctx, NULL) < 0) {
    return -2;
  }

  reader->stream_idx = av_find_best_stream(reader->fmt_ctx, media_type, -1, -1, &reader->codec, 0);
  if (reader->stream_idx < 0) {
    return -2;
  }

  reader->c = avcodec_alloc_context3(reader->codec);
  if (!reader->c) {
    return -2;
  }

  // TODO why is this actually needed?
  if (avcodec_parameters_to_context(reader->c,
                                    reader->fmt_ctx->streams[reader->stream_idx]->codecpar) < 0) {
    return -2;
  }

  reader->frame = av_frame_alloc();
  if (!reader->frame) {
    return -2;
  }

  reader->pkt = av_packet_alloc();
  if (!reader->pkt) {
    return -2;
  }

  if (avcodec_open2(reader->c, reader->codec, NULL) < 0) {
    return -2;
  }

  if (reader->media_type == AVMEDIA_TYPE_AUDIO) {
    reader->in_format_name = "rgb";
    reader->out_format_name = "rgb";
  } else {
    reader->in_format_name = av_get_pix_fmt_name(reader->c->pix_fmt);
    reader->out_format_name = "rgb";
  }

  return 0;
}

int reader_next_frame(struct Reader *reader) {
  XAV_LOG_DEBUG("Trying to receive frame");

  int ret = avcodec_receive_frame(reader->c, reader->frame);

  if (ret == 0) {
    XAV_LOG_DEBUG("Received frame");
    goto fin;
  } else if (ret == AVERROR_EOF) {
    XAV_LOG_DEBUG("EOF");
    return ret;
  } else if (ret != AVERROR(EAGAIN)) {
    XAV_LOG_DEBUG("Error when trying to receive frame");
    return ret;
  } else {
    XAV_LOG_DEBUG("Need more data");
  }

  int frame_ready = 0;
  while (!frame_ready && (ret = av_read_frame(reader->fmt_ctx, reader->pkt)) >= 0) {

    if (reader->pkt->stream_index != reader->stream_idx) {
      continue;
    }

    XAV_LOG_DEBUG("Read packet from input. Sending to decoder");

    ret = avcodec_send_packet(reader->c, reader->pkt);
    if (ret < 0) {
      return ret;
    }

    // it's unclear when av_packet_unref should
    // be called - right after calling avcodec_send_packet
    // or after receiving the last decoded frame using
    // avcodec_receive_frame?
    //
    // according to docs for avcodec_send_packet
    //
    // Ownership of the packet remains with the caller,
    // and the decoder will not write to the packet.
    // The decoder may create a reference to the packet data
    // (or copy it if the packet is not reference-counted).
    // Unlike with older APIs, the packet is always fully consumed.
    //
    // so it sounds like we can call av_packet_unref
    // right after avcodec_send_packet as packet is always
    // fully consumed
    av_packet_unref(reader->pkt);

    XAV_LOG_DEBUG("Trying to receive frame");

    ret = avcodec_receive_frame(reader->c, reader->frame);

    if (ret == 0) {
      frame_ready = 1;
    } else if (ret == AVERROR_EOF) {
      XAV_LOG_DEBUG("EOF");
      return ret;
    } else if (ret != AVERROR(EAGAIN)) {
      XAV_LOG_DEBUG("Error when trying to receive frame");
      return ret;
    } else {
      XAV_LOG_DEBUG("Need more data");
    }
  }

  if (ret == AVERROR_EOF) {
    XAV_LOG_DEBUG("EOF. Flushing decoder");

    ret = avcodec_send_packet(reader->c, NULL);
    if (ret < 0) {
      return ret;
    }

    XAV_LOG_DEBUG("Trying to receive frame");
    ret = avcodec_receive_frame(reader->c, reader->frame);

    if (ret == AVERROR_EOF) {
      XAV_LOG_DEBUG("EOF");
      return ret;
    } else if (ret == AVERROR(EAGAIN)) {
      XAV_LOG_DEBUG("Need more data");
    } else if (ret < 0) {
      return ret;
    } else {
      XAV_LOG_DEBUG("Received frame");
    }
  }

fin:
  if (reader->media_type == AVMEDIA_TYPE_VIDEO && reader->frame->format != AV_PIX_FMT_RGB24) {
    XAV_LOG_DEBUG("Converting video to RGB");
    convert_to_rgb(reader->frame, reader->rgb_dst_data, reader->rgb_dst_linesize);
    reader->frame_data = reader->rgb_dst_data;
    reader->frame_linesize = reader->rgb_dst_linesize;
  } else if (reader->media_type == AVMEDIA_TYPE_VIDEO) {
    reader->frame_data = reader->frame->data;
    reader->frame_linesize = reader->frame->linesize;
  } else if (reader->media_type == AVMEDIA_TYPE_AUDIO) {
    XAV_LOG_DEBUG("Converting audio to desired out format");

    if (reader->converter == NULL) {
      XAV_LOG_DEBUG("Converter not initialized. Initializing.");
      ret = init_converter(reader);
      if (ret < 0) {
        return ret;
      }
    }

    return converter_convert(reader->converter, reader->frame, &reader->out_data,
                             &reader->out_samples, &reader->out_size);
  }

  return 0;
}

void reader_free_frame(struct Reader *reader) {
  av_frame_unref(reader->frame);
  if (reader->media_type == AVMEDIA_TYPE_AUDIO && reader->frame_data == reader->rgb_dst_data) {
    av_freep(&reader->frame_data[0]);
  } else if (reader->media_type == AVMEDIA_TYPE_VIDEO &&
             reader->frame_data == reader->rgb_dst_data) {
    av_freep(&reader->frame_data[0]);
  }

  if (reader->out_data != NULL) {
    free(reader->out_data);
    reader->out_data = NULL;
  }
}

void reader_free(struct Reader *reader) {
  XAV_LOG_DEBUG("Freeing Reader object");
  if (reader->swr_ctx != NULL) {
    swr_free(&reader->swr_ctx);
  }
  avcodec_free_context(&reader->c);
  av_packet_free(&reader->pkt);
  av_frame_free(&reader->frame);
  avformat_close_input(&reader->fmt_ctx);
  XAV_FREE(reader->path);
}

static int init_converter(struct Reader *reader) {
  reader->converter = (struct Converter *)calloc(1, sizeof(struct Converter));
  int out_sample_rate = 16000;
  enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLT;

  struct ChannelLayout in_chlayout, out_chlayout;
#if LIBAVUTIL_VERSION_MAJOR >= 58
  in_chlayout.layout = reader->c->ch_layout;
  av_channel_layout_from_mask(&out_chlayout.layout, AV_CH_LAYOUT_MONO);
#else
  in_chlayout.layout = reader->frame->channel_layout;
  out_chlayout.layout = AV_CH_LAYOUT_MONO;

  if (reader->frame->channel_layout == 0 && reader->frame->channels > 0) {
    // In newer FFmpeg versions, 0 means that the order of channels is
    // unspecified but there still might be information about channels number.
    // Let's check againts it and take default channel order for the given channels number.
    // This is also what newer FFmpeg versions do under the hood when passing
    // unspecified channel order.
    in_chlayout.layout = av_get_default_channel_layout(reader->frame->channels);
  } else if (reader->frame->channel_layout == 0) {
    XAV_LOG_DEBUG("Both channel layout and channels are unset. Cannot init converter.");
    return -1;
  }

#endif

  return converter_init(reader->converter, in_chlayout, reader->c->sample_rate,
                        reader->c->sample_fmt, out_chlayout, out_sample_rate, out_sample_fmt);
}