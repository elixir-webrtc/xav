#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <erl_nif.h>
#include <libavcodec/avcodec.h>

#define INBUF_SIZE (4096 + AV_INPUT_BUFFER_PADDING_SIZE)

struct Reader {
    char *path;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c;
    uint8_t inbuf[INBUF_SIZE];
    size_t offset;
};

ErlNifResourceType *reader_resource_type;

ERL_NIF_TERM new_reader(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {

    if (argc != 1) {
        return enif_make_atom(env, "error");
    }

    ErlNifBinary bin;
    if (!enif_inspect_binary(env, argv[0], &bin)) {
        return enif_make_atom(env, "error");
    }

    struct Reader *reader = enif_alloc_resource(reader_resource_type, sizeof(struct Reader));
    reader->path = enif_alloc(bin.size + 1);
    memcpy(reader->path, bin.data, bin.size);
    reader->path[bin.size] = '\0';
    
    fprintf(stdout, "trying to open %s\n", reader->path);

    reader->f = fopen(reader->path, "rb");
    if (!reader->f) {
        perror("open file");
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_open_file");
        return enif_raise_exception(env, reason);
    }

    reader->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!reader->codec) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_find_codec");
        return enif_raise_exception(env, reason); 
    }

    reader->parser = av_parser_init(reader->codec->id);
    if (!reader->parser) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_find_parser");
        return enif_raise_exception(env, reason);
    }

    reader->c = avcodec_alloc_context3(reader->codec);
    if (!reader->c) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_alloc_context");
        return enif_raise_exception(env, reason);
    }

    reader->frame = av_frame_alloc();
    if (!reader->c) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_alloc_frame");
        return enif_raise_exception(env, reason);
    }

    reader->offset = 0;
    reader->pkt = av_packet_alloc();
    if (!reader->c) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_alloc_packet");
        return enif_raise_exception(env, reason);
    }

    if (avcodec_open2(reader->c, reader->codec, NULL) < 0) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_open_codec");
        return enif_raise_exception(env, reason);
    }

    ERL_NIF_TERM reader_ref = enif_make_resource(env, reader);

    return reader_ref;
}

ERL_NIF_TERM next_frame(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    fprintf(stdout, "reading next frame\n");
    if (argc != 1) {
        return enif_make_atom(env, "error");
    }

    struct Reader *reader;
    if (!enif_get_resource(env, argv[0], reader_resource_type, (void **)&reader)) {
        ERL_NIF_TERM reason = enif_make_atom(env, "couldnt_get_reader_resource");
        return enif_raise_exception(env, reason);
    }    

    fprintf(stdout, "got reader\n");

    size_t data_size;
    int eof = 0;
    int frame_ready = 0;
    do {
        if (reader->offset == 0) {
            data_size = fread(reader->inbuf, 1, INBUF_SIZE, reader->f);
            if (ferror(reader->f)) {
                ERL_NIF_TERM reason = enif_make_atom(env, "read_error");
                return enif_raise_exception(env, reason);
            }
            eof = !data_size;
            fprintf(stdout, "read: %d\n", data_size);
            reader->offset = 0;
        } else {
            data_size = INBUF_SIZE - reader->offset;
            fprintf(stdout, "data_size %d\n", data_size);
        }

        fprintf(stdout, "trying to parse: %d\n", data_size);

        int ret = av_parser_parse2(reader->parser, reader->c, &reader->pkt->data, &reader->pkt->size,
            &reader->inbuf[reader->offset], data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        if (ret < 0) {
            ERL_NIF_TERM reason = enif_make_atom(env, "parse_error");
            return enif_raise_exception(env, reason);
        }

        fprintf(stdout, "parsed: %d\n", ret);

        reader->offset += ret;
        if (reader->offset == INBUF_SIZE) {
            reader->offset = 0;
        }

        if (reader->pkt->size) {
            if (avcodec_send_packet(reader->c, reader->pkt) < 0){
                ERL_NIF_TERM reason = enif_make_atom(env, "send_packet");
                return enif_raise_exception(env, reason);
            }

            int ret = avcodec_receive_frame(reader->c, reader->frame);

            if (ret == 0) {
             frame_ready = 1;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                continue; 
            } else if (ret < 0) {
                ERL_NIF_TERM reason = enif_make_atom(env, "receive_frame");
                return enif_raise_exception(env, reason);     
            }
        }
     } while(!eof && !frame_ready);

    printf("frame size: %d\n", reader->pkt->size);
    
    ERL_NIF_TERM ret_term;
    unsigned char *ptr= enif_make_new_binary(env, reader->frame->linesize[0], &ret_term);
    memcpy(ptr, reader->frame->data[0], reader->frame->linesize[0]);
    return ret_term;
}

static ErlNifFunc xav_funcs[] = {
    {"new_reader", 1, new_reader},
    {"next_frame", 1, next_frame}
};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
    reader_resource_type = enif_open_resource_type(env, NULL, "Reader", NULL, ERL_NIF_RT_CREATE, NULL);
    return 0;
}

ERL_NIF_INIT(Elixir.Xav.NIF, xav_funcs, &load, NULL, NULL, NULL);