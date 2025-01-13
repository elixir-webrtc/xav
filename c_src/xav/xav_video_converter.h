#include "video_converter.h"
#include "utils.h"

struct XavVideoConverter {
    struct VideoConverter *vc;
    enum AVPixelFormat out_format;
    AVFrame* frame;
};