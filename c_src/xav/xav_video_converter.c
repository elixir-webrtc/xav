#include "xav_video_converter.h"

ErlNifResourceType * xav_video_converter_resource_type;

static int init_video_converter(struct XavVideoConverter *converter) {
  converter->vc = video_converter_alloc();
  if (converter->vc == NULL) {
    return -1;
  }

  AVFrame *in_frame = converter->frame;

  enum AVPixelFormat out_pix_fmt = converter->out_format;
  if (out_pix_fmt == AV_PIX_FMT_NONE) {
    out_pix_fmt = in_frame->format;
  }

  return video_converter_init(converter->vc, in_frame->width, in_frame->height, in_frame->format, 
                              converter->out_width,  converter->out_height, out_pix_fmt);
}

ERL_NIF_TERM new(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[]) {  
  if (argc != 3) {
    return xav_nif_error(env, "invalid_arg_count");
  }

  ERL_NIF_TERM ret;
  enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
  int width, height;
  char *format = NULL;

  if (!xav_get_atom(env, argv[0], &format)) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  if (strcmp(format, "nil") != 0) {
    pix_fmt = av_get_pix_fmt(format);
    if (pix_fmt == AV_PIX_FMT_NONE) {
      ret = xav_nif_raise(env, "unknown_format");
      goto clean;
    }
  }

  if (!enif_get_int(env, argv[1], &width)) {
    ret = xav_nif_raise(env, "failed_to_get_int");
    goto clean;
  }

  if (!enif_get_int(env, argv[2], &height)) {
    ret = xav_nif_raise(env, "failed_to_get_int");
    goto clean;
  }

  struct XavVideoConverter *xav_video_converter = enif_alloc_resource(xav_video_converter_resource_type, 
                                                                      sizeof(struct XavVideoConverter));
  xav_video_converter->vc = NULL;
  xav_video_converter->frame = av_frame_alloc();
  xav_video_converter->out_format = pix_fmt;
  xav_video_converter->out_width = width;
  xav_video_converter->out_height = height;

  ERL_NIF_TERM converter_term = enif_make_resource(env, xav_video_converter);
  enif_release_resource(xav_video_converter);

  ret = xav_nif_ok(env, converter_term);

clean:
  XAV_FREE(format);

  return ret;
}

ERL_NIF_TERM convert(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 5) {
    return xav_nif_raise(env, "invalid_arg_count");
  }

  struct XavVideoConverter *xav_video_converter;
  if (!enif_get_resource(env, argv[0], xav_video_converter_resource_type, (void**) &xav_video_converter)) {
    return xav_nif_raise(env, "couldnt_get_converter_resource");
  }

  ERL_NIF_TERM ret;
  ErlNifBinary in_data;
  int width, height;
  char *format = NULL;
  enum AVPixelFormat pix_fmt;

  if (!enif_inspect_binary(env, argv[1], &in_data)) {
    return xav_nif_raise(env, "failed_to_inspect_binary");
  }

  if (!enif_get_int(env, argv[2], &width)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  if (!enif_get_int(env, argv[3], &height)) {
    return xav_nif_raise(env, "failed_to_get_int");
  }

  if (!xav_get_atom(env, argv[4], &format)) {
    return xav_nif_raise(env, "failed_to_get_atom");
  }

  pix_fmt = av_get_pix_fmt(format);
  if (pix_fmt == AV_PIX_FMT_NONE) {
    ret = xav_nif_raise(env, "unknown_format");
    goto clean;
  }

  AVFrame *src_frame = xav_video_converter->frame;
  src_frame->width = width;
  src_frame->height = height;
  src_frame->format = pix_fmt;

  int int_ret = av_image_fill_arrays(src_frame->data, src_frame->linesize, in_data.data, 
                              src_frame->format, width, height, 1);

  if (int_ret < 0) { 
    ret = xav_nif_raise(env, "failed_to_fill_arrays");
    goto clean;
  }

  if (xav_video_converter->vc == NULL) {
    if (init_video_converter(xav_video_converter) < 0) {
      ret = xav_nif_raise(env, "failed_to_init_converter");
      goto clean;
    }
  }

  if (video_converter_convert(xav_video_converter->vc, src_frame) < 0) {
    ret = xav_nif_raise(env, "failed_to_convert");
    goto clean;
  }

  ret = xav_nif_video_frame_to_term(env, xav_video_converter->vc->dst_frame);

clean:
  if (format != NULL) XAV_FREE(format);

  return ret;
}

void free_xav_video_converter(ErlNifEnv * env, void * obj) {
  XAV_LOG_DEBUG("Freeing XavVideoConverter object");
  struct XavVideoConverter * xav_video_converter = (struct XavVideoConverter * ) obj;
  if (xav_video_converter->vc != NULL) {
    video_converter_free(&xav_video_converter->vc);
  }

  av_frame_free(&xav_video_converter->frame);
}

static ErlNifFunc xav_funcs[] = {{"new", 3, new}, 
                                 {"convert", 5, convert, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int load(ErlNifEnv * env, void ** priv, ERL_NIF_TERM load_info) {
  xav_video_converter_resource_type = 
    enif_open_resource_type(env, NULL, "XavVideoConverter", free_xav_video_converter, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.Xav.VideoConverter.NIF, xav_funcs, &load, NULL, NULL, NULL);