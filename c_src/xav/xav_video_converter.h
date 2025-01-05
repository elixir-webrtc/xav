#include "video_converter.h"
#include "utils.h"

struct XavVideoConverter {
    struct VideoConverter *vc;
    AVFrame* frame;
};