# KEEL OS — build
CC      ?= cc
MARCH   ?= native
CFLAGS  ?= -O3 -march=$(MARCH) -ffast-math -fno-math-errno
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

# Static, portable build for running as PID 1 on a bare framebuffer device.
static: build/obj
	$(CC) -O3 -march=x86-64-v2 -ffast-math -fno-math-errno -std=gnu11 -DKEEL_NO_X11 -static \
	    -o build/keelos-static $(SRC) -lm -lpthread

# Bootable initramfs: KEEL as /init on any Linux kernel with a framebuffer.
image: static
	./tools/mkimage.sh

test: build/keelos
	./tests/run_tests.sh

clean:
	rm -rf build

.PHONY: all static image test clean
