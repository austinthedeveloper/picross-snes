PVSNESLIB_HOME ?= ../pvsneslib
CONFIG ?= debug
VERSION_NAME := 0.1.0
BUILD_ID ?= $(shell id=$$(git rev-parse --short HEAD 2>/dev/null || echo dev); \
	if test -n "$$(git status --porcelain 2>/dev/null)"; then \
		echo "$$id-dirty"; else echo "$$id"; fi)
ROMNAME := picross-$(VERSION_NAME)-$(CONFIG)-$(BUILD_ID)
ROMTITLE := PICROSS 94

# FastROM LoROM, 8 banks of 32 KiB (ADR-001); grow ROMBANKS and ROMSIZE
# together (ROMSIZE is log2 of the size in KiB: $08 = 256 KiB).
FASTROM := 1
ROMBANKS := 8
ROMSIZE := 08

# ROM + RAM + battery, 8 KiB SRAM: two save copies (ADR-003).
CARTRIDGETYPE := 02
SRAMSIZE := 03

export ROMNAME ROMTITLE FASTROM

# Sound (ADR-006): tools/sfx.py writes sfx.it; smconv turns it into a
# SNESMOD soundbank at bank 6 (one bank: under 32 KiB). -f checks it fits
# sound RAM.
AUDIOFILES := assets/generated/sfx.it
export SOUNDBANK := assets/generated/soundbank
SMCONVFLAGS := -s -o $(SOUNDBANK) -b 6 -f

CONFIG_TAG := $(CONFIG)
ifeq ($(CONFIG),debug)
CONFIG_TAG := DBG
endif
ifeq ($(CONFIG),release)
CONFIG_TAG := REL
endif

CFLAGS += -Iinclude -Iassets/generated -DBUILD_ID_STRING=\"$(VERSION_NAME)-$(CONFIG_TAG)-$(BUILD_ID)\"
ifeq ($(CONFIG),debug)
CFLAGS += -DBUILD_DEBUG=1
endif

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

.PHONY: all debug release rom assets check host-check asset-check tcc-check clean smoke import

all: debug

debug:
	@$(MAKE) --no-print-directory rom CONFIG=debug PVSNESLIB_DEBUG=0

release:
	@$(MAKE) --no-print-directory rom CONFIG=release PVSNESLIB_DEBUG=0

rom: assets
	@$(MAKE) --no-print-directory cleanBuildRes CONFIG=$(CONFIG)
	@rm -f $(ROMNAME).sfc $(ROMNAME).sym $(ROMNAME).symfull
	@$(MAKE) --no-print-directory $(SOUNDBANK).asm CONFIG=$(CONFIG)
	@$(MAKE) --no-print-directory $(ROMNAME).sfc CONFIG=$(CONFIG)
	@mkdir -p build/$(CONFIG)
	@rm -f build/$(CONFIG)/picross-*
	@# The linker writes to the repo root; move (not copy) so only build/ keeps ROMs.
	@mv $(ROMNAME).sfc build/$(CONFIG)/$(ROMNAME).sfc
	@mv $(ROMNAME).sym build/$(CONFIG)/$(ROMNAME).sym
	@mv $(ROMNAME).symfull build/$(CONFIG)/$(ROMNAME).map
	@echo "Built build/$(CONFIG)/$(ROMNAME).sfc"

assets:
	@python3 tools/gen_assets.py

# Re-import the pack from the web game (sibling checkout ../../picross).
import:
	@python3 tools/import_pack.py
	@python3 tools/gen_assets.py

asset-check:
	@python3 tools/gen_assets.py --check

# The core (no PPU, no pads) builds on the host for tests.
HOST_CORE := src/puzzle.c src/board.c src/history.c src/play.c src/save.c \
	src/progress.c src/stats.c assets/generated/pack_data.c test/host_sram.c
HOST_TESTS := core save
CC_HOST ?= cc

host-check: assets
	@mkdir -p build/host
	@for t in $(HOST_TESTS); do \
		$(CC_HOST) -std=c99 -Wall -Wextra -Werror -DHOST_TEST -Iinclude \
			-Iassets/generated test/test_$$t.c $(HOST_CORE) \
			-o build/host/test_$$t && build/host/test_$$t || exit 1; \
	done

# C that 816-tcc miscompiles (CLAUDE.md): the host tests can't see it.
tcc-check:
	@python3 tools/check_tcc_pitfalls.py

check: host-check asset-check tcc-check

MESEN_BIN ?= /Applications/Mesen.app/Contents/MacOS/Mesen
MESEN_SAVES ?= $(HOME)/Library/Application Support/MesenCE/Saves
SMOKE ?= boot
SMOKE_FRAMES ?= 600

# Headless MesenCE run of test/smoke/$(SMOKE).timeline against the debug ROM;
# screenshots land in build/smoke/$(SMOKE)/. Starts from a blank battery
# save unless KEEP_SAVE=1.
smoke: debug
	@rm -rf build/smoke/$(SMOKE) && mkdir -p build/smoke/$(SMOKE)
	@test "$(KEEP_SAVE)" = 1 || rm -f "$(MESEN_SAVES)/picross-$(VERSION_NAME)-debug-$(BUILD_ID).srm"
	@SMOKE_TIMELINE=$(CURDIR)/test/smoke/$(SMOKE).timeline \
		SMOKE_OUT=$(CURDIR)/build/smoke/$(SMOKE) SMOKE_FRAMES=$(SMOKE_FRAMES) \
		"$(MESEN_BIN)" --testRunner --doNotSaveSettings --noAudio \
		--debug.scriptWindow.allowIoOsAccess=true \
		$(CURDIR)/tools/mesen_smoke.lua \
		$(CURDIR)/build/debug/picross-$(VERSION_NAME)-debug-$(BUILD_ID).sfc
	@ls build/smoke/$(SMOKE) | wc -l | xargs echo "smoke screenshots:"

clean: cleanBuildRes cleanRom cleanLogs
	@rm -rf build assets/generated
	@rm -f picross-*.sfc picross-*.sym picross-*.symfull picross-*.log
