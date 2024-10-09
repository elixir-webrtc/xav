
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <stdint.h>

int video_converter_convert(AVFrame *src_frame, uint8_t *out_data[4], int out_linesize[4]);
