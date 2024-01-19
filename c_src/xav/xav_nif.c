#include "reader.h"
#include "decoder.h"

ErlNifResourceType *reader_resource_type;
ErlNifResourceType *decoder_resource_type;

ERL_NIF_TERM new_reader(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {

  if (argc != 3) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[0], &bin)) {
    return xav_nif_raise(env, "invalid_path");
  }

  int device_flag;
  if (!enif_get_int(env, argv[1], &device_flag)) {
    return xav_nif_raise(env, "invalid_device_flag");
  }

  int media_type_flag;
  enum AVMediaType media_type;
  if (!enif_get_int(env, argv[2], &media_type_flag)) {
    return xav_nif_raise(env, "invalid_media_type_flag");
  }

  if (media_type_flag == 1) {
    media_type = AVMEDIA_TYPE_VIDEO;
  } else {
    media_type = AVMEDIA_TYPE_AUDIO;
  }

  struct Reader *reader = enif_alloc_resource(reader_resource_type, sizeof(struct Reader));

  int ret = reader_init(reader, bin.data, bin.size, device_flag, media_type);

  if (ret == -1) {
    return xav_nif_error(env, "couldnt_open_avformat_input");
  } else if (ret == -2) {
    return xav_nif_raise(env, "couldnt_create_new_reader");
  }

  ERL_NIF_TERM in_format_term = enif_make_atom(env, reader->in_format_name);
  ERL_NIF_TERM out_format_term = enif_make_atom(env, reader->out_format_name);

  ERL_NIF_TERM sample_rate_term;
  if (reader->media_type == AVMEDIA_TYPE_AUDIO) {
    sample_rate_term = enif_make_int(env, reader->c->sample_rate);
  }

  ERL_NIF_TERM bit_rate_term = enif_make_int64(env, reader->fmt_ctx->bit_rate);
  ERL_NIF_TERM duration_term = enif_make_int64(env, reader->fmt_ctx->duration / AV_TIME_BASE);
  ERL_NIF_TERM codec_term = enif_make_atom(env, reader->codec->name);

  ERL_NIF_TERM reader_term = enif_make_resource(env, reader);
  enif_release_resource(reader);

  ERL_NIF_TERM ok_term = enif_make_atom(env, "ok");

  if (reader->media_type == AVMEDIA_TYPE_AUDIO) {
    return enif_make_tuple(env, 8, ok_term, reader_term, in_format_term, out_format_term,
                           sample_rate_term, bit_rate_term, duration_term, codec_term);
  }

  return enif_make_tuple(env, 7, ok_term, reader_term, in_format_term, out_format_term,
                         bit_rate_term, duration_term, codec_term);
}

ERL_NIF_TERM next_frame(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct Reader *reader;
  if (!enif_get_resource(env, argv[0], reader_resource_type, (void **)&reader)) {
    return xav_nif_raise(env, "couldnt_get_reader_resource");
  }

  int ret = reader_next_frame(reader);

  if (ret == AVERROR_EOF) {
    return xav_nif_error(env, "eof");
  } else if (ret != 0) {
    return xav_nif_raise(env, "receive_frame");
  }

  XAV_LOG_DEBUG("Returning to Erlang");

  ERL_NIF_TERM frame_term;
  if (reader->media_type == AVMEDIA_TYPE_VIDEO) {
    frame_term = xav_nif_frame_to_term(env, reader->frame_data, reader->frame_linesize,
                                       reader->out_format_name, reader->frame->width,
                                       reader->frame->height, reader->frame->pts);
  } else if (reader->media_type == AVMEDIA_TYPE_AUDIO) {
    size_t unpadded_linesize = reader->frame->nb_samples *
                               av_get_bytes_per_sample(reader->frame->format) *
                               reader->frame->channels;
    ERL_NIF_TERM data_term;
    unsigned char *ptr = enif_make_new_binary(env, unpadded_linesize, &data_term);
    memcpy(ptr, reader->frame_data[0], unpadded_linesize);

    ERL_NIF_TERM samples_term = enif_make_int(env, reader->frame->nb_samples);
    ERL_NIF_TERM format_term = enif_make_atom(env, reader->out_format_name);
    ERL_NIF_TERM pts_term = enif_make_int(env, reader->frame->pts);

    frame_term = enif_make_tuple(env, 4, data_term, format_term, samples_term, pts_term);
  }

  reader_free_frame(reader);

  return xav_nif_ok(env, frame_term);
}

