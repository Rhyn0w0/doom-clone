CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
CPPFLAGS += -Isrc
LDLIBS += -lm

# Raylib frontend (required). Install via: brew install raylib
RAY_CFLAGS := $(shell pkg-config --cflags raylib)
RAY_LIBS := $(shell pkg-config --libs raylib)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
ifeq ($(strip $(RAY_LIBS)),)
$(error raylib not found via pkg-config — install it, e.g. `brew install raylib`)
endif
endif
CPPFLAGS += $(RAY_CFLAGS)
LDLIBS += $(RAY_LIBS)

SRCS = src/main.c src/map.c src/tex.c src/render.c src/game.c
OBJS = $(SRCS:.c=.o)
BIN = doomc

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)

run: $(BIN)
	./$(BIN)

.PHONY: all clean run
