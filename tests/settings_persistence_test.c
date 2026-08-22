/* Copyright (C) 2026 Danny Nunez (dnunezx) */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "fatfs/ff.h"

static char ui_settings_file[512];
static UINT ui_settings_size;
static bool ui_settings_open;
static unsigned sanitize_calls;

uint32_t ui_theme_preset;
uint32_t ui_wallpaper;
uint32_t ui_background_color;
uint32_t ui_accent_color;
uint32_t ui_selection_color;
uint32_t ui_contrast;

unsigned lang_lookup(uint16_t code) {
  (void)code;
  return 0;
}

uint16_t lang_getcode(void) {
  return 'e' | ('n' << 8);
}

unsigned parseuint(const char *value) {
  return strtoul(value, NULL, 10);
}

void ui_theme_sanitize(void) {
  sanitize_calls++;
}

FRESULT f_mkdir(const TCHAR *path) {
  (void)path;
  return FR_OK;
}

FRESULT f_chmod(const TCHAR *path, BYTE attr, BYTE mask) {
  (void)path;
  (void)attr;
  (void)mask;
  return FR_OK;
}

FRESULT f_open(FIL *file, const TCHAR *path, BYTE mode) {
  (void)file;
  ui_settings_open = false;
  if (strcmp(path, UISETTINGS_FILEPATH))
    return FR_NO_FILE;
  if ((mode & FA_READ) && !ui_settings_size)
    return FR_NO_FILE;
  if (mode & FA_CREATE_ALWAYS)
    ui_settings_size = 0;
  ui_settings_open = true;
  return FR_OK;
}

FRESULT f_close(FIL *file) {
  (void)file;
  ui_settings_open = false;
  return FR_OK;
}

FRESULT f_write(FIL *file, const void *data, UINT size, UINT *written) {
  (void)file;
  assert(ui_settings_open);
  assert(size < sizeof(ui_settings_file));
  memcpy(ui_settings_file, data, size);
  ui_settings_file[size] = 0;
  ui_settings_size = size;
  *written = size;
  return FR_OK;
}

FRESULT f_read(FIL *file, void *data, UINT size, UINT *read) {
  (void)file;
  assert(ui_settings_open);
  *read = ui_settings_size < size ? ui_settings_size : size;
  memcpy(data, ui_settings_file, *read);
  return FR_OK;
}

int main(void) {
  browser_sort_descending = 1;
  browser_game_filter = BrowserGameGBC;
  browser_hide_folders = 1;
  browser_hide_unknown = 1;

  assert(save_ui_settings());
  assert(strstr(ui_settings_file, "browse_sort_descending=1\n"));
  assert(strstr(ui_settings_file, "browse_game_filter=4\n"));
  assert(strstr(ui_settings_file, "browse_hide_folders=1\n"));
  assert(strstr(ui_settings_file, "browse_hide_unknown=1\n"));

  browser_sort_descending = 0;
  browser_game_filter = BrowserGameAllFiles;
  browser_hide_folders = 0;
  browser_hide_unknown = 0;
  load_settings();

  assert(browser_sort_descending == 1);
  assert(browser_game_filter == BrowserGameGBC);
  assert(browser_hide_folders == 1);
  assert(browser_hide_unknown == 1);
  assert(sanitize_calls == 1);
  return 0;
}
