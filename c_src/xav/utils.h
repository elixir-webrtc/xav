#include <erl_nif.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef XAV_DEBUG
#define XAV_LOG_DEBUG(X, ...)                                                                      \
  fprintf(stderr, "[XAV DEBUG %s] %s:%d " X "\n", __TIME__, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define XAV_LOG_DEBUG(...)
#endif

#define XAV_ALLOC(X) enif_alloc(X)
#define XAV_FREE(X) enif_free(X)
ERL_NIF_TERM xav_nif_ok(ErlNifEnv *env, ERL_NIF_TERM data_term);
ERL_NIF_TERM xav_nif_error(ErlNifEnv *env, char *reason);
ERL_NIF_TERM xav_nif_raise(ErlNifEnv *env, char *msg);
ERL_NIF_TERM xav_nif_video_frame_to_term(ErlNifEnv *env, AVFrame *frame, uint8_t *out_data[4],
                                         int out_linesize[4], const char *out_format);
ERL_NIF_TERM xav_nif_audio_frame_to_term(ErlNifEnv *env, uint8_t **out_data, int out_samples,
                                         int out_size, const char *out_format, int pts);
