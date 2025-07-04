# Env vars provided by elixir_make
# see https://hexdocs.pm/elixir_make/Mix.Tasks.Compile.ElixirMake.html#module-default-environment-variables
# ERTS_INCLUDE_DIR
# MIX_APP_PATH

XAV_DIR = c_src/xav
PRIV_DIR = $(MIX_APP_PATH)/priv
XAV_DECODER_SO = $(PRIV_DIR)/libxavdecoder.so
XAV_ENCODER_SO = $(PRIV_DIR)/libxavencoder.so
XAV_READER_SO = $(PRIV_DIR)/libxavreader.so
XAV_VIDEO_CONVERTER_SO = $(PRIV_DIR)/libxavvideoconverter.so

# uncomment to compile with debug logs
# XAV_DEBUG_LOGS = -DXAV_DEBUG=1

DECODER_HEADERS = $(XAV_DIR)/xav_decoder.h $(XAV_DIR)/decoder.h $(XAV_DIR)/video_converter.h $(XAV_DIR)/audio_converter.h $(XAV_DIR)/utils.h $(XAV_DIR)/channel_layout.h
DECODER_SOURCES = $(XAV_DIR)/xav_decoder.c $(XAV_DIR)/decoder.c $(XAV_DIR)/video_converter.c $(XAV_DIR)/audio_converter.c $(XAV_DIR)/utils.c $(XAV_DIR)/channel_layout.c

ENCODER_HEADERS = $(XAV_DIR)/xav_encoder.h $(XAV_DIR)/encoder.h $(XAV_DIR)/utils.h $(XAV_DIR)/channel_layout.h
ENCODER_SOURCES = $(XAV_DIR)/xav_encoder.c $(XAV_DIR)/encoder.c $(XAV_DIR)/utils.c $(XAV_DIR)/channel_layout.c

READER_HEADERS = $(XAV_DIR)/xav_reader.h $(XAV_DIR)/reader.h $(XAV_DIR)/video_converter.h $(XAV_DIR)/audio_converter.h $(XAV_DIR)/utils.h $(XAV_DIR)/channel_layout.h
READER_SOURCES = $(XAV_DIR)/xav_reader.c $(XAV_DIR)/reader.c $(XAV_DIR)/video_converter.c $(XAV_DIR)/audio_converter.c $(XAV_DIR)/utils.c

VIDEO_CONVERTER_HEADERS = $(XAV_DIR)/xav_video_converter.h $(XAV_DIR)/video_converter.h $(XAV_DIR)/utils.h
VIDEO_CONVERTER_SOURCES = $(XAV_DIR)/xav_video_converter.c $(XAV_DIR)/video_converter.c $(XAV_DIR)/utils.c

CFLAGS += $(XAV_DEBUG_LOGS) -fPIC -shared
IFLAGS = -I$(ERTS_INCLUDE_DIR) -I$(XAV_DIR)
LDFLAGS = -lavcodec -lswscale -lavutil -lavformat -lavdevice -lswresample

# Flags for MacOS
ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -m),arm64)
		IFLAGS += $$(pkg-config --cflags-only-I libavcodec libswscale libavutil libavformat libavdevice libswresample)
		LFLAGS += $$(pkg-config --libs-only-L libavcodec libswscale libavutil libavformat libavdevice libswresample)
		CFLAGS += -undefined dynamic_lookup
	else
		CFLAGS += -undefined dynamic_lookup 
	endif
endif

# Flags for Fedora
ifneq (,$(wildcard /etc/fedora-release))
	IFLAGS += $$(pkg-config --cflags-only-I libavcodec libswscale libavutil libavformat libavdevice libswresample)
	LFLAGS += $$(pkg-config --libs-only-L libavcodec libswscale libavutil libavformat libavdevice libswresample)
endif

all: $(XAV_DECODER_SO) $(XAV_READER_SO) $(XAV_VIDEO_CONVERTER_SO) $(XAV_ENCODER_SO)

$(XAV_DECODER_SO): Makefile $(DECODER_SOURCES) $(DECODER_HEADERS)
	mkdir -p $(PRIV_DIR)
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(DECODER_SOURCES) -o $(XAV_DECODER_SO) $(LDFLAGS)

$(XAV_READER_SO): Makefile $(READER_SOURCES) $(READER_HEADERS)
	mkdir -p $(PRIV_DIR)
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(READER_SOURCES) -o $(XAV_READER_SO) $(LDFLAGS)

$(XAV_VIDEO_CONVERTER_SO): Makefile $(VIDEO_CONVERTER_SOURCES) $(VIDEO_CONVERTER_HEADERS)
	mkdir -p $(PRIV_DIR)
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(VIDEO_CONVERTER_SOURCES) -o $(XAV_VIDEO_CONVERTER_SO) $(LDFLAGS)


$(XAV_ENCODER_SO): Makefile $(ENCODER_SOURCES) $(ENCODER_HEADERS)
	mkdir -p $(PRIV_DIR)
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(ENCODER_SOURCES) -o $(XAV_ENCODER_SO) $(LDFLAGS)

format:
	clang-format -i $(XAV_DIR)/*

.PHONY: format
