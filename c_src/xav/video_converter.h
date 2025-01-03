
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <stdint.h>

int video_converter_convert(AVFrame *src_frame, AVFrame **dst_frame, enum AVPixelFormat out_format);
