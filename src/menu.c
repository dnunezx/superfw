/*
 * Copyright (C) 2024 David Guillen Fandos <david@davidgf.net>
 * Copyright (C) 2026 Danny Nunez (dnunezx)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "compiler.h"
#include "cover.h"
#include "superr7.h"
#ifdef UI_BROWSER_V2
#include "menu_nav.h"
#include "ui_browser_v2.h"
#endif
#include "gbahw.h"
#include "patchengine.h"
#include "fatfs/ff.h"
#include "common.h"
#include "settings.h"
#include "util.h"
#include "utf_util.h"
#include "fonts/font_render.h"
#include "nanoprintf.h"
#include "messages.h"
#include "save.h"
#include "cheats.h"
#include "ingame.h"
#include "emu.h"
#include "recent.h"
#include "flash_mgr.h"
#include "flash.h"
#include "sha256.h"
#include "supercard_driver.h"

#include "res/icons.h"
#ifndef UI_BROWSER_V2
#include "res/logo.h"
#endif

extern t_card_info sd_info;
extern bool fastew;
extern bool slowsd;

enum {
#ifdef UI_BROWSER_V2
  MENUTAB_FAVORITES,     // Phase 4 destination; library support follows later.
#endif
  MENUTAB_RECENT,        // Browses recently loaded ROMs (can be disabled / hidden)
  MENUTAB_ROMBROWSE,     // Browses ROMs and launches games.
  #ifdef SUPPORT_NORGAMES
  MENUTAB_NORBROWSE,     // Browses Flash games and launches them.
  #endif
  MENUTAB_SETTINGS,      // General settings / defaults
  MENUTAB_UILANG,        // UI / Language settings
#ifdef UI_BROWSER_V2
  MENUTAB_APPEARANCE,    // Wallpaper, preset and safe derived colors.
#endif
  MENUTAB_TOOLS,         // Tools (advaned menu)
  MENUTAB_INFO,          // Info / About / Updater?
  MENUTAB_MAX,
};

#define ANIM_INITIAL_WAIT     128    // Intial wait (in anim cycles)

enum {
  POPUP_NONE,
  POPUP_GBA_LOAD,              // Load a GBA ROM
  POPUP_SAVFILE,               // Load/Store a SAV file
  POPUP_FWFLASH,               // Flash a new firmware image
  POPUP_FILE_MGR,              // Write ROM to flash, delete, hide/unhide...
#ifdef SUPPORT_NORGAMES
  POPUP_GBA_NORWRITE,          // Write a GBA ROM to NOR
  POPUP_GBA_NORLOAD,           // Launch a NOR game
#endif
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
  POPUP_BROWSER_VIEW,          // Browse sorting and file visibility.
#endif
};

#ifdef UI_BROWSER_V2
#define BROWSER_ROWS UI_BROWSER_V2_ROWS
#else
#define BROWSER_ROWS                 8
#endif
#ifdef UI_BROWSER_V2
#define RECENT_ROWS UI_BROWSER_V2_ROWS
#define FAVORITES_ROWS UI_BROWSER_V2_ROWS
#else
#define RECENT_ROWS                  9
#endif
#define NORGAMES_ROWS                8

#ifdef COVER_ART_V3
#define COVER_PANEL_LEFT           160
#else
#define COVER_PANEL_LEFT           164
#endif
#define COVER_PANEL_TOP             42
#define COVER_IMAGE_LEFT           (COVER_PANEL_LEFT + 2)
#define COVER_IMAGE_TOP            (COVER_PANEL_TOP + 2)
#define COVER_LIST_RIGHT           COVER_PANEL_LEFT
#define COVER_SELECTOR_RIGHT       160

_Static_assert (COVER_IMAGE_LEFT + COVER_WIDTH <= SCREEN_WIDTH, "cover exceeds screen width");
_Static_assert (COVER_IMAGE_TOP + COVER_HEIGHT <= SCREEN_HEIGHT, "cover exceeds screen height");

// First entries reserved for the logo palette.
#define FG_COLOR         16
#define BG_COLOR         17
#define FT_COLOR         18
#define HI_COLOR         19
#define IGM_PAL_FG      240
#define IGM_PAL_BG      241
#define IGM_PAL_HI      242
#define IGM_PAL_SH      243
#define IGM_PAL_BL      244
#define SEL_COLOR       255

#define FLASH_UNLOCK_KEYS      (KEY_BUTTDOWN|KEY_BUTTB|KEY_BUTTSTA)
#define FLASH_GO_KEYS          (KEY_BUTTUP|KEY_BUTTL|KEY_BUTTR)

enum {
  UiSetLang  = 0,
  UiSetRect  = 1,
  UiSetASpd  = 2,
  UiSetHid   = 3,
  UiSetSave  = 4,
  UiSetMAX   = 4,
  UiSetCNT   = 5,
};

#ifdef UI_BROWSER_V2
enum {
  InfoSystem = 0,
  InfoMaintainer,
  InfoBasedOn,
  InfoBuild,
  InfoFlashDevice,
  InfoFlashCapacity,
  InfoUpdater,
  InfoPatchDatabase,
  InfoPatchCount,
  InfoSDType,
  InfoSDCapacity,
  InfoSDCardId,
  InfoCNT,
};
#endif

enum {
#ifdef UI_BROWSER_V2
  ToolsAppearance = 0,
  ToolsInterface,
  ToolsSettings,
  ToolsInfo,
  ToolsResetFavorites,
  ToolsResetRecent,
  ToolsSDRAMTest,
#else
  ToolsSDRAMTest = 0,
#endif
  ToolsSRAMTest,
  ToolsBatteryTest,
  ToolsSDBench,
  ToolsFlashBak,
  #ifdef SUPPORT_NORGAMES
  ToolsFlashClr,
  #endif
  ToolsMAX,
};

#ifdef UI_BROWSER_V2
enum {
  AppearancePreset = 0,
  AppearanceWallpaper,
  AppearanceBackground,
  AppearanceAccent,
  AppearanceSelection,
  AppearanceContrast,
  AppearanceReset,
  AppearanceMAX = AppearanceReset,
};
#endif

enum {
  SettTitle1 = 0,
  SettHotkey,
  SettBootType,
  SettFastSD,
  #ifdef SUPPORT_NORGAMES
  SettVerifyNOR,
  #endif
  SettFastEWRAM,
  SettSaveLoc,
  #ifdef SUPPORT_NORGAMES
  SettSaveLocNOR,
  #endif
  SettSaveBkp,
  SettStateLoc,
  SettCheatEn,
  SettTitle2,
  DefsPatchEng,
  DefsGamMenu,
  DefsRTCEnb,
  DefsRTCVal,
  DefsRTCSpeed,
  DefsLoadPol,
  DefsSavePol,
  DefsPrefDS,
  SettSave,
  SettMAX
};

enum {
  DefsSave     = 4,
  DefsMAX      = 4,
};

enum {
  GbaLoadPopInfo  = 0,
  GbaLoadPopLoadS = 1,
  GbaLoadPopPatch = 2,
  GbaLoadPopDetails = 3,
#ifdef UI_BROWSER_V2
  GbaLoadCNT      = 4,
#else
  GbaLoadCNT      = 3,
#endif

  GbaNorWrPatch   = 1,
  GbaNorWrCNT     = 2,

  GbaNorLoad      = 1,
  GbaNorLoadCNT   = 2,
};

enum {
  GBALoadButt    = 0,
#ifdef UI_BROWSER_V2
  GBAFavoriteButt = 1,
  GBAOptionsButt = 2,
  GBADetailsButt = 3,
  GBAInfoCNT     = 4,
#else
  GBAOptionsButt = 1,
  GBADetailsButt = 2,
  GBAInfoCNT     = 1,
#endif

  GBALdSetCNT    = 5,
  GBALdSetLoadP  = 0,
  GBALdSetSaveP  = 1,
  GBALdSetRTC    = 2,
  GBALdSetCheats = 3,
  GBALdRemember  = 4,

  GBAPatchCNT  = 5,
  GBALoadPatch = 0,
  GBASavePatch = 1,
  GBAInGameMen = 2,
  GBARTCPatch  = 3,
  GBAPatchGen  = 4,
};

enum {
  SaveWrite  = 0,
  SavLoad    = 1,
  SavClear   = 2,
  SavQuit    = 3,
  SavMAX     = 3,
};

enum {
  FlashingReady    = 0,
  FlashingLoading  = 1,
  FlashingChecking = 2,
  FlashingErasing  = 3,
  FlashingWriting  = 4,
};

enum {
  FiMgrDelete,
  FiMgrHide,
#ifdef SUPPORT_NORGAMES
  FiMgrWriteNOR,
#endif
  FiMgrCNT
};

const struct {
  uint16_t fg_color;     // Foreground elements color
  uint16_t bg_color;     // Background color
  uint16_t ft_color;     // Font color
  uint16_t hi_color;     // Item/Buttom highlight
  uint16_t hi_blend;     // Menu highlight color (browser)
  uint16_t sh_color;     // Menu shadow/disabled color
} themes[] = {
  { RGB2GBA(0xaaaaaa), RGB2GBA(0xffffff), RGB2GBA(0x000000), RGB2GBA(0xcccccc), RGB2GBA(0x9999bb), RGB2GBA(0xc08888) }, // White
};

typedef struct {
  // ROM information
  char romfn[MAX_FN_LEN];             // File to load/write
  uint32_t romfs;                     // File ROM size
  char gcode[5];                      // ASCII sanitized game code.
  t_rom_header romh;                  // ROM header (for info purposes)
  // Patching info
  t_patch patches_datab;              // Loaded patches (from DB)
  t_patch patches_cache;              // Loaded patches (from patch engine's cache)
  bool patches_datab_found;           // Whether we had a patch match in the database
  bool patches_cache_found;           // Same but for the patch cache
  // Patching configuration
  t_patch_policy patch_type;          // Patching type
  bool use_dsaving;                   // Whether we use direct-saving mode
  bool ingame_menu_enabled;           // Enable the in-game menu.
  bool rtc_patch_enabled;             // Patch for RTC workarounds.
} t_load_gba_info;

typedef struct {
  // Save read/write policies and info
  t_sram_load_policy sram_load_type;  // SRAM loading policy
  t_sram_save_policy sram_save_type;  // SRAM auto-saving policy
  char savefn[MAX_FN_LEN];            // Save file path.
  bool savefile_found;                // Whether there's a .sav file.
  // RTC config
  uint32_t rtcval;                    // Initial RTC value.
  // Cheats policy
  bool use_cheats;                    // Whether we want to load cheats to use them.
  bool cheats_found;                  // Whether there's a cheats file (not parsed tho!)
  unsigned cheats_size;               // Size of the cheat buffer
  char cheatsfn[MAX_FN_LEN];          // Cheats file path.
} t_load_gba_lcfg;

typedef void (*t_mrender_fn)(volatile uint8_t *frame);
typedef void (*t_mkeyupd_fn)(unsigned newkeys);

// Info and state for the menu tab
static struct {
  uint8_t menu_tab;

  unsigned anim_state;            // Animation (text rotation) status.

#ifdef UI_BROWSER_V2
  // Favorite ROMs state
  struct {
    int selector;                 // Pointed favorite offset
    int seloff;                   // Entry at the top of the list
    int maxentries;               // Total favorite count
  } favorites;
#endif

  // Recent ROMs state
  struct {
    int selector;                 // Pointed file offset
    int seloff;                   // Entry at the top of the list
    int maxentries;               // Total file/dir count in current dir
  } recent;

  // ROM browser state
  struct {
    char cpath[MAX_FN_LEN];       // Current path
    int selector;                 // Pointed file offset
    int seloff;                   // Entry at the top of the list
    int maxentries;               // Total file/dir count in current dir
    int dispentries;              // Maximum number of visible entries (filtered)
    uint16_t selhist[16];         // History of directory offsets
  } browser;

  // Flash ROM browser state
  struct {
    int selector;                 // Pointed file offset
    int seloff;                   // Entry at the top of the list
    uint8_t maxentries;           // Total file/dir count in current dir
    uint8_t usedblks, freeblks;   // NOR usage info
  } fbrowser;

  // UI settings
  struct {
    int selector;                 // Pointed option
  } uiset;

  // Main settings
  struct {
    int selector;                 // Pointed option
  } set;

  // Tools menu
  struct {
    int selector;                 // Render panel
    int seloff;                   // First visible Phase 4 tool.
  } tools;

#ifdef UI_BROWSER_V2
  // Appearance settings
  struct {
    int selector;
  } appearance;
#endif

  // Info/About menu
  struct {
    int selector;                 // Selected information row / legacy panel
#ifdef UI_BROWSER_V2
    int seloff;                   // First visible information row
#endif
    char tstr[64];                // Temp message render
  } info;
} smenu;

// Same but for popups.
static struct {
  const char *alert_msg;          // Extra pop-up message

  uint8_t pop_num;                // Current pop-up in display
  char submenu;                   // Which submenu tab we are in (if any)
  char selector;                  // Option selector (if any)
  unsigned anim;                  // Animation state

  // Pop up message (for whatever action). Allows returning to previous popup.
  struct {
    const char *message;
    const char *default_button;
    const char *confirm_button;
    void (*callback)(bool confirm);       // Function to call on "confirm".
    uint8_t option;                       // Selected button
    bool clear_popup_ok;                  // Whether any pop up must be cleared.
  } qpop;

  // RTC time set pop up, a bit special.
  struct {
    t_dec_date val;
    int selector;
    void (*callback)();                   // Function to call on "save"
  } rtcpop;

  union {
    // GBA launch ROM pop up menu
    struct {
      t_load_gba_info i;                  // ROM/Patch info and patch policy.
      t_load_gba_lcfg l;                  // ROM loading info and settings;
    } load;

    // Write GBA game to NOR memory
    struct {
      t_load_gba_info i;                  // ROM/Patch info and patch policy.
    } norwr;
    // Launch GBA game from NOR memory
    struct {
      t_load_gba_lcfg l;                  // ROM loading info and settings;
      const t_flash_game_entry *e;        // NOR game entry on RAM
    } norld;

    // Save file menu (.sav files)
    struct {
      char savfn[MAX_FN_LEN];             // SAV file to load/store/mangle
    } savopt;
    // Update menu (for .fw files)
    struct {
      char fn[MAX_FN_LEN];                // FW file to load and flash
      bool issfw;                         // The firmware is a superFW image.
      uint32_t superfw_ver;               // Reported FW version.
      uint32_t fw_size;                   // Size in bytes reported by stat.
      unsigned curr_state;                // Flashing FSM state.
    } update;

    // Not really a pop up, but used as "popup" data for menu questions.
    struct {
      char fn[MAX_FN_LEN];
      unsigned fs;
    } pdb_ld;
  } p;
} spop;

typedef struct {
  uint32_t filesize;
  uint16_t isdir;
  uint16_t attr;
  char fname[MAX_FN_LEN];
  uint16_t sortname[MAX_FN_LEN];       // Pre-decoded and sort-friendly name.
} t_centry;
_Static_assert (sizeof(t_centry) % 4 == 0, "t_centry must be word-friendly");

// Pointer to SDRAM, where we place some data:
//  - Scratch area 2MiB (for FW updates)
//  - File list order (~64KiB)
//  - Browser file information (~13MB)
//  - Recently played ROMs table (~64KiB)
//  - Font data (placed by the bootloader at the 15..16MB range)
// At the end of the SDRAM, ro-data can be loaded by the loader.
#ifdef COVER_ART_DEMO
#define scratch_mem_size 0
#else
#define scratch_mem_size (2*1024*1024)
#endif
typedef struct {
  uint8_t scratch[scratch_mem_size];
  t_centry *fileorder[BROWSER_MAXFN_CNT];
  t_centry fentries[BROWSER_MAXFN_CNT];
#ifdef UI_BROWSER_V2
  t_rentry faventries[FAVORITES_MAXFN_CNT];
#endif
  t_rentry rentries[RECENT_MAXFN_CNT];
  t_reg_entry_max nordata;
} t_sdram_state;

_Static_assert (sizeof(t_sdram_state) <= 14.5*1024*1024, "scratch SDRAM doesn't exceed 14.5MB");

#ifdef COVER_ART_DEMO
static t_sdram_state demo_sdr_state __attribute__((section(".sbss")));
t_sdram_state *sdr_state = &demo_sdr_state;
#else
t_sdram_state *sdr_state = (t_sdram_state*)0x08000000;
#endif
uint8_t *hiscratch = (uint8_t*)ROM_HISCRATCH_U8;

// FatFs enables the SuperCard SD interface while opening and reading files.
// That remaps the cartridge address space, so neither the pathname passed to
// FatFs nor its destination buffer may live in SuperCard SDRAM at 0x08000000.
// Keep the complete cover cache in GBA EWRAM instead.
static t_cover_cache menu_cover_cache __attribute__((section(".sbss")));
#ifdef UI_BROWSER_V2
static t_menu_page_repeat browse_page_repeat;
#endif

#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
typedef struct {
  uint16_t x, y;
  unsigned tn;
} t_oamobj;
#endif

static bool enable_flashing = false;
static unsigned framen = 0;
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES) && !defined(COVER_ART_DEMO)
static bool first_menu_frame_pending = false;
#endif
#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
static unsigned objnum = 0;
static t_oamobj fobjs[64];
#endif
#if defined(COVER_ART_DEMO) && defined(UI_BROWSER_V2)
static int demo_loading_progress = -1;
#endif

unsigned lang_lookup(uint16_t code) {
  for (unsigned i = 0; i < LANG_COUNT; i++)
    if (lang_codes[i] == code)
      return i;

  return 0;  // Fallback to default (english)
}

uint16_t lang_getcode() {
  return lang_codes[lang_id];
}

inline bool isascii(char code) {
  // Abuse signed :D
  return code >= 32;
}

bool is_superfw(const t_rom_header *h) {
  return !memcmp(&h->data[SUPERFW_COMMENT_DOFFSET], "SUPERFW~DAVIDGF", 16);
}

static int strcmp16(const uint16_t *a, const uint16_t *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return *a - *b;
}

NOINLINE int filesort(const void *a, const void *b) {
  const t_centry *ca = *(t_centry**)a;
  const t_centry *cb = *(t_centry**)b;

  // Directories some up first.
  if (ca->isdir != cb->isdir)
    return cb->isdir - ca->isdir;

  // Other files are string-ordered.
  int comparison = strcmp16(ca->sortname, cb->sortname);
#ifdef UI_BROWSER_V2
  return menu_browser_sort_compare(comparison, browser_sort_descending);
#else
  return comparison;
#endif
}

NOINLINE int romsort(const void *a, const void *b) {
  const t_flash_game_entry *ca = (t_flash_game_entry*)a;
  const t_flash_game_entry *cb = (t_flash_game_entry*)b;

  return strcasecmp(&ca->game_name[ca->bnoffset], &cb->game_name[cb->bnoffset]);
}

static void loadrom_progress(unsigned done, unsigned total) {
  // Draws and flips the buffer, do not care about vsync here
  volatile uint8_t *frame = &MEM_VRAM_U8[0xA000*framen];

#ifdef UI_BROWSER_V2
  bool game_loading = spop.pop_num == POPUP_GBA_LOAD;
  ui_browser_v2_load_palette(MEM_PALETTE);
  ui_browser_v2_render_loading(
    frame,
    game_loading ? file_basename(spop.p.load.i.romfn) : "WORKING",
    game_loading ? cover_cache_pixels(&menu_cover_cache) : NULL,
    game_loading ? menu_cover_cache.state : CoverEmpty,
    done, total);
#else
  // Render the full background to a solid color
  dma_memset16(&frame[0], dup8(BG_COLOR), SCREEN_WIDTH*SCREEN_HEIGHT/2);

  // Render a simple progress bar
  unsigned prog = done * 200 / total;
  for (unsigned i = 76; i < 84; i++)
    dma_memset16(&frame[SCREEN_WIDTH * i + 20], dup8(FG_COLOR), prog/2);
#endif

  dma_memset16(MEM_OAM, 0, 256);  // Clear icons

  REG_DISPCNT = (REG_DISPCNT & ~0x10) | (framen << 4);
  framen ^= 1;
}

static bool loadrom_progress_abort(unsigned done, unsigned total) {
  loadrom_progress(done, total);

  // Capture A/B buttons to abort the progress
  return ((~REG_KEYINPUT) & KEY_BUTTSTA);
}


bool generate_patches_progress(const char *fn, unsigned fs) {
  // Open ROM and load it in the SDRAM. We load it in 4MB chunks. Not ideal but
  // we want to preserve the data loaded in the SDRAM (ie. fonts).
  FIL fd;
  FRESULT res = f_open(&fd, fn, FA_READ);
  if (res != FR_OK)
    return false;

  t_patch_builder pb;
  patchengine_init(&pb, fs);
  const unsigned max_hiscratch = 8*1024*1024;

  for (unsigned i = 0; i < fs; i += max_hiscratch) {
    for (unsigned j = 0; j < max_hiscratch && i + j < fs; j += 4096) {
      UINT rdbytes;
      uint32_t tmp[4096/4];
      if (FR_OK != f_read(&fd, tmp, sizeof(tmp), &rdbytes))
        return false;

      set_supercard_mode(MAPPED_SDRAM, true, false);
      dma_memcpy32(&hiscratch[j], tmp, sizeof(tmp)/4);
      set_supercard_mode(MAPPED_SDRAM, true, true);
      if (j & ~0xFFFF)
        loadrom_progress((i*2 + j) >> 8, fs >> 7);
    }
    // Amount to process.
    unsigned blksize = MIN(max_hiscratch, fs - i);

    void upd_pe_prog(unsigned prog) {
      unsigned p = i*2 + blksize + prog*4;
      loadrom_progress(p >> 8, fs >> 7);
    }

    // Process patches. Adds them to the existing patchset.
    set_supercard_mode(MAPPED_SDRAM, true, false);
    patchengine_process_rom((uint32_t*)hiscratch, blksize, &pb, upd_pe_prog);
    set_supercard_mode(MAPPED_SDRAM, true, true);
  }

  f_close(&fd);
  patchengine_finalize(&pb);

  WRITE_LOG("Patch engine done. Found wcnt: %d save: %d (save mode: %d) irqh: %d rtc: %d",
            pb.p.wcnt_ops, pb.p.save_ops, pb.p.save_mode, pb.p.irqh_ops, pb.p.rtc_ops);

  // Proceed to write patches to their cache.
  return write_patches_cache(fn, &pb.p);
}

bool dump_flashmem_backup() {
  f_mkdir(SUPERFW_DIR);

  // Use a different file name to ensure we do not overwrite firmwares by
  // accident. This adds some minimal overhead.
  SHA256_State st;
  sha256_init(&st);

  FIL fd;
  FRESULT res = f_open(&fd, FLASHBACKUPTMP_FILEPATH, FA_WRITE | FA_CREATE_ALWAYS);
  if (res != FR_OK)
    return false;

  const unsigned fsize = flashinfo.size ? flashinfo.size : FW_MAX_SIZE_KB*1024;
  for (unsigned i = 0; i < fsize; i += 4*1024) {
    const uint8_t *faddr = (uint8_t*)(ROM_FLASHFIRMW_ADDR + i);

    uint32_t tmp[4096/4];
    set_supercard_mode(MAPPED_FIRMWARE, true, false);
    dma_memcpy32(tmp, faddr, 1024);
    set_supercard_mode(MAPPED_SDRAM, true, true);

    sha256_transform(&st, tmp, sizeof(tmp));

    UINT wrbytes;
    if (FR_OK != f_write(&fd, tmp, sizeof(tmp), &wrbytes) || wrbytes != sizeof(tmp)) {
      f_close(&fd);
      return false;
    }

    loadrom_progress(i >> 10, fsize >> 10);
  }

  f_close(&fd);

  // Calculate the final hash, use a hash prefix as the filename.
  uint8_t h256[32];
  sha256_finalize(&st, h256);

  char finalfn[64];
  npf_snprintf(finalfn, sizeof(finalfn), FLASHBACKUP_FILEPTRN,
               h256[0], h256[1], h256[2], h256[3]);
  f_rename(FLASHBACKUPTMP_FILEPATH, finalfn);

  return true;
}

void patch_gen_callback(bool confirm);

void sram_battery_test_callback(bool confirm) {
  if (confirm) {
    // Fill SRAM with some pseudorandom data to test later.
    sram_pseudo_fill();
    // Program a check on the next reboot!
    program_sram_check();

    spop.alert_msg = msgs[lang_id][MSG_SRAMTST_RDY];
  }
}


static const t_patch * get_game_patch(const t_load_gba_info *info) {
  return info->patch_type == PatchDatabase && info->patches_datab_found ? &info->patches_datab :
         info->patch_type == PatchEngine   && info->patches_cache_found ? &info->patches_cache : NULL;
}

bool ingame_menu_avail_sdram(const t_load_gba_info *info) {
  const t_patch *p = get_game_patch(info);
  // Necessary size to load the IGM (+fonts +cheats)
  const unsigned igm_reqsz = ROUND_UP2(ingame_menu_payload.menu_rsize + font_block_size() + spop.p.load.l.cheats_size, 1024);

  // If the ROM is too big, must use some hole to load the menu.
  if (info->romfs > MAX_GBA_ROM_SIZE - igm_reqsz) {
    // Discard holes that are too small, or not well formed.
    if (!p || p->hole_size < igm_reqsz || p->hole_addr + p->hole_size > info->romfs)
      return false;   // Too big to fit the menu!
  }

  // Check if the patches exist and have proper IRQ support.
  return p && p->irqh_ops > 0;
}

bool ingame_menu_avail_flash(const t_load_gba_info *info) {
  const t_patch *p = get_game_patch(info);

  // Checks if the ROM is small enough so the last 4MiB block can be remapped.
  if (info->romfs > MAX_GBA_ROM_SIZE - NOR_BLOCK_SIZE) {
    // Otherwise find a gap to flash on NOR our tiny payload
    if (!p || p->hole_size < DIRSAVE_REQ_SPACE || p->hole_addr + p->hole_size > info->romfs)
      return false;   // Too big to fit!
  }

  // Check if the patches exist and have proper IRQ support.
  return p && p->irqh_ops > 0;
}

// Calculates whether DirectSaving can be used given some information.
bool dirsav_avail_sdram(const t_load_gba_info *info) {
  const t_patch *p = get_game_patch(info);

  // Check if there's enough space for it! (Placing it at the end).
  if (info->romfs > MAX_GBA_ROM_SIZE - DIRSAVE_REQ_SPACE) {
    if (!p || p->hole_size < DIRSAVE_REQ_SPACE || p->hole_addr + p->hole_size > info->romfs)
      return false;   // Too big to fit!
  }

  return (p && supports_directsave(p->save_mode));
}

bool dirsav_avail_flash(const t_load_gba_info *info) {
  const t_patch *p = get_game_patch(info);

  // Checks if the ROM is small enough so the last 4MiB block can be remapped.
  if (info->romfs > MAX_GBA_ROM_SIZE - NOR_BLOCK_SIZE) {
    // Otherwise find a gap to flash on NOR our tiny payload
    if (!p || p->hole_size < DIRSAVE_REQ_SPACE || p->hole_addr + p->hole_size > info->romfs)
      return false;   // Too big to fit!
  }

  return (p && supports_directsave(p->save_mode));
}

bool rtcemu_avail(const t_load_gba_info *info) {
  const t_patch *p = get_game_patch(info);
  return (p && p->rtc_ops);
}

static bool prepare_gba_info(
  t_load_gba_info *info, const t_rom_load_settings *st,
  const char *fn, uint32_t fs,
  bool load_sdram
) {
  // Pre-load ROM header
  if (preload_gba_rom(fn, fs, &info->romh))
    return false;

  // Fill/copy ROM info.
  if (fn != info->romfn)
    strcpy(info->romfn, fn);
  info->romfs = fs;

  // Sanitize the game code for display
  for (unsigned i = 0; i < 4; i++)
    info->gcode[i] = isascii(info->romh.gcode[i]) ? info->romh.gcode[i] : 0x1A;
  info->gcode[4] = 0;

  // Look up patches, have them handy.
  uint8_t gamecode[5] = {
    info->romh.gcode[0], info->romh.gcode[1],
    info->romh.gcode[2], info->romh.gcode[3],
    info->romh.version
  };
  set_supercard_mode(MAPPED_SDRAM, true, false);
  info->patches_datab_found = patchmem_lookup(gamecode, (uint8_t*)ROM_PATCHDB_U8, &info->patches_datab);
  set_supercard_mode(MAPPED_SDRAM, true, true);

  if (!info->patches_datab_found)
    WRITE_LOG("No patches in PatchDB found for '%s' with gamecode %c%c%c%c-%d",
              fn, gamecode[0], gamecode[1], gamecode[2], gamecode[3], (int)gamecode[4]);

  // Attempt to load any existing patch and check also the PE cache dir.
  info->patches_cache_found = load_rom_patches(fn, &info->patches_cache);
  if (!info->patches_cache_found) {
    WRITE_LOG("No patch file found for '%s'", fn);
    info->patches_cache_found = load_cached_patches(fn, &info->patches_cache);
    if (!info->patches_cache_found)
      WRITE_LOG("No patch file found in patches cache dir for '%s'", fn);
  }

  // If PatchAuto is selected, resolve it. Downgrade if not found.
  if (st->patch_policy == PatchAuto) {
    if (info->patches_cache_found)
      info->patch_type = PatchEngine;      // Try existing patches
    else if (info->patches_datab_found)
      info->patch_type = PatchDatabase;    // Try the database then
    else
      info->patch_type = PatchNone;
  }
  // Downgrade to no patches if the specified was not found.
  else if (st->patch_policy == PatchDatabase) {
    if (!info->patches_datab_found)
      info->patch_type = PatchNone;
  }
  else if (st->patch_policy == PatchEngine) {
    if (!info->patches_cache_found)
      info->patch_type = PatchNone;
  }
  else
    info->patch_type = st->patch_policy;

  // Fill defaults as requested if possible.
  bool allowds = load_sdram ? dirsav_avail_sdram(info) : dirsav_avail_flash(info);
  bool allowigm = load_sdram ? ingame_menu_avail_sdram(info) : ingame_menu_avail_flash(info);

  info->rtc_patch_enabled = st->use_rtc && rtcemu_avail(info);
  info->use_dsaving = st->use_dsaving && allowds;
  info->ingame_menu_enabled = st->use_igm && allowigm;

  return true;
}

static void prepare_gba_cheats(const char *gcode, uint8_t ver, t_load_gba_lcfg *data, const char *fn, bool prefer_cheats) {
  // Attempt to find a cheat file if cheats are enabled.
  data->cheats_size = 0;
  data->cheats_found = false;
  if (enable_cheats) {
    strcpy(data->cheatsfn, fn);
    replace_extension(data->cheatsfn, ".cht");
    data->cheats_found = check_file_exists(data->cheatsfn);
    if (!data->cheats_found) {
      WRITE_LOG("No cheat file found at '%s'", data->cheatsfn);
      // Create a path using the game ID and version.
      npf_snprintf(data->cheatsfn, sizeof(data->cheatsfn), CHEATS_PATH "%c%c%c%c-%02x.cht",
                   gcode[0], gcode[1], gcode[2], gcode[3], ver);
      data->cheats_found = check_file_exists(data->cheatsfn);
      WRITE_LOG("No cheat file found at '%s'", data->cheatsfn);

      // Load the cheats into memory if enabled.
      if (data->cheats_found) {
        // Load the cheats to the ROM area, just after the font pack. This is for easier relocation.
        uint8_t *cheat_area = (uint8_t*)(ROM_FONTBASE_U8 + font_block_size());
        unsigned max_area = 1536*1024 - font_block_size();    // 1.5MB is reserved at the end.
        int cheatsz = open_read_cheats(cheat_area, max_area, data->cheatsfn);
        if (cheatsz < 0)
          data->cheats_found = false;
        else
          data->cheats_size = cheatsz;

        WRITE_LOG("Loaded cheats returned %d", cheatsz);
      }
    }
  }
  data->use_cheats = enable_cheats && data->cheats_found && prefer_cheats;
}

static void prepare_gba_settings(t_load_gba_lcfg *data, bool uses_dsaving, uint32_t rtcts, bool game_no_save) {
  // Calculate the .sav file name, and check its existance.
  data->savefile_found = check_file_exists(data->savefn);
  if (data->savefile_found)
    WRITE_LOG("Savefile found at '%s'", data->savefn);
  else
    WRITE_LOG("No savefile found for '%s'", data->savefn);

  // Use default settings (and file existance) to fill in default choice.
  // DirectSaving enabled overrides the other settings.
  if (uses_dsaving) {
    data->sram_load_type = data->savefile_found ? SaveLoadSav : SaveLoadReset;
    data->sram_save_type = SaveDirect;
  }
  else {
    data->sram_load_type = game_no_save         ? SaveLoadDisable :
                           !autoload_default    ? SaveLoadDisable :
                           data->savefile_found ? SaveLoadSav :
                                                  SaveLoadReset;
    data->sram_save_type = autosave_default && !game_no_save ? SaveReboot : SaveDisable;
  }

  data->rtcval = rtcts;
}


static void browser_open_gba(const char *fn, uint32_t fs, bool prompt_patchgen) {
  if (fs > MAX_GBA_ROM_SIZE) {
    // The ROM is too big to be loaded!
    spop.alert_msg = msgs[lang_id][MSG_ERR_TOOBIG];
  } else {
    // Default to global settings (in case the file is not found).
    t_rom_load_settings ld_sett = {
      .patch_policy = patcher_default,
      .use_igm = ingamemenu_default,
      .use_rtc = rtcpatch_default,
      .use_dsaving = autosave_prefer_ds
    };
    t_rom_launch_settings lh_sett = {
      .use_cheats = true,              // Defaults to true (just preferred, might be disabled/N/A)
      .rtcts = rtcvalue_default
    };
    // Check for any game-specific config file, so we don't have to guess the config.
    // The config file can be partial, hence the defaults.
    load_rom_settings(fn, &ld_sett, &lh_sett);

    if (!prepare_gba_info(&spop.p.load.i, &ld_sett, fn, fs, true))
      spop.alert_msg = msgs[lang_id][MSG_ERR_READ];
    else {
      const t_rom_header *rmh = &spop.p.load.i.romh;

      // If patch engine is selected but no patches found, prompt for generation.
      // If auto is selected and no patches nor DB entries found, do prompt too.
      bool no_patches = (ld_sett.patch_policy == PatchAuto &&
                         !spop.p.load.i.patches_datab_found && !spop.p.load.i.patches_cache_found);
      bool no_engine  = (ld_sett.patch_policy == PatchEngine && !spop.p.load.i.patches_cache_found);
      bool issfw = is_superfw(rmh);

      if (prompt_patchgen && !issfw && (no_patches || no_engine)) {
        // No patches found, ask the user if they want to generate patches
        // using the patch engine.
        spop.qpop.message = msgs[lang_id][no_patches ? MSG_Q1_NOPATCH : MSG_Q1_PATCHENG];
        spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
        spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
        spop.qpop.option = 0;
        spop.qpop.callback = patch_gen_callback;
        spop.qpop.clear_popup_ok = true;
        return;
      }

      // What if the game doesn't have a save method? Select sane defaults.
      const t_patch *p = get_game_patch(&spop.p.load.i);
      bool game_no_save = (p && p->save_mode == SaveTypeNone) || issfw;

      // Attempt to find a cheat file if cheats are enabled.
      prepare_gba_cheats((char*)&rmh->gcode[0], rmh->version, &spop.p.load.l, fn, lh_sett.use_cheats);

      // Load and set default and sane settings honoring defaults and preferences.
      sram_filename_calc(fn, spop.p.load.l.savefn, save_path_default);
      prepare_gba_settings(&spop.p.load.l, spop.p.load.i.use_dsaving, lh_sett.rtcts, game_no_save);

      // Show load ROM menu.
      spop.pop_num = POPUP_GBA_LOAD;
      spop.anim = 0;
      spop.submenu = GbaLoadPopInfo;
      spop.selector = GBALoadButt;
    }
  }
}

#ifdef SUPPORT_NORGAMES
static void browser_open_nor(const t_flash_game_entry * e) {
  // Use attributes to determine patched save method.
  const bool game_no_save = GET_GATTR_SAVEM(e->gattrs) <= SaveTypeNone;
  const bool game_uses_dsaving = (e->gattrs & GATTR_SAVEDS);

  t_rom_launch_settings lh_sett = {
    .use_cheats = true,              // Defaults to true (just preferred, might be disabled/N/A)
    .rtcts = rtcvalue_default
  };
  load_rom_settings(e->game_name, NULL, &lh_sett);

  // Attempt to find a cheat file if cheats are enabled.
  prepare_gba_cheats((char*)&e->gamecode, e->gamever, &spop.p.norld.l, e->game_name, lh_sett.use_cheats);

  // Load and set default and sane settings honoring defaults and preferences.
  sram_filename_calc(e->game_name, spop.p.norld.l.savefn, save_path_nor_default);
  WRITE_LOG("loadp: %d savep: %d uses_ds: %d gamens: %d",
            spop.p.norld.l.sram_load_type, spop.p.norld.l.sram_save_type,
            game_uses_dsaving, game_no_save);
  prepare_gba_settings(&spop.p.norld.l, game_uses_dsaving, lh_sett.rtcts, game_no_save);

  // Save entry pointer
  spop.p.norld.e = e;

  // Show load ROM menu.
  spop.pop_num = POPUP_GBA_NORLOAD;
  spop.submenu = GbaLoadPopInfo;
  spop.selector = 0;
}
#endif

void patch_gen_callback(bool confirm) {
  // Generate patches if confirm was selected
  if (confirm) {
    generate_patches_progress(spop.p.load.i.romfn, spop.p.load.i.romfs);
    spop.alert_msg = msgs[lang_id][MSG_PATCHGEN_OK];
  }

  // Either way, show the popup screen afterwards without prompt
  browser_open_gba(spop.p.load.i.romfn, spop.p.load.i.romfs, false);
}

static void load_patchdb_action(bool confirm) {
  if (confirm) {
    FIL fd;
    FRESULT res = f_open(&fd, spop.p.pdb_ld.fn, FA_READ);
    if (res != FR_OK) {
      spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
      return;
    } else {
      for (unsigned off = 0; off < spop.p.pdb_ld.fs; off += 1024) {
        UINT rdbytes;
        uint32_t tmp[1024/4];
        if (FR_OK != f_read(&fd, tmp, sizeof(tmp), &rdbytes)) {
          spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
          return;
        }

        set_supercard_mode(MAPPED_SDRAM, true, false);
        dma_memcpy32(ROM_PATCHDB_U8 + off, tmp, sizeof(tmp)/4);
        set_supercard_mode(MAPPED_SDRAM, true, true);
      }
    }
    spop.alert_msg = msgs[lang_id][MSG_OK_GENERIC];
  }
}

unsigned guess_file_type(const uint8_t *header) {
  const t_rom_header *gbah = (t_rom_header*)header;
  uint32_t sig = *(uint32_t*)header;

  if (gbah->fixed == 0x96 && gbah->unit_code == 0x00 && gbah->devtype == 0x00 &&
      header[3] == 0xEA /* Starts with an unconditional branch */ &&
      validate_gba_header(header))
    return FileTypeGBA;
  else if (validate_gb_header(&header[0x100]))
    return FileTypeGB;
  else if (sig == 0x1A53454E)
    return FileTypeNES;
  else if (sig == 0x31424450)
    return FileTypePatchDB;

  return FileTypeUnknown;
}

