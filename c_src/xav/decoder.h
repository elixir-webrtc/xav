#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

#include "audio_converter.h"
#include "channel_layout.h"
#include "utils.h"

#define MAX_FLUSH_BUFFER 16

/* FFmpeg ≤ 7 used FF_PROFILE_UNKNOWN, FFmpeg ≥ 8 uses AV_PROFILE_UNKNOWN. */
#if defined(AV_PROFILE_UNKNOWN)
#elif defined(FF_PROFILE_UNKNOWN)
#  define AV_PROFILE_UNKNOWN FF_PROFILE_UNKNOWN
#else
#  define AV_PROFILE_UNKNOWN (-99)
#endif

struct Decoder {
  enum AVMediaType media_type;
  AVFrame *frame;
  AVPacket *pkt;
  const AVCodec *codec;
  AVCodecContext *c;
};

struct Decoder *decoder_alloc();

int decoder_init(struct Decoder *decoder, const AVCodec *codec, int channels);

int decoder_decode(struct Decoder *decoder, AVPacket *pkt, AVFrame *frame);

int decoder_flush(struct Decoder *decoder, AVFrame **frames, int *frames_count);

void decoder_free_frame(struct Decoder *decoder);

void decoder_free(struct Decoder **decoder);
