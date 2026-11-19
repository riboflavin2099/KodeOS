# Host build of the same Walnut glue the device runs. No ESP-IDF.
CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -fno-strict-aliasing
INCLUDES = -I../main -I../third_party/walnut-cgb -I../third_party/walnut-cgb/minigb_apu
DEFS     = -DGBC_HOST -DMINIGB_APU_AUDIO_FORMAT_S16SYS

CORE_SRC = ../main/gbc_core.c ../main/gbc_mix.c \
           ../third_party/walnut-cgb/minigb_apu/minigb_apu.c

.PHONY: all run header clean

all: test_run test_header

test_run: test_run.c $(CORE_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFS) -o $@ test_run.c $(CORE_SRC)

test_header: test_header.c ../main/gbc_header.c
	$(CC) $(CFLAGS) -I../main -o $@ test_header.c ../main/gbc_header.c

run: test_run
	./test_run $(ROM)

header: test_header
	./test_header

clean:
	rm -f test_run test_header frame_30.ppm frame_90.ppm
