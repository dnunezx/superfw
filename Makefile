
VERSION_WORD := 0x00000015
VERSION_SLUG_WORD := $(shell git rev-parse --short=8 HEAD || echo FFFFFFFF)

PREFIX		:= arm-none-eabi-
CC		:= $(PREFIX)gcc
CXX		:= $(PREFIX)g++
CPP		:= $(PREFIX)cpp
OBJDUMP		:= $(PREFIX)objdump
OBJCOPY		:= $(PREFIX)objcopy

COMPRESSION_RATIO ?= 4
HOSTCXX ?= g++

GLOBAL_DEFINES = -D__GBA__

# BOARD can be "sd", "lite", "chis"
BOARD ?= sd

ifeq ($(BOARD),lite)
  GLOBAL_DEFINES += -DSUPERCARD_LITE_IO
  COMPRESS_FIRMWARE = 1
  MAXFSIZE = 496
  FWFLAVOUR = "Lite"
else ifeq ($(BOARD),sd)
  GLOBAL_DEFINES += -DSUPERCARD_FLASH_ADDRPERM
  BUNDLE_GBC_EMULATOR = 1
  COMPRESS_FIRMWARE = 1
  MAXFSIZE = 512
  FWFLAVOUR = "SD"
else ifeq ($(BOARD),chis)
  GLOBAL_DEFINES += -DSUPPORT_NORGAMES -DSUPERCHIS_IO -DFONTS_EXT
  BUNDLE_GBC_EMULATOR = 1
  BUNDLE_OTHER_EMULATORS = 1
  # Can't be over 2MiB (hardlimit)
  MAXFSIZE = 2048
  FWFLAVOUR = "Chis"
else
  $(error No valid board specified in BOARD)
endif

FWBINFILES=firmware.ewram.gba res/patches.db res/fonts.pack

ifeq ($(BOARD),chis)
  FWBINFILES=firmware.ewram.gba res/patches.db res/fonts-ext.pack
endif

ifeq ($(ENABLE_DISK_LOGGING),1)
  PAYLOADFLAGS += -DENABLE_DISK_LOGGING
endif
ifeq ($(ENABLE_EMU_LOGGING),1)
  PAYLOADFLAGS += -DENABLE_EMU_LOGGING
endif
ifeq ($(ENABLE_UART_LOGGING),1)
  PAYLOADFLAGS += -DENABLE_UART_LOGGING
endif

ifeq ($(COMPRESS_FIRMWARE),1)
  GLOBAL_DEFINES += -DCOMPRESS_FONTS -DCOMPRESS_PATCHES -DCOMPRESS_FIRMWARE
  FWBINFILES := $(addsuffix .comp,$(FWBINFILES))
endif

SUPERR7_UI_PAYLOAD=firmware.ui.ewram.gba
SUPERR7_UI_V3_PAYLOAD=firmware.ui.v3.ewram.gba
SUPERR7_PATCH_ASSET=res/patches.db
SUPERR7_FONT_ASSET=$(if $(filter chis,$(BOARD)),res/fonts-ext.pack,res/fonts.pack)

ifeq ($(COMPRESS_FIRMWARE),1)
  SUPERR7_UI_PAYLOAD := $(SUPERR7_UI_PAYLOAD).comp
  SUPERR7_UI_V3_PAYLOAD := $(SUPERR7_UI_V3_PAYLOAD).comp
  SUPERR7_PATCH_ASSET := $(SUPERR7_PATCH_ASSET).comp
  SUPERR7_FONT_ASSET := res/fonts.pack.comp
endif

ifeq ($(BUNDLE_GBC_EMULATOR),1)
  GLOBAL_DEFINES += -DBUNDLE_GBC_EMULATOR
  BIEMUFILES += emu/jagoombacolor_v0.5.gba.comp
endif

ifeq ($(BUNDLE_OTHER_EMULATORS),1)
  GLOBAL_DEFINES += -DBUNDLE_OTHER_EMULATOR
  BIEMUFILES += emu/pocketnes_20130701.gba.comp \
                emu/wasabigba_v0.2.4.gba.comp \
                emu/NGPGBA_v0.5.7.gba.comp \
                emu/pceadvance-v7.5-scptch.gba.comp
endif

BASEFLAGS=$(GLOBAL_DEFINES) -mcpu=arm7tdmi -mtune=arm7tdmi

CFLAGS=-O2 -ggdb \
       $(BASEFLAGS) $(PAYLOADFLAGS) \
       -DFW_MAX_SIZE_KB=$(MAXFSIZE) -DFW_FLAVOUR="\"$(FWFLAVOUR)\"" \
       -DSC_FAST_ROM_MIRROR="use_fast_mirror()" \
       -DSD_PREERASE_BLOCKS_WRITE \
       -DVERSION_WORD="$(VERSION_WORD)" \
       -DVERSION_SLUG_WORD="0x$(VERSION_SLUG_WORD)" \
       -Wall -Isrc -I. -mthumb -flto -flto-partition=none