ERL_NIF_TERM new_decoder(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  struct Decoder *decoder = enif_alloc_resource(decoder_resource_type, sizeof(struct Decoder));

  decoder->codec = avcodec_find_decoder(AV_CODEC_ID_VP8);
  if (!decoder->codec){
    return xav_nif_raise(env, "decoder_not_found");
  }

  decoder->c = avcodec_alloc_context3(decoder->codec);
  if (!decoder->c) {
    return xav_nif_raise(env, "failed_to_alloc_context");
  }

  if (avcodec_open2(decoder->c, decoder->codec, NULL) <0){
    return xav_nif_raise(env, "failed_to_open_codec");
  }

  ERL_NIF_TERM decoder_term = enif_make_resource(env, decoder);
  enif_release_resource(decoder);

  return xav_nif_ok(env, decoder_term);
}

ERL_NIF_TERM decode(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 4) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct Decoder *decoder;
  if (!enif_get_resource(env, argv[0], decoder_resource_type, (void **)&decoder)) {
    return xav_nif_raise(env, "couldnt_get_decoder_resource");
  }

  ErlNifBinary data;
  if (!enif_inspect_binary(env, argv[1], &data)) {
    return xav_nif_raise(env, "couldnt_inspect_binary");
  }

  int pts;
  if (!enif_get_int64(env, argv[2], &pts)) {
    return xav_nif_raise(env, "couldnt_get_int");
  }

  int dts;
  if (!enif_get_int64(env, argv[3], &pts)) {
    return xav_nif_raise(env, "couldnt_get_int");
  }

  AVPacket *pkt = NULL;
  pkt = av_packet_alloc();

  if (!pkt) {
    return xav_nif_raise(env, "couldnt_alloc_av_packet");
  }

  pkt->data = data.data;
  pkt->size = data.size;
  pkt->pts = pts;
  pkt->dts = dts;

  // printf("size: %d\n", data.size);

  int ret;
  ret = avcodec_send_packet(decoder->c, pkt);
  if (ret != 0) {
    return enif_make_atom(env, "error");
  }

  // printf("send ret: %d\n", ret);

  AVFrame *frame = av_frame_alloc();
  
  if(!frame) {
    return xav_nif_raise(env, "couldnt_alloc_frame");
  }

  ret = avcodec_receive_frame(decoder->c, frame);
  if (ret != 0) {
    return xav_nif_raise(env, "failed_to_decode1");
  }

  // printf("%d %d %d %d %d\n", frame->width, frame->height, frame->format, frame->key_frame, frame->linesize);
  // ret = avcodec_receive_frame(decoder->c, frame);


  // if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
  //   return xav_nif_raise(env, "failed_to_decode2");
  // }

  uint8_t **frame_data;
  int *frame_linesize;
  uint8_t *rgb_dst_data[4];
  int rgb_dst_linesize[4];
  if (frame->format != AV_PIX_FMT_RGB24) {
    // printf("Converting to rgb\n");
    convert_to_rgb(frame, rgb_dst_data, rgb_dst_linesize);
    frame_data=rgb_dst_data;
    frame_linesize=rgb_dst_linesize;
  } else {
    frame_data = frame->data;
    frame_linesize = frame->linesize;
  }

  ERL_NIF_TERM frame_term = xav_nif_frame_to_term(env, frame_data, frame_linesize, "vp8", frame->width, frame->height, frame->pts);

  av_frame_free(&frame);
  return xav_nif_ok(env, frame_term);
}

void free_reader(ErlNifEnv *env, void *obj) {
  struct Reader *reader = (struct Reader *)obj;
  reader_free(reader);
}

void free_decoder(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing Decoder object");
  struct Decoder *decoder = (struct Decoder *)obj;
  if (decoder->c) {
    avcodec_free_context(&decoder->c);
  }
}

static ErlNifFunc xav_funcs[] = {{"new_reader", 3, new_reader},
                                 {"next_frame", 1, next_frame, ERL_NIF_DIRTY_JOB_CPU_BOUND},
                                 {"new_decoder", 0, new_decoder},
                                 {"decode", 4, decode, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  reader_resource_type =
      enif_open_resource_type(env, NULL, "Reader", free_reader, ERL_NIF_RT_CREATE, NULL);

  decoder_resource_type = 
      enif_open_resource_type(env, NULL, "Decoder", free_decoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.NIF, xav_funcs, &load, NULL, NULL, NULL);
