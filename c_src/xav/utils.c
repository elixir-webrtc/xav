#include <libavcodec/avcodec.h>

void print_supported_pix_fmts(AVCodec *codec) {
  if (codec->pix_fmts == NULL) {
    fprintf(stdout, "unknown\n");
  } else {
    for (int i = 0; codec->pix_fmts[i] != -1; i++) {
      fprintf(stdout, "fmt: %d\n", codec->pix_fmts[i]);
    }
  }
}