INGAME_CFLAGS=-Os -ggdb \
              $(BASEFLAGS) \
              -DNO_SUPERCARD_INIT \
              -DSD_PREERASE_BLOCKS_WRITE \
              -Wall -Isrc -I. \
              -mthumb -flto

DLDI_CFLAGS=-Os -ggdb \
            $(BASEFLAGS) -DSDDRV_TIMEOUT_MULT=2 \
            -DSD_PREERASE_BLOCKS_WRITE \
            -Wall -Isrc -I. \
            -mthumb -flto -fPIC

DIRECTSAVE_CFLAGS=-Os -ggdb \
                 $(BASEFLAGS) -DSDDRV_TIMEOUT_MULT=16 \
                 -DNO_SUPERCARD_INIT \
                 -Wall -Isrc -I. \
                 -marm -flto -fPIC

FATFSFILES=fatfs/diskio.c \
           fatfs/ff.c \
           fatfs/ffsystem.c \
           fatfs/ffunicode.c

DLDIFILES=src/dldi.S \
          src/dldi_driver.c \
          src/crc.c \
          src/supercard_io.S \
          src/supercard_driver.c

DIRECTSAVEFILES=src/directsaver.S \
                src/directsave_emu.c \
                src/crc.c \
                src/supercard_driver.c

MENUFILES=src/ingame.S \
          src/ingame_menu.c \
          src/cimpl.c \
          src/fonts/font_render.c \
          src/save.c \
          src/util.c \
          src/utf_util.c \
          src/fileutil.c \
          src/crc.c \
          src/nanoprintf.c \
          src/supercard_io.S \
          src/supercard_driver.c \
          ${FATFSFILES}

INGAME_DEMO_INFILES=tests/ingame_menu_demo.c \
                    $(filter-out src/ingame.S,$(MENUFILES)) \
                    src/patches.S \
                    src/ui_theme.c

INFILES=src/gba_ewram_crt0.S \
        src/main.c \
        src/log.c \
        src/cimpl.c \
        src/settings.c \
        src/loader.c \
        src/save.c \
        src/patchengine.c \
        src/patcher.c \
        src/patches.S \
        src/menu.c \
        src/cover.c \
        src/recent.c \
        src/cheats.c \
        src/flash.c \
        src/sha256.c \
        src/misc.c \
        src/util.c \
        src/utf_util.c \
        src/emu.c \
        src/fileutil.c \
        src/asmutil.S \
        src/gbahw.c \
        src/virtfs.c \
        src/flash_mgr.c \
        src/binassets.S \
        src/crc.c \
        src/nds_loader.c \
        src/dldi_patcher.c \
        src/supercard_driver.c \
        src/supercard_io.S \
        src/heapsort.c \
        src/nanoprintf.c \
        src/fonts/font_render.c \
        ${FATFSFILES}

DEMO_INFILES=$(INFILES) src/ui_browser_v2.c src/ui_theme.c
UI_INFILES=$(INFILES) src/ui_browser_v2.c src/ui_theme.c

all:	$(FWBINFILES) $(BIEMUFILES) directsave.payload ingame_trampoline.payload
	# Wrap the firmware around a ROM->EWRAM loader
	$(CC) $(CFLAGS) -o firmware.elf rom_boot.S -T ldscripts/gba_romboot.ld -nostartfiles -nostdlib -Wl,--defsym,MAX_FLASH_SIZE=$(MAXFSIZE)K
	$(OBJCOPY) --output-target=binary firmware.elf superfw.gba
	# Fix the header/checksum.
	./tools/finalize_gba_image.py superfw.gba

superfw-ui.gba: $(SUPERR7_UI_PAYLOAD) $(SUPERR7_PATCH_ASSET) $(SUPERR7_FONT_ASSET) $(BIEMUFILES) directsave.payload ingame_trampoline.payload
	$(CC) $(CFLAGS) -DFW_EWRAM_PAYLOAD='"$(SUPERR7_UI_PAYLOAD)"' \
		-o firmware.ui.elf rom_boot.S -T ldscripts/gba_romboot.ld -nostartfiles -nostdlib \
		-Wl,--defsym,MAX_FLASH_SIZE=$(MAXFSIZE)K
	$(OBJCOPY) --output-target=binary firmware.ui.elf superfw-ui.gba
	./tools/finalize_gba_image.py superfw-ui.gba