static bool insert_recent_flush(const char *fn, unsigned flags) {
  // Insert element.
  smenu.recent.maxentries = insert_recent_fn(sdr_state->rentries, smenu.recent.maxentries, fn, flags);
  return recent_flush(sdr_state->rentries, smenu.recent.maxentries);
}

static bool recent_flush_current(void) {
  return recent_flush(sdr_state->rentries, smenu.recent.maxentries);
}

static bool delete_recent_flush(unsigned entry_num) {
  smenu.recent.maxentries = delete_recent(sdr_state->rentries, smenu.recent.maxentries, entry_num);

  if (smenu.recent.maxentries) {
    smenu.recent.selector = MIN(smenu.recent.maxentries - 1,
                                smenu.recent.selector);
#ifdef UI_BROWSER_V2
    smenu.recent.seloff = (smenu.recent.selector / RECENT_ROWS) * RECENT_ROWS;
#endif
  } else {
    smenu.recent.selector = 0;
    smenu.recent.seloff = 0;
    smenu.menu_tab = MENUTAB_ROMBROWSE;
  }

  return recent_flush(sdr_state->rentries, smenu.recent.maxentries);
}

static void recent_reload() {
  smenu.recent.selector = 0;
  smenu.recent.seloff = 0;
  smenu.anim_state = 0;
  smenu.recent.maxentries = recent_load(RECENT_FILEPATH, sdr_state->rentries);
}

#ifdef UI_BROWSER_V2
static int favorite_find(const char *fn) {
  for (int i = 0; i < smenu.favorites.maxentries; i++)
    if (!strcmp(sdr_state->faventries[i].fpath, fn))
      return i;
  return -1;
}

static void favorite_copy_path(unsigned entry_num, char *path) {
  /* FatFs must never receive a path pointer into cartridge SDRAM. */
  dma_memcpy16(path, sdr_state->faventries[entry_num].fpath,
               MAX_FN_LEN / 2);
  path[MAX_FN_LEN - 1] = 0;
}

NOINLINE static bool favorites_flush() {
#ifdef COVER_ART_DEMO
  return true;
#else
  FIL fo;
  if (FR_OK != f_open(&fo, FAVORITES_FILEPATH,
                      FA_WRITE | FA_CREATE_ALWAYS))
    return false;

  unsigned coff = 0;
  char tmpbuf[1024];
  for (int i = 0; i < smenu.favorites.maxentries; i++) {
    char path[MAX_FN_LEN];
    favorite_copy_path(i, path);
    unsigned fnlen = strlen(path);
    memcpy(&tmpbuf[coff], path, fnlen);
    coff += fnlen;
    tmpbuf[coff++] = '\n';

    if (coff >= 512) {
      UINT wrbytes;
      if (FR_OK != f_write(&fo, tmpbuf, 512, &wrbytes) ||
          wrbytes != 512) {
        f_close(&fo);
        return false;
      }
      memmove(tmpbuf, &tmpbuf[512], coff - 512);
      coff -= 512;
    }
  }

  if (coff) {
    UINT wrbytes;
    if (FR_OK != f_write(&fo, tmpbuf, coff, &wrbytes) || wrbytes != coff) {
      f_close(&fo);
      return false;
    }
  }

  return FR_OK == f_close(&fo);
#endif
}

static bool favorite_add_flush(const char *fn) {
  if (favorite_find(fn) >= 0)
    return true;
  unsigned fnlen = strlen(fn);
  if (fnlen >= MAX_FN_LEN ||
      smenu.favorites.maxentries >= FAVORITES_MAXFN_CNT)
    return false;

  t_rentry *entry = &sdr_state->faventries[smenu.favorites.maxentries];
  entry->fname_offset = file_basename(fn) - fn;
  dma_memcpy16(entry->fpath, fn, (fnlen + 2) / 2);
  smenu.favorites.maxentries++;
  if (favorites_flush())
    return true;

  smenu.favorites.maxentries--;
  return false;
}

static bool favorite_delete_flush(unsigned entry_num) {
  if (entry_num >= (unsigned)smenu.favorites.maxentries)
    return false;

  const int old_selector = smenu.favorites.selector;
  const int old_seloff = smenu.favorites.seloff;
  t_rentry removed;
  dma_memcpy16(&removed, &sdr_state->faventries[entry_num],
               sizeof(removed) / 2);

  if (entry_num + 1 < (unsigned)smenu.favorites.maxentries)
    memmove32(&sdr_state->faventries[entry_num],
              &sdr_state->faventries[entry_num + 1],
              (smenu.favorites.maxentries - entry_num - 1) *
              sizeof(sdr_state->faventries[0]));
  smenu.favorites.maxentries--;

  if (smenu.favorites.maxentries) {
    smenu.favorites.selector = MIN(smenu.favorites.selector,
                                   smenu.favorites.maxentries - 1);
    smenu.favorites.seloff = (smenu.favorites.selector / FAVORITES_ROWS) *
                             FAVORITES_ROWS;
  } else {
    smenu.favorites.selector = 0;
    smenu.favorites.seloff = 0;
  }

  if (favorites_flush())
    return true;

  if (entry_num < (unsigned)smenu.favorites.maxentries)
    memmove32(&sdr_state->faventries[entry_num + 1],
              &sdr_state->faventries[entry_num],
              (smenu.favorites.maxentries - entry_num) *
              sizeof(sdr_state->faventries[0]));
  dma_memcpy16(&sdr_state->faventries[entry_num], &removed,
               sizeof(removed) / 2);
  smenu.favorites.maxentries++;
  smenu.favorites.selector = old_selector;
  smenu.favorites.seloff = old_seloff;
  return false;
}

static void favorite_append_loaded(char *path, unsigned length) {
  if (length && path[length - 1] == '\r')
    path[--length] = 0;
  if (!length || favorite_find(path) >= 0 ||
      smenu.favorites.maxentries >= FAVORITES_MAXFN_CNT)
    return;

  t_rentry *entry = &sdr_state->faventries[smenu.favorites.maxentries++];
  entry->fname_offset = file_basename(path) - path;
  dma_memcpy16(entry->fpath, path, (length + 2) / 2);
}

static void favorites_reload() {
  smenu.favorites.selector = 0;
  smenu.favorites.seloff = 0;
  smenu.favorites.maxentries = 0;
#ifndef COVER_ART_DEMO
  FIL fi;
  if (FR_OK != f_open(&fi, FAVORITES_FILEPATH, FA_READ))
    return;

  char block[512];
  char path[MAX_FN_LEN];
  unsigned path_length = 0;
  bool overlong = false;
  while (smenu.favorites.maxentries < FAVORITES_MAXFN_CNT) {
    UINT rdbytes;
    if (FR_OK != f_read(&fi, block, sizeof(block), &rdbytes) || !rdbytes)
      break;

    for (unsigned i = 0; i < rdbytes; i++) {
      if (block[i] == '\n') {
        if (!overlong) {
          path[path_length] = 0;
          favorite_append_loaded(path, path_length);
        }
        path_length = 0;
        overlong = false;
      } else if (!overlong) {
        if (path_length + 1 < sizeof(path))
          path[path_length++] = block[i];
        else
          overlong = true;
      }
    }
  }
  if (!overlong && path_length) {
    path[path_length] = 0;
    favorite_append_loaded(path, path_length);
  }
  f_close(&fi);
#endif
}

static unsigned favorite_pending_delete;

static void favorite_delete_callback(bool confirm) {
  if (confirm && !favorite_delete_flush(favorite_pending_delete))
    spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
}

static void favorites_reset_callback(bool confirm) {
  if (!confirm)
    return;

  bool saved = menu_list_reset_flush(&smenu.favorites.selector,
                                     &smenu.favorites.seloff,
                                     &smenu.favorites.maxentries,
                                     favorites_flush);
  spop.alert_msg = msgs[lang_id][saved ? MSG_OK_GENERIC : MSG_ERR_GENERIC];
}

static void recent_reset_callback(bool confirm) {
  if (!confirm)
    return;

  bool saved = menu_list_reset_flush(&smenu.recent.selector,
                                     &smenu.recent.seloff,
                                     &smenu.recent.maxentries,
                                     recent_flush_current);
  spop.alert_msg = msgs[lang_id][saved ? MSG_OK_GENERIC : MSG_ERR_GENERIC];
}
#endif

void start_emu_game(const t_emu_loader *ldinfo, const char *fn, uint32_t fs) {
  // Load: Sav/Reset Save: Reboot/Disable
  sram_filename_calc(fn, spop.p.load.l.savefn, save_path_default);
  t_sram_load_policy lp = check_file_exists(spop.p.load.l.savefn) ? SaveLoadSav : SaveLoadReset;
  unsigned errsave = prepare_sram_based_savegame(lp, SaveReboot, spop.p.load.l.savefn);
  if (errsave) {
    unsigned errmsg = (errsave == ERR_SAVE_BADSAVE)   ? MSG_ERR_SAVERD :
                                                        MSG_ERR_SAVEWR;
    spop.alert_msg = msgs[lang_id][errmsg];
  }
  else {
    // Try to load the emu and ROM, keep trying if there's more than one emulatior option.
    unsigned errcode = ERR_LOAD_NOEMU;
    while (ldinfo->emu_name) {
      if (recent_menu)
        insert_recent_flush(fn, FLAG_RECENT_SD);

      errcode = load_extemu_rom(fn, fs, ldinfo, loadrom_progress);
      if (errcode && errcode != ERR_LOAD_NOEMU)
        break;
      ldinfo++;
    }
    unsigned errmsg = (errcode == ERR_LOAD_NOEMU) ? MSG_ERR_NOEMU :
                                                    MSG_ERR_READ;
    spop.alert_msg = msgs[lang_id][errmsg];
  }
}

NOINLINE static void browser_open(const char *fn, uint32_t fs) {
  unsigned l = strlen(fn);
  if (!strcasecmp(&fn[l-4], ".gba"))
    // GBA ROMs (most likely)
    browser_open_gba(fn, fs, true);
  else if (!strcasecmp(&fn[l-4], ".sav")) {
    spop.pop_num = POPUP_SAVFILE;
    spop.selector = SavMAX;
    strcpy(spop.p.savopt.savfn, fn);
  }
  else if (!strcasecmp(&fn[l-3], ".fw")) {
    // A SuperFW firmware update is selected!
    if (!enable_flashing)
      spop.alert_msg = msgs[lang_id][MSG_FWUP_DISABLED];
    else if (fs > FW_MAX_SIZE_KB*1024 || (flashinfo.size && fs > flashinfo.size))
      spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRSZ];
    else {
      // Read the header and perform some more basic checks!
      FIL fd;
      FRESULT res = f_open(&fd, fn, FA_READ);
      if (res != FR_OK)
        spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRRD];
      else {
        UINT rdbytes;
        uint8_t tmp[512];
        if (FR_OK != f_read(&fd, tmp, sizeof(tmp), &rdbytes) || rdbytes != sizeof(tmp))
          spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRRD];
        else if (!validate_gba_header(tmp))  // Is it a valid GBA ROM header?
          spop.alert_msg = msgs[lang_id][MSG_FWUP_BADHD];
        else {
          spop.p.update.issfw = check_superfw(tmp, &spop.p.update.superfw_ver);
          spop.p.update.fw_size = fs;
          spop.p.update.curr_state = FlashingReady;
          spop.pop_num = POPUP_FWFLASH;
          strcpy(spop.p.update.fn, fn);
          f_close(&fd);
        }
      }
    }
  }
  else {
    // Any emulator-based console supported
    const char *ext = find_extension(fn);
    if (ext) {
      const t_emu_loader *ldinfo = get_emu_info(&ext[1]);
      if (ldinfo) {
        start_emu_game(ldinfo, fn, fs);
        return;
      }
    }

    // Attempt to load the file magic and detect what kind of file this is.
    if (fs >= 512) {
      FIL fi;
      if (FR_OK == f_open(&fi, fn, FA_READ)) {
        uint32_t tmphdr[512 / 4];
        UINT rdbytes;
        if (FR_OK == f_read(&fi, tmphdr, sizeof(tmphdr), &rdbytes) && rdbytes == sizeof(tmphdr)) {
          unsigned guesstype = guess_file_type((uint8_t*)tmphdr);
          switch (guesstype) {
          case FileTypeGBA:
            browser_open_gba(fn, fs, true); break;
          case FileTypeGB:
            start_emu_game(get_emu_info("gbc"), fn, fs);
            break;
          case FileTypePatchDB:
            strcpy(spop.p.pdb_ld.fn, fn);
            spop.p.pdb_ld.fs = fs;
            spop.qpop.message = msgs[lang_id][MSG_Q3_LOADPDB];
            spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
            spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
            spop.qpop.option = 0;
            spop.qpop.callback = load_patchdb_action;
            spop.qpop.clear_popup_ok = false;
            break;
          default:
            spop.alert_msg = msgs[lang_id][MSG_ERR_UNKTYP];
            break;
          };
        }
        f_close(&fi);
      }
    }
  }
}

