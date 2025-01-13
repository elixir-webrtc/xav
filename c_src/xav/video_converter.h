
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <stdint.h>

struct VideoConverter {
    struct SwsContext *sws_ctx;
    int in_width;
    int in_height;
    enum AVPixelFormat in_format;
    enum AVPixelFormat out_format;
    AVFrame *dst_frame;
};

struct VideoConverter *video_converter_alloc();

int video_converter_init(struct VideoConverter* converter, int in_width, int in_height, 
                         enum AVPixelFormat in_format, enum AVPixelFormat out_format);

int video_converter_convert(struct VideoConverter *converter, AVFrame *src_frame);

void video_converter_free(struct VideoConverter **converter);
