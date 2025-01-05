#include "xav_video_converter.h"

ErlNifResourceType * xav_video_converter_resource_type;

ERL_NIF_TERM new(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 4) {
    return xav_nif_error(env, "invalid_arg_count");
  }

  ERL_NIF_TERM ret;

  int in_width;
  if (!enif_get_int(env, argv[0], &in_width)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  int in_height;
  if (!enif_get_int(env, argv[1], &in_height)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  unsigned int in_format_len;
  if (!enif_get_atom_length(env, argv[2], &in_format_len, ERL_NIF_LATIN1)) {
    return xav_nif_raise(env, "failed_to_get_atom_length");
  }

  char *in_format = (char *)XAV_ALLOC((in_format_len + 1) * sizeof(char *));
  if (enif_get_atom(env, argv[2], in_format, in_format_len + 1, ERL_NIF_LATIN1) == 0) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  unsigned int out_format_len;
  if (!enif_get_atom_length(env, argv[3], &out_format_len, ERL_NIF_LATIN1)) {
    return xav_nif_raise(env, "failed_to_get_atom_length");
  }

  char *out_format = (char *)XAV_ALLOC((out_format_len * 1) * sizeof(char *));
  if (enif_get_atom(env, argv[3], out_format, out_format_len + 1, ERL_NIF_LATIN1) == 0) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  enum AVPixelFormat in_pix_fmt = av_get_pix_fmt(in_format);
  enum AVPixelFormat out_pix_fmt = av_get_pix_fmt(out_format);

  if (in_pix_fmt == AV_PIX_FMT_NONE || out_pix_fmt == AV_PIX_FMT_NONE) {
    ret = xav_nif_error(env, "unknown_format");
    goto fail;
  }

  struct XavVideoConverter *xav_video_converter = enif_alloc_resource(xav_video_converter_resource_type, 
                                                                      sizeof(xav_video_converter));
  xav_video_converter->vc = video_converter_alloc();
  if (video_converter_init(xav_video_converter->vc, in_width, in_height, in_pix_fmt, out_pix_fmt) < 0) {
    return xav_nif_raise(env, "failed_to_init_converter");
  }
  xav_video_converter->frame = av_frame_alloc();
  xav_video_converter->frame->format = in_pix_fmt;

  ERL_NIF_TERM converter_term = enif_make_resource(env, xav_video_converter);
  enif_release_resource(xav_video_converter);

  ret = xav_nif_ok(env, converter_term);

fail:
  XAV_FREE(in_format);
  XAV_FREE(out_format);

  return ret;
}

ERL_NIF_TERM convert(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 4) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavVideoConverter *xav_video_converter;
  if (!enif_get_resource(env, argv[0], xav_video_converter_resource_type, (void**) &xav_video_converter)) {
    return xav_nif_raise(env, "couldnt_get_converter_resource");
  }

  ErlNifBinary in_data;
  if (!enif_inspect_binary(env, argv[1], & in_data)) {
    return xav_nif_raise(env, "failed_to_inspect_binary");
  }

  int width;
  if (!enif_get_int(env, argv[2], &width)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  int height;
  if (!enif_get_int(env, argv[3], &height)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  int ret;
  AVFrame *src_frame = xav_video_converter->frame;
  src_frame->width = width;
  src_frame->height = height;

  ret = av_image_fill_arrays(src_frame->data, src_frame->linesize, in_data.data, 
                              xav_video_converter->vc->in_format, width, height, 1);


  if (ret < 0) { 
    return xav_nif_raise(env, "failed_to_fill_arrays");
  }

  ret = video_converter_convert(xav_video_converter->vc, src_frame);
  if (ret < 0) {
    return xav_nif_raise(env, "failed_to_convert");
  }

  return xav_nif_video_frame_to_term(env, xav_video_converter->vc->dst_frame);
}

void free_xav_video_converter(ErlNifEnv * env, void * obj) {
  XAV_LOG_DEBUG("Freeing XavVideoConverter object");
  struct XavVideoConverter * xav_video_converter = (struct XavVideoConverter * ) obj;
  if (xav_video_converter->vc != NULL) {
    video_converter_free(&xav_video_converter->vc);
  }

  av_frame_free(&xav_video_converter->frame);
}

static ErlNifFunc xav_funcs[] = {{"new", 4, new}, 
                                 {"convert", 4, convert, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv * env, void ** priv, ERL_NIF_TERM load_info) {
  xav_video_converter_resource_type = 
    enif_open_resource_type(env, NULL, "XavVideoConverter", free_xav_video_converter, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.VideoConverter.NIF, xav_funcs, &load, NULL, NULL, NULL);