#ifdef UI_BROWSER_V2
enum {
  BrowserFamilyNone = 0,
  BrowserFamilyGBA,
  BrowserFamilyGB,
  BrowserFamilyGBC,
};

static bool browser_extension_is(const char *name, const char *extension) {
  const char *found = find_extension(name);
  return found && !strcasecmp(&found[1], extension);
}

static unsigned browser_game_family(const char *name) {
  if (browser_extension_is(name, "gba"))
    return BrowserFamilyGBA;
  if (browser_extension_is(name, "gb"))
    return BrowserFamilyGB;
  if (browser_extension_is(name, "gbc"))
    return BrowserFamilyGBC;
  return BrowserFamilyNone;
}

static bool browser_is_other_game(const char *name) {
  const char *extension = find_extension(name);
  return extension && get_emu_info(&extension[1]);
}

static bool browser_is_known_non_game(const char *name) {
  return browser_extension_is(name, "sav") ||
         browser_extension_is(name, "fw") ||
         browser_extension_is(name, "db");
}
#endif

static void browser_reload_filter() {
  // Instead of sorting the actual list of files, which requires moving lots
  // of memory, we use a list of pointers.
  t_centry *selected_entry = smenu.browser.selector >= 0 &&
                             smenu.browser.selector < smenu.browser.dispentries ?
    sdr_state->fileorder[smenu.browser.selector] : NULL;
  unsigned fcount = 0;
  for (unsigned i = 0; i < smenu.browser.maxentries; i++) {
    t_centry *entry = &sdr_state->fentries[i];
    if ((entry->attr & AM_HID) && hide_hidden)
      continue;

#ifdef UI_BROWSER_V2
    unsigned family = browser_game_family(entry->fname);
    bool recognized = family || browser_is_other_game(entry->fname) ||
                      browser_is_known_non_game(entry->fname);
    if (!menu_browser_entry_visible(entry->isdir, family, recognized,
                                    browser_game_filter,
                                    browser_hide_folders,
                                    browser_hide_unknown))
      continue;
#endif

    sdr_state->fileorder[fcount++] = entry;
  }

  heapsort4(sdr_state->fileorder, fcount, sizeof(t_centry*) / sizeof(uint32_t), filesort);

  int selected_index = -1;
  for (unsigned i = 0; selected_entry && i < fcount; i++)
    if (sdr_state->fileorder[i] == selected_entry) {
      selected_index = i;
      break;
    }
  if (selected_index >= 0)
    smenu.browser.selector = selected_index;
  else if (fcount)
    smenu.browser.selector = MIN(smenu.browser.selector, (int)fcount - 1);
  else
    smenu.browser.selector = 0;
#ifdef UI_BROWSER_V2
  smenu.browser.seloff = fcount ?
    (smenu.browser.selector / BROWSER_ROWS) * BROWSER_ROWS : 0;
#else
  smenu.browser.seloff = MAX(0, smenu.browser.selector - BROWSER_ROWS / 2);
#endif
  smenu.browser.dispentries = fcount;
}

// Loads a new directory list in the ROM browser.
// TODO: Implement filtering (.gba/.rom/.bin... etc) using settings
static void browser_reload() {
  smenu.anim_state = 0;
  smenu.browser.dispentries = 0;

  unsigned fcount = 0;
  DIR d;
  if (FR_OK != f_opendir(&d, smenu.browser.cpath))
    return;   // FIXME: Implement error reporting!

  while (1) {
    FILINFO info;
    if (f_readdir(&d, &info) != FR_OK || !info.fname[0])
      break;

    if (fcount >= BROWSER_MAXFN_CNT)
      break;

    t_centry *e = &sdr_state->fentries[fcount++];
    e->filesize = (uint32_t) info.fsize;  // TODO: Support 4GB+ files?
    e->isdir = (info.fattrib & AM_DIR) ? 1 : 0;
    e->attr = info.fattrib;
    dma_memcpy16(e->fname, info.fname, MAX_FN_LEN/2);
    sortable_utf8_u16(info.fname, e->sortname);
  }
  smenu.browser.maxentries = fcount;

  // Filter and sort list of files/dirs
  browser_reload_filter();
}

// Loads NOR game entries so they can be browsed.
static void flashbrowser_reload() {
  #ifdef SUPPORT_NORGAMES
  smenu.fbrowser.selector = 0;
  smenu.anim_state = 0;

  if (!flashmgr_load(ROM_FLASHMETA_ADDR, FLASH_METADATA_SIZE, (t_reg_entry*)&sdr_state->nordata))
    // No data found, reset the entries
    memset(&sdr_state->nordata, 0, sizeof(sdr_state->nordata));

  // Calculate block usage, free space, etc.
  smenu.fbrowser.usedblks = 0;
  for (unsigned i = 0; i < sdr_state->nordata.gamecnt; i++) {
    const t_flash_game_entry *e = &sdr_state->nordata.games[i];
    for (unsigned j = 0; j < MAX_GAME_BLOCKS; j++)
      if (e->blkmap[j])
        smenu.fbrowser.usedblks++;
  }
  smenu.fbrowser.freeblks = NOR_GAMEBLOCK_COUNT - smenu.fbrowser.usedblks;

  smenu.fbrowser.maxentries = sdr_state->nordata.gamecnt;
  heapsort4(sdr_state->nordata.games, smenu.fbrowser.maxentries, sizeof(t_flash_game_entry) / sizeof(uint32_t), romsort);
  #endif
}

#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
static inline void render_icon(unsigned x, unsigned y, unsigned iconn) {
  fobjs[objnum++] = (t_oamobj){x, y, 8*iconn };
}

static inline void render_icon_trans(unsigned x, unsigned y, unsigned iconn) {
  fobjs[objnum++] = (t_oamobj){x, y | 0x0400, 8*iconn };
}
#endif

// Guess the file type based on the file name.
static unsigned guessicon(const char *path) {
  unsigned l = strlen(path);
  if (l < 4)
    return ICON_BINFILE;

  if (!strcasecmp(&path[l-4], ".gba"))
    return ICON_GBACART;
  else if (!strcasecmp(&path[l-3], ".gb"))
    return ICON_GBCART;
  else if (!strcasecmp(&path[l-4], ".gbc"))
    return ICON_GBCCART;
  else if (!strcasecmp(&path[l-4], ".nes"))
    return ICON_NESCART;
  else if (!strcasecmp(&path[l-4], ".sms"))
    return ICON_SMSCART;
  else if (!strcasecmp(&path[l-3], ".fw"))
    return ICON_UPDFILE;

  return ICON_BINFILE;
}

#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
// Draws text adding some support for overflow.
#define THREEDOTS_WIDTH  9
static void draw_text_ovf(const char *t, volatile uint8_t *frame, unsigned x, unsigned y, unsigned maxw) {
  uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x];
  unsigned twidth = font_width(t);
  if (twidth <= maxw)
    draw_text_idx8_bus16(t, basept, SCREEN_WIDTH, FT_COLOR);
  else {
    char tmpbuf[256];
    unsigned numchars = font_width_cap(t, maxw - THREEDOTS_WIDTH);
    memcpy(tmpbuf, t, numchars);
    memcpy(&tmpbuf[numchars], "...", 4);
    draw_text_idx8_bus16(tmpbuf, basept, SCREEN_WIDTH, FT_COLOR);
  }
}

static void draw_text_leftovf(const char *t, volatile uint8_t *frame, unsigned x, unsigned y, unsigned maxw) {
  uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x];
  unsigned numchars = font_width_lcap(t, maxw - THREEDOTS_WIDTH);
  if (numchars) {
    draw_text_idx8_bus16("...", basept, SCREEN_WIDTH, FT_COLOR);
    draw_text_idx8_bus16(&t[numchars], basept + THREEDOTS_WIDTH, SCREEN_WIDTH, FT_COLOR);
  } else {
    draw_text_idx8_bus16(t, basept, SCREEN_WIDTH, FT_COLOR);
  }
}

static void draw_text_ovf_rotate(const char *t, volatile uint8_t *frame, unsigned x, unsigned y, unsigned maxw, unsigned *franim) {
  uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x];
  unsigned twidth = font_width(t);
  if (twidth <= maxw)
    draw_text_idx8_bus16(t, basept, SCREEN_WIDTH, FT_COLOR);
  else {
    unsigned anim = *franim > ANIM_INITIAL_WAIT ? (*franim - ANIM_INITIAL_WAIT) >> 4 : 0;

    // Wrap around once the text end reaches the mid point aprox.
    char tmpbuf[540];
    strcpy(tmpbuf, t);
    strcat(tmpbuf, "      ");
    unsigned pixw = font_width(tmpbuf);
    if (anim > pixw)
      *franim = ANIM_INITIAL_WAIT + ((anim - pixw) << 4);
    strcat(tmpbuf, t);

    draw_text_idx8_bus16_range(tmpbuf, basept, anim, maxw, SCREEN_WIDTH, FT_COLOR);
  }
}

static void draw_box_outline(volatile uint8_t *frame, unsigned left, unsigned right, unsigned top, unsigned bottom, uint8_t color) {
  dma_memset16(&frame[SCREEN_WIDTH * top + left], dup8(color), (right - left) / 2);
  dma_memset16(&frame[SCREEN_WIDTH * (top + 1) + left], dup8(color), (right - left) / 2);
  dma_memset16(&frame[SCREEN_WIDTH * (bottom - 1) + left], dup8(color), (right - left) / 2);
  dma_memset16(&frame[SCREEN_WIDTH * (bottom - 2) + left], dup8(color), (right - left) / 2);
  while (top < bottom) {
    *((uint16_t*)&frame[SCREEN_WIDTH * top + left]) = dup8(color);
    *((uint16_t*)&frame[SCREEN_WIDTH * top + right - 2]) = dup8(color);
    top++;
  }
}

static void draw_box_full(
  volatile uint8_t *frame, unsigned left, unsigned right, unsigned top, unsigned bottom,
  uint8_t outlinecolor, uint8_t bgcolor
) {
  draw_box_outline(frame, left, right, top, bottom, outlinecolor);
  for (unsigned i = top + 2; i < bottom - 2; i++)
    dma_memset16(&frame[SCREEN_WIDTH * i + left + 2], dup8(bgcolor), (right - left - 4) / 2);
}

static void draw_button_box(
  volatile uint8_t *frame, unsigned left, unsigned right, unsigned top, unsigned bottom, bool selected
) {
  if (selected)
    draw_box_full(frame, left, right, top, bottom, FG_COLOR, HI_COLOR);
  else
    draw_box_outline(frame, left, right, top, bottom, FG_COLOR);
}


static void draw_rightj_text(const char *t, volatile uint8_t *frame, unsigned x, unsigned y) {
  unsigned twidth = font_width(t);
  uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x - twidth];
  draw_text_idx8_bus16(t, basept, SCREEN_WIDTH, FT_COLOR);
}

static void draw_central_text(const char *t, volatile uint8_t *frame, unsigned x, unsigned y) {
  unsigned twidth = font_width(t);
  uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x - twidth / 2];
  draw_text_idx8_bus16(t, basept, SCREEN_WIDTH, FT_COLOR);
}

static void draw_central_text_ovf(const char *t, volatile uint8_t *frame, unsigned x, unsigned y, unsigned maxw) {
  unsigned twidth = font_width(t);
  if (twidth <= maxw) {
    uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x - twidth / 2];
    draw_text_idx8_bus16(t, basept, SCREEN_WIDTH, FT_COLOR);
  } else {
    char tmpbuf[256];
    unsigned numchars = font_width_cap(t, maxw - THREEDOTS_WIDTH);
    memcpy(tmpbuf, t, numchars);
    memcpy(&tmpbuf[numchars], "...", 4);
    uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x - maxw / 2];
    draw_text_idx8_bus16(tmpbuf, basept, SCREEN_WIDTH, FT_COLOR);
  }
}

static void draw_central_text_wrapped(const char *t, volatile uint8_t *frame, unsigned x, unsigned y, unsigned maxw) {
  while (*t) {
    char tmp[128];
    unsigned outw;
    unsigned linechars = font_width_cap_space(t, maxw, &outw);
    unsigned charcnt = linechars ?: utf8_strlen(t);
    uint8_t *basept = (uint8_t*)&frame[y * SCREEN_WIDTH + x - outw / 2];

    memcpy(tmp, t, charcnt);
    tmp[charcnt] = 0;
    draw_text_idx8_bus16(tmp, basept, SCREEN_WIDTH, FT_COLOR);

    t += charcnt;      // Advance text
    y += 16;           // Move down in the buffer
  }
}

void render_recent(volatile uint8_t *frame) {
  // Render the list from memory.
  for (unsigned i = 0; i < RECENT_ROWS; i++) {
    if (smenu.recent.seloff + i >= smenu.recent.maxentries)
      break;

    t_rentry *e = &sdr_state->rentries[smenu.recent.seloff + i];
    char *fn = &e->fpath[e->fname_offset];
    render_icon(2, (i+1)*16, guessicon(fn));

    // Animate the row entries if they are too long!
    if (i == smenu.recent.selector - smenu.recent.seloff)
      draw_text_ovf_rotate(fn, frame, 20, (1 + i) * 16,
                           COVER_LIST_RIGHT - 24, &smenu.anim_state);
    else
      draw_text_ovf(fn, frame, 20, (1 + i) * 16, COVER_LIST_RIGHT - 24);
  }

  for (unsigned i = 0; i < COVER_SELECTOR_RIGHT; i += 16)
    render_icon_trans(i, (smenu.recent.selector - smenu.recent.seloff + 1)*16, 63);
}

#ifdef SUPPORT_NORGAMES
void render_flashbrowser(volatile uint8_t *frame) {
  // Render bar below to show block info
  dma_memset16(&frame[240*144], dup8(FG_COLOR), 240*16/2);

  // Render the list from memory.
  if (!smenu.fbrowser.maxentries)
    draw_central_text(msgs[lang_id][MSG_NOR_EMPTY], frame, SCREEN_WIDTH/2, SCREEN_HEIGHT/2-8);
  else {
    for (unsigned i = 0; i < NORGAMES_ROWS; i++) {
      if (smenu.fbrowser.seloff + i >= smenu.fbrowser.maxentries)
        break;

      const t_flash_game_entry *e = &sdr_state->nordata.games[smenu.fbrowser.seloff + i];
      render_icon(2, (i+1)*16, ICON_GBACART);

      // Animate the row entries if they are too long!
      char szstr[16];
      human_size(szstr, sizeof(szstr), e->numblks * NOR_BLOCK_SIZE);
      draw_rightj_text(szstr, frame, SCREEN_WIDTH - 2, (1 + i) * 16);

      // Animate the row entries if they are too long!
      const char *romname = &e->game_name[e->bnoffset];
      if (i == smenu.fbrowser.selector - smenu.fbrowser.seloff)
        draw_text_ovf_rotate(romname, frame, 20, (1 + i) * 16,
                             SCREEN_WIDTH - 26 - font_width(szstr), &smenu.anim_state);
      else
        draw_text_ovf(romname, frame, 20, (1 + i) * 16, SCREEN_WIDTH - 26 - font_width(szstr));
    }

    for (unsigned i = 0; i < 240; i += 16)
      render_icon_trans(i, (smenu.fbrowser.selector - smenu.fbrowser.seloff + 1)*16, 63);
  }

  char tmp[32], tmp1[32], tmp2[32];
  npf_snprintf(tmp, sizeof(tmp), "%u/%d", smenu.fbrowser.selector + 1, smenu.fbrowser.maxentries);
  draw_rightj_text(tmp, frame, SCREEN_WIDTH - 1, 1);

  human_size(tmp1, sizeof(tmp1), smenu.fbrowser.usedblks * NOR_BLOCK_SIZE);
  human_size(tmp2, sizeof(tmp2), NOR_GAMEBLOCK_COUNT * NOR_BLOCK_SIZE);
  npf_snprintf(tmp, sizeof(tmp), "Flash usage: %s/%s", tmp1, tmp2);
  draw_text_ovf(tmp, frame, 8, 144, SCREEN_WIDTH - 16);
}
#endif

void render_browser(volatile uint8_t *frame) {
  // Render bar below to show path URI
  dma_memset16(&frame[240*144], dup8(FG_COLOR), 240*16/2);

  if (!smenu.browser.dispentries)
    draw_central_text_ovf(msgs[lang_id][MSG_BROW_EMPTY], frame,
                          COVER_LIST_RIGHT / 2, SCREEN_HEIGHT/2-8,
                          COVER_LIST_RIGHT - 16);
  else {
    for (unsigned i = 0; i < BROWSER_ROWS; i++) {
      if (smenu.browser.seloff + i >= smenu.browser.dispentries)
        break;

      char szstr[16] = {0};
      t_centry *e = sdr_state->fileorder[smenu.browser.seloff + i];

      unsigned iconidx = (e->attr & AM_HID) ? ((e->attr & AM_DIR) ? ICON_HFOLDER : ICON_HFILE) :
                         (e->attr & AM_DIR) ? ICON_FOLDER :
                         guessicon(e->fname);

      render_icon(2, (i+1)*16, iconidx);

      if (!(e->attr & AM_DIR)) {
        human_size(szstr, sizeof(szstr), e->filesize);
        draw_rightj_text(szstr, frame, COVER_LIST_RIGHT - 2, (1 + i) * 16);
      }

      // Animate the row entries if they are too long!
      if (i == smenu.browser.selector - smenu.browser.seloff)
        draw_text_ovf_rotate(e->fname, frame, 20, (1 + i) * 16,
                             COVER_LIST_RIGHT - 26 - font_width(szstr), &smenu.anim_state);
      else
        draw_text_ovf(e->fname, frame, 20, (1 + i) * 16,
                      COVER_LIST_RIGHT - 26 - font_width(szstr));
    }

    for (unsigned i = 0; i < COVER_SELECTOR_RIGHT; i += 16)
      render_icon_trans(i, (smenu.browser.selector - smenu.browser.seloff + 1)*16, 63);
  }

  // Draw path, cut left part if necessary.
  draw_text_leftovf(smenu.browser.cpath, frame, 8, 144, SCREEN_WIDTH - 8);

  char selinfo[16];
  npf_snprintf(selinfo, sizeof(selinfo), "%u/%d", smenu.browser.selector + 1, smenu.browser.dispentries);
  draw_rightj_text(selinfo, frame, SCREEN_WIDTH - 1, 1);
}

static void render_cover_panel(volatile uint8_t *frame) {
  draw_box_outline(frame, COVER_PANEL_LEFT, SCREEN_WIDTH, COVER_PANEL_TOP,
                   COVER_IMAGE_TOP + COVER_HEIGHT + 2, FG_COLOR);

  if (menu_cover_cache.state == CoverReady) {
    const uint8_t *pixels = cover_cache_pixels(&menu_cover_cache);
    for (unsigned row = 0; row < COVER_HEIGHT; row++)
      dma_memcpy16(&frame[(COVER_IMAGE_TOP + row) * SCREEN_WIDTH + COVER_IMAGE_LEFT],
                   &pixels[row * COVER_WIDTH], COVER_WIDTH / 2);
  } else {
    char diagnostic[16];
    const char *message = menu_cover_cache.state == CoverPending ? "Loading..." :
                          menu_cover_cache.state == CoverMissing ? "No cover" :
                          menu_cover_cache.state == CoverInvalid ? "Bad cover" :
                          menu_cover_cache.state == CoverIoError ? "SD error" : NULL;
#ifdef __GBA__
    if (menu_cover_cache.state == CoverMissing) {
      uint32_t results = cover_fatfs_last_results();
      if ((results & 0xFF) != 0xFF) {
        npf_snprintf(diagnostic, sizeof(diagnostic), "R7 %u/%u/%u",
                     (unsigned)(results & 0xFF),
                     (unsigned)((results >> 8) & 0xFF),
                     (unsigned)((results >> 16) & 0xFF));
        message = diagnostic;
      }
    }
#endif
    if (message)
      draw_central_text(message, frame, COVER_IMAGE_LEFT + COVER_WIDTH / 2,
                        COVER_IMAGE_TOP + COVER_HEIGHT / 2 - 4);
  }
}

void render_fw_flash_popup(volatile uint8_t *frame) {
  // Render a box to give a pop-up feeling
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  draw_central_text(msgs[lang_id][MSG_FWUPD_MENU], frame, 120, 30);

  draw_box_outline(frame, 16, 224, 64, 92, FG_COLOR);
  if (spop.p.update.issfw) {
    char tmp[32];
    npf_snprintf(tmp, sizeof(tmp), SUPERR7_PRODUCT_NAME " (ver %lu.%lu)",
                 spop.p.update.superfw_ver >> 16,
                 spop.p.update.superfw_ver & 0xFFFF);
    draw_central_text(tmp, frame, 120, 70);
  } else {
    draw_central_text(msgs[lang_id][MSG_FWUPD_UNK], frame, 120, 70);
  }

  const char *smsg[] = {
    msgs[lang_id][MSG_FWUPD_GO],
    msgs[lang_id][MSG_FWUPD_LOADING],
    msgs[lang_id][MSG_FWUPD_CHECKING],
    msgs[lang_id][MSG_FWUPD_ERASING],
    msgs[lang_id][MSG_FWUPD_PROGRAM],
  };

  draw_central_text(smsg[spop.p.update.curr_state], frame, 120, 120);
}

void render_sav_menu_popup(volatile uint8_t *frame) {
  // Render a box to give a pop-up feeling
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  for (unsigned i = 0; i < 3; i++) {
    draw_button_box(frame, 20, 220, 32 + 28 * i, 32 + 28 * i + 20, spop.selector == i);
    draw_central_text(msgs[lang_id][MSG_SAVOPT_OPT0 + i], frame, 120, 34 + 28 * i);
  }
  draw_button_box(frame, 20, 220, 124, 144, spop.selector == SavQuit);
  draw_central_text(msgs[lang_id][MSG_CANCEL], frame, 120, 126);
}

static void render_gbarom_info(volatile uint8_t *frame, const char *dispname,
                               bool issf, const char *gcode, uint8_t ver, int save_type) {
  char tmp[64];
  draw_central_text(msgs[lang_id][MSG_GBALOAD_MINFO], frame, SCREEN_WIDTH/2, 23);

  const char *romname = file_basename(dispname);
  unsigned twidth = font_width(romname);
  if (twidth > SCREEN_WIDTH - 20)
    draw_text_ovf_rotate(romname, frame, 10, 52,
                         SCREEN_WIDTH - 20, &spop.anim);
  else
    draw_central_text_ovf(romname, frame, SCREEN_WIDTH/2, 52, SCREEN_WIDTH - 20);

  npf_snprintf(tmp, sizeof(tmp), msgs[lang_id][MSG_LOADINFO_GAME], gcode, ver);
  draw_central_text_ovf(tmp, frame, SCREEN_WIDTH/2, 82, SCREEN_WIDTH - 20);

  if (save_type < 0)
    draw_central_text_ovf(msgs[lang_id][MSG_LOADINFO_UNKW], frame, SCREEN_WIDTH/2, 102, SCREEN_WIDTH - 20);
  else if (issf)
    draw_central_text_ovf(SUPERR7_PRODUCT_NAME " firmware", frame,
                          SCREEN_WIDTH/2, 102, SCREEN_WIDTH - 20);
  else {
    const char *stype[] = {
      msgs[lang_id][MSG_SAVETYPE_NONE],       // SaveTypeNone
      msgs[lang_id][MSG_SAVETYPE_SRAM],       // SaveTypeSRAM
      msgs[lang_id][MSG_SAVETYPE_EEPROM],     // SaveTypeEEPROM4K
      msgs[lang_id][MSG_SAVETYPE_EEPROM],     // SaveTypeEEPROM64K
      msgs[lang_id][MSG_SAVETYPE_FLASH],      // SaveTypeFlash512K
      msgs[lang_id][MSG_SAVETYPE_FLASH],      // SaveTypeFlash1024K
    };
    const char *ssize[] = {
      "0KB",       // SaveTypeNone
      "32KB",      // SaveTypeSRAM
      "0.5KB",     // SaveTypeEEPROM4K
      "8KB",       // SaveTypeEEPROM64K
      "64KB",      // SaveTypeFlash512K
      "128KB",     // SaveTypeFlash1024K
    };

    npf_snprintf(tmp, sizeof(tmp), msgs[lang_id][MSG_LOADINFO_SAVE], stype[save_type], ssize[save_type]);
    draw_central_text_ovf(tmp, frame, SCREEN_WIDTH/2, 102, SCREEN_WIDTH - 20);
  }

  draw_box_full(frame, 20, 220, 132, 152, FG_COLOR, HI_COLOR);
}

