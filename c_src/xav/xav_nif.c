#include "decoder.h"
#include "reader.h"

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
    frame_term = xav_nif_video_frame_to_term(env, reader->frame, reader->frame_data,
                                             reader->frame_linesize, reader->out_format_name);
  } else if (reader->media_type == AVMEDIA_TYPE_AUDIO) {
    frame_term = xav_nif_audio_frame_to_term(env, reader->frame, reader->frame_data,
                                             reader->out_format_name);
  }

  reader_free_frame(reader);

  return xav_nif_ok(env, frame_term);
}

ERL_NIF_TERM new_decoder(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct Decoder *decoder = enif_alloc_resource(decoder_resource_type, sizeof(struct Decoder));

  int codec_len;
  if (!enif_get_atom_length(env, argv[0], &codec_len, ERL_NIF_LATIN1)) {
    return xav_nif_raise(env, "failed_to_get_atom_length");
  }

  char *codec = (char *)calloc(codec_len + 1, sizeof(char *));

  if (enif_get_atom(env, argv[0], codec, codec_len + 1, ERL_NIF_LATIN1) == 0) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  if (decoder_init(decoder, codec) != 0) {
    return xav_nif_raise(env, "failed_to_init_decoder");
  }

  ERL_NIF_TERM decoder_term = enif_make_resource(env, decoder);
  enif_release_resource(decoder);

  return decoder_term;
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
  if (!enif_get_int(env, argv[2], &pts)) {
    return xav_nif_raise(env, "couldnt_get_int");
  }

  int dts;
  if (!enif_get_int(env, argv[3], &pts)) {
    return xav_nif_raise(env, "couldnt_get_int");
  }

  AVPacket *pkt = av_packet_alloc();
  if (!pkt) {
    return xav_nif_raise(env, "couldnt_alloc_av_packet");
  }

  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    return xav_nif_raise(env, "couldnt_alloc_frame");
  }

  pkt->data = data.data;
  pkt->size = data.size;
  pkt->pts = pts;
  pkt->dts = dts;

  if (decoder_decode(decoder, pkt, frame) != 0) {
    return xav_nif_raise(env, "failed_to_decode");
  }

  ERL_NIF_TERM frame_term;
  if (decoder->media_type == AVMEDIA_TYPE_VIDEO) {

    frame_term = xav_nif_video_frame_to_term(env, frame, decoder->frame_data,
                                             decoder->frame_linesize, "rgb");

  } else if (decoder->media_type == AVMEDIA_TYPE_AUDIO) {
    frame_term =
        xav_nif_audio_frame_to_term(env, frame, decoder->frame_data, decoder->out_format_name);
  }

  av_frame_free(&frame);
  av_packet_free(&pkt);
  return xav_nif_ok(env, frame_term);
}

void free_reader(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing Reader object");
  struct Reader *reader = (struct Reader *)obj;
  reader_free(reader);
}

void free_decoder(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing Decoder object");
  struct Decoder *decoder = (struct Decoder *)obj;
  decoder_free(decoder);
}

static ErlNifFunc xav_funcs[] = {{"new_reader", 3, new_reader},
                                 {"next_frame", 1, next_frame, ERL_NIF_DIRTY_JOB_CPU_BOUND},
                                 {"new_decoder", 1, new_decoder},
                                 {"decode", 4, decode, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  reader_resource_type =
      enif_open_resource_type(env, NULL, "Reader", free_reader, ERL_NIF_RT_CREATE, NULL);

  decoder_resource_type =
      enif_open_resource_type(env, NULL, "Decoder", free_decoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.NIF, xav_funcs, &load, NULL, NULL, NULL);
