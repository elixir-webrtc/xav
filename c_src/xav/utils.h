#include <erl_nif.h>
#include <libavcodec/avcodec.h>

#ifdef XAV_DEBUG
#define XAV_LOG_DEBUG(X, ...)                                                                      \
  fprintf(stderr, "[XAV DEBUG %s] %s:%d " X "\n", __TIME__, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define XAV_LOG_DEBUG(...)
#endif

#define XAV_ALLOC(X) enif_alloc(X)
#define XAV_FREE(X) enif_free(X)

void print_supported_pix_fmts(AVCodec *codec);

ERL_NIF_TERM xav_nif_ok(ErlNifEnv *env, ERL_NIF_TERM data_term);
ERL_NIF_TERM xav_nif_error(ErlNifEnv *env, char *reason);
ERL_NIF_TERM xav_nif_raise(ErlNifEnv *env, char *msg);
ERL_NIF_TERM xav_nif_frame_to_term(ErlNifEnv *env, unsigned char *data[], int *linesize, int width,
                                   int height, int64_t pts);