static const char *render_gbarom_patching(volatile uint8_t *frame, const t_load_gba_info *info, int selector) {
  draw_central_text(msgs[lang_id][MSG_GBALOAD_MPATCH], frame, SCREEN_WIDTH/2, 23);
  draw_text_ovf(msgs[lang_id][MSG_DEFS_PATCH], frame, 12, 44, 224);
  draw_central_text(msgs[lang_id][MSG_PATCH_TYPE0 + info->patch_type], frame, 162, 44);
  draw_text_ovf(msgs[lang_id][MSG_LOADER_SAVET], frame, 12, 62, 224);
  draw_central_text(msgs[lang_id][MSG_LOADER_ST0 + (info->use_dsaving ? 0 : 1)], frame, 170, 62);
  draw_text_ovf(msgs[lang_id][MSG_LOADER_MENU], frame, 12, 80, 224);
  draw_central_text(msgs[lang_id][info->ingame_menu_enabled ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, 170, 80);
  draw_text_ovf(msgs[lang_id][MSG_LOADER_RTCE], frame, 12, 98, 224);
  draw_central_text(msgs[lang_id][info->rtc_patch_enabled ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, 170, 98);

  draw_text_ovf(msgs[lang_id][MSG_LOADER_PTCH], frame, 12, 116, 224);
  draw_box_outline(frame, 170 - 20, 170 + 20, 115, 133, FG_COLOR);
  draw_central_text("▸", frame, 170, 116);

  return (selector == GBALoadPatch) ? msgs[lang_id][MSG_PATCH_TYPE_I0 + info->patch_type] :
         (selector == GBASavePatch) ? msgs[lang_id][MSG_LOADER_ST_I0 + (info->use_dsaving ? 0 : 1)] :
         (selector == GBAInGameMen) ? msgs[lang_id][MSG_INGAME_I] :
         (selector == GBARTCPatch)  ? msgs[lang_id][MSG_PATCHRTC_I] :
         (selector == GBAPatchGen)  ? msgs[lang_id][MSG_PATCHE_I] : NULL;
}

static const char *render_gbarom_loading(volatile uint8_t *frame, const t_load_gba_lcfg *data, bool rtc_patching, int selector) {
  char tmp[64];
  draw_central_text(msgs[lang_id][MSG_GBALOAD_OPTS], frame, SCREEN_WIDTH/2, 23);
  draw_text_ovf(msgs[lang_id][MSG_LOADER_LOADP], frame, 12, 44, 224);
  draw_central_text(msgs[lang_id][MSG_LOADER_LOADP0 + data->sram_load_type], frame, 170, 44);
  draw_text_ovf(msgs[lang_id][MSG_LOADER_SAVEP], frame, 12, 62, 224);
  draw_central_text(msgs[lang_id][MSG_LOADER_SAVEP0 + data->sram_save_type], frame, 170, 62);
  draw_text_ovf(msgs[lang_id][MSG_DEF_RTCVAL], frame, 12, 80, 224);
  if (rtc_patching) {
    t_dec_date d;
    timestamp2date(data->rtcval, &d);
    npf_snprintf(tmp, sizeof(tmp), "20%02d/%02d/%02d %02d:%02d",
      d.year, d.month, d.day, d.hour, d.min);
    draw_central_text(tmp, frame, 170, 80);
  }
  else
    draw_central_text("-", frame, 170, 80);
  draw_text_ovf(msgs[lang_id][MSG_SETT_LDCHT], frame, 12, 98, 224);
  draw_central_text(msgs[lang_id][data->use_cheats ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, 170, 98);

  draw_box_outline(frame, 170 - 20, 170 + 20, 115, 133, FG_COLOR);
  draw_text_ovf(msgs[lang_id][MSG_SETT_REMEMB], frame, 12, 116, 224);
  render_icon(170-8, 116, ICON_DISK);

  return (selector == GBALdSetLoadP) ? msgs[lang_id][MSG_LOADER_LOADP_I0 + data->sram_load_type] :
         (selector == GBALdSetSaveP) ? msgs[lang_id][MSG_LOADER_SAVEP_I0 + data->sram_save_type] :
         (selector == GBALdSetCheats && !enable_cheats) ? msgs[lang_id][MSG_CHEATSDIS_I] :
         (selector == GBALdSetCheats && !data->cheats_found) ? msgs[lang_id][MSG_CHEATSNOA_I] :
         (selector == GBALdRemember) ? msgs[lang_id][MSG_REMEMB_I] : NULL;
}

void render_gba_load_popup(volatile uint8_t *frame) {
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);
  draw_text_ovf("⯇", frame, 10, 23, 64);
  draw_rightj_text("⯈", frame, SCREEN_WIDTH - 10, 23);

  const t_load_gba_info *info = &spop.p.load.i;
  const t_patch *p = get_game_patch(info);
  const char *ht = NULL;
  switch (spop.submenu) {
  case GbaLoadPopInfo:
    render_gbarom_info(frame, info->romfn, is_superfw(&info->romh), info->gcode,
                       info->romh.version, p ? p->save_mode : -1);
    draw_central_text(msgs[lang_id][MSG_LOAD_GBA], frame, 120, 134);
    break;
  case GbaLoadPopLoadS:
    ht = render_gbarom_loading(frame, &spop.p.load.l, info->rtc_patch_enabled, spop.selector);
    break;
  case GbaLoadPopPatch:
    ht = render_gbarom_patching(frame, &spop.p.load.i, spop.selector);
    break;
  };

  // Show some help if necessary
  if (ht) {
    unsigned twidth = font_width(ht);
    if (twidth > SCREEN_WIDTH - 20)
      draw_text_ovf_rotate(ht, frame, 10, 137, SCREEN_WIDTH - 20, &spop.anim);
    else
      draw_central_text_ovf(ht, frame, SCREEN_WIDTH/2, 137, SCREEN_WIDTH - 20);
  }

  if (spop.submenu != GbaLoadPopInfo) {
    const unsigned offy = 43;
    for (unsigned i = 8; i < 232; i += 16) {
      render_icon_trans(i, offy + 0 + spop.selector * 18, 63);
      render_icon_trans(i, offy + 2 + spop.selector * 18, 63);
    }
  }
}

void render_filemgr(volatile uint8_t *frame) {
  // Draw the file name and the options available
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  t_centry *e = sdr_state->fileorder[smenu.browser.selector];
  const char *bn = file_basename(e->fname);

  unsigned twidth = font_width(bn);
  if (twidth > SCREEN_WIDTH - 20)
    draw_text_ovf_rotate(bn, frame, 10, 32, SCREEN_WIDTH - 20, &spop.anim);
  else
    draw_central_text_ovf(bn, frame, SCREEN_WIDTH/2, 32, SCREEN_WIDTH - 20);

  for (unsigned i = 0; i < FiMgrCNT; i++)
    draw_button_box(frame, 20, 220, 60 + i*30, 80 + i*30, i == spop.selector);

  draw_central_text(msgs[lang_id][MSG_FMGR_DEL], frame, 120, 62 + 30*FiMgrDelete);
  draw_central_text(msgs[lang_id][(e->attr & AM_HID) ? MSG_FMGR_UNHIDE : MSG_FMGR_HIDE], frame, 120, 62 + 30*FiMgrHide);

  #ifdef SUPPORT_NORGAMES
  draw_central_text(msgs[lang_id][MSG_NOR_WRITE], frame, 120, 62 + 30*FiMgrWriteNOR);
  #endif
}


#ifdef SUPPORT_NORGAMES
void render_gba_norwrite(volatile uint8_t *frame) {
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  draw_text_ovf("⯇", frame, 10, 23, 64);
  draw_rightj_text("⯈", frame, SCREEN_WIDTH - 10, 23);

  if (spop.submenu == GbaLoadPopInfo) {
    const t_load_gba_info *info = &spop.p.norwr.i;
    const t_patch *p = get_game_patch(info);
    render_gbarom_info(frame, info->romfn, is_superfw(&info->romh),
                       info->gcode, info->romh.version, p ? p->save_mode : -1);
    draw_central_text(msgs[lang_id][MSG_NOR_WRITE], frame, 120, 134);
  } else {
    const char *ht = render_gbarom_patching(frame, &spop.p.norwr.i, spop.selector);
    if (ht) {
      unsigned twidth = font_width(ht);
      if (twidth > SCREEN_WIDTH - 20)
        draw_text_ovf_rotate(ht, frame, 10, 137, SCREEN_WIDTH - 20, &spop.anim);
      else
        draw_central_text_ovf(ht, frame, SCREEN_WIDTH/2, 137, SCREEN_WIDTH - 20);
    }
    const unsigned offy = 43;
    for (unsigned i = 8; i < 232; i += 16) {
      render_icon_trans(i, offy + 0 + spop.selector * 18, 63);
      render_icon_trans(i, offy + 2 + spop.selector * 18, 63);
    }
  }
}

void render_gba_norload(volatile uint8_t *frame) {
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  draw_text_ovf("⯇", frame, 10, 23, 64);
  draw_rightj_text("⯈", frame, SCREEN_WIDTH - 10, 23);

  const t_flash_game_entry *e = spop.p.norld.e;
  if (spop.submenu == GbaLoadPopInfo) {
    int save_type = GET_GATTR_SAVEM(e->gattrs);
    char gcode[5] = {
      e->gamecode & 0xFF, (e->gamecode >> 8) & 0xFF, (e->gamecode >> 16) & 0xFF, e->gamecode >> 24, 0
    };
    render_gbarom_info(frame, e->game_name, false, gcode, e->gamever, save_type);
    draw_central_text(msgs[lang_id][MSG_NOR_LAUNCH], frame, 120, 134);
  } else {
    bool rtc_patching = e->gattrs & GATTR_RTC;
    const char *ht = render_gbarom_loading(frame, &spop.p.norld.l, rtc_patching, spop.selector);
    if (ht) {
      unsigned twidth = font_width(ht);
      if (twidth > SCREEN_WIDTH - 20)
        draw_text_ovf_rotate(ht, frame, 10, 137, SCREEN_WIDTH - 20, &spop.anim);
      else
        draw_central_text_ovf(ht, frame, SCREEN_WIDTH/2, 137, SCREEN_WIDTH - 20);
    }
    const unsigned offy = 43;
    for (unsigned i = 8; i < 232; i += 16) {
      render_icon_trans(i, offy + 0 + spop.selector * 18, 63);
      render_icon_trans(i, offy + 2 + spop.selector * 18, 63);
    }
  }
}
#endif

void render_popupq(volatile uint8_t *frame, unsigned fcnt) {
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  // Draw question and two buttons
  draw_central_text_wrapped(spop.qpop.message, frame, SCREEN_WIDTH/2, 32, SCREEN_WIDTH - 20);

  if (spop.qpop.option == 0) {
    draw_box_full(frame, 20, 220, 90, 90 + 20, FG_COLOR, HI_COLOR);
    draw_box_outline(frame, 20, 220, 120, 120 + 20, FG_COLOR);
  } else {
    draw_box_full(frame, 20, 220, 120, 120 + 20, FG_COLOR, HI_COLOR);
    draw_box_outline(frame, 20, 220, 90, 90 + 20, FG_COLOR);
  }

  draw_central_text(spop.qpop.default_button, frame, 120, 92);
  draw_central_text(spop.qpop.confirm_button, frame, 120, 122);
}

void render_rtcpop(volatile uint8_t *frame) {
  draw_box_outline(frame, 2, 240-2, 18, 158, FG_COLOR);

  draw_central_text(msgs[lang_id][MSG_DEF_RTCVAL], frame, SCREEN_WIDTH/2, 32);

  const t_dec_date *v = &spop.rtcpop.val;
  char thour[3] = {'0' + v->hour /10, '0' + v->hour  % 10, 0};
  char tmins[3] = {'0' + v->min  /10, '0' + v->min   % 10, 0};
  char tdays[3] = {'0' + v->day  /10, '0' + v->day   % 10, 0};
  char tmont[3] = {'0' + v->month/10, '0' + v->month % 10, 0};
  char tyear[5] = {'2', '0', '0' + v->year/10, '0' + v->year % 10, 0};

  draw_central_text(tyear, frame,  60, 70);
  draw_central_text("-",   frame,  80, 70);
  draw_central_text(tmont, frame,  94, 70);
  draw_central_text("-",   frame, 106, 70);
  draw_central_text(tdays, frame, 120, 70);
  draw_central_text(thour, frame, 154, 70);
  draw_central_text(":",   frame, 166, 70);
  draw_central_text(tmins, frame, 180, 70);

  const uint8_t cox[] = {
    60, 94, 120, 154, 180
  };
  draw_central_text("⯅", frame, cox[spop.rtcpop.selector], 54);
  draw_central_text("⯆", frame, cox[spop.rtcpop.selector], 84);
}

#endif

#ifndef UI_BROWSER_V2
void render_settings(volatile uint8_t *frame) {
  char tmp[128];
  unsigned baseopt = smenu.set.selector <= 2  ? 0 :
                     smenu.set.selector >= SettMAX - 3 ? SettMAX - 5 :
                     smenu.set.selector - 2;

  if (smenu.set.selector > 2)
    draw_central_text("⯅", frame, 120, 15);
  if (smenu.set.selector < SettSave - 2)
    draw_central_text("⯆", frame, 120, 125);

  const unsigned maxrows = 5;
  unsigned optnum = 0, optcnt = 0;
  const unsigned colx = 170;           // Center point for the selection boxes
  const unsigned offy = 29;
  const unsigned rowh = 20;

  if (optnum++ >= baseopt && optcnt < maxrows)
    draw_central_text(msgs[lang_id][MSG_SET_TITL1], frame, SCREEN_WIDTH/2, offy + rowh*optcnt++);

  if (optnum++ >= baseopt && optcnt < maxrows) {
    npf_snprintf(tmp, sizeof(tmp), "< %s >", hotkey_list[hotkey_combo].cname);
    draw_text_ovf(msgs[lang_id][MSG_SETT_HOTK], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(tmp, frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_BOOT], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][MSG_BOOT_TYPE0 + boot_bios_splash], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_FASTSD], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][use_slowld ? MSG_KNOB_DISABLED : MSG_KNOB_ENABLED], frame, colx, offy + rowh*optcnt++);
  }

  #ifdef SUPPORT_NORGAMES
  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_VERNOR], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][use_verify_nor ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, colx, offy + rowh*optcnt++);
  }
  #endif

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_FASTEW], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][use_fastew ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_SAVET], frame, 8, offy + rowh*optcnt, 224);

    if (save_path_default == SaveRomName)
      draw_central_text(msgs[lang_id][MSG_NEXTTO_ROM], frame, colx, offy + rowh*optcnt++);
    else {
      npf_snprintf(tmp, sizeof(tmp), "< %s >", save_paths[save_path_default]);
      draw_central_text(tmp, frame, colx, offy + rowh*optcnt++);
    }
  }

  #ifdef SUPPORT_NORGAMES
  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_SAVETX], frame, 8, offy + rowh*optcnt, 224);
    npf_snprintf(tmp, sizeof(tmp), "< %s >", save_paths[save_path_nor_default]);
    draw_central_text(tmp, frame, colx, offy + rowh*optcnt++);
  }
  #endif

  if (optnum++ >= baseopt && optcnt < maxrows) {
    npf_snprintf(tmp, sizeof(tmp), "< %u >", backup_sram_default);
    draw_text_ovf(msgs[lang_id][MSG_SETT_SAVEBK], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(tmp, frame, colx, offy + rowh*optcnt++ );
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_STATET], frame, 8, offy + rowh*optcnt, 224);
    npf_snprintf(tmp, sizeof(tmp), "< %s >", savestates_paths_display[state_path_default]);
    draw_central_text(tmp, frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_SETT_CHTEN], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][enable_cheats ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows)
    draw_central_text(msgs[lang_id][MSG_SET_TITL2], frame, SCREEN_WIDTH/2, offy + rowh*optcnt++);

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_DEFS_PATCH], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][MSG_PATCH_TYPE0 + patcher_default], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_LOADER_MENU], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][MSG_KNOB_DISABLED + ingamemenu_default], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_LOADER_RTCE], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][MSG_KNOB_DISABLED + rtcpatch_default], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    t_dec_date d;
    timestamp2date(rtcvalue_default, &d);
    npf_snprintf(tmp, sizeof(tmp), "20%02d/%02d/%02d %02d:%02d",
      d.year, d.month, d.day, d.hour, d.min);
    draw_text_ovf(msgs[lang_id][MSG_DEF_RTCVAL], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(tmp, frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    unsigned spdmsg = rtcspeed_default ? (MSG_UIS_SPD0 + rtcspeed_default - 1) :
                                          MSG_STILLRTC;
    npf_snprintf(tmp, sizeof(tmp), "< %s >", msgs[lang_id][spdmsg]);
    draw_text_ovf(msgs[lang_id][MSG_DEF_SPEED], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(tmp, frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_LOADER_LOADP], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][MSG_DEF_LOADP0 + (autoload_default ^ 1)], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_LOADER_SAVEP], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][autosave_default ? MSG_DEF_SAVEP0 : MSG_DEF_SAVEP1], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_text_ovf(msgs[lang_id][MSG_LOADER_PREFDS], frame, 8, offy + rowh*optcnt, 224);
    draw_central_text(msgs[lang_id][autosave_prefer_ds ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, colx, offy + rowh*optcnt++);
  }

  if (optnum++ >= baseopt && optcnt < maxrows) {
    draw_button_box(frame, 20, 220, 112, 132, smenu.set.selector == SettSave);
    draw_central_text(msgs[lang_id][MSG_UIS_SAVE], frame, 132, 114);
  }

  // Render bar below for help messge
  dma_memset16(&frame[240*140], dup8(FG_COLOR), 240*20/2);

  if (smenu.set.selector == SettSaveLoc) {
    if (save_path_default == SaveRomName)
      draw_text_ovf_rotate(msgs[lang_id][MSG_SAVE_TYPE_NR], frame, 4, SCREEN_HEIGHT - 18, 232, &smenu.anim_state);
    else {
      npf_snprintf(tmp, sizeof(tmp), msgs[lang_id][MSG_SAVE_TYPE_PT], save_paths[save_path_default]);
      draw_text_ovf_rotate(tmp, frame, 4, SCREEN_HEIGHT - 18, 232, &smenu.anim_state);
    }
  }
  else if (smenu.set.selector == SettStateLoc) {
    npf_snprintf(tmp, sizeof(tmp), msgs[lang_id][MSG_STATE_TYPE_PT], savestates_paths[state_path_default]);
    draw_text_ovf_rotate(tmp, frame, 4, SCREEN_HEIGHT - 18, 232, &smenu.anim_state);
  }
  #ifdef SUPPORT_NORGAMES
  else if (smenu.set.selector == SettSaveLocNOR) {
    npf_snprintf(tmp, sizeof(tmp), msgs[lang_id][MSG_SAVE_TYPE_PTX], save_paths[save_path_nor_default]);
    draw_text_ovf_rotate(tmp, frame, 4, SCREEN_HEIGHT - 18, 232, &smenu.anim_state);
  }
  #endif
  else {
    unsigned help_msg = smenu.set.selector == SettBootType ? MSG_BOOT_TYPE_I0 + boot_bios_splash :
                        smenu.set.selector == SettSaveBkp  ? MSG_BACKUP_I :
                        smenu.set.selector == SettFastSD   ? MSG_FASTSD_I :
                        smenu.set.selector == SettFastEWRAM? MSG_FASTEW_I :
                        smenu.set.selector == DefsPatchEng ? MSG_PATCH_TYPE_I0 + patcher_default :
                        smenu.set.selector == DefsLoadPol  ? MSG_DEF_LOADP_I0 + (autoload_default ^ 1) :
                        smenu.set.selector == DefsSavePol  ? MSG_DEF_SAVEP_I0 + (autosave_default ^ 1) :
                        smenu.set.selector == DefsPrefDS   ? MSG_LOADER_PREFDSI :
                        #ifdef SUPPORT_NORGAMES
                        smenu.set.selector == SettVerifyNOR ? MSG_VERNOR_I :
                        #endif
                        MSG_EMPTY;
    draw_text_ovf_rotate(msgs[lang_id][help_msg], frame, 4, SCREEN_HEIGHT - 18, 232, &smenu.anim_state);
  }

  if (smenu.set.selector != SettSave)
    for (unsigned i = 0; i < 240; i += 16)
      render_icon_trans(i, offy + (smenu.set.selector - baseopt) * 20, 63);
}

void render_ui_settings(volatile uint8_t *frame) {
  const unsigned colx = 170;
  char tmpbuf[64];
  npf_snprintf(tmpbuf, sizeof(tmpbuf), "< %u >", menu_theme + 1U);
  draw_text_ovf(msgs[lang_id][MSG_UIS_THEME], frame, 8, 22, 224);
  draw_central_text(tmpbuf, frame, colx, 22 );

  draw_text_ovf(msgs[lang_id][MSG_UIS_RECNT], frame, 8, 22 + 20, 224);
  draw_central_text(msgs[lang_id][recent_menu ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED], frame, colx, 22 + 20 );

  npf_snprintf(tmpbuf, sizeof(tmpbuf), "< %s >", msgs[lang_id][MSG_UIS_SPD0 + anim_speed]);
  draw_text_ovf(msgs[lang_id][MSG_UIS_ANSPD], frame, 8, 22 + 40, 224);
  draw_central_text(tmpbuf, frame, colx, 22 + 40 );

  draw_text_ovf(msgs[lang_id][MSG_UIS_BHID], frame, 8, 22 + 60, 224);
  draw_central_text(msgs[lang_id][hide_hidden ? MSG_KNOB_DISABLED : MSG_KNOB_ENABLED], frame, colx, 22 + 60 );

  if (smenu.uiset.selector != UiSetSave)
    for (unsigned i = 0; i < 240; i += 16)
      render_icon_trans(i, 22 + smenu.uiset.selector * 20, 63);

  draw_button_box(frame, 20, 220, 132, 152, smenu.uiset.selector == UiSetSave);
  draw_central_text(msgs[lang_id][MSG_UIS_SAVE], frame, 120, 134);
}

void render_info(volatile uint8_t *frame) {
  char tmp[64], tmp2[32];

  init_logo_palette(&MEM_PALETTE[1]);
  render_logo((uint16_t*)frame, SCREEN_WIDTH/2, 40, 4);

  switch (smenu.info.selector) {
  case 0:
    draw_central_text(SUPERR7_PRODUCT_NAME, frame, 120, 66);
    draw_central_text("Maintainer: " SUPERR7_MAINTAINER, frame, 120, 84);
    draw_central_text("Based on: " SUPERR7_UPSTREAM_NAME, frame, 120, 104);
    draw_central_text("Build:", frame, 120, 123);
    break;
  case 1:
    draw_central_text("Flash info", frame, 120, 70);
    npf_snprintf(tmp, sizeof(tmp), "Dev ID: %08lx", flashinfo.deviceid);
    draw_central_text(tmp, frame, 120, 95);
    if (flashinfo.size && flashinfo.blksize && flashinfo.blkcount) {
      human_size_kb(tmp2, sizeof(tmp2), flashinfo.size >> 10);
      npf_snprintf(tmp, sizeof(tmp), "%s [%lu * %lu]", tmp2, flashinfo.blksize, flashinfo.blkcount);
      if (flashinfo.regioncnt != 1)
        strcat(tmp, " !");
      draw_central_text(tmp, frame, 120, 115);
    } else {
      npf_snprintf(tmp, sizeof(tmp), "No CFI! (hardwired %dKiB)", FW_MAX_SIZE_KB);
      draw_central_text(tmp, frame, 120, 115);
    }
    break;
  case 2:
    draw_central_text(msgs[lang_id][MSG_DBPINFO], frame, 120, 70);
    npf_snprintf(tmp, sizeof(tmp), "%s - %s", pdbinfo.version, pdbinfo.date);
    draw_central_text(tmp, frame, 120, 90);
    npf_snprintf(tmp, sizeof(tmp), "Game count: %lu", pdbinfo.patch_count);
    draw_central_text(tmp, frame, 120, 110);
    break;
  case 3:
    if (sd_info.sdhc)
      draw_central_text("SD card type: SDHC", frame, 120, 70);
    else
      draw_central_text("SD card type: SDSC", frame, 120, 70);
    human_size_kb(tmp2, sizeof(tmp2), sd_info.block_cnt / 2);
    npf_snprintf(tmp, sizeof(tmp), msgs[lang_id][MSG_CAPACITY], tmp2);
    draw_central_text(tmp, frame, 120, 90);
    npf_snprintf(tmp, sizeof(tmp), "Card ID: %02x | %04x", sd_info.manufacturer, sd_info.oemid);
    draw_central_text(tmp, frame, 120, 110);
    break;
  }

  // Flashing info
  dma_memset16(&frame[138*SCREEN_WIDTH], dup8(FG_COLOR), SCREEN_WIDTH*22/2);
  draw_text_ovf_rotate(enable_flashing ? msgs[lang_id][MSG_FWUP_ENABLED] : msgs[lang_id][MSG_FWUP_HOTKEY], frame,
                       4, 141, SCREEN_WIDTH - 8, &smenu.anim_state);
}

void render_tools(volatile uint8_t *frame) {
  for (unsigned i = 0; i < ToolsMAX; i++)
    draw_text_ovf(msgs[lang_id][MSG_TOOLS0_SDRAM + i], frame, 22, 26 + 22 * i, 144);

  smenu.anim_state = (smenu.anim_state + 1) & 255;
  draw_central_text("▸", frame, 11 + (smenu.anim_state >> 6), 26 + 22 * smenu.tools.selector);

  for (unsigned i = 0; i < 240; i += 16)
    render_icon_trans(i, 26 + smenu.tools.selector * 22, 63);
}
#endif

void reload_theme(void) {
  // Palette 0..15 contains the main menu template colors
  MEM_PALETTE[FG_COLOR] = themes[0].fg_color;
  MEM_PALETTE[BG_COLOR] = themes[0].bg_color;
  MEM_PALETTE[FT_COLOR] = themes[0].ft_color;
  MEM_PALETTE[HI_COLOR] = themes[0].hi_color;
  // In-game menu palette
  MEM_PALETTE[IGM_PAL_FG] = themes[0].fg_color;
  MEM_PALETTE[IGM_PAL_BG] = themes[0].bg_color;
  MEM_PALETTE[IGM_PAL_HI] = themes[0].ft_color;
  MEM_PALETTE[IGM_PAL_SH] = themes[0].sh_color;
  MEM_PALETTE[IGM_PAL_BL] = themes[0].hi_blend;

  // Palette entries for icons and other objects
  MEM_PALETTE[256 + SEL_COLOR] = themes[0].hi_blend;
}

#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
static const struct {
  const t_mrender_fn render;
  const int max_submenu;
} popup_windows[] = {
  { render_gba_load_popup, GbaLoadCNT },
  { render_sav_menu_popup, 1 },
  { render_fw_flash_popup, 1 },
  { render_filemgr,        1 },
  #ifdef SUPPORT_NORGAMES
  { render_gba_norwrite,   GbaNorWrCNT },
  { render_gba_norload,    GbaNorLoadCNT },
  #endif
};
#else
static const uint8_t popup_submenus[] = {
  0, GbaLoadCNT, 1, 1, 1, 1,
};
#endif

#ifdef UI_BROWSER_V2
static t_ui_browser_v2_entry_kind ui_browser_v2_entry_kind(const t_centry *entry) {
  if (entry->attr & AM_DIR)
    return !strcmp(entry->fname, "..") ? UiBrowserV2Parent : UiBrowserV2Folder;

  unsigned icon = guessicon(entry->fname);
  if (icon == ICON_UPDFILE)
    return UiBrowserV2Firmware;
  if (icon != ICON_BINFILE)
    return UiBrowserV2Game;

  unsigned length = strlen(entry->fname);
  if (length >= 4 && !strcasecmp(&entry->fname[length - 4], ".sav"))
    return UiBrowserV2Save;
  return UiBrowserV2Other;
}

