#include <stdio.h>
#include <erl_nif.h>
#include <libavcodec/avcodec.h>

static ErlNifFunc xav_funcs[] = {};

ERL_NIF_INIT(Elixir.Xav.NIF, xav_funcs, NULL, NULL, NULL, NULL);