superfw-ui-v3.gba: $(SUPERR7_UI_V3_PAYLOAD) $(SUPERR7_PATCH_ASSET) $(SUPERR7_FONT_ASSET) $(BIEMUFILES) directsave.payload ingame_trampoline.payload
	$(CC) $(CFLAGS) -DCOVER_ART_V3 -DFW_EWRAM_PAYLOAD='"$(SUPERR7_UI_V3_PAYLOAD)"' \
		-o firmware.ui.v3.elf rom_boot.S -T ldscripts/gba_romboot.ld -nostartfiles -nostdlib \
		-Wl,--defsym,MAX_FLASH_SIZE=$(MAXFSIZE)K
	$(OBJCOPY) --output-target=binary firmware.ui.v3.elf superfw-ui-v3.gba
	./tools/finalize_gba_image.py superfw-ui-v3.gba

# Official board-aware SuperR7 build. BOARD selects the cartridge-specific
# payload, fonts, I/O implementation, and flash-size ceiling.
superr7.gba: superfw-ui-v3.gba
	cp superfw-ui-v3.gba superr7.gba

cover-demo.gba: firmware.demo.ewram.gba src/cover_demo_boot.S ldscripts/gba_cover_demo.ld
	$(CC) $(CFLAGS) -DCOVER_ART_DEMO -o cover-demo.elf src/cover_demo_boot.S \
		-T ldscripts/gba_cover_demo.ld -nostartfiles -nostdlib
	$(OBJCOPY) --output-target=binary cover-demo.elf cover-demo.gba
	./tools/finalize_gba_image.py --header-only cover-demo.gba

cover-demo-v3.gba: firmware.demo.v3.ewram.gba src/cover_demo_boot.S ldscripts/gba_cover_demo.ld
	$(CC) $(CFLAGS) -DCOVER_ART_DEMO -DCOVER_ART_V3 \
		-DCOVER_DEMO_PAYLOAD='"firmware.demo.v3.ewram.gba"' \
		-o cover-demo-v3.elf src/cover_demo_boot.S \
		-T ldscripts/gba_cover_demo.ld -nostartfiles -nostdlib
	$(OBJCOPY) --output-target=binary cover-demo-v3.elf cover-demo-v3.gba
	./tools/finalize_gba_image.py --header-only cover-demo-v3.gba

ingame-menu-demo.gba: firmware.ingame.demo.ewram.gba src/cover_demo_boot.S ldscripts/gba_cover_demo.ld
	$(CC) $(CFLAGS) -DCOVER_ART_DEMO \
		-DCOVER_DEMO_PAYLOAD='"firmware.ingame.demo.ewram.gba"' \
		-o ingame-menu-demo.elf src/cover_demo_boot.S \
		-T ldscripts/gba_cover_demo.ld -nostartfiles -nostdlib
	$(OBJCOPY) --output-target=binary ingame-menu-demo.elf ingame-menu-demo.gba
	./tools/finalize_gba_image.py --header-only ingame-menu-demo.gba

firmware.ingame.demo.ewram.gba: $(INGAME_DEMO_INFILES) src/gba_ewram_crt0.S src/menu_messages.h ldscripts/gba_ewram.ld.i
	$(CC) $(CFLAGS) -DUI_BROWSER_V2 -DINGAME_MENU_DEMO \
		-o firmware.ingame.demo.ewram.elf src/gba_ewram_crt0.S $(INGAME_DEMO_INFILES) \
		-T ldscripts/gba_ewram.ld.i -nostartfiles \
		-Wl,-Map=firmware.ingame.demo.ewram.map \
		-Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.ingame.demo.ewram.elf firmware.ingame.demo.ewram.gba

firmware.demo.ewram.gba: $(DEMO_INFILES) ingamemenu.payload superfw.dldi.payload directsave.payload ingame_trampoline.payload src/messages_data.h ldscripts/gba_ewram.ld.i
	$(CC) $(CFLAGS) -DCOVER_ART_DEMO -DUI_BROWSER_V2 -o firmware.demo.ewram.elf $(DEMO_INFILES) \
		-T ldscripts/gba_ewram.ld.i -nostartfiles -Wl,-Map=firmware.demo.ewram.map \
		-Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.demo.ewram.elf firmware.demo.ewram.gba

firmware.demo.v3.ewram.gba: $(DEMO_INFILES) ingamemenu.payload superfw.dldi.payload directsave.payload ingame_trampoline.payload src/messages_data.h ldscripts/gba_ewram.ld.i
	$(CC) $(CFLAGS) -DCOVER_ART_DEMO -DCOVER_ART_V3 -DUI_BROWSER_V2 -o firmware.demo.v3.ewram.elf $(DEMO_INFILES) \
		-T ldscripts/gba_ewram.ld.i -nostartfiles -Wl,-Map=firmware.demo.v3.ewram.map \
		-Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.demo.v3.ewram.elf firmware.demo.v3.ewram.gba

