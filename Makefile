# Env vars provided by elixir_make
# see https://hexdocs.pm/elixir_make/Mix.Tasks.Compile.ElixirMake.html#module-default-environment-variables
# ERTS_INCLUDE_DIR
# MIX_APP_PATH

XAV_DIR = c_src/xav
PRIV_DIR = $(MIX_APP_PATH)/priv
XAV_SO = $(PRIV_DIR)/libxav.so

# uncomment to compile with debug logs
# XAV_DEBUG_LOGS = -DXAV_DEBUG=1

HEADERS = $(XAV_DIR)/reader.h $(XAV_DIR)/decoder.h $(XAV_DIR)/converter.h $(XAV_DIR)/utils.h
SOURCES = $(XAV_DIR)/xav_nif.c $(XAV_DIR)/reader.c $(XAV_DIR)/decoder.c $(XAV_DIR)/converter.c $(XAV_DIR)/utils.c

CFLAGS = $(XAV_DEBUG_LOGS) -fPIC -shared
IFLAGS = -I$(ERTS_INCLUDE_DIR) -I$(XAV_DIR)
LDFLAGS = -lavcodec -lswscale -lavutil -lavformat -lavdevice -lswresample

ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -m),arm64)
		IFLAGS += $$(pkg-config --cflags-only-I libavcodec libswscale libavutil libavformat libavdevice libswresample)
		LFLAGS += $$(pkg-config --libs-only-L libavcodec libswscale libavutil libavformat libavdevice libswresample)
		CFLAGS += -undefined dynamic_lookup
	else
		CFLAGS += -undefined dynamic_lookup 
	endif
endif

$(XAV_SO): Makefile $(SOURCES) $(HEADERS)
	mkdir -p $(PRIV_DIR)
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(SOURCES) -o $(XAV_SO) $(LDFLAGS)

format:
	clang-format -i $(XAV_DIR)/*

.PHONY: format