static unsigned ui_tools_message(unsigned tool) {
  switch (tool) {
  case ToolsAppearance: return MSG_TOOLS_APPEAR;
  case ToolsInterface:  return MSG_TOOLS_INTERFACE;
  case ToolsSettings:   return MSG_TOOLS_SETTINGS;
  case ToolsInfo:       return MSG_TOOLS_INFO;
  case ToolsResetFavorites: return MSG_TOOLS_RESET_FAVORITES;
  case ToolsResetRecent:    return MSG_TOOLS_RESET_RECENT;
  default:              return MSG_TOOLS0_SDRAM + tool - ToolsSDRAMTest;
  }
}

static const char *ui_setting_value(unsigned option, char *tmp, unsigned tmpsz) {
  switch (option) {
  case SettHotkey:
    return hotkey_list[hotkey_combo].cname;
  case SettBootType:
    return msgs[lang_id][MSG_BOOT_TYPE0 + boot_bios_splash];
  case SettFastSD:
    return msgs[lang_id][use_slowld ? MSG_KNOB_DISABLED : MSG_KNOB_ENABLED];
#ifdef SUPPORT_NORGAMES
  case SettVerifyNOR:
    return msgs[lang_id][use_verify_nor ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
#endif
  case SettFastEWRAM:
    return msgs[lang_id][use_fastew ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
  case SettSaveLoc:
    return save_path_default == SaveRomName ? msgs[lang_id][MSG_NEXTTO_ROM] :
                                             save_paths[save_path_default];
#ifdef SUPPORT_NORGAMES
  case SettSaveLocNOR:
    return save_paths[save_path_nor_default];
#endif
  case SettSaveBkp:
    npf_snprintf(tmp, tmpsz, "%u", backup_sram_default);
    return tmp;
  case SettStateLoc:
    return savestates_paths[state_path_default];
  case SettCheatEn:
    return msgs[lang_id][enable_cheats ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
  case DefsPatchEng:
    return msgs[lang_id][MSG_PATCH_TYPE0 + patcher_default];
  case DefsGamMenu:
    return msgs[lang_id][MSG_KNOB_DISABLED + ingamemenu_default];
  case DefsRTCEnb:
    return msgs[lang_id][MSG_KNOB_DISABLED + rtcpatch_default];
  case DefsRTCVal: {
    t_dec_date date;
    timestamp2date(rtcvalue_default, &date);
    npf_snprintf(tmp, tmpsz, "20%02d/%02d/%02d %02d:%02d",
                 date.year, date.month, date.day, date.hour, date.min);
    return tmp;
  }
  case DefsRTCSpeed: {
    unsigned speed = rtcspeed_default ? MSG_UIS_SPD0 + rtcspeed_default - 1 :
                                        MSG_STILLRTC;
    return msgs[lang_id][speed];
  }
  case DefsLoadPol:
    return msgs[lang_id][MSG_DEF_LOADP0 + (autoload_default ^ 1)];
  case DefsSavePol:
    return msgs[lang_id][autosave_default ? MSG_DEF_SAVEP0 : MSG_DEF_SAVEP1];
  case DefsPrefDS:
    return msgs[lang_id][autosave_prefer_ds ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
  case SettSave:
    return "A: SAVE";
  default:
    return NULL;
  }
}

static unsigned ui_current_dock(void) {
  return smenu.menu_tab == MENUTAB_FAVORITES ? 0 :
         smenu.menu_tab == MENUTAB_RECENT ? 1 :
         smenu.menu_tab == MENUTAB_ROMBROWSE ? 2 : 3;
}

static void render_ui_phase5_popup(volatile uint8_t *frame) {
  t_ui_browser_v2_model model;
  char value1[48];
  memset(&model, 0, sizeof(model));
  model.wide = true;
  model.selected_dock = ui_current_dock();

  if (spop.alert_msg) {
    model.row_top = 40;
    model.entry_count = 2;
    model.selected_row = 1;
    model.entries[0].name = spop.alert_msg;
    model.entries[1].name = "OK";
    model.entries[1].value = "A/B: CLOSE";
  } else if (spop.qpop.message) {
    model.row_top = 28;
    model.entry_count = 3;
    model.selected_row = spop.qpop.option + 1;
    model.entries[0].name = spop.qpop.message;
    model.entries[1].name = spop.qpop.default_button;
    model.entries[2].name = spop.qpop.confirm_button;
    model.entries[1].value = "A: SELECT";
    model.entries[2].value = "A: SELECT";
  } else if (spop.rtcpop.callback) {
    static const char *const fields[] = {
      "Year", "Month", "Day", "Hour", "Minute",
    };
    t_dec_date *date = &spop.rtcpop.val;
    model.row_top = 28;
    model.entry_count = 3;
    model.selected_row = 1;
    model.entries[0].name = "RTC date and time";
    npf_snprintf(value1, sizeof(value1), "20%02d/%02d/%02d %02d:%02d",
                 date->year, date->month, date->day, date->hour, date->min);
    model.entries[0].value = value1;
    model.entries[1].name = fields[spop.rtcpop.selector];
    model.entries[1].value = "UP/DOWN";
    model.entries[2].name = "Accept";
    model.entries[2].value = "A: OK  B: BACK";
  } else if (spop.pop_num == POPUP_GBA_LOAD) {
    const t_load_gba_info *info = &spop.p.load.i;
    model.hide_dock = true;
    if (spop.submenu == GbaLoadPopInfo) {
      model.layout = UiBrowserV2LayoutLaunch;
      model.show_cover = true;
      model.cover_state = menu_cover_cache.state;
      model.cover_pixels = cover_cache_pixels(&menu_cover_cache);
      model.anim_state = spop.anim;
      model.entry_count = 5;
      model.selected_row = spop.selector + 1;
      model.entries[0].name = file_basename(info->romfn);
      model.entries[0].centered = true;
      model.entries[1].name = "Launch game";
      model.entries[1].centered = true;
      model.entries[2].name = favorite_find(info->romfn) >= 0 ?
                              "Remove Favorite" : "Add to Favorites";
      model.entries[2].centered = true;
      model.entries[3].name = "Options";
      model.entries[3].value = "AUTOMATIC";
      model.entries[4].name = "Details";
      model.entries[4].value = "INFO";
      model.footer_left = "A: SELECT";
      model.footer_right = "B: BACK";
    } else if (spop.submenu == GbaLoadPopLoadS) {
      const t_load_gba_lcfg *load = &spop.p.load.l;
      model.wide = true;
      model.entry_count = 6;
      model.selected_row = spop.selector + 1;
      model.entries[0].name = "Game options";
      model.entries[0].value = "R: ADVANCED";
      model.entries[1].name = "Load save";
      model.entries[1].value = msgs[lang_id][MSG_LOADER_LOADP0 + load->sram_load_type];
      model.entries[2].name = "Save progress";
      model.entries[2].value = msgs[lang_id][MSG_LOADER_SAVEP0 + load->sram_save_type];
      model.entries[3].name = "RTC time";
      if (info->rtc_patch_enabled) {
        t_dec_date date;
        timestamp2date(load->rtcval, &date);
        npf_snprintf(value1, sizeof(value1), "20%02d/%02d/%02d %02d:%02d",
                     date.year, date.month, date.day, date.hour, date.min);
        model.entries[3].value = value1;
      } else {
        model.entries[3].value = "OFF";
      }
      model.entries[4].name = "Cheats";
      model.entries[4].value = msgs[lang_id][load->use_cheats ?
                               MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
      model.entries[5].name = "Remember settings";
      model.entries[5].value = "A: SAVE";
      model.footer_left = "L/R: CHANGE";
      model.footer_right = "B: BACK";
    } else if (spop.submenu == GbaLoadPopPatch) {
      model.wide = true;
      model.entry_count = 6;
      model.selected_row = spop.selector + 1;
      model.entries[0].name = "Advanced";
      model.entries[0].value = "L: OPTIONS";
      model.entries[1].name = "Compatibility";
      model.entries[1].value = msgs[lang_id][MSG_PATCH_TYPE0 + info->patch_type];
      model.entries[2].name = "DirectSave";
      model.entries[2].value = msgs[lang_id][MSG_LOADER_ST0 +
                               (info->use_dsaving ? 0 : 1)];
      model.entries[3].name = "In-game menu";
      model.entries[3].value = msgs[lang_id][info->ingame_menu_enabled ?
                               MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
      model.entries[4].name = "RTC patch";
      model.entries[4].value = msgs[lang_id][info->rtc_patch_enabled ?
                               MSG_KNOB_ENABLED : MSG_KNOB_DISABLED];
      model.entries[5].name = "Build patches";
      model.entries[5].value = "A: BUILD";
      model.footer_left = "L/R: CHANGE";
      model.footer_right = "B: BACK";
    } else {
      static const char *const patch_details[] = {
        "Built-in DB", "Patch engine", "Not needed", "Automatic",
      };
      model.wide = true;
      model.entry_count = 6;
      model.selected_row = 0xFF;
      model.entries[0].name = "Game details";
      model.entries[1].name = "Game";
      model.entries[1].value = file_basename(info->romfn);
      model.entries[2].name = "Game code";
      npf_snprintf(value1, sizeof(value1), "%s v%u", info->gcode,
                   info->romh.version);
      model.entries[2].value = value1;
      model.entries[3].name = "Compatibility";
      model.entries[3].value = "READY";
      model.entries[4].name = "Patching";
      model.entries[4].value = patch_details[info->patch_type];
      model.entries[5].name = "Save system";
      model.entries[5].value = msgs[lang_id][MSG_LOADER_ST0 +
                               (info->use_dsaving ? 0 : 1)];
      model.footer_left = "INFO";
      model.footer_right = "B: BACK";
    }
  } else if (spop.pop_num == POPUP_SAVFILE) {
    model.row_top = 18;
    model.entry_count = 4;
    model.selected_row = spop.selector;
    for (unsigned i = 0; i < 3; i++)
      model.entries[i].name = msgs[lang_id][MSG_SAVOPT_OPT0 + i];
    model.entries[3].name = msgs[lang_id][MSG_CANCEL];
#if !defined(SUPPORT_NORGAMES)
  } else if (spop.pop_num == POPUP_BROWSER_VIEW) {
    static const char *const game_filters[] = {
      "ALL FILES", "GBA/GB/GBC", "GBA ONLY", "GB ONLY", "GBC ONLY",
    };
    model.hide_dock = true;
    model.row_top = 18;
    model.entry_count = 5;
    model.selected_row = spop.selector + 1;
    model.entries[0].name = "View & sort";
    model.entries[0].centered = true;
    model.entries[1].name = "Filename order";
    model.entries[1].value = browser_sort_descending ? "Z-A" : "A-Z";
    model.entries[2].name = "Game type";
    model.entries[2].value = game_filters[browser_game_filter];
    model.entries[3].name = "Folders";
    model.entries[3].value = browser_hide_folders ? "HIDDEN" : "SHOWN";
    model.entries[4].name = "Unknown files";
    model.entries[4].value = browser_hide_unknown ? "HIDDEN" : "SHOWN";
    model.footer_left = "A: CHANGE";
    model.footer_right = "START/B: BACK";
#endif
  } else if (spop.pop_num == POPUP_FILE_MGR) {
    t_centry *entry = sdr_state->fileorder[smenu.browser.selector];
    model.row_top = 18;
    model.entry_count = FiMgrCNT;
    model.selected_row = spop.selector;
    model.entries[0].name = msgs[lang_id][MSG_FMGR_DEL];
    model.entries[1].name = msgs[lang_id][(entry->attr & AM_HID) ?
                            MSG_FMGR_UNHIDE : MSG_FMGR_HIDE];
#ifdef SUPPORT_NORGAMES
    model.entries[2].name = msgs[lang_id][MSG_NOR_WRITE];
#endif
  } else if (spop.pop_num == POPUP_FWFLASH) {
    model.row_top = 28;
    model.entry_count = 3;
    model.selected_row = 2;
    model.entries[0].name = msgs[lang_id][MSG_FWUPD_MENU];
    model.entries[1].name = spop.p.update.issfw ? SUPERR7_PRODUCT_NAME " firmware" :
                                                msgs[lang_id][MSG_FWUPD_UNK];
    static const unsigned states[] = {
      MSG_FWUPD_GO, MSG_FWUPD_LOADING, MSG_FWUPD_CHECKING,
      MSG_FWUPD_ERASING, MSG_FWUPD_PROGRAM,
    };
    model.entries[2].name = msgs[lang_id][states[spop.p.update.curr_state]];
    model.entries[2].value = "UP+L+R";
  }

  for (unsigned i = 0; i < model.entry_count; i++)
    model.entries[i].kind = UiBrowserV2Other;
  ui_browser_v2_render(frame, &model);
}

static void render_ui_browser_phase4(volatile uint8_t *frame) {
  t_ui_browser_v2_model model;
  char browse_page_text[24];
  memset(&model, 0, sizeof(model));

  model.cover_state = menu_cover_cache.state;
  model.cover_pixels = cover_cache_pixels(&menu_cover_cache);
  model.anim_state = smenu.anim_state;

  model.selected_dock = 2;
  if (smenu.menu_tab == MENUTAB_FAVORITES)
    model.selected_dock = 0;
  else if (smenu.menu_tab == MENUTAB_RECENT)
    model.selected_dock = 1;
  else if (smenu.menu_tab != MENUTAB_ROMBROWSE)
    model.selected_dock = 3;

  if (smenu.menu_tab == MENUTAB_FAVORITES) {
    if (smenu.favorites.maxentries) {
      model.show_cover = true;
      unsigned selected_row = smenu.favorites.selector -
                              smenu.favorites.seloff;
      unsigned available = smenu.favorites.maxentries -
                           smenu.favorites.seloff;
      model.entry_count = MIN(available, UI_BROWSER_V2_ROWS);
      model.selected_row = MIN(selected_row, model.entry_count - 1);

      for (unsigned i = 0; i < model.entry_count; i++) {
        t_rentry *entry =
          &sdr_state->faventries[smenu.favorites.seloff + i];
        model.entries[i].name = &entry->fpath[entry->fname_offset];
        model.entries[i].kind = UiBrowserV2Game;
      }
    }
  } else if (smenu.menu_tab == MENUTAB_ROMBROWSE) {
    model.show_cover = true;
    npf_snprintf(browse_page_text, sizeof(browse_page_text), "%u/%u",
                 menu_page_number(smenu.browser.selector,
                                  smenu.browser.dispentries, BROWSER_ROWS),
                 menu_page_count(smenu.browser.dispentries, BROWSER_ROWS));
    model.page_text = browse_page_text;
    unsigned selected_row = smenu.browser.selector - smenu.browser.seloff;
    unsigned available = smenu.browser.dispentries - smenu.browser.seloff;
    model.entry_count = MIN(available, UI_BROWSER_V2_ROWS);
    model.selected_row = MIN(selected_row, model.entry_count ? model.entry_count - 1 : 0);

    for (unsigned i = 0; i < model.entry_count; i++) {
      t_centry *entry = sdr_state->fileorder[smenu.browser.seloff + i];
      model.entries[i].name = entry->fname;
      model.entries[i].kind = ui_browser_v2_entry_kind(entry);
      model.entries[i].hidden = !!(entry->attr & AM_HID);
    }
  } else if (smenu.menu_tab == MENUTAB_RECENT) {
    model.show_cover = true;
    unsigned selected_row = smenu.recent.selector - smenu.recent.seloff;
    unsigned available = smenu.recent.maxentries - smenu.recent.seloff;
    model.entry_count = MIN(available, UI_BROWSER_V2_ROWS);
    model.selected_row = MIN(selected_row, model.entry_count ? model.entry_count - 1 : 0);

    for (unsigned i = 0; i < model.entry_count; i++) {
      t_rentry *entry = &sdr_state->rentries[smenu.recent.seloff + i];
      model.entries[i].name = &entry->fpath[entry->fname_offset];
      model.entries[i].kind = UiBrowserV2Game;
    }
  } else if (smenu.menu_tab == MENUTAB_TOOLS) {
    model.wide = true;
    model.entry_count = MIN(ToolsMAX - smenu.tools.seloff,
                            UI_BROWSER_V2_ROWS);
    model.selected_row = smenu.tools.selector - smenu.tools.seloff;
    for (unsigned i = 0; i < model.entry_count; i++) {
      unsigned tool = smenu.tools.seloff + i;
      model.entries[i].name = msgs[lang_id][ui_tools_message(tool)];
      model.entries[i].kind = UiBrowserV2Other;
    }
  } else if (smenu.menu_tab == MENUTAB_APPEARANCE) {
    static const unsigned labels[] = {
      MSG_APPEAR_PRESET, MSG_APPEAR_WALLPAPER, MSG_APPEAR_BACKGROUND,
      MSG_APPEAR_ACCENT, MSG_APPEAR_SELECTION, MSG_APPEAR_CONTRAST,
      MSG_APPEAR_RESET,
    };
    const char *values[] = {
      ui_theme_preset_names[ui_theme_preset],
      ui_wallpaper_names[ui_wallpaper],
      ui_theme_color_names[ui_background_color],
      ui_theme_color_names[ui_accent_color],
      ui_theme_color_names[ui_selection_color],
      ui_contrast_names[ui_contrast],
      "A: RESET",
    };
    model.wide = true;
    model.entry_count = ARRAY_SIZE(labels);
    model.selected_row = smenu.appearance.selector;
    for (unsigned i = 0; i < model.entry_count; i++) {
      model.entries[i].name = msgs[lang_id][labels[i]];
      model.entries[i].value = values[i];
      model.entries[i].kind = UiBrowserV2Other;
    }
  } else if (smenu.menu_tab == MENUTAB_SETTINGS) {
    static const unsigned labels[] = {
      MSG_SET_TITL1, MSG_SETT_HOTK, MSG_SETT_BOOT, MSG_SETT_FASTSD,
#ifdef SUPPORT_NORGAMES
      MSG_SETT_VERNOR,
#endif
      MSG_SETT_FASTEW, MSG_SETT_SAVET,
#ifdef SUPPORT_NORGAMES
      MSG_SETT_SAVETX,
#endif
      MSG_SETT_SAVEBK, MSG_SETT_STATET,
      MSG_SETT_CHTEN, MSG_SET_TITL2, MSG_DEFS_PATCH, MSG_LOADER_MENU,
      MSG_LOADER_RTCE, MSG_DEF_RTCVAL, MSG_DEF_SPEED, MSG_LOADER_LOADP,
      MSG_LOADER_SAVEP, MSG_LOADER_PREFDS, MSG_UIS_SAVE,
    };
    _Static_assert(ARRAY_SIZE(labels) == SettMAX,
                   "settings labels must match settings enum");
    char values[UI_BROWSER_V2_ROWS][32];
    unsigned base = smenu.set.selector <= 3 ? 0 :
                    MIN((unsigned)smenu.set.selector - 3,
                        SettMAX - UI_BROWSER_V2_ROWS);
    model.wide = true;
    model.entry_count = MIN(SettMAX - base, UI_BROWSER_V2_ROWS);
    model.selected_row = smenu.set.selector - base;
    for (unsigned i = 0; i < model.entry_count; i++) {
      unsigned option = base + i;
      model.entries[i].name = msgs[lang_id][labels[option]];
      model.entries[i].value = ui_setting_value(option, values[i], sizeof(values[i]));
      model.entries[i].kind = UiBrowserV2Other;
    }
  } else if (smenu.menu_tab == MENUTAB_UILANG) {
    static const unsigned labels[UiSetCNT] = {
      MSG_UIS_LANG, MSG_UIS_RECNT, MSG_UIS_ANSPD, MSG_UIS_BHID, MSG_UIS_SAVE,
    };
    const char *values[UiSetCNT] = {
      msgs[lang_id][MSG_LANG_NAME],
      msgs[lang_id][recent_menu ? MSG_KNOB_ENABLED : MSG_KNOB_DISABLED],
      msgs[lang_id][MSG_UIS_SPD0 + anim_speed],
      msgs[lang_id][hide_hidden ? MSG_KNOB_DISABLED : MSG_KNOB_ENABLED],
      "A: SAVE",
    };
    model.wide = true;
    model.entry_count = UiSetCNT;
    model.selected_row = smenu.uiset.selector;
    for (unsigned i = 0; i < model.entry_count; i++) {
      model.entries[i].name = msgs[lang_id][labels[i]];
      model.entries[i].value = values[i];
      model.entries[i].kind = UiBrowserV2Other;
    }
  } else if (smenu.menu_tab == MENUTAB_INFO) {
    char values[UI_BROWSER_V2_ROWS][40];
    unsigned base = smenu.info.seloff;
    model.wide = true;
    model.entry_count = MIN(InfoCNT - base, UI_BROWSER_V2_ROWS);
    model.selected_row = smenu.info.selector - base;
    for (unsigned i = 0; i < model.entry_count; i++) {
      unsigned row = base + i;
      model.entries[i].kind = UiBrowserV2Other;
      switch (row) {
      case InfoSystem:
        model.entries[i].name = "System information";
        model.entries[i].value = SUPERR7_PRODUCT_NAME;
        break;
      case InfoMaintainer:
        model.entries[i].name = "Maintainer";
        model.entries[i].value = SUPERR7_MAINTAINER;
        break;
      case InfoBasedOn:
        model.entries[i].name = "Based on";
        model.entries[i].value = SUPERR7_UPSTREAM_NAME;
        break;
      case InfoBuild:
        model.entries[i].name = "Build";
        model.entries[i].value = SUPERR7_BUILD_FINGERPRINT;
        break;
      case InfoFlashDevice:
        model.entries[i].name = "Flash device ID";
        npf_snprintf(values[i], sizeof(values[i]), "%08lx", flashinfo.deviceid);
        model.entries[i].value = values[i];
        break;
      case InfoFlashCapacity:
        model.entries[i].name = "Flash capacity";
        if (flashinfo.size)
          human_size_kb(values[i], sizeof(values[i]), flashinfo.size >> 10);
        else
          npf_snprintf(values[i], sizeof(values[i]), "%d KiB", FW_MAX_SIZE_KB);
        model.entries[i].value = values[i];
        break;
      case InfoUpdater:
        model.entries[i].name = "Updater";
        model.entries[i].value = enable_flashing ? "< Unlocked >" : "< Locked >";
        break;
      case InfoPatchDatabase:
        model.entries[i].name = "Patch database";
        npf_snprintf(values[i], sizeof(values[i]), "%s - %s",
                     pdbinfo.version, pdbinfo.date);
        model.entries[i].value = values[i];
        break;
      case InfoPatchCount:
        model.entries[i].name = "Patch game count";
        npf_snprintf(values[i], sizeof(values[i]), "%lu", pdbinfo.patch_count);
        model.entries[i].value = values[i];
        break;
      case InfoSDType:
        model.entries[i].name = "SD card type";
        model.entries[i].value = sd_info.sdhc ? "SDHC" : "SDSC";
        break;
      case InfoSDCapacity:
        model.entries[i].name = "SD capacity";
        human_size_kb(values[i], sizeof(values[i]), sd_info.block_cnt / 2);
        model.entries[i].value = values[i];
        break;
      case InfoSDCardId:
        model.entries[i].name = "SD card ID";
        npf_snprintf(values[i], sizeof(values[i]), "%02x | %04x",
                     sd_info.manufacturer, sd_info.oemid);
        model.entries[i].value = values[i];
        break;
      }
    }
  } else {
    model.wide = true;
  }

  ui_browser_v2_render(frame, &model);
}
#endif

// Renders the menu. Arg0 represents the frame count difference with the
// previous rendered frame (for animations and similar stuff).
void menu_render(unsigned fcnt) {
#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
  objnum = 0;
#endif
  volatile uint8_t *frame = &MEM_VRAM_U8[0xA000*framen];

#if defined(COVER_ART_DEMO) && defined(UI_BROWSER_V2)
  if (demo_loading_progress >= 0) {
    ui_browser_v2_load_palette(MEM_PALETTE);
    ui_browser_v2_render_loading(
      frame, file_basename(spop.p.load.i.romfn),
      cover_cache_pixels(&menu_cover_cache), menu_cover_cache.state,
      demo_loading_progress, 100);
    demo_loading_progress = MIN(100, demo_loading_progress + fcnt);
    return;
  }
#endif

#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
  #ifndef COVER_ART_DEMO
  if (!first_menu_frame_pending)
  #endif
    ui_browser_v2_load_palette(MEM_PALETTE);
  if (spop.alert_msg || spop.qpop.message || spop.rtcpop.callback ||
      spop.pop_num)
    render_ui_phase5_popup(frame);
  else
    render_ui_browser_phase4(frame);
  smenu.anim_state += fcnt * animspd_lut[anim_speed];
  REG_WIN0H = 0;
  REG_WIN0V = 0;
#else
#ifdef UI_BROWSER_V2
  bool phase4_tab = smenu.menu_tab == MENUTAB_FAVORITES ||
                    smenu.menu_tab == MENUTAB_RECENT ||
                    smenu.menu_tab == MENUTAB_ROMBROWSE ||
                    smenu.menu_tab == MENUTAB_SETTINGS ||
                    smenu.menu_tab == MENUTAB_UILANG ||
                    smenu.menu_tab == MENUTAB_APPEARANCE ||
                    smenu.menu_tab == MENUTAB_TOOLS ||
                    smenu.menu_tab == MENUTAB_INFO;
  bool use_browser_v2 = phase4_tab;
  if (use_browser_v2) {
    ui_browser_v2_load_palette(MEM_PALETTE);
    if (spop.alert_msg || spop.qpop.message || spop.rtcpop.callback ||
        spop.pop_num)
      render_ui_phase5_popup(frame);
    else
      render_ui_browser_phase4(frame);
    smenu.anim_state += fcnt * animspd_lut[anim_speed];
  } else {
    reload_theme();
#endif

  // Render the tab menu on top (rows 0..15), highlighting the selected option
  dma_memset16(&frame[0], dup8(FG_COLOR), SCREEN_WIDTH*16/2);

  // Render icon bar
  int mintab = (recent_menu && smenu.recent.maxentries) ? MENUTAB_RECENT : MENUTAB_ROMBROWSE;
  for (unsigned i = mintab; i < MENUTAB_MAX; i++)
    if (i == smenu.menu_tab)
      render_icon((i - mintab)*16, 0, i + ICON_RECENT);
    else
      render_icon_trans((i - mintab)*16, 0, i + ICON_RECENT);

  // Render the main area
  dma_memset16(&frame[16*SCREEN_WIDTH], dup8(BG_COLOR), SCREEN_WIDTH*(SCREEN_HEIGHT-16) / 2);

  if (spop.qpop.message)
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
    render_ui_phase5_popup(frame);
#else
    render_popupq(frame, fcnt);
#endif
  else if (spop.rtcpop.callback)
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
    render_ui_phase5_popup(frame);
#else
    render_rtcpop(frame);
#endif
  else {
    if (spop.pop_num) {
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
      ui_browser_v2_load_palette(MEM_PALETTE);
      render_ui_phase5_popup(frame);
#else
      popup_windows[spop.pop_num - 1].render(frame);
      spop.anim += fcnt * animspd_lut[anim_speed];
#endif
    } else {
      static const t_mrender_fn renderfns[] = {
#ifdef UI_BROWSER_V2
        render_browser,
#endif
        render_recent,
        render_browser,
        #ifdef SUPPORT_NORGAMES
        render_flashbrowser,
        #endif
#ifdef UI_BROWSER_V2
        render_browser,
        render_browser,
        render_browser,
        render_browser,
        render_browser,
#else
        render_settings,
        render_ui_settings,
        render_tools,
        render_info,
#endif
      };
      renderfns[smenu.menu_tab](frame);
      if (smenu.menu_tab == MENUTAB_RECENT || smenu.menu_tab == MENUTAB_ROMBROWSE)
        render_cover_panel(frame);
      smenu.anim_state += fcnt * animspd_lut[anim_speed];
    }
  }

#ifdef UI_BROWSER_V2
  }
#endif

  // Render popup window. Use windowing to ensure the pop up is not covered by OBJs.
#ifdef UI_BROWSER_V2
  if (spop.alert_msg && !use_browser_v2) {
#else
  if (spop.alert_msg) {
#endif
    draw_box_full(frame, 15, 227, SCREEN_HEIGHT / 2 - 20, SCREEN_HEIGHT / 2 + 20, FG_COLOR, HI_COLOR);
    draw_central_text(spop.alert_msg, frame, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 8);
    REG_WIN0H = 226 | (14 << 8);
    REG_WIN0V = (SCREEN_HEIGHT / 2 + 20) | ((SCREEN_HEIGHT / 2 - 20) << 8);
  } else {
    REG_WIN0H = 0;
    REG_WIN0V = 0;
  }
#endif
}

void menu_flip() {
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES) && !defined(COVER_ART_DEMO)
  if (first_menu_frame_pending) {
    // Commit the UI palette in the same VBlank as the hidden-page flip so the
    // boot logo cannot turn into a white or partially recolored transition.
    ui_browser_v2_load_palette(MEM_PALETTE);
    first_menu_frame_pending = false;
  }
#endif
#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
  for (unsigned i = 0; i < objnum; i++) {
    MEM_OAM[i*4+0] = fobjs[i].y | 0x2000;  // Use 256 entries palette
    MEM_OAM[i*4+1] = fobjs[i].x | 0x4000;  // Size 16x16
    MEM_OAM[i*4+2] = fobjs[i].tn + 512;    // OBJ numbers start at 512 for Mode 4
  }
  dma_memset16(&MEM_OAM[objnum*4], 0, 256 - objnum*2);  // Clear unused objects
#endif
  REG_DISPCNT = (REG_DISPCNT & ~0x10) | (framen << 4);
  framen ^= 1;
}

static void menu_cover_request_selected() {
  const char *rom_path = NULL;
  char browser_path[MAX_FN_LEN];

  if (smenu.menu_tab == MENUTAB_FAVORITES &&
      smenu.favorites.maxentries) {
    favorite_copy_path(smenu.favorites.selector, browser_path);
    rom_path = browser_path;
  } else if (smenu.menu_tab == MENUTAB_RECENT && smenu.recent.maxentries) {
    rom_path = sdr_state->rentries[smenu.recent.selector].fpath;
  } else if (smenu.menu_tab == MENUTAB_ROMBROWSE && smenu.browser.dispentries) {
    t_centry *entry = sdr_state->fileorder[smenu.browser.selector];
    unsigned directory_len = strlen(smenu.browser.cpath);
    unsigned filename_len = strlen(entry->fname);
    if (!entry->isdir && directory_len + filename_len + 1 <= sizeof(browser_path)) {
      memcpy(browser_path, smenu.browser.cpath, directory_len);
      memcpy(browser_path + directory_len, entry->fname, filename_len + 1);
      rom_path = browser_path;
    }
  }

  if (rom_path)
    cover_cache_request(&menu_cover_cache, rom_path, systime());
  else
    cover_cache_clear(&menu_cover_cache);
}

#ifdef UI_BROWSER_V2
static void menu_browse_repeat_update(uint32_t now) {
  int direction = 0;
  bool browse_ready = smenu.menu_tab == MENUTAB_ROMBROWSE &&
                      smenu.browser.dispentries && !spop.alert_msg &&
                      !spop.qpop.message && !spop.rtcpop.callback &&
                      !spop.pop_num;
  if (browse_ready) {
    unsigned held = (~REG_KEYINPUT) & 0x3FF;
    unsigned dpad = held & (KEY_BUTTLEFT | KEY_BUTTRIGHT |
                            KEY_BUTTUP | KEY_BUTTDOWN);
    if (dpad == KEY_BUTTLEFT)
      direction = -1;
    else if (dpad == KEY_BUTTRIGHT)
      direction = 1;
  }

  int page_delta = menu_page_repeat_step(&browse_page_repeat, direction, now);
  if (!page_delta)
    return;

  int previous = smenu.browser.selector;
  menu_list_navigate(&smenu.browser.selector, &smenu.browser.seloff,
                     smenu.browser.dispentries, BROWSER_ROWS, 0, page_delta);
  if (smenu.browser.selector != previous) {
    smenu.anim_state = 0;
    menu_cover_request_selected();
  }
}
#endif

void menu_update() {
  uint32_t now = systime();
#ifdef UI_BROWSER_V2
  menu_browse_repeat_update(now);
#endif
  unsigned previous_state = menu_cover_cache.state;
#ifdef COVER_ART_DEMO
  cover_cache_poll(&menu_cover_cache, now, cover_demo_read);
#else
  cover_cache_poll(&menu_cover_cache, now, cover_fatfs_read);
#endif
  if (previous_state != CoverReady && menu_cover_cache.state == CoverReady)
    dma_memcpy16(&MEM_PALETTE[COVER_PALETTE_BASE],
                 cover_cache_palette(&menu_cover_cache),
                 menu_cover_cache.info.palette_count);
}

static void menu_setup_display() {
#if !defined(UI_BROWSER_V2) || defined(SUPPORT_NORGAMES)
  // Load icons into VRAM
  dma_memcpy16(MEM_VRAM_OBJS, icons_img, sizeof(icons_img) / 2);
  dma_memcpy16(&MEM_PALETTE[256], icons_pal, sizeof(icons_pal) / 2);
  // Generate some icons (selector)
  dma_memset16(&MEM_VRAM_OBJS[63 * 256], dup8(SEL_COLOR), 256 / 2);
#else
  // The redesigned SD UI draws directly into the Mode 4 framebuffer and does
  // not use the classic OBJ icon atlas or selector sprites.
  dma_memset16(MEM_OAM, 0, 256);
#endif

  // Further setup initial video regs. BG2 is setup in the bootloader already!
  REG_WININ  = 0x0004;     // Only BG2 is enabled in Win0
  REG_WINOUT = 0x0014;     // BG2 and OBJ enabled outside of Win0
  REG_WIN0H = 0;
  REG_WIN0V = 0;
  REG_DISPCNT |= 0x2000;   // Enable window 0

  // Setup alpha blending for the selector knob
  REG_BLDCNT = 0x1F40;
  REG_BLDALPHA = 0x0808;  // 50% alpha
}

#ifdef COVER_ART_DEMO
void menu_demo_init() {
  static const char *browser_names[] = {
    "Aurora.gba",
    "Checker.gba",
    "Missing.gba",
    "Broken.gba",
    "Folder",
    "A Very Long Adventure Game Name.gba",
    "Seventh Entry.sav",
    "Eighth Entry.fw",
    "Notes.txt",
  };
  static const char *recent_paths[] = {
    "/DEMO/Aurora.gba",
    "/DEMO/Checker.gba",
    "/DEMO/Missing.gba",
    "/DEMO/Broken.gba",
  };

  memset(&smenu, 0, sizeof(smenu));
  memset(&spop, 0, sizeof(spop));
  memset(sdr_state, 0, sizeof(*sdr_state));
  strcpy(smenu.browser.cpath, "/DEMO/");

  for (unsigned i = 0; i < ARRAY_SIZE(browser_names); i++) {
    t_centry *entry = &sdr_state->fentries[i];
    strcpy(entry->fname, browser_names[i]);
    entry->filesize = (i + 1) * 1024 * 1024;
    entry->isdir = i == 4;
    entry->attr = entry->isdir ? AM_DIR : 0;
    sortable_utf8_u16(entry->fname, entry->sortname);
    sdr_state->fileorder[i] = entry;
  }
  smenu.browser.maxentries = ARRAY_SIZE(browser_names);
  smenu.browser.dispentries = ARRAY_SIZE(browser_names);

  for (unsigned i = 0; i < ARRAY_SIZE(recent_paths); i++) {
    strcpy(sdr_state->rentries[i].fpath, recent_paths[i]);
    sdr_state->rentries[i].fname_offset = sizeof("/DEMO/") - 1;
  }
  smenu.recent.maxentries = ARRAY_SIZE(recent_paths);

  cover_cache_init(&menu_cover_cache);
  reload_theme();
#ifdef UI_BROWSER_V2
  ui_theme_reset(UiThemeElectricBlue);
  ui_selection_color = UiColorRed;
  ui_browser_v2_load_palette(MEM_PALETTE);
#endif
  smenu.menu_tab = MENUTAB_ROMBROWSE;
  menu_cover_request_selected();
  menu_setup_display();
}
#endif

void menu_init(int sram_testres) {
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES) && !defined(COVER_ART_DEMO)
  // Page 0 contains the boot logo. Build the first browser frame on hidden
  // page 1 and reveal it only after it is complete.
  framen = 1;
  first_menu_frame_pending = true;
#endif
  // Reset to ROM browser and SD card root.
  memset(&smenu, 0, sizeof(smenu));
  memset(&spop, 0, sizeof(spop));
  cover_cache_init(&menu_cover_cache);

  // Reset the file browser as well.
  strcpy(smenu.browser.cpath, "/");
  browser_reload();
  flashbrowser_reload();

  // Load recent ROMs (we could disable this for speed)
  recent_reload();
#ifdef UI_BROWSER_V2
  favorites_reload();
#endif

  reload_theme();

#ifdef UI_BROWSER_V2
  smenu.menu_tab = MENUTAB_ROMBROWSE;
#else
  smenu.menu_tab = (recent_menu && smenu.recent.maxentries) ? MENUTAB_RECENT : MENUTAB_ROMBROWSE;
#endif
  menu_cover_request_selected();
  menu_setup_display();

  // If there's a test result to report, create a popup
  if (sram_testres >= 0)
    spop.alert_msg = sram_testres ? msgs[lang_id][MSG_SRAMTST_FAIL] :
                                    msgs[lang_id][MSG_SRAMTST_OK];
}

int movedir_up() {
  char *p = smenu.browser.cpath;
  p = &p[strlen(p)-1];

  if (p != smenu.browser.cpath) {
    do {
      p--;
      if (*p == '/') {
        p[1] = 0;   // Shorten the path here
        return 1;
      }
    } while (p != smenu.browser.cpath);
  }
  return 0;
}

void start_flash_update(const char *fn, unsigned fwsize, bool validate_superfw) {
  // We read the file into SDRAM, apply the update from there.
  FIL fd;
  FRESULT res = f_open(&fd, fn, FA_READ);
  if (res != FR_OK)
    spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRRD];
  else {
    // Loading file...
    spop.p.update.curr_state = FlashingLoading;
    menu_render(1); menu_flip();
    for (unsigned i = 0; i < fwsize; i += 4*1024) {
      UINT rdbytes;
      unsigned tord = fwsize >= i + 4*1024 ? 4*1024 : fwsize - i;
      uint32_t tmp[1024];
      if (FR_OK != f_read(&fd, tmp, tord, &rdbytes) || rdbytes != tord) {
        spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRRD];
        return;
      }
      // Copy (ensure aligned copy!)
      dma_memcpy32(&sdr_state->scratch[i], tmp, 1024);
    }
    spop.p.update.curr_state = FlashingChecking;
    menu_render(1); menu_flip();

    // Now proceed to validate the superfw if necessary.
    if (validate_superfw && !validate_superfw_variant(sdr_state->scratch))
      spop.alert_msg = msgs[lang_id][MSG_FWUP_BADFL];
    else if (validate_superfw && !validate_superfw_checksum(sdr_state->scratch, fwsize))
      spop.alert_msg = msgs[lang_id][MSG_FWUPD_BADCHK];
    else {
      // Can start the flashing!
      spop.p.update.curr_state = FlashingErasing;
      menu_render(1); menu_flip();

      bool erased_ok;
      #ifdef SUPPORT_NORGAMES
      if (flashinfo.blksize)
        erased_ok = flash_erase_sectors(ROM_FLASHFIRMW_ADDR, flashinfo.blksize,
                                        (fwsize + flashinfo.blksize - 1) / flashinfo.blksize);
      else
      #endif
        erased_ok = flash_erase_chip();

      if (!erased_ok)
        spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRCL];
      else {
        spop.p.update.curr_state = FlashingWriting;
        menu_render(1); menu_flip();

        bool programmed_ok;
        #ifdef SUPPORT_NORGAMES
        if (flashinfo.size && flashinfo.blksize && flashinfo.blkcount && flashinfo.blkwrite)
          programmed_ok = flash_program_buffered(ROM_FLASHFIRMW_ADDR, sdr_state->scratch, fwsize, flashinfo.blkwrite);
        else
        #endif
          programmed_ok = flash_program(ROM_FLASHFIRMW_ADDR, sdr_state->scratch, fwsize);

        if (!programmed_ok)
          spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRPG];
        else {
          if (!flash_verify(ROM_FLASHFIRMW_ADDR, sdr_state->scratch, fwsize))
            spop.alert_msg = msgs[lang_id][MSG_FWUP_ERRVR];
          else {
            // Done! Show a pop up, also go up with pop ups too.
            spop.alert_msg = msgs[lang_id][MSG_FWUPD_DONE];
          }
        }
      }
    }
    spop.pop_num = 0;
  }
}

static void keypress_popup_loadgba(unsigned newkeys) {
  const unsigned maxm[] = {
    GBAInfoCNT,
    GBALdSetCNT,
    GBAPatchCNT,
#ifdef UI_BROWSER_V2
    1,
#endif
  };
  unsigned maxsel = maxm[(int)spop.submenu];

  const int psel = spop.selector;
  if (newkeys & KEY_BUTTUP)
    spop.selector += maxsel - 1;
  if (newkeys & KEY_BUTTDOWN)
    spop.selector++;

  // Limit selector to its max value
  spop.selector %= maxsel;

  if (newkeys & KEY_BUTTLEFT) {
    if (spop.submenu == GbaLoadPopLoadS) {
      if (spop.selector == GBALdSetCheats)
        spop.p.load.l.use_cheats = !spop.p.load.l.use_cheats;
      if (spop.p.load.i.use_dsaving) {
        if (spop.selector == GBALdSetLoadP)
          spop.p.load.l.sram_load_type = (spop.p.load.l.sram_load_type + SaveLoadDSCNT - 1) % SaveLoadDSCNT;
      } else {
        if (spop.selector == GBALdSetLoadP)
          spop.p.load.l.sram_load_type = (spop.p.load.l.sram_load_type + SaveLoadCNT - 1) % SaveLoadCNT;
        else if (spop.selector == GBALdSetSaveP)
          spop.p.load.l.sram_save_type = (spop.p.load.l.sram_save_type + SaveCNT - 1) % SaveCNT;
      }
    }
    else if (spop.submenu == GbaLoadPopPatch) {
      if (spop.selector == GBALoadPatch)
        spop.p.load.i.patch_type = (spop.p.load.i.patch_type + PatchOptCNT - 1) % PatchOptCNT;
      else if (spop.selector == GBAInGameMen)
        spop.p.load.i.ingame_menu_enabled = !spop.p.load.i.ingame_menu_enabled;
      else if (spop.selector == GBASavePatch)
        spop.p.load.i.use_dsaving = !spop.p.load.i.use_dsaving;
      else if (spop.selector == GBARTCPatch)
        spop.p.load.i.rtc_patch_enabled = !spop.p.load.i.rtc_patch_enabled;
    }

    // Handle the different cases where the user attempts to select an invalid option.
    if (!spop.p.load.i.patches_cache_found && spop.p.load.i.patch_type == PatchEngine)
      spop.p.load.i.patch_type = PatchDatabase;  // Might be invalid, handled below.
    if (!spop.p.load.i.patches_datab_found && spop.p.load.i.patch_type == PatchDatabase)
      spop.p.load.i.patch_type = PatchNone;

    if (!dirsav_avail_sdram(&spop.p.load.i))
      spop.p.load.i.use_dsaving = false;

    // DirSav forces automatic saving
    if (spop.p.load.i.use_dsaving)
      spop.p.load.l.sram_save_type = SaveDirect;
    else if (spop.p.load.l.sram_save_type == SaveDirect)
      spop.p.load.l.sram_save_type = autosave_default ? SaveReboot : SaveDisable;

    // If DS is selected, do not allow manual mode.
    if (spop.p.load.l.sram_load_type == SaveLoadDisable && spop.p.load.i.use_dsaving)
      spop.p.load.l.sram_load_type = SaveLoadSav;
    // If no .sav is available, do not allow that option!
    if (spop.p.load.l.sram_load_type == SaveLoadSav && !spop.p.load.l.savefile_found)
      spop.p.load.l.sram_load_type = spop.p.load.i.use_dsaving ? SaveLoadReset : SaveLoadDisable;
  }
  if (newkeys & KEY_BUTTRIGHT) {
    if (spop.submenu == GbaLoadPopLoadS) {
      if (spop.selector == GBALdSetCheats)
        spop.p.load.l.use_cheats = !spop.p.load.l.use_cheats;
      if (spop.p.load.i.use_dsaving) {
        if (spop.selector == GBALdSetLoadP)
          spop.p.load.l.sram_load_type = (spop.p.load.l.sram_load_type + 1) % SaveLoadDSCNT;
      } else {
        if (spop.selector == GBALdSetLoadP)
          spop.p.load.l.sram_load_type = (spop.p.load.l.sram_load_type + 1) % SaveLoadCNT;
        else if (spop.selector == GBALdSetSaveP)
          spop.p.load.l.sram_save_type = (spop.p.load.l.sram_save_type + 1) % SaveCNT;
      }
    }
    else if (spop.submenu == GbaLoadPopPatch) {
      if (spop.selector == GBALoadPatch)
        spop.p.load.i.patch_type = (spop.p.load.i.patch_type + 1) % PatchOptCNT;
      else if (spop.selector == GBAInGameMen)
        spop.p.load.i.ingame_menu_enabled = !spop.p.load.i.ingame_menu_enabled;
      else if (spop.selector == GBASavePatch)
        spop.p.load.i.use_dsaving = !spop.p.load.i.use_dsaving;
      else if (spop.selector == GBARTCPatch)
        spop.p.load.i.rtc_patch_enabled = !spop.p.load.i.rtc_patch_enabled;
    }

    // If the database has no entry, then do not let the user select that mode.
    if (!spop.p.load.i.patches_datab_found && spop.p.load.i.patch_type == PatchDatabase)
      spop.p.load.i.patch_type = PatchEngine;  // Might be invalid, handled below.
    if (!spop.p.load.i.patches_cache_found && spop.p.load.i.patch_type == PatchEngine)
      spop.p.load.i.patch_type = PatchNone;

    if (!dirsav_avail_sdram(&spop.p.load.i))
      spop.p.load.i.use_dsaving = false;

    // DirSav forces automatic saving
    if (spop.p.load.i.use_dsaving)
      spop.p.load.l.sram_save_type = SaveDirect;
    else if (spop.p.load.l.sram_save_type == SaveDirect)
      spop.p.load.l.sram_save_type = autosave_default ? SaveReboot : SaveDisable;

    // If DS is selected, do not allow manual mode.
    if (spop.p.load.l.sram_load_type == SaveLoadDisable && spop.p.load.i.use_dsaving)
      spop.p.load.l.sram_load_type = SaveLoadSav;
    // If no .sav is available, do not allow that option!
    if (spop.p.load.l.sram_load_type == SaveLoadSav && !spop.p.load.l.savefile_found)
      spop.p.load.l.sram_load_type = SaveLoadReset;
  }

  // Disable ingame-menu if not available.
  if (!ingame_menu_avail_sdram(&spop.p.load.i))
    spop.p.load.i.ingame_menu_enabled = false;

  // If no RTC patches are available, force them to false.
  if (!rtcemu_avail(&spop.p.load.i))
    spop.p.load.i.rtc_patch_enabled = false;

  // Disable cheat loading if no cheats are avail, or IGM is disabled
  if (!spop.p.load.l.cheats_found || !spop.p.load.i.ingame_menu_enabled)
    spop.p.load.l.use_cheats = false;

  if (newkeys & KEY_BUTTA) {
    if (spop.submenu == GbaLoadPopLoadS && spop.selector == GBALdSetRTC && spop.p.load.i.rtc_patch_enabled) {
      void accept_rtc() {
        spop.p.load.l.rtcval = date2timestamp(&spop.rtcpop.val);
      }
      if (spop.p.load.i.rtc_patch_enabled) {
        timestamp2date(spop.p.load.l.rtcval, &spop.rtcpop.val);
        spop.rtcpop.callback = accept_rtc;
      }
    }
    else if (spop.submenu == GbaLoadPopPatch && spop.selector == GBAPatchGen) {
      generate_patches_progress(spop.p.load.i.romfn, spop.p.load.i.romfs);
      spop.alert_msg = msgs[lang_id][MSG_PATCHGEN_OK];
      // Try/Load the just-generated patches.
      spop.p.load.i.patches_cache_found = load_cached_patches(spop.p.load.i.romfn, &spop.p.load.i.patches_cache);
    }
    else if (spop.submenu == GbaLoadPopLoadS && spop.selector == GBALdRemember) {
      // Save settings to disk now!
      t_rom_load_settings ld_sett = {
        .patch_policy = spop.p.load.i.patch_type,
        .use_igm = spop.p.load.i.ingame_menu_enabled,
        .use_rtc = spop.p.load.i.rtc_patch_enabled,
        .use_dsaving = spop.p.load.i.use_dsaving
      };
      t_rom_launch_settings lh_sett = {
        .use_cheats = spop.p.load.l.use_cheats,
        .rtcts = spop.p.load.l.rtcval
      };

      save_rom_settings(spop.p.load.i.romfn, &ld_sett, &lh_sett);
      spop.alert_msg = msgs[lang_id][MSG_REMEMB_CFG_OK];
    }
#ifdef UI_BROWSER_V2
    else if (GbaLoadPopInfo == spop.submenu &&
             spop.selector == GBAFavoriteButt) {
      int favorite = favorite_find(spop.p.load.i.romfn);
      bool saved = favorite >= 0 ? favorite_delete_flush(favorite) :
                                   favorite_add_flush(spop.p.load.i.romfn);
      if (!saved)
        spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
    }
    else if (GbaLoadPopInfo == spop.submenu &&
             spop.selector == GBAOptionsButt) {
      spop.submenu = GbaLoadPopLoadS;
      spop.selector = 0;
    }
    else if (GbaLoadPopInfo == spop.submenu &&
             spop.selector == GBADetailsButt) {
      spop.submenu = GbaLoadPopDetails;
      spop.selector = 0;
    }
#endif
    else if (GbaLoadPopInfo == spop.submenu &&
             spop.selector == GBALoadButt) {
#if defined(COVER_ART_DEMO) && defined(UI_BROWSER_V2)
      demo_loading_progress = 0;
      return;
#endif
      // Insert the ROM into the recent list (or move it around). Flush to disk!
      if (recent_menu)
        insert_recent_flush(spop.p.load.i.romfn, FLAG_RECENT_SD);

      // Honor load.patch_type.
      const t_patch *p = get_game_patch(&spop.p.load.i);
      EnumSavetype st = p ? p->save_mode : SaveTypeNone;

      // Prepare the savegame (load and store stuff, directsave...)
      t_dirsave_info dsinfo;
      unsigned errsave = prepare_savegame(
        spop.p.load.l.sram_load_type, spop.p.load.l.sram_save_type,
        st, &dsinfo, spop.p.load.l.savefn);
      if (errsave) {
        unsigned errmsg = (errsave == ERR_SAVE_BADSAVE)   ? MSG_ERR_SAVERD :
                          (errsave == ERR_SAVE_CANTALLOC) ? MSG_ERR_SAVEPR :
                          (errsave == ERR_SAVE_BADARG)    ? MSG_ERR_SAVEIT :
                                                            MSG_ERR_SAVEWR;
        spop.alert_msg = msgs[lang_id][errmsg];
        return;
      }

      t_rtc_info rtci = {
        .timestamp = spop.p.load.l.rtcval,
        .ts_step = rtcspeed_default
      };

      unsigned err = load_gba_rom(
        spop.p.load.i.romfn, spop.p.load.i.romfs,
        spop.p.load.l.sram_save_type == SaveDisable ? NULL : spop.p.load.l.savefn, p,
        spop.p.load.l.sram_save_type == SaveDirect ? &dsinfo : NULL,
        spop.p.load.i.ingame_menu_enabled,
        spop.p.load.i.rtc_patch_enabled ? &rtci : NULL,
        spop.p.load.l.use_cheats ? spop.p.load.l.cheats_size : 0,
        loadrom_progress);
      if (err) {
        // Show any errors that might have happened!
        spop.alert_msg = msgs[lang_id][MSG_ERR_READ];
        // TODO: We cannot (in many cases) continue since we trash the SDRAM!
      }
    }
  }

  if (psel != spop.selector)
    spop.anim = 0;
}

static void keypress_popup_savefile(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    spop.selector = MAX(0, spop.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    spop.selector = MIN(SavMAX, spop.selector + 1);

  if (newkeys & KEY_BUTTA) {
    switch (spop.selector) {
    case SaveWrite:
      if (write_save_sram(spop.p.savopt.savfn))
        spop.alert_msg = msgs[lang_id][MSG_SAVOPT_MSG0];
      else
        spop.alert_msg = msgs[lang_id][MSG_SAVOPT_MSG_WERR];
      break;
    case SavLoad:
      if (load_save_sram(spop.p.savopt.savfn))
        spop.alert_msg = msgs[lang_id][MSG_SAVOPT_MSG1];
      else
        spop.alert_msg = msgs[lang_id][MSG_SAVOPT_MSG_RERR];
      break;
    case SavClear:
      if (wipe_sav_file(spop.p.savopt.savfn))
        spop.alert_msg = msgs[lang_id][MSG_SAVOPT_MSG2];
      else
        spop.alert_msg = msgs[lang_id][MSG_SAVOPT_MSG_WERR];
      break;
    case SavQuit:
      spop.pop_num = 0;
      break;
    };
  }
}

static void keypress_popup_flash(unsigned newkeys) {
  if ((newkeys & FLASH_GO_KEYS) == FLASH_GO_KEYS)
    start_flash_update(spop.p.update.fn, spop.p.update.fw_size, spop.p.update.issfw);
}

#ifdef SUPPORT_NORGAMES
static void keypress_popup_norwrite(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    spop.selector = MAX(0, spop.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    spop.selector = MIN(GBAPatchCNT - 1, spop.selector + 1);

  if (spop.submenu == GbaNorWrPatch) {
    if (newkeys & (KEY_BUTTLEFT|KEY_BUTTRIGHT)) {
      if (spop.selector == GBALoadPatch)
        spop.p.norwr.i.patch_type = (spop.p.norwr.i.patch_type +
                                     ((newkeys & KEY_BUTTRIGHT) ? 1 : PatchOptCNT - 1)) % PatchOptCNT;
      else if (spop.selector == GBAInGameMen)
        spop.p.norwr.i.ingame_menu_enabled = !spop.p.norwr.i.ingame_menu_enabled;
      else if (spop.selector == GBASavePatch)
        spop.p.norwr.i.use_dsaving = !spop.p.norwr.i.use_dsaving;
      else if (spop.selector == GBARTCPatch)
        spop.p.norwr.i.rtc_patch_enabled = !spop.p.norwr.i.rtc_patch_enabled;
    }

    if (newkeys & KEY_BUTTLEFT) {
      // Handle the different cases where the user attempts to select an invalid option.
      if (!spop.p.norwr.i.patches_cache_found && spop.p.norwr.i.patch_type == PatchEngine)
        spop.p.norwr.i.patch_type = PatchDatabase;  // Might be invalid, handled below.
      if (!spop.p.norwr.i.patches_datab_found && spop.p.norwr.i.patch_type == PatchDatabase)
        spop.p.norwr.i.patch_type = PatchNone;
    }
    if (newkeys & KEY_BUTTRIGHT) {
      // If the database has no entry, then do not let the user select that mode.
      if (!spop.p.norwr.i.patches_datab_found && spop.p.norwr.i.patch_type == PatchDatabase)
        spop.p.norwr.i.patch_type = PatchEngine;  // Might be invalid, handled below.
      if (!spop.p.norwr.i.patches_cache_found && spop.p.norwr.i.patch_type == PatchEngine)
        spop.p.norwr.i.patch_type = PatchNone;
    }

    // Disable certain features (depends on patch types)
    if (!dirsav_avail_flash(&spop.p.norwr.i))
      spop.p.norwr.i.use_dsaving = false;
    if (!ingame_menu_avail_flash(&spop.p.norwr.i))
      spop.p.norwr.i.ingame_menu_enabled = false;
    if (!rtcemu_avail(&spop.p.norwr.i))
      spop.p.norwr.i.rtc_patch_enabled = false;

    if ((newkeys & KEY_BUTTA) && spop.selector == GBAPatchGen) {
      generate_patches_progress(spop.p.norwr.i.romfn, spop.p.norwr.i.romfs);
      spop.alert_msg = msgs[lang_id][MSG_PATCHGEN_OK];
      // Try/Load the just-generated patches.
      spop.p.norwr.i.patches_cache_found = load_cached_patches(spop.p.norwr.i.romfn, &spop.p.norwr.i.patches_cache);
    }
  } else {
    if (newkeys & KEY_BUTTA) {
      // Check whether we have enough space.
      unsigned blkcnt = (spop.p.norwr.i.romfs + NOR_BLOCK_SIZE - 1) / NOR_BLOCK_SIZE;
      if (smenu.fbrowser.freeblks < blkcnt || smenu.fbrowser.maxentries + 1 >= FLASHG_MAXFN_CNT)
        spop.alert_msg = msgs[lang_id][MSG_ERR_NORSPC];
      else {
        const t_load_gba_info *info = &spop.p.norwr.i;
        const t_patch *p = get_game_patch(info);

        // Allocate the last entry for the new game.
        t_flash_game_entry ne = {
          .gamecode = *(uint32_t*)info->romh.gcode,
          .gamever = info->romh.version,
          .numblks = blkcnt,
          .gattrs = (info->use_dsaving         ? GATTR_SAVEDS : 0) |
                    (info->ingame_menu_enabled ? GATTR_IGM    : 0) |
                    (info->rtc_patch_enabled   ? GATTR_RTC    : 0) |
                    GATTR_SAVEM(p),
          .bnoffset = (uint8_t)(file_basename(info->romfn) - info->romfn),
          .entry_addr = ROM_ENTRYPOINT(info->romh)
        };
        memset(&ne.blkmap, 0, sizeof(ne.blkmap));
        strcpy(ne.game_name, info->romfn);

        flashmgr_allocate_blocks(ne.blkmap, blkcnt, (t_reg_entry*)&sdr_state->nordata);

        // Go ahead and start the flasher-loader with patching support.
        unsigned errc = flash_gba_nor(info->romfn, info->romfs, &info->romh, p,
                                      info->use_dsaving, info->ingame_menu_enabled,
                                      info->rtc_patch_enabled,
                                      ne.blkmap, loadrom_progress,
                                      sdr_state->scratch, scratch_mem_size);
        if (errc)
          spop.alert_msg = msgs[lang_id][errc == ERR_LOAD_BADROM ? MSG_ERR_READ : MSG_ERR_NORUPD];
        else {
          // Now we can just write the metadata entry!
          memcpy32(&sdr_state->nordata.games[smenu.fbrowser.maxentries], &ne, sizeof(ne));
          sdr_state->nordata.gamecnt++;
          if (!flashmgr_store(ROM_FLASHMETA_ADDR, FLASH_METADATA_SIZE, (t_reg_entry*)&sdr_state->nordata))
            spop.alert_msg = msgs[lang_id][MSG_ERR_NORUPD];
          else {
            spop.alert_msg = msgs[lang_id][MSG_NOR_WROK];
            spop.pop_num = POPUP_NONE;
          }
        }

        flashbrowser_reload();
      }
    }
  }
}

static void keypress_popup_norload(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    spop.selector = MAX(0, spop.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    spop.selector = MIN(GBALdSetCNT - 1, spop.selector + 1);

  const t_flash_game_entry *e = spop.p.norld.e;
  bool uses_dsave = e->gattrs & GATTR_SAVEDS;
  bool uses_igm   = e->gattrs & GATTR_IGM;
  bool uses_rtc   = e->gattrs & GATTR_RTC;

  if (newkeys & KEY_BUTTLEFT) {
    if (spop.submenu == GbaNorLoad) {
      if (spop.selector == GBALdSetCheats)
        spop.p.norld.l.use_cheats = !spop.p.norld.l.use_cheats;
      if (uses_dsave) {
        if (spop.selector == GBALdSetLoadP)
          spop.p.norld.l.sram_load_type = (spop.p.norld.l.sram_load_type + SaveLoadDSCNT - 1) % SaveLoadDSCNT;
      } else {
        if (spop.selector == GBALdSetLoadP)
          spop.p.norld.l.sram_load_type = (spop.p.norld.l.sram_load_type + SaveLoadCNT - 1) % SaveLoadCNT;
        else if (spop.selector == GBALdSetSaveP)
          spop.p.norld.l.sram_save_type = (spop.p.norld.l.sram_save_type + SaveCNT - 1) % SaveCNT;
      }
    }

    // DirSav forces automatic saving
    if (uses_dsave)
      spop.p.norld.l.sram_save_type = SaveDirect;
    else if (spop.p.norld.l.sram_save_type == SaveDirect)
      spop.p.norld.l.sram_save_type = autosave_default ? SaveReboot : SaveDisable;

    // If DS is selected, do not allow manual mode.
    if (spop.p.norld.l.sram_load_type == SaveLoadDisable && uses_dsave)
      spop.p.norld.l.sram_load_type = SaveLoadSav;
    // If no .sav is available, do not allow that option!
    if (spop.p.norld.l.sram_load_type == SaveLoadSav && !spop.p.norld.l.savefile_found)
      spop.p.norld.l.sram_load_type = uses_dsave ? SaveLoadReset : SaveLoadDisable;
  }
  if (newkeys & KEY_BUTTRIGHT) {
    if (spop.submenu == GbaNorLoad) {
      if (spop.selector == GBALdSetCheats)
        spop.p.norld.l.use_cheats = !spop.p.norld.l.use_cheats;
      if (uses_dsave) {
        if (spop.selector == GBALdSetLoadP)
          spop.p.norld.l.sram_load_type = (spop.p.norld.l.sram_load_type + 1) % SaveLoadDSCNT;
      } else {
        if (spop.selector == GBALdSetLoadP)
          spop.p.norld.l.sram_load_type = (spop.p.norld.l.sram_load_type + 1) % SaveLoadCNT;
        else if (spop.selector == GBALdSetSaveP)
          spop.p.norld.l.sram_save_type = (spop.p.norld.l.sram_save_type + 1) % SaveCNT;
      }
    }

    // DirSav forces automatic saving
    if (uses_dsave)
      spop.p.norld.l.sram_save_type = SaveDirect;
    else if (spop.p.norld.l.sram_save_type == SaveDirect)
      spop.p.norld.l.sram_save_type = autosave_default ? SaveReboot : SaveDisable;

    // If DS is selected, do not allow manual mode.
    if (spop.p.norld.l.sram_load_type == SaveLoadDisable && uses_dsave)
      spop.p.norld.l.sram_load_type = SaveLoadSav;
    // If no .sav is available, do not allow that option!
    if (spop.p.norld.l.sram_load_type == SaveLoadSav && !spop.p.norld.l.savefile_found)
      spop.p.norld.l.sram_load_type = SaveLoadReset;
  }

  // Disable cheat loading if no cheats are avail, or IGM is disabled
  if (!spop.p.norld.l.cheats_found || !uses_igm)
    spop.p.norld.l.use_cheats = false;

  if (newkeys & KEY_BUTTA) {
    if (spop.submenu == GbaLoadPopInfo) {
      const int stype = GET_GATTR_SAVEM(e->gattrs);
      const EnumSavetype st = stype < 0 ? SaveTypeNone : stype;
      bool uses_dsave = e->gattrs & GATTR_SAVEDS;
      bool uses_igm   = e->gattrs & GATTR_IGM;
      bool uses_rtc   = e->gattrs & GATTR_RTC;

      t_dirsave_info dsinfo;
      unsigned errsave = prepare_savegame(
        spop.p.norld.l.sram_load_type, spop.p.norld.l.sram_save_type,
        st, &dsinfo, spop.p.norld.l.savefn);
      if (errsave) {
        unsigned errmsg = (errsave == ERR_SAVE_BADSAVE)   ? MSG_ERR_SAVERD :
                          (errsave == ERR_SAVE_CANTALLOC) ? MSG_ERR_SAVEPR :
                          (errsave == ERR_SAVE_BADARG)    ? MSG_ERR_SAVEIT :
                                                            MSG_ERR_SAVEWR;
        spop.alert_msg = msgs[lang_id][errmsg];
        return;
      }
      t_rtc_info rtci = {
        .timestamp = spop.p.norld.l.rtcval,
        .ts_step = rtcspeed_default
      };

      if (recent_menu)
        insert_recent_flush(e->game_name, FLAG_RECENT_NOR);

      unsigned err = launch_gba_nor(
        e->game_name,
        spop.p.norld.l.sram_save_type == SaveDisable ? NULL : spop.p.norld.l.savefn,
        e->blkmap, e->numblks,
        uses_dsave ? &dsinfo : NULL,
        uses_rtc ? &rtci : NULL,
        uses_igm,
        spop.p.norld.l.use_cheats ? spop.p.norld.l.cheats_size : 0);
      if (err)
        spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
    }
    else if (spop.selector == GBALdRemember) {
      // Save settings to disk now!
      t_rom_load_settings ld_sett = {  // Use defaults in case it doesn't really exist
        .patch_policy = patcher_default,
        .use_igm = ingamemenu_default,
        .use_rtc = rtcpatch_default,
        .use_dsaving = autosave_prefer_ds
      };
      t_rom_launch_settings lh_sett = {
        .use_cheats = spop.p.norld.l.use_cheats,
        .rtcts = spop.p.norld.l.rtcval
      };

      // We load the loading settings to ensure we do not overwrite them.
      load_rom_settings(e->game_name, &ld_sett, NULL);
      save_rom_settings(e->game_name, &ld_sett, &lh_sett);
      spop.alert_msg = msgs[lang_id][MSG_REMEMB_CFG_OK];
    }
    else if (spop.selector == GBALdSetRTC) {
      void accept_rtc() {
        spop.p.norld.l.rtcval = date2timestamp(&spop.rtcpop.val);
      }
      if (uses_rtc) {
        timestamp2date(spop.p.norld.l.rtcval, &spop.rtcpop.val);
        spop.rtcpop.callback = accept_rtc;
      }
    }
  }
}
#endif

static void keypress_popup_filemgr(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    spop.selector = MAX(0, spop.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    spop.selector = MIN(FiMgrCNT - 1, spop.selector + 1);

  if (newkeys & KEY_BUTTA) {
    t_centry *e = sdr_state->fileorder[smenu.browser.selector];
    switch (spop.selector) {
    case FiMgrDelete:
      {
        void remove_file_action(bool confirm) {
          char tmpfn[MAX_FN_LEN];
          strcpy(tmpfn, smenu.browser.cpath);
          strcat(tmpfn, sdr_state->fileorder[smenu.browser.selector]->fname);

          if (confirm) {
            if (FR_OK != f_unlink(tmpfn))
              spop.alert_msg = msgs[lang_id][MSG_ERR_DELFILE];
            else
              spop.alert_msg = msgs[lang_id][MSG_OK_DELFILE];

            browser_reload();   // Force reload so the file disappears!
          }
        }
        spop.qpop.message = msgs[lang_id][MSG_Q0_DELFILE];
        spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
        spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
        spop.qpop.option = 0;
        spop.qpop.callback = remove_file_action;
        spop.qpop.clear_popup_ok = true;
      }
      break;
    case FiMgrHide:
      {
        char tmpfn[MAX_FN_LEN];
        strcpy(tmpfn, smenu.browser.cpath);
        strcat(tmpfn, sdr_state->fileorder[smenu.browser.selector]->fname);

        if (FR_OK == f_chmod(tmpfn, e->attr ^ AM_HID, AM_HID))
          e->attr ^= AM_HID;
        else
          spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
      }
      spop.pop_num = POPUP_NONE;
      break;

    #ifdef SUPPORT_NORGAMES
    case FiMgrWriteNOR:
      if (e->filesize > MAX_GBA_ROM_SIZE)
        spop.alert_msg = msgs[lang_id][MSG_ERR_TOOBIG];
      else {
        char path[MAX_FN_LEN];
        strcpy(path, smenu.browser.cpath);
        strcat(path, e->fname);

        // Load default loading settings if any.
        t_rom_load_settings ld_sett = {
          .patch_policy = patcher_default,
          .use_igm = ingamemenu_default,
          .use_rtc = rtcpatch_default,
          .use_dsaving = autosave_prefer_ds
        };
        load_rom_settings(path, &ld_sett, NULL);

        if (!prepare_gba_info(&spop.p.norwr.i, &ld_sett, path, e->filesize, false))
          spop.alert_msg = msgs[lang_id][MSG_ERR_READ];
        else {
          spop.pop_num = POPUP_GBA_NORWRITE;
          spop.submenu = GbaLoadPopInfo;
          spop.selector = 0;
        }
      }
      break;
    #endif
    };
  }
}

#ifdef UI_BROWSER_V2
static void keypress_menu_list_navigation(unsigned newkeys, int *selector,
                                           int *seloff, int maxentries,
                                           int rows) {
  int page_delta = (newkeys & KEY_BUTTLEFT) ? -1 :
                   (newkeys & KEY_BUTTRIGHT) ? 1 : 0;
  int item_delta = page_delta ? 0 :
                   (newkeys & KEY_BUTTUP) ? -1 :
                   (newkeys & KEY_BUTTDOWN) ? 1 : 0;
  menu_list_navigate(selector, seloff, maxentries, rows,
                     item_delta, page_delta);
}

static void keypress_menu_favorites(unsigned newkeys) {
  if (!smenu.favorites.maxentries)
    return;

  keypress_menu_list_navigation(newkeys, &smenu.favorites.selector,
                                &smenu.favorites.seloff,
                                smenu.favorites.maxentries, FAVORITES_ROWS);

#ifndef COVER_ART_DEMO
  if (newkeys & KEY_BUTTA) {
    char path[MAX_FN_LEN];
    favorite_copy_path(smenu.favorites.selector, path);
    FILINFO info;
    if (FR_OK == f_stat(path, &info))
      browser_open(path, info.fsize);
    else
      spop.alert_msg = msgs[lang_id][MSG_ERR_READ];
  } else
#endif
  if (newkeys & KEY_BUTTSEL) {
    favorite_pending_delete = smenu.favorites.selector;
    spop.qpop.message = "Remove Favorite?";
    spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
    spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
    spop.qpop.option = 0;
    spop.qpop.callback = favorite_delete_callback;
    spop.qpop.clear_popup_ok = false;
  }
}

static uint32_t cycle_setting(uint32_t value, unsigned count, int direction) {
  return (value + count + direction) % count;
}

static void keypress_menu_appearance(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    smenu.appearance.selector = MAX(0, smenu.appearance.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    smenu.appearance.selector = MIN(AppearanceMAX,
                                    smenu.appearance.selector + 1);

  int direction = (newkeys & KEY_BUTTLEFT) ? -1 :
                  (newkeys & KEY_BUTTRIGHT) ? 1 : 0;
  if (direction) {
    switch (smenu.appearance.selector) {
    case AppearancePreset:
      ui_theme_reset(cycle_setting(ui_theme_preset, UiThemePresetCount,
                                   direction));
      break;
    case AppearanceWallpaper:
      ui_wallpaper = cycle_setting(ui_wallpaper, UiWallpaperCount, direction);
      break;
    case AppearanceBackground:
      ui_background_color = cycle_setting(ui_background_color, UiColorCount,
                                          direction);
      break;
    case AppearanceAccent:
      ui_accent_color = cycle_setting(ui_accent_color, UiColorCount, direction);
      break;
    case AppearanceSelection:
      ui_selection_color = cycle_setting(ui_selection_color, UiColorCount,
                                         direction);
      break;
    case AppearanceContrast:
      ui_contrast = cycle_setting(ui_contrast, UiContrastCount, direction);
      break;
    }
  }

  if ((newkeys & KEY_BUTTA) &&
      smenu.appearance.selector == AppearanceReset)
    ui_theme_reset(ui_theme_preset);
}
#endif

static void keypress_menu_recent(unsigned newkeys) {
  if (smenu.recent.maxentries) {
#ifdef UI_BROWSER_V2
    keypress_menu_list_navigation(newkeys, &smenu.recent.selector,
                                  &smenu.recent.seloff,
                                  smenu.recent.maxentries, RECENT_ROWS);
#else
    if (newkeys & KEY_BUTTUP)
      smenu.recent.selector = MAX(0, smenu.recent.selector - 1);
    else if (newkeys & KEY_BUTTDOWN)
      smenu.recent.selector = MIN(smenu.recent.maxentries - 1, smenu.recent.selector + 1);
    if (newkeys & KEY_BUTTLEFT) {
      smenu.recent.selector = MAX(0, smenu.recent.selector - RECENT_ROWS);
      smenu.recent.seloff   = MAX(0, smenu.recent.seloff - RECENT_ROWS);
    }
    else if (newkeys & KEY_BUTTRIGHT) {
      smenu.recent.selector = MIN(smenu.recent.maxentries - 1, smenu.recent.selector + RECENT_ROWS);
      smenu.recent.seloff   = MIN(smenu.recent.maxentries - 1, smenu.recent.seloff   + RECENT_ROWS);
    }
#endif
#ifndef COVER_ART_DEMO
    if (newkeys & KEY_BUTTA) {
      t_rentry *e = &sdr_state->rentries[smenu.recent.selector];
      #ifdef SUPPORT_NORGAMES
      if (e->flags & FLAG_RECENT_NOR) {
        // Try to find the ROM in the current flash metadata.
        for (unsigned i = 0; i < sdr_state->nordata.gamecnt; i++) {
          const t_flash_game_entry *fe = &sdr_state->nordata.games[i];
          if (!strcmp(e->fpath, fe->game_name)) {
            browser_open_nor(fe);
            return;
          }
        }
        spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
      }
      else
      #endif
      {
        char path[MAX_FN_LEN];
        dma_memcpy16(path, e->fpath, MAX_FN_LEN / 2);
        path[MAX_FN_LEN - 1] = 0;
        // stat() the file since we need the size, and validate that it exists!
        FILINFO info;
        FRESULT res = f_stat(path, &info);
        if (res == FR_OK)
          browser_open(path, info.fsize);
        else
          spop.alert_msg = msgs[lang_id][MSG_ERR_READ];
      }
    }
    else if (newkeys & KEY_BUTTSEL) {
      void recent_del_cb(bool confirm) {
        if (confirm)
          delete_recent_flush(smenu.recent.selector);
      }
      spop.qpop.message = msgs[lang_id][MSG_Q4_DELREC];
      spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
      spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
      spop.qpop.option = 0;
      spop.qpop.callback = recent_del_cb;
      spop.qpop.clear_popup_ok = false;
    }
#endif
  }

#ifndef UI_BROWSER_V2
  if (smenu.recent.selector < smenu.recent.seloff)
    smenu.recent.seloff = smenu.recent.selector;
  else if (smenu.recent.selector >= smenu.recent.seloff + RECENT_ROWS)
    smenu.recent.seloff = smenu.recent.selector - RECENT_ROWS + 1;
#endif
}

#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
static void keypress_popup_browser_view(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    spop.selector = MAX(0, spop.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    spop.selector = MIN(3, spop.selector + 1);

  if (!(newkeys & KEY_BUTTA))
    return;

  switch (spop.selector) {
  case 0:
    browser_sort_descending ^= 1;
    break;
  case 1:
    browser_game_filter = cycle_setting(browser_game_filter,
                                        BrowserGameFilterCount,
                                        1);
    break;
  case 2:
    browser_hide_folders ^= 1;
    break;
  case 3:
    browser_hide_unknown ^= 1;
    break;
  }

  browser_reload_filter();
  smenu.anim_state = 0;
  menu_cover_request_selected();
#ifndef COVER_ART_DEMO
  if (!save_ui_settings())
    spop.alert_msg = msgs[lang_id][MSG_ERR_SETSAVE];
#endif
}
#endif

static void keypress_menu_browse(unsigned newkeys) {
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
  if (newkeys & KEY_BUTTSTA) {
    spop.pop_num = POPUP_BROWSER_VIEW;
    spop.selector = 0;
    return;
  }
#endif

  if (smenu.browser.dispentries) {
#ifdef UI_BROWSER_V2
    keypress_menu_list_navigation(newkeys, &smenu.browser.selector,
                                  &smenu.browser.seloff,
                                  smenu.browser.dispentries, BROWSER_ROWS);
#else
    // Move menu up and down
    if (newkeys & KEY_BUTTUP)
      smenu.browser.selector = MAX(0, smenu.browser.selector - 1);
    if (newkeys & KEY_BUTTDOWN)
      smenu.browser.selector = MIN(smenu.browser.dispentries - 1, smenu.browser.selector + 1);
    if (newkeys & KEY_BUTTLEFT) {
      smenu.browser.selector = MAX(0, smenu.browser.selector - BROWSER_ROWS);
      smenu.browser.seloff   = MAX(0, smenu.browser.seloff - BROWSER_ROWS);
    }
    if (newkeys & KEY_BUTTRIGHT) {
      smenu.browser.selector = MIN(smenu.browser.dispentries - 1, smenu.browser.selector + BROWSER_ROWS);
      smenu.browser.seloff   = MIN(smenu.browser.dispentries - 1, smenu.browser.seloff   + BROWSER_ROWS);
    }
#endif
    // Move into a new dir and/or open a file
#ifndef COVER_ART_DEMO
    if (newkeys & KEY_BUTTA) {
      t_centry *e = sdr_state->fileorder[smenu.browser.selector];
      if (e->isdir) {
        strcat(smenu.browser.cpath, e->fname);
        strcat(smenu.browser.cpath, "/");
        // Push selector history and reset it in the new dir
        memmove(&smenu.browser.selhist[1], &smenu.browser.selhist[0],
                sizeof(smenu.browser.selhist) - sizeof(smenu.browser.selhist[0]));
        smenu.browser.selhist[0] = smenu.browser.selector;
        smenu.browser.selector = 0;
        browser_reload();
      } else {
        char path[MAX_FN_LEN];
        strcpy(path, smenu.browser.cpath);
        strcat(path, e->fname);
        browser_open(path, e->filesize);
      }
    }
    else if (newkeys & KEY_BUTTSEL) {
      // Shows a file management menu.
      spop.pop_num = POPUP_FILE_MGR;
      spop.anim = 0;
      spop.selector = 0;
    }
#else
#ifdef UI_BROWSER_V2
    if (newkeys & KEY_BUTTSEL) {
      ui_selection_color = ui_selection_color == UiColorRed ?
                           UiColorCyan : UiColorRed;
      ui_browser_v2_load_palette(MEM_PALETTE);
    }
    else
#endif
    if (newkeys & KEY_BUTTA) {
      t_centry *e = sdr_state->fileorder[smenu.browser.selector];
      unsigned length = strlen(e->fname);
      if (length >= 4 && !strcasecmp(&e->fname[length - 4], ".gba")) {
        memset(&spop.p.load, 0, sizeof(spop.p.load));
        strcpy(spop.p.load.i.romfn, e->fname);
        strcpy(spop.p.load.i.gcode, "DEMO");
        spop.p.load.i.romh.version = 1;
        spop.p.load.i.patch_type = PatchNone;
        spop.p.load.i.use_dsaving = true;
        spop.p.load.i.ingame_menu_enabled = true;
        spop.p.load.i.rtc_patch_enabled = true;
        spop.p.load.l.sram_load_type = SaveLoadReset;
        spop.p.load.l.sram_save_type = SaveDirect;
        spop.p.load.l.rtcval = rtcvalue_default;
        spop.p.load.l.cheats_found = true;
        spop.p.load.l.use_cheats = true;
        spop.pop_num = POPUP_GBA_LOAD;
        spop.submenu = GbaLoadPopInfo;
        spop.selector = GBALoadButt;
      } else if (length >= 4 && !strcasecmp(&e->fname[length - 4], ".sav")) {
        strcpy(spop.p.savopt.savfn, e->fname);
        spop.pop_num = POPUP_SAVFILE;
        spop.selector = 0;
      } else if (guessicon(e->fname) == ICON_UPDFILE) {
        strcpy(spop.p.update.fn, e->fname);
        spop.p.update.issfw = true;
        spop.p.update.curr_state = 0;
        spop.pop_num = POPUP_FWFLASH;
        spop.selector = 0;
      }
#ifndef UI_BROWSER_V2
    } else if (newkeys & KEY_BUTTSEL) {
      spop.pop_num = POPUP_FILE_MGR;
      spop.anim = 0;
      spop.selector = 0;
#endif
    }
#endif
  }
#ifndef COVER_ART_DEMO
  if (newkeys & KEY_BUTTB) {
    // Try to go up in the dir structure
    if (movedir_up()) {
      smenu.browser.selector = smenu.browser.selhist[0];
      memmove(&smenu.browser.selhist[0], &smenu.browser.selhist[1],
              sizeof(smenu.browser.selhist) - sizeof(smenu.browser.selhist[0]));
      browser_reload();
    }
  }
#endif

#ifndef UI_BROWSER_V2
  // Selector was updated, figure out how we update the menu params so it
  // can be rendered properly.
  if (smenu.browser.selector < smenu.browser.seloff)
    smenu.browser.seloff = smenu.browser.selector;
  else if (smenu.browser.selector >= smenu.browser.seloff + BROWSER_ROWS)
    smenu.browser.seloff = smenu.browser.selector - BROWSER_ROWS + 1;
#endif
}

#ifdef SUPPORT_NORGAMES
static void keypress_menu_norbrowse(unsigned newkeys) {
  if (smenu.fbrowser.maxentries) {
    if (newkeys & KEY_BUTTUP)
      smenu.fbrowser.selector = MAX(0, smenu.fbrowser.selector - 1);
    if (newkeys & KEY_BUTTDOWN)
      smenu.fbrowser.selector = MIN(smenu.fbrowser.maxentries - 1, smenu.fbrowser.selector + 1);
    if (newkeys & KEY_BUTTLEFT) {
      smenu.fbrowser.selector = MAX(0, smenu.fbrowser.selector - NORGAMES_ROWS);
      smenu.fbrowser.seloff   = MAX(0, smenu.fbrowser.seloff - NORGAMES_ROWS);
    }
    if (newkeys & KEY_BUTTRIGHT) {
      smenu.fbrowser.selector = MIN(smenu.fbrowser.maxentries - 1, smenu.fbrowser.selector + NORGAMES_ROWS);
      smenu.fbrowser.seloff   = MIN(smenu.fbrowser.maxentries - 1, smenu.fbrowser.seloff   + NORGAMES_ROWS);
    }

    if (newkeys & KEY_BUTTA)
      browser_open_nor(&sdr_state->nordata.games[smenu.fbrowser.selector]);
    else if (newkeys & KEY_BUTTSEL) {
      // Prompt NOR entry deletion.
      void remove_nor_action(bool confirm) {
        if (!confirm)
          return;

        // Remove game entry, just memmove the other games on top.
        sdr_state->nordata.gamecnt--;
        memmove32(&sdr_state->nordata.games[smenu.fbrowser.selector],
                  &sdr_state->nordata.games[smenu.fbrowser.selector + 1],
                  (sdr_state->nordata.gamecnt - smenu.fbrowser.selector) * sizeof(t_flash_game_entry));

        // Go ahead and write a new metadata entry;
        if (!flashmgr_store(ROM_FLASHMETA_ADDR, FLASH_METADATA_SIZE, (t_reg_entry*)&sdr_state->nordata))
          spop.alert_msg = msgs[lang_id][MSG_ERR_NORUPD];
        flashbrowser_reload();   // Force list reload, free block calculation, etc.
      }
      spop.qpop.message = msgs[lang_id][MSG_Q5_DELNORG];
      spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
      spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
      spop.qpop.option = 0;
      spop.qpop.callback = remove_nor_action;
      spop.qpop.clear_popup_ok = true;
    }

    if (smenu.fbrowser.selector < smenu.fbrowser.seloff)
      smenu.fbrowser.seloff = smenu.fbrowser.selector;
    else if (smenu.fbrowser.selector >= smenu.fbrowser.seloff + NORGAMES_ROWS)
      smenu.fbrowser.seloff = smenu.fbrowser.selector - NORGAMES_ROWS + 1;
  }
}
#endif

static void keypress_menu_settings(unsigned newkeys) {
#ifdef UI_BROWSER_V2
  if (smenu.set.selector == SettTitle1 || smenu.set.selector == SettTitle2)
    smenu.set.selector++;
  if (newkeys & KEY_BUTTUP) {
    smenu.set.selector = MAX(SettHotkey, smenu.set.selector - 1);
    if (smenu.set.selector == SettTitle2)
      smenu.set.selector--;
  }
  if (newkeys & KEY_BUTTDOWN) {
    smenu.set.selector = MIN(SettMAX - 1, smenu.set.selector + 1);
    if (smenu.set.selector == SettTitle2)
      smenu.set.selector++;
  }
#else
  if (newkeys & KEY_BUTTUP)
    smenu.set.selector = MAX(0, smenu.set.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    smenu.set.selector = MIN(SettMAX - 1, smenu.set.selector + 1);
#endif
  if (newkeys & KEY_BUTTLEFT) {
    if (smenu.set.selector == SettHotkey)
      hotkey_combo = (hotkey_combo + hotkey_listcnt - 1) % hotkey_listcnt;
    else if (smenu.set.selector == SettSaveLoc)
      save_path_default = (save_path_default + SaveDirCNT - 1) % SaveDirCNT;
    #ifdef SUPPORT_NORGAMES
    else if (smenu.set.selector == SettSaveLocNOR)
      save_path_nor_default = (save_path_nor_default + SaveDirNORCNT - 1) % SaveDirNORCNT;
    #endif
    else if (smenu.set.selector == SettStateLoc)
      state_path_default = (state_path_default + StateDirCNT - 1) % StateDirCNT;
    else if (smenu.set.selector == SettSaveBkp)
      backup_sram_default = backup_sram_default ? backup_sram_default - 1 : 0;
    else if (smenu.set.selector == DefsPatchEng)
      patcher_default = (patcher_default + PatchTotalCNT - 1) % PatchTotalCNT;
    else if (smenu.set.selector == DefsRTCSpeed)
      rtcspeed_default = (rtcspeed_default + RTC_SPEED_CNT - 1) % RTC_SPEED_CNT;
  }
  if (newkeys & KEY_BUTTRIGHT) {
    if (smenu.set.selector == SettHotkey)
      hotkey_combo = (hotkey_combo + 1) % hotkey_listcnt;
    else if (smenu.set.selector == SettSaveLoc)
      save_path_default = (save_path_default + 1) % SaveDirCNT;
    #ifdef SUPPORT_NORGAMES
    else if (smenu.set.selector == SettSaveLocNOR)
      save_path_nor_default = (save_path_nor_default + 1) % SaveDirNORCNT;
    #endif
    else if (smenu.set.selector == SettStateLoc)
      state_path_default = (state_path_default + 1) % StateDirCNT;
    else if (smenu.set.selector == SettSaveBkp)
      backup_sram_default = MIN(MAX_BACKUP_CNT, backup_sram_default + 1);
    else if (smenu.set.selector == DefsPatchEng)
      patcher_default = (patcher_default + 1) % PatchTotalCNT;
    else if (smenu.set.selector == DefsRTCSpeed)
      rtcspeed_default = (rtcspeed_default + 1) % RTC_SPEED_CNT;
  }
  if (newkeys & (KEY_BUTTLEFT | KEY_BUTTRIGHT)) {
    if (smenu.set.selector == SettBootType)
      boot_bios_splash ^= 1;
    else if (smenu.set.selector == SettCheatEn)
      enable_cheats ^= 1;
    else if (smenu.set.selector == DefsGamMenu)
      ingamemenu_default ^= 1;
    else if (smenu.set.selector == DefsRTCEnb)
      rtcpatch_default ^= 1;
    else if (smenu.set.selector == DefsLoadPol)
      autoload_default ^= 1;
    else if (smenu.set.selector == DefsSavePol)
      autosave_default ^= 1;
    else if (smenu.set.selector == DefsPrefDS)
      autosave_prefer_ds ^= 1;
    else if (smenu.set.selector == SettFastSD)
      use_slowld ^= 1;
    else if (smenu.set.selector == SettFastEWRAM)
      use_fastew = fastew ? (use_fastew ^ 1) : 0;
    #ifdef SUPPORT_NORGAMES
    else if (smenu.set.selector == SettVerifyNOR)
      use_verify_nor ^= 1;
    #endif
  }

  if (newkeys & KEY_BUTTA && smenu.set.selector == DefsRTCVal) {
    void accept_rtc() {
      rtcvalue_default = date2timestamp(&spop.rtcpop.val);
    }
    timestamp2date(rtcvalue_default, &spop.rtcpop.val);
    spop.rtcpop.callback = accept_rtc;
  }
  if (newkeys & KEY_BUTTA && smenu.set.selector == SettSave) {
#ifdef UI_BROWSER_V2
    smenu.set.selector = SettHotkey;
#else
    smenu.set.selector = 0;
#endif
    if (save_settings())
      spop.alert_msg = msgs[lang_id][MSG_OK_SETSAVE];
    else
      spop.alert_msg = msgs[lang_id][MSG_ERR_SETSAVE];
  }
}

static void keypress_menu_uisettings(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    smenu.uiset.selector = MAX(0, smenu.uiset.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    smenu.uiset.selector = MIN(UiSetMAX, smenu.uiset.selector + 1);
  if (newkeys & KEY_BUTTLEFT) {
    if (smenu.uiset.selector == UiSetASpd)
      anim_speed = anim_speed ? anim_speed - 1 : 0;
    else if (smenu.uiset.selector == UiSetHid)
      hide_hidden ^= 1;
    else if (smenu.uiset.selector == UiSetRect)
      recent_menu ^= 1;
    else if (smenu.uiset.selector == UiSetLang)
      lang_id = (lang_id + LANG_COUNT - 1) % LANG_COUNT;
  }
  if (newkeys & KEY_BUTTRIGHT) {
    if (smenu.uiset.selector == UiSetASpd)
      anim_speed = MIN(animspd_cnt - 1, anim_speed + 1);
    else if (smenu.uiset.selector == UiSetHid)
      hide_hidden ^= 1;
    else if (smenu.uiset.selector == UiSetRect)
      recent_menu ^= 1;
    else if (smenu.uiset.selector == UiSetLang)
      lang_id = (lang_id + 1) % LANG_COUNT;
  }

  if (newkeys & KEY_BUTTA && smenu.uiset.selector == UiSetSave) {
    smenu.uiset.selector = UiSetLang;
    if (save_ui_settings())
      spop.alert_msg = msgs[lang_id][MSG_OK_SETSAVE];
    else
      spop.alert_msg = msgs[lang_id][MSG_ERR_SETSAVE];
  }

  reload_theme();
}

static void keypress_menu_tools(unsigned newkeys) {
  if (newkeys & KEY_BUTTUP)
    smenu.tools.selector = MAX(0, smenu.tools.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    smenu.tools.selector = MIN(ToolsMAX - 1, smenu.tools.selector + 1);

#ifdef UI_BROWSER_V2
  if (smenu.tools.selector < smenu.tools.seloff)
    smenu.tools.seloff = smenu.tools.selector;
  else if (smenu.tools.selector >= smenu.tools.seloff + UI_BROWSER_V2_ROWS)
    smenu.tools.seloff = smenu.tools.selector - UI_BROWSER_V2_ROWS + 1;
#endif

  if (newkeys & KEY_BUTTA) {
#ifdef UI_BROWSER_V2
    if (smenu.tools.selector == ToolsAppearance) {
      smenu.menu_tab = MENUTAB_APPEARANCE;
      return;
    }
    if (smenu.tools.selector == ToolsInterface) {
      smenu.uiset.selector = UiSetLang;
      smenu.menu_tab = MENUTAB_UILANG;
      return;
    }
    if (smenu.tools.selector == ToolsSettings) {
      smenu.set.selector = SettHotkey;
      smenu.menu_tab = MENUTAB_SETTINGS;
      return;
    }
    if (smenu.tools.selector == ToolsInfo) {
      smenu.menu_tab = MENUTAB_INFO;
      return;
    }
    if (smenu.tools.selector == ToolsResetFavorites) {
      spop.qpop.message = msgs[lang_id][MSG_Q_RESET_FAVORITES];
      spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
      spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
      spop.qpop.option = 0;
      spop.qpop.callback = favorites_reset_callback;
      spop.qpop.clear_popup_ok = true;
      return;
    }
    if (smenu.tools.selector == ToolsResetRecent) {
      spop.qpop.message = msgs[lang_id][MSG_Q_RESET_RECENT];
      spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
      spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
      spop.qpop.option = 0;
      spop.qpop.callback = recent_reset_callback;
      spop.qpop.clear_popup_ok = true;
      return;
    }
#endif
    if (smenu.tools.selector == ToolsSDRAMTest) {
      // Performs a test on the SRAM/SDRAM, ensure they are fine.
      set_supercard_mode(MAPPED_SDRAM, true, false);

      if (sdram_test(loadrom_progress_abort))
        spop.alert_msg = msgs[lang_id][MSG_BAD_SDRAM];
      else
        spop.alert_msg = msgs[lang_id][MSG_GOOD_RAM];

      set_supercard_mode(MAPPED_SDRAM, true, true);
    }
    if (smenu.tools.selector == ToolsSRAMTest) {
      if (sram_test())
        spop.alert_msg = msgs[lang_id][MSG_BAD_SRAM];
      else
        spop.alert_msg = msgs[lang_id][MSG_GOOD_RAM];
    }
    else if (smenu.tools.selector == ToolsBatteryTest) {
      // Go ahead and fill in SRAM with a pattern.
      spop.qpop.message = msgs[lang_id][MSG_Q2_SRAMTST];
      spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
      spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
      spop.qpop.option = 0;
      spop.qpop.callback = sram_battery_test_callback;
      spop.qpop.clear_popup_ok = true;
    }
    else if (smenu.tools.selector == ToolsSDBench) {
      slowsd = use_slowld;
      int ret = sdbench_read(loadrom_progress_abort);
      slowsd = true;
      if (ret < 0)
        spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];
      else {
        unsigned speed = 8*1024*1024 / (unsigned)ret;
        npf_snprintf(smenu.info.tstr, sizeof(smenu.info.tstr), msgs[lang_id][MSG_BENCHSPD], speed);
        spop.alert_msg = smenu.info.tstr;
      }
    }
    else if (smenu.tools.selector == ToolsFlashBak) {
      // Backup the flash contents to a file.
      if (dump_flashmem_backup())
        spop.alert_msg = msgs[lang_id][MSG_FLASH_READOK];
      else
        spop.alert_msg = msgs[lang_id][MSG_ERR_GENERIC];

      browser_reload();
    }
    #ifdef SUPPORT_NORGAMES
    else if (smenu.tools.selector == ToolsFlashClr) {
      void flash_clear_callback(bool confirm) {
        if (confirm) {
          // Delete all metadata (data is not really wiped, takes too long)
          if (flashmgr_wipe(ROM_FLASHMETA_ADDR, FLASH_METADATA_SIZE))
            spop.alert_msg = msgs[lang_id][MSG_NOR_CLOK];
          else
            spop.alert_msg = msgs[lang_id][MSG_ERR_NORUPD];

          flashbrowser_reload();     // Ensure we clear the NOR entries from RAM.
        }
      }
      // Prompt the user for clearing the memory.
      spop.qpop.message = msgs[lang_id][MSG_Q6_CLRNOR];
      spop.qpop.default_button = msgs[lang_id][MSG_Q_NO];
      spop.qpop.confirm_button = msgs[lang_id][MSG_Q_YES];
      spop.qpop.option = 0;
      spop.qpop.callback = flash_clear_callback;
      spop.qpop.clear_popup_ok = true;
    }
    #endif
  }
}

static void keypress_menu_info(unsigned newkeys) {
#ifdef UI_BROWSER_V2
  if (newkeys & KEY_BUTTUP)
    smenu.info.selector = MAX(0, smenu.info.selector - 1);
  if (newkeys & KEY_BUTTDOWN)
    smenu.info.selector = MIN(InfoCNT - 1, smenu.info.selector + 1);

  if (smenu.info.selector < smenu.info.seloff)
    smenu.info.seloff = smenu.info.selector;
  else if (smenu.info.selector >= smenu.info.seloff + UI_BROWSER_V2_ROWS)
    smenu.info.seloff = smenu.info.selector - UI_BROWSER_V2_ROWS + 1;

  if (smenu.info.selector == InfoUpdater &&
      (newkeys & (KEY_BUTTA | KEY_BUTTLEFT | KEY_BUTTRIGHT)))
    enable_flashing ^= 1;
#else
  if (newkeys & KEY_BUTTA)
    smenu.info.selector = (smenu.info.selector + 1) % 4;
  if ((newkeys & FLASH_UNLOCK_KEYS) == FLASH_UNLOCK_KEYS)
    enable_flashing = true;
#endif
}

#ifdef UI_BROWSER_V2
static unsigned phase4_dock_for_tab(unsigned tab) {
  if (tab == MENUTAB_FAVORITES)
    return 0;
  if (tab == MENUTAB_RECENT)
    return 1;
  if (tab == MENUTAB_ROMBROWSE)
    return 2;
  return 3;
}

static void phase4_select_dock(unsigned dock) {
  static const uint8_t tabs[] = {
    MENUTAB_FAVORITES, MENUTAB_RECENT, MENUTAB_ROMBROWSE, MENUTAB_TOOLS,
  };
  smenu.menu_tab = tabs[dock % ARRAY_SIZE(tabs)];
}
#endif


void menu_keypress(unsigned newkeys) {
#if defined(COVER_ART_DEMO) && defined(UI_BROWSER_V2)
  if (demo_loading_progress >= 0) {
    if (newkeys & KEY_BUTTSEL) {
      ui_theme_reset((ui_theme_preset + 1) % UiThemePresetCount);
      demo_loading_progress = 50;
    } else if (newkeys & KEY_BUTTB) {
      demo_loading_progress = -1;
      spop.submenu = GbaLoadPopInfo;
      spop.selector = GBALoadButt;
    }
    return;
  }
#endif

  if (spop.alert_msg) {
    // Modal message pop up!
    if (newkeys & (KEY_BUTTA | KEY_BUTTB))
      spop.alert_msg = NULL;
  }
  else if (spop.qpop.message) {
    // Modal confirm/question dialog
    if (newkeys & (KEY_BUTTUP | KEY_BUTTDOWN))
      spop.qpop.option ^= 1;
    else if (newkeys & KEY_BUTTB)
      spop.qpop.message = NULL;   // Exit the modal dialog.
    else if (newkeys & KEY_BUTTA) {
      if (spop.qpop.callback) {
        if (spop.qpop.option && spop.qpop.clear_popup_ok)
          spop.pop_num = POPUP_NONE;
        spop.qpop.callback(spop.qpop.option);
      }
      spop.qpop.message = NULL;   // Exit the modal dialog.
    }
  }
  else if (spop.rtcpop.callback) {
    if (newkeys & KEY_BUTTLEFT)
      spop.rtcpop.selector = MAX(0, spop.rtcpop.selector - 1);
    if (newkeys & KEY_BUTTRIGHT)
      spop.rtcpop.selector = MIN(4, spop.rtcpop.selector + 1);

    if (newkeys & KEY_BUTTUP)
      ((uint8_t*)&spop.rtcpop.val)[spop.rtcpop.selector]++;
    if (newkeys & KEY_BUTTDOWN)
      ((uint8_t*)&spop.rtcpop.val)[spop.rtcpop.selector]--;

    if (newkeys & (KEY_BUTTUP|KEY_BUTTDOWN))
      fixdate(&spop.rtcpop.val);

    if (newkeys & KEY_BUTTB) {
      spop.rtcpop.selector = 0;
      spop.rtcpop.callback = NULL;
    }
    else if (newkeys & KEY_BUTTA) {
      spop.rtcpop.selector = 0;
      spop.rtcpop.callback();
      spop.rtcpop.callback = NULL;
    }
  }
  else if (spop.pop_num) {
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
    if (spop.pop_num == POPUP_BROWSER_VIEW && (newkeys & KEY_BUTTSTA)) {
      spop.pop_num = POPUP_NONE;
      return;
    }
#endif
#if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
    const int subcnt = popup_submenus[spop.pop_num];
#else
    const int subcnt = popup_windows[spop.pop_num - 1].max_submenu;
#endif
    if (newkeys & KEY_BUTTL)
      spop.submenu = (spop.submenu + subcnt - 1) % subcnt;
    if (newkeys & KEY_BUTTR)
      spop.submenu = (spop.submenu + 1) % subcnt;

    // Close pop-up on B button
    if (newkeys & KEY_BUTTB) {
#ifdef UI_BROWSER_V2
      if (spop.pop_num == POPUP_GBA_LOAD &&
          spop.submenu != GbaLoadPopInfo) {
        spop.submenu = GbaLoadPopInfo;
        spop.selector = GBALoadButt;
      } else
#endif
        spop.pop_num = 0;
    }
    else {
      const t_mkeyupd_fn keyfns[] = {
        NULL,
        keypress_popup_loadgba,
        keypress_popup_savefile,
        keypress_popup_flash,
        keypress_popup_filemgr,
        #if defined(UI_BROWSER_V2) && !defined(SUPPORT_NORGAMES)
        keypress_popup_browser_view,
        #endif
        #ifdef SUPPORT_NORGAMES
        keypress_popup_norwrite,
        keypress_popup_norload,
        #endif
      };
      keyfns[spop.pop_num](newkeys);
    }
  } else {
#ifdef UI_BROWSER_V2
    bool secondary_tools = smenu.menu_tab == MENUTAB_SETTINGS ||
                           smenu.menu_tab == MENUTAB_UILANG ||
                           smenu.menu_tab == MENUTAB_APPEARANCE ||
                           smenu.menu_tab == MENUTAB_INFO;
    if ((newkeys & KEY_BUTTB) && secondary_tools) {
      if (smenu.menu_tab == MENUTAB_APPEARANCE && !save_ui_settings())
        spop.alert_msg = msgs[lang_id][MSG_ERR_SETSAVE];
      smenu.menu_tab = MENUTAB_TOOLS;
      newkeys &= ~KEY_BUTTB;
    }

    // Shoulder buttons move only between the four visible dock destinations.
    unsigned dock = phase4_dock_for_tab(smenu.menu_tab);
    if (newkeys & KEY_BUTTL) {
      if (smenu.menu_tab == MENUTAB_APPEARANCE)
        save_ui_settings();
      phase4_select_dock((dock + 3) % 4);
    } else if (newkeys & KEY_BUTTR) {
      if (smenu.menu_tab == MENUTAB_APPEARANCE)
        save_ui_settings();
      phase4_select_dock((dock + 1) % 4);
    }
#else
    // Menu change via trigger buttons
    int mintab = (recent_menu && smenu.recent.maxentries) ? MENUTAB_RECENT : MENUTAB_ROMBROWSE;
    if (newkeys & KEY_BUTTL)
      smenu.menu_tab = MAX((int)smenu.menu_tab - 1, mintab);
    else if (newkeys & KEY_BUTTR)
      smenu.menu_tab = MIN(smenu.menu_tab + 1, MENUTAB_MAX - 1);
#endif

    if (newkeys & (KEY_BUTTL | KEY_BUTTR | KEY_BUTTUP | KEY_BUTTDOWN))
      smenu.anim_state = 0;

    const t_mkeyupd_fn keyfns[] = {
#ifdef UI_BROWSER_V2
      keypress_menu_favorites,
#endif
      keypress_menu_recent,
      keypress_menu_browse,
      #ifdef SUPPORT_NORGAMES
      keypress_menu_norbrowse,
      #endif
      keypress_menu_settings,
      keypress_menu_uisettings,
#ifdef UI_BROWSER_V2
      keypress_menu_appearance,
#endif
      keypress_menu_tools,
      keypress_menu_info,
    };
    keyfns[smenu.menu_tab](newkeys);
  }
  menu_cover_request_selected();
}

