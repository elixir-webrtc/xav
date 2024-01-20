#include "reader.h"
#include "utils.h"

int reader_init(struct Reader *reader, char *path, size_t path_size, int device_flag,
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
    reader->swr_ctx = swr_alloc();
    enum AVSampleFormat out_sample_fmt = av_get_alt_sample_fmt(reader->c->sample_fmt, 0);
    av_opt_set_channel_layout(reader->swr_ctx, "in_channel_layout", reader->c->channel_layout, 0);
    av_opt_set_channel_layout(reader->swr_ctx, "out_channel_layout", reader->c->channel_layout, 0);
    av_opt_set_int(reader->swr_ctx, "in_sample_rate", reader->c->sample_rate, 0);
    av_opt_set_int(reader->swr_ctx, "out_sample_rate", reader->c->sample_rate, 0);
    av_opt_set_sample_fmt(reader->swr_ctx, "in_sample_fmt", reader->c->sample_fmt, 0);
    av_opt_set_sample_fmt(reader->swr_ctx, "out_sample_fmt", out_sample_fmt, 0);

    ret = swr_init(reader->swr_ctx);
    if (ret < 0) {
      return ret;
    }

    reader->in_format_name = av_get_sample_fmt_name(reader->c->sample_fmt);
    reader->out_format_name = av_get_sample_fmt_name(out_sample_fmt);
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
      XAV_LOG_DEBUG("Received frame");
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
  // convert to rgb
  if (reader->media_type == AVMEDIA_TYPE_VIDEO && reader->frame->format != AV_PIX_FMT_RGB24) {
    XAV_LOG_DEBUG("Converting to RGB");
    // clock_t begin = clock();

    convert_to_rgb(reader->frame, reader->rgb_dst_data, reader->rgb_dst_linesize);

    // clock_t end = clock();
    // double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    // fprintf(stdout, "time swscale: %f\n", time_spent);
    reader->frame_data = reader->rgb_dst_data;
    reader->frame_linesize = reader->rgb_dst_linesize;
  } else if (reader->media_type == AVMEDIA_TYPE_VIDEO) {
    reader->frame_data = reader->frame->data;
    reader->frame_linesize = reader->frame->linesize;
  } else if (reader->media_type == AVMEDIA_TYPE_AUDIO &&
             av_sample_fmt_is_planar(reader->frame->format) == 1) {
    // convert to interleaved
    int channels = reader->frame->channels;
    int samples_per_channel = reader->frame->nb_samples;

    // reader->frame_data = (uint8_t**)malloc(sizeof(uint8_t *));
    // reader->frame_data[0] = (uint8_t*)malloc(sizeof(uint8_t) *
    // av_get_bytes_per_sample(out_sample_fmt) * samples_per_channel * channels);
    ret = av_samples_alloc(&reader->rgb_dst_data[0], &reader->rgb_dst_linesize[0], channels,
                           samples_per_channel, reader->frame->format, 0);
    if (ret < 0) {
      return ret;
    }

    ret = swr_convert(reader->swr_ctx, &reader->rgb_dst_data[0], samples_per_channel,
                      (const uint8_t **)reader->frame->data, samples_per_channel);
    if (ret < 0) {
      return ret;
    }

    reader->frame_data = reader->rgb_dst_data;
    reader->frame_linesize = reader->rgb_dst_linesize;
  } else {
    reader->frame_data = reader->frame->extended_data;
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