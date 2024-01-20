# Env vars provided by elixir_make
# see https://hexdocs.pm/elixir_make/Mix.Tasks.Compile.ElixirMake.html#module-default-environment-variables
# ERTS_INCLUDE_DIR
# MIX_APP_PATH

XAV_DIR = c_src/xav
PRIV_DIR = $(MIX_APP_PATH)/priv
XAV_SO = $(PRIV_DIR)/libxav.so

FFMPEG_INCLUDE_DIR = ffmpeg_build/include
FFMPEG_LIB_DIR = ffmpeg_build/lib

# uncomment to compile with debug logs
# XAV_DEBUG_LOGS = -DXAV_DEBUG=1

HEADERS = $(XAV_DIR)/reader.h $(XAV_DIR)/decoder.h $(XAV_DIR)/utils.h
SOURCES = $(XAV_DIR)/xav_nif.c $(XAV_DIR)/reader.c $(XAV_DIR)/decoder.c $(XAV_DIR)/utils.c

ifeq ($(USE_BUNDLED_FFMPEG), true)
CFLAGS = -fPIC -I$(ERTS_INCLUDE_DIR) -I${FFMPEG_INCLUDE_DIR} -I${XAV_DIR} -shared $(XAV_DEBUG_LOGS)
LDFLAGS = -L$(FFMPEG_LIB_DIR) -Wl,--whole-archive -lavcodec -lswscale -lavutil -lavformat -Wl,--no-whole-archive 
else
CFLAGS = -fPIC -I$(ERTS_INCLUDE_DIR) -I${XAV_DIR} -shared $(XAV_DEBUG_LOGS)
LDFLAGS = -lavcodec -lswscale -lavutil -lavformat -lavdevice -lswresample
endif

$(XAV_SO): Makefile $(SOURCES) $(HEADERS)
	mkdir -p $(PRIV_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $(XAV_SO) $(LDFLAGS)

format:
	clang-format -i $(XAV_DIR)/*

.PHONY: format
