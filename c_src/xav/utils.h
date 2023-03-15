#include <erl_nif.h>
#include <libavcodec/avcodec.h>

void print_supported_pix_fmts(AVCodec *codec);
ERL_NIF_TERM xav_nif_raise(ErlNifEnv *env, char *msg);