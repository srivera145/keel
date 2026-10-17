# KEEL OS — build
CC      ?= cc
CFLAGS  ?= -O3 -march=native -ffast-math -fno-math-errno
CFLAGS  += -std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-format-truncation
LDLIBS  := -lm -lpthread

SRC := $(wildcard src/*.c)
X11 := $(shell pkg-config --exists x11 xext 2>/dev/null && echo yes || (test -f /usr/include/X11/extensions/XShm.h && echo yes))
ifeq ($(X11),yes)
  LDLIBS += -lX11 -lXext
else
  CFLAGS += -DKEEL_NO_X11
endif

OBJ := $(SRC:src/%.c=build/obj/%.o)

all: build/keelos

build/keelos: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/obj/%.o: src/%.c src/*.h | build/obj
	$(CC) $(CFLAGS) -c -o $@ $<

build/obj:
	mkdir -p $@

# Static build for running as PID 1 / on a bare framebuffer device.
static: CFLAGS += -DKEEL_NO_X11 -static
static: LDLIBS := -lm -lpthread
static: build/obj
	$(CC) $(CFLAGS) -o build/keelos-static $(SRC) $(LDLIBS)

test: build/keelos
	./tests/run_tests.sh

clean:
	rm -rf build

.PHONY: all static test clean
