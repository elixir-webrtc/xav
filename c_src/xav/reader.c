#include "reader.h"
#include "utils.h"
#include <libavutil/samplefmt.h>
#include <libavutil/version.h>

#include <inttypes.h>

static int init_converter(struct Reader *reader);

struct Reader *reader_alloc() {

  struct Reader *reader = (struct Reader *)XAV_ALLOC(sizeof(struct Reader));

  reader->path = NULL;
  reader->frame = NULL;
  reader->pkt = NULL;
  reader->codec = NULL;
  reader->c = NULL;
  reader->fmt_ctx = NULL;
  reader->input_format = NULL;
  reader->options = NULL;

  return reader;
}

int reader_init(struct Reader *reader, unsigned char *path, size_t path_size, int device_flag,
                enum AVMediaType media_type) {
  int ret;
  reader->path = XAV_ALLOC(path_size + 1);
  memcpy(reader->path, path, path_size);
  reader->path[path_size] = '\0';

  reader->media_type = media_type;

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

  return 0;
}

int reader_next_frame(struct Reader *reader) {
  XAV_LOG_DEBUG("Trying to receive frame");

  int ret = avcodec_receive_frame(reader->c, reader->frame);

  if (ret == 0) {
    XAV_LOG_DEBUG("Received frame");
    return 0;
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

  return 0;
}

void reader_free_frame(struct Reader *reader) { av_frame_unref(reader->frame); }

void reader_free(struct Reader **reader) {
  XAV_LOG_DEBUG("Freeing Reader object");
  if (*reader != NULL) {
    struct Reader *r = *reader;

    if (r->c != NULL) {
      avcodec_free_context(&r->c);
    }

    if (r->pkt != NULL) {
      av_packet_free(&r->pkt);
    }

    if (r->frame != NULL) {
      av_frame_free(&r->frame);
    }

    if (r->fmt_ctx != NULL) {
      avformat_close_input(&r->fmt_ctx);
    }

    if (r->path != NULL) {
      XAV_FREE(r->path);
    }

    XAV_FREE(r);
    *reader = NULL;
  }
}
