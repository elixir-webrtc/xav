#include <erl_nif.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

void convert_to_rgb(AVFrame *frame, uint8_t *dst_data[], int dst_linesize[]);

struct Reader {
  char *path;
  AVFrame *frame;
  AVPacket *pkt;
  const AVCodec *codec;
  AVCodecContext *c;
  AVFormatContext *fmt_ctx;
  int stream_idx;
};

ErlNifResourceType *reader_resource_type;

ERL_NIF_TERM new_reader(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {

  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[0], &bin)) {
    return xav_nif_raise(env, "invalid_path");
  }

  struct Reader *reader = enif_alloc_resource(reader_resource_type, sizeof(struct Reader));
  reader->path = enif_alloc(bin.size + 1);
  memcpy(reader->path, bin.data, bin.size);
  reader->path[bin.size] = '\0';

  // TODO which of those are really needed?
  reader->fmt_ctx = NULL;
  reader->codec = NULL;
  reader->c = NULL;
  reader->frame = NULL;
  reader->pkt = NULL;

  XAV_LOG_DEBUG("Trying to open %s", reader->path);

  if (avformat_open_input(&reader->fmt_ctx, reader->path, NULL, NULL) < 0) {
    return xav_nif_raise(env, "couldnt_open_avformat_input");
  }

  if (avformat_find_stream_info(reader->fmt_ctx, NULL) < 0) {
    return xav_nif_raise(env, "couldnt_get_stream_info");
  }

  reader->stream_idx =
      av_find_best_stream(reader->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &reader->codec, 0);
  if (reader->stream_idx < 0) {
    return xav_nif_raise(env, "couldnt_find_best_stream");
  }

  reader->c = avcodec_alloc_context3(reader->codec);
  if (!reader->c) {
    return xav_nif_raise(env, "couldnt_alloc_codec_context");
  }

  // TODO why is this actually needed?
  if (avcodec_parameters_to_context(reader->c,
                                    reader->fmt_ctx->streams[reader->stream_idx]->codecpar) < 0) {
    return xav_nif_raise(env, "couldnt_copy_stream_params");
  }

  reader->frame = av_frame_alloc();
  if (!reader->frame) {
    return xav_nif_raise(env, "couldnt_alloc_frame");
  }

  reader->pkt = av_packet_alloc();
  if (!reader->pkt) {
    return xav_nif_raise(env, "couldnt_alloc_packet");
  }

  if (avcodec_open2(reader->c, reader->codec, NULL) < 0) {
    return xav_nif_raise(env, "couldnt_open_codec");
  }

  ERL_NIF_TERM ret_term = enif_make_resource(env, reader);
  enif_release_resource(reader);
  return ret_term;
}

ERL_NIF_TERM next_frame(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct Reader *reader;
  if (!enif_get_resource(env, argv[0], reader_resource_type, (void **)&reader)) {
    return xav_nif_raise(env, "couldnt_get_reader_resource");
  }

  XAV_LOG_DEBUG("Trying to receive frame");

  int ret = avcodec_receive_frame(reader->c, reader->frame);

  if (ret == 0) {
    XAV_LOG_DEBUG("Received frame");
    goto fin;
  } else if (ret == AVERROR_EOF) {
    XAV_LOG_DEBUG("EOF");
    return xav_nif_error(env, "eof");
  } else if (ret != AVERROR(EAGAIN)) {
    XAV_LOG_DEBUG("Error when trying to receive frame");
    return xav_nif_raise(env, "receive_frame");
  } else {
    XAV_LOG_DEBUG("Need more data");
  }

  int frame_ready = 0;
  while (!frame_ready && (ret = av_read_frame(reader->fmt_ctx, reader->pkt)) >= 0) {

    if (reader->pkt->stream_index != reader->stream_idx) {
      continue;
    }

    XAV_LOG_DEBUG("Read packet from input. Sending to decoder");

    if (avcodec_send_packet(reader->c, reader->pkt) < 0) {
      return xav_nif_raise(env, "send_packet");
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
      return xav_nif_error(env, "eof");
    } else if (ret != AVERROR(EAGAIN)) {
      XAV_LOG_DEBUG("Error when trying to receive frame");
      return xav_nif_raise(env, "receive_frame");
    } else {
      XAV_LOG_DEBUG("Need more data");
    }
  }

  if (ret == AVERROR_EOF) {
    XAV_LOG_DEBUG("EOF. Flushing decoder");

    if (avcodec_send_packet(reader->c, NULL) < 0) {
      return xav_nif_raise(env, "send_packet");
    }

    XAV_LOG_DEBUG("Trying to receive frame");
    ret = avcodec_receive_frame(reader->c, reader->frame);

    if (ret == AVERROR_EOF) {
      XAV_LOG_DEBUG("EOF");
      return xav_nif_error(env, "eof");
    } else if (ret == AVERROR(EAGAIN)) {
      XAV_LOG_DEBUG("Need more data");
    } else if (ret < 0) {
      return xav_nif_raise(env, "receive_frame");
    } else {
      XAV_LOG_DEBUG("Received frame");
    }
  }

fin:
  uint8_t **frame_data;
  int *frame_linesize;
  // convert to rgb
  if (reader->frame->format != AV_PIX_FMT_RGB24) {
    XAV_LOG_DEBUG("Converting to RGB");
    uint8_t *dst_data[4];
    int dst_linesize[4];
    // clock_t begin = clock();

    convert_to_rgb(reader->frame, dst_data, dst_linesize);

    // clock_t end = clock();
    // double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    // fprintf(stdout, "time swscale: %f\n", time_spent);
    frame_data = dst_data;
    frame_linesize = dst_linesize;
  } else {
    frame_data = reader->frame->data;
    frame_linesize = reader->frame->linesize;
  }

  XAV_LOG_DEBUG("Returning to Erlang");

  ERL_NIF_TERM frame_term =
      xav_nif_frame_to_term(env, frame_data, frame_linesize, reader->frame->width,
                            reader->frame->height, reader->frame->pts);

  av_frame_unref(reader->frame);

  return xav_nif_ok(env, frame_term);
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

static ErlNifFunc xav_funcs[] = {{"new_reader", 1, new_reader},
                                 {"next_frame", 1, next_frame, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

void free_reader(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing Reader object");
  struct Reader *reader = (struct Reader *)obj;
  avcodec_free_context(&reader->c);
  av_packet_free(&reader->pkt);
  av_frame_free(&reader->frame);
  avformat_close_input(&reader->fmt_ctx);
}

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  reader_resource_type =
      enif_open_resource_type(env, NULL, "Reader", free_reader, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.NIF, xav_funcs, &load, NULL, NULL, NULL);