firmware.ui.ewram.gba: $(UI_INFILES) ingamemenu.payload superfw.dldi.payload directsave.payload ingame_trampoline.payload src/messages_data.h ldscripts/gba_ewram.ld.i
	$(CC) $(CFLAGS) -DUI_BROWSER_V2 -o firmware.ui.ewram.elf $(UI_INFILES) \
		-T ldscripts/gba_ewram.ld.i -nostartfiles -Wl,-Map=firmware.ui.ewram.map \
		-Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.ui.ewram.elf firmware.ui.ewram.gba

firmware.ui.v3.ewram.gba: $(UI_INFILES) ingamemenu.payload superfw.dldi.payload directsave.payload ingame_trampoline.payload src/messages_data.h ldscripts/gba_ewram.ld.i
	$(CC) $(CFLAGS) -DCOVER_ART_V3 -DUI_BROWSER_V2 -o firmware.ui.v3.ewram.elf $(UI_INFILES) \
		-T ldscripts/gba_ewram.ld.i -nostartfiles -Wl,-Map=firmware.ui.v3.ewram.map \
		-Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.ui.v3.ewram.elf firmware.ui.v3.ewram.gba

firmware.ewram.gba: $(INFILES) ingamemenu.payload superfw.dldi.payload directsave.payload ingame_trampoline.payload src/messages_data.h ldscripts/gba_ewram.ld.i
	# Build the actual firmware image
	$(CC) $(CFLAGS) -o firmware.ewram.elf $(INFILES) -T ldscripts/gba_ewram.ld.i -nostartfiles -Wl,-Map=firmware.ewram.map -Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.ewram.elf firmware.ewram.gba

superfw.dldi.payload:	$(DLDIFILES)
	# Build in-game menu
	$(CC) $(DLDI_CFLAGS) -o superfw.dldi.elf $(DLDIFILES) -T ldscripts/gba_dldi.ld \
			-nostartfiles -fno-builtin -Wl,-Map=superfw.dldi.map -Wl,--print-memory-usage
	$(OBJCOPY) --output-target=binary superfw.dldi.elf superfw.dldi.payload

directsave.payload:	$(DIRECTSAVEFILES)
	$(CC) $(DIRECTSAVE_CFLAGS) -o directsave.elf $(DIRECTSAVEFILES) -T ldscripts/gba_directsave.ld \
			-nostartfiles -fno-builtin -Wl,-Map=directsave.map -Wl,--print-memory-usage
	$(OBJCOPY) --output-target=binary directsave.elf directsave.payload

ingamemenu.payload:	$(MENUFILES) src/menu_messages.h
	# Build in-game menu
	$(CC) $(INGAME_CFLAGS) -o ingamemenu.elf $(MENUFILES) -T ldscripts/gba_ingame.ld \
			-nostartfiles -fno-builtin -Wl,-Map=firmware.ingame.map -Wl,--print-memory-usage
	$(OBJCOPY) --output-target=binary ingamemenu.elf ingamemenu.payload

ingame_trampoline.payload:	src/ingame_trampoline.S
	$(CC) $(BASEFLAGS) -nostartfiles -T ldscripts/gba_ingametramp.ld -o ingame_trampoline.elf src/ingame_trampoline.S
	$(OBJCOPY) --output-target=binary ingame_trampoline.elf ingame_trampoline.payload

src/messages_data.h:	res/messages.py
	./res/messages.py h main > src/messages_data.h

src/menu_messages.h:	res/messages.py
	./res/messages.py h menu > src/menu_messages.h

firmware.ewram.gba.comp:	firmware.ewram.gba ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

firmware.ui.ewram.gba.comp:	firmware.ui.ewram.gba ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

firmware.ui.v3.ewram.gba.comp:	firmware.ui.v3.ewram.gba ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

%.gba.comp:	%.gba.bin ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

%.db.comp:	%.db ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

%.pack.comp:	%.pack apultra.elf
	./apultra.elf $< $@

%.ld.i:	%.ld
	$(CPP) $< -o $@

apultra.elf:	tools/apultra.cc
	$(HOSTCXX) -std=c++20 -O3 $< -o $@

upkr.elf:	tools/upkr.cc
	$(HOSTCXX) -o $@ $< -O3 -ffast-math

clean:
	rm -f ldscripts/*.i *.gba *.elf *.payload *.map res/*.comp emu/*.comp *.comp src/menu_messages.h src/messages_data.h

