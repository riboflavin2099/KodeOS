# Host build. `make test` is the WAD scanner. `make engine` links Chocolate
# Doom, Heretic and Hexen and runs Doom headless.
ROOT := ..
CHOC := $(ROOT)/third_party/chocolate-doom/src
PORT := $(ROOT)/port
BUILD := build

CC := gcc
AR := ar
CFLAGS := -std=gnu17 -O2 -g -DDOOM_HOST -D_GNU_SOURCE -Wall -Wextra \
	-Wno-unused-parameter -Wno-unused-function -Wno-unused-variable \
	-Wno-unused-but-set-variable -Wno-sign-compare -Wno-pointer-sign \
	-Wno-misleading-indentation -Wno-override-init -Wno-unused-const-variable \
	-Wno-type-limits -Wno-absolute-value -Wno-implicit-fallthrough \
	-Wno-old-style-declaration -Wno-missing-field-initializers \
	-Wno-enum-conversion -Wno-shift-negative-value -Wno-format

COMMON_SRC := $(filter-out $(CHOC)/net_gui.c $(CHOC)/z_native.c $(CHOC)/d_dedicated.c,$(wildcard $(CHOC)/*.c))
DOOM_SRC := $(filter-out %_icon.c,$(wildcard $(CHOC)/doom/*.c))
HERETIC_SRC := $(filter-out %_icon.c,$(wildcard $(CHOC)/heretic/*.c))
HEXEN_SRC := $(filter-out %_icon.c,$(wildcard $(CHOC)/hexen/*.c))
PORT_SRC := $(wildcard $(PORT)/*.c)

COMMON_OBJ := $(patsubst $(CHOC)/%.c,$(BUILD)/common/%.o,$(COMMON_SRC))
DOOM_OBJ := $(patsubst $(CHOC)/doom/%.c,$(BUILD)/doom/%.o,$(DOOM_SRC))
HERETIC_OBJ := $(patsubst $(CHOC)/heretic/%.c,$(BUILD)/heretic/%.o,$(HERETIC_SRC))
HEXEN_OBJ := $(patsubst $(CHOC)/hexen/%.c,$(BUILD)/hexen/%.o,$(HEXEN_SRC))
PORT_OBJ := $(patsubst $(PORT)/%.c,$(BUILD)/port/%.o,$(PORT_SRC))

.PHONY: all engine test clean

all: engine test

engine: $(BUILD)/doom_engine

test: $(BUILD)/test_doom
	./$(BUILD)/test_doom

$(BUILD)/common/%.o: $(CHOC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(PORT) -I$(CHOC) -c $< -o $@

$(BUILD)/doom/%.o: $(CHOC)/doom/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(PORT) -I$(CHOC)/doom -I$(CHOC) -c $< -o $@

$(BUILD)/heretic/%.o: $(CHOC)/heretic/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(PORT) -I$(CHOC)/heretic -I$(CHOC) -c $< -o $@

$(BUILD)/hexen/%.o: $(CHOC)/hexen/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(PORT) -I$(CHOC)/hexen -I$(CHOC) -c $< -o $@

$(BUILD)/port/%.o: $(PORT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Werror -I$(PORT) -I$(CHOC) -c $< -o $@

$(BUILD)/libchoc.a: $(COMMON_OBJ) $(DOOM_OBJ)
	rm -f $@
	$(AR) rcs $@ $^

$(BUILD)/libheretic.a: $(HERETIC_OBJ)
	$(AR) rcs $@ $^
	python3 $(PORT)/prefix_objs.py --archive $@ --prefix heretic_

$(BUILD)/libhexen.a: $(HEXEN_OBJ)
	$(AR) rcs $@ $^
	python3 $(PORT)/prefix_objs.py --archive $@ --prefix hexen_

$(BUILD)/doom_engine: engine_run.c $(PORT_OBJ) $(BUILD)/libchoc.a $(BUILD)/libheretic.a $(BUILD)/libhexen.a
	$(CC) $(CFLAGS) -I$(PORT) -I$(CHOC) engine_run.c $(PORT_OBJ) \
		-Wl,--whole-archive $(BUILD)/libchoc.a $(BUILD)/libheretic.a $(BUILD)/libhexen.a \
		-Wl,--no-whole-archive -Wl,--export-dynamic -lm -o $@

$(BUILD)/test_doom: test_doom.c $(ROOT)/main/doom_wad.c $(ROOT)/main/doom_layout.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -I$(ROOT)/main \
		test_doom.c $(ROOT)/main/doom_wad.c $(ROOT)/main/doom_layout.c -o $@

clean:
	rm -rf $(BUILD)
