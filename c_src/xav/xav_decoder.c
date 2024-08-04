#include "xav_decoder.h"

ErlNifResourceType *xav_decoder_resource_type;

ERL_NIF_TERM new(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 1) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  unsigned int codec_len;
  if (!enif_get_atom_length(env, argv[0], &codec_len, ERL_NIF_LATIN1)) {
    return xav_nif_raise(env, "failed_to_get_atom_length");
  }

  char *codec = (char *)calloc(codec_len + 1, sizeof(char *));

  if (enif_get_atom(env, argv[0], codec, codec_len + 1, ERL_NIF_LATIN1) == 0) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  struct XavDecoder *xav_decoder =
      enif_alloc_resource(xav_decoder_resource_type, sizeof(struct XavDecoder));
  xav_decoder->decoder = NULL;
  xav_decoder->converter = NULL;

  xav_decoder->decoder = (struct Decoder *)calloc(1, sizeof(struct Decoder));

  if (decoder_init(xav_decoder->decoder, codec) != 0) {
    return xav_nif_raise(env, "failed_to_init_decoder");
  }

  ERL_NIF_TERM decoder_term = enif_make_resource(env, xav_decoder);
  enif_release_resource(xav_decoder);

  return decoder_term;
}

ERL_NIF_TERM decode(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM term;

  if (argc != 4) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavDecoder *xav_decoder;
  if (!enif_get_resource(env, argv[0], xav_decoder_resource_type, (void **)&xav_decoder)) {
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

  int ret = decoder_decode(xav_decoder->decoder, pkt, frame);
  if (ret == -2) {
    term = xav_nif_error(env, "no_keyframe");
    goto cleanup;
  } else if (ret != 0) {
    term = xav_nif_raise(env, "failed_to_decode");
    goto cleanup;
  }

  ERL_NIF_TERM frame_term;
  if (xav_decoder->decoder->media_type == AVMEDIA_TYPE_VIDEO) {

    frame_term = xav_nif_video_frame_to_term(env, frame, xav_decoder->decoder->frame_data,
                                             xav_decoder->decoder->frame_linesize, "rgb");

  } else if (xav_decoder->decoder->media_type == AVMEDIA_TYPE_AUDIO) {
    const char *out_format =
        av_get_sample_fmt_name(xav_decoder->decoder->converter->out_sample_fmt);

    frame_term = xav_nif_audio_frame_to_term(
        env, xav_decoder->decoder->out_data, xav_decoder->decoder->out_samples,
        xav_decoder->decoder->out_size, out_format, frame->pts);
  }

  term = xav_nif_ok(env, frame_term);

cleanup:
  av_frame_free(&frame);
  av_packet_free(&pkt);

  if (xav_decoder->decoder->out_data != NULL) {
    free(xav_decoder->decoder->out_data);
    xav_decoder->decoder->out_data = NULL;
  }

  return term;
}

void free_xav_decoder(ErlNifEnv *env, void *obj) {
  XAV_LOG_DEBUG("Freeing XavDecoder object");
  struct XavDecoder *xav_decoder = (struct XavDecoder *)obj;
  if (xav_decoder->decoder != NULL) {
    decoder_free(xav_decoder->decoder);
  }

  if (xav_decoder->converter != NULL) {
    converter_free(xav_decoder->converter);
  }
}

static ErlNifFunc xav_funcs[] = {{"new", 1, new},
                                 {"decode", 4, decode, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  xav_decoder_resource_type =
      enif_open_resource_type(env, NULL, "XavDecoder", free_xav_decoder, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.Decoder.NIF, xav_funcs, &load, NULL, NULL, NULL);
