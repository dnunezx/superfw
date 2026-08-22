/*
 * Copyright (C) 2026 Danny Nunez (dnunezx)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 */

#ifndef _UI_BROWSER_V2_H__
#define _UI_BROWSER_V2_H__

#include <stdint.h>

#include "cover.h"

#define UI_BROWSER_V2_ROWS 7
#define UI_BROWSER_V2_DOCK_TOP 144
#ifdef COVER_ART_V3
#define UI_BROWSER_V2_COVER_LEFT 4
#define UI_BROWSER_V2_COVER_TOP 33
#define UI_BROWSER_V2_COVER_FRAME_SIZE 78
#else
#define UI_BROWSER_V2_COVER_LEFT 6
#define UI_BROWSER_V2_COVER_TOP 35
#define UI_BROWSER_V2_COVER_FRAME_SIZE 77
#endif

typedef enum {
  UiBrowserV2Game = 0,
  UiBrowserV2Folder,
  UiBrowserV2Parent,
  UiBrowserV2Save,
  UiBrowserV2Firmware,
  UiBrowserV2Other,
  UiBrowserV2Unsupported,
} t_ui_browser_v2_entry_kind;

typedef struct {
  const char *name;
  const char *value;
  uint8_t kind;
  uint8_t hidden;
  uint8_t centered;
} t_ui_browser_v2_entry;

typedef enum {
  UiBrowserV2LayoutRows = 0,
  UiBrowserV2LayoutLaunch,
} t_ui_browser_v2_layout;

typedef struct {
  t_ui_browser_v2_entry entries[UI_BROWSER_V2_ROWS];
  const uint8_t *cover_pixels;
  const char *footer_left;
  const char *footer_right;
  const char *page_text;
  uint8_t entry_count;
  uint8_t selected_row;
  uint8_t selected_dock;
  uint8_t cover_state;
  uint8_t row_top;
  uint8_t show_cover;
  uint8_t wide;
  uint8_t hide_dock;
  uint8_t layout;
  uint32_t anim_state;
} t_ui_browser_v2_model;

void ui_browser_v2_load_palette(volatile uint16_t *palette);
void ui_browser_v2_render(volatile uint8_t *frame,
                          const t_ui_browser_v2_model *model);
void ui_browser_v2_render_loading(volatile uint8_t *frame, const char *title,
                                  const uint8_t *cover_pixels,
                                  uint8_t cover_state,
                                  unsigned done, unsigned total);

#endif
