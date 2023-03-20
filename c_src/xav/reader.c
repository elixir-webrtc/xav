#include "reader.h"

void convert_to_rgb(AVFrame *src_frame, uint8_t *dst_data[], int dst_linesize[]);

int reader_init(struct Reader *reader, char *path, size_t path_size) {
  reader->path = XAV_ALLOC(path_size + 1);
  memcpy(reader->path, path, path_size);
  reader->path[path_size] = '\0';

  // TODO which of those are really needed?
  reader->fmt_ctx = NULL;
  reader->codec = NULL;
  reader->c = NULL;
  reader->frame = NULL;
  reader->pkt = NULL;

  XAV_LOG_DEBUG("Trying to open %s", reader->path);

  if (avformat_open_input(&reader->fmt_ctx, reader->path, NULL, NULL) < 0) {
    return -1;
  }

  if (avformat_find_stream_info(reader->fmt_ctx, NULL) < 0) {
    return -2;
  }

  reader->stream_idx =
      av_find_best_stream(reader->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &reader->codec, 0);
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
  if (reader->frame->format != AV_PIX_FMT_RGB24) {
    XAV_LOG_DEBUG("Converting to RGB");
    // clock_t begin = clock();

    convert_to_rgb(reader->frame, reader->rgb_dst_data, reader->rgb_dst_linesize);

    // clock_t end = clock();
    // double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    // fprintf(stdout, "time swscale: %f\n", time_spent);
    reader->frame_data = reader->rgb_dst_data;
    reader->frame_linesize = reader->rgb_dst_linesize;
  } else {
    reader->frame_data = reader->frame->data;
    reader->frame_linesize = reader->frame->linesize;
  }

  return 0;
}

void reader_unref_frame(struct Reader *reader) { av_frame_unref(reader->frame); }

void reader_free(struct Reader *reader) {
  XAV_LOG_DEBUG("Freeing Reader object");
  avcodec_free_context(&reader->c);
  av_packet_free(&reader->pkt);
  av_frame_free(&reader->frame);
  avformat_close_input(&reader->fmt_ctx);
  XAV_FREE(reader->path);
}

void convert_to_rgb(AVFrame *src_frame, uint8_t *dst_data[], int dst_linesize[]) {
  struct SwsContext *sws_ctx =
      sws_getContext(src_frame->width, src_frame->height, src_frame->format, src_frame->width,
                     src_frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

  av_image_alloc(dst_data, dst_linesize, src_frame->width, src_frame->height, AV_PIX_FMT_RGB24, 1);

  // is this (const uint8_t * const*) cast really correct?
  sws_scale(sws_ctx, (const uint8_t *const *)src_frame->data, src_frame->linesize, 0,
            src_frame->height, dst_data, dst_linesize);
}
