/*
 * Copyright (C) 2026 Danny Nunez (dnunezx)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 */

#ifdef UI_BROWSER_V2

#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "gbahw.h"
#include "fonts/font_render.h"
#include "ui_browser_v2.h"
#include "ui_theme.h"

static const uint8_t dock_icons[4][8] = {
  {0x18, 0x18, 0xFF, 0x7E, 0x3C, 0x7E, 0x66, 0x00},
  {0x3C, 0x42, 0x91, 0x91, 0x89, 0x42, 0x3C, 0x00},
  {0x70, 0xFC, 0x82, 0x82, 0x82, 0x82, 0xFE, 0x00},
  {0xC3, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0xC3},
};

static const char *const dock_labels[4] = {
  "FAVORITE", "RECENT", "BROWSE", "TOOLS",
};

/* Compact dock-only font. Each row uses the low five bits. */
static const uint8_t dock_font[26][7] = {
  {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* A */
  {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, /* B */
  {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, /* C */
  {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, /* D */
  {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, /* E */
  {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, /* F */
  {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, /* G */
  {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* H */
  {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* I */
  {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}, /* J */
  {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, /* K */
  {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, /* L */
  {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, /* M */
  {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11}, /* N */
  {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* O */
  {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, /* P */
  {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, /* Q */
  {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, /* R */
  {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, /* S */
  {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* T */
  {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* U */
  {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, /* V */
  {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, /* W */
  {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, /* X */
  {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, /* Y */
  {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, /* Z */
};

static const uint8_t dock_number_font[11][7] = {
  {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, /* 0 */
  {0x0C, 0x14, 0x04, 0x04, 0x04, 0x04, 0x1F}, /* 1 */
  {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, /* 2 */
  {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}, /* 3 */
  {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, /* 4 */
  {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}, /* 5 */
  {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}, /* 6 */
  {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, /* 7 */
  {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, /* 8 */
  {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}, /* 9 */
  {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}, /* / */
};

_Static_assert(COVER_PALETTE_BASE == 20,
               "phase 2 UI palette must end before cover colors");
_Static_assert(UI_BROWSER_V2_COVER_LEFT % 2 == 0,
               "cover rows must be halfword aligned");
_Static_assert(UI_BROWSER_V2_COVER_LEFT + COVER_WIDTH <= 82,
               "cover exceeds its frame");

static void set_pixel(volatile uint8_t *frame, unsigned x, unsigned y,
                      uint8_t color) {
  volatile uint16_t *pixel = (volatile uint16_t *)&frame[y * SCREEN_WIDTH + (x & ~1u)];
  uint16_t old = *pixel;
  *pixel = x & 1 ? (old & 0x00FF) | ((uint16_t)color << 8) :
                   (old & 0xFF00) | color;
}

static void fill_rect(volatile uint8_t *frame, unsigned left, unsigned top,
                      unsigned width, unsigned height, uint8_t color) {
  for (unsigned y = top; y < top + height; y++) {
    unsigned x = left;
    unsigned count = width;
    if (x & 1) {
      set_pixel(frame, x++, y, color);
      count--;
    }
    if (count >= 2)
      dma_memset16(&frame[y * SCREEN_WIDTH + x], dup8(color), count / 2);
    if (count & 1)
      set_pixel(frame, x + count - 1, y, color);
  }
}

static void draw_rect_outline(volatile uint8_t *frame, unsigned left,
                              unsigned top, unsigned width, unsigned height,
                              uint8_t color) {
  for (unsigned x = left; x < left + width; x++) {
    set_pixel(frame, x, top, color);
    set_pixel(frame, x, top + height - 1, color);
  }
  for (unsigned y = top + 1; y < top + height - 1; y++) {
    set_pixel(frame, left, y, color);
    set_pixel(frame, left + width - 1, y, color);
  }
}

static void draw_box(volatile uint8_t *frame, unsigned left, unsigned top,
                     unsigned width, unsigned height, uint8_t fill,
                     uint8_t edge, uint8_t shadow, bool glow) {
  if (glow) {
    draw_rect_outline(frame, left - 2, top - 2, width + 4, height + 4,
                      shadow);
    draw_rect_outline(frame, left - 1, top - 1, width + 2, height + 2,
                      edge);
  }
  fill_rect(frame, left + 2, top + 2, width, height, shadow);
  fill_rect(frame, left, top, width, height, edge);
  fill_rect(frame, left + 2, top + 2, width - 4, height - 4, fill);
}

static void draw_tech_frame(volatile uint8_t *frame, unsigned height) {
  unsigned bottom = height - 3;
  fill_rect(frame, 1, 18, 1, 34, UiV2StripeLight);
  fill_rect(frame, 1, 61, 1, 24, UiV2Stripe);
  fill_rect(frame, 1, 99, 1, 27, UiV2StripeLight);
  fill_rect(frame, 238, 12, 1, 42, UiV2Stripe);
  fill_rect(frame, 238, 69, 1, 47, UiV2StripeLight);
  fill_rect(frame, 0, 18, 3, 2, UiV2AccentDark);
  fill_rect(frame, 237, 69, 3, 2, UiV2AccentDark);
  fill_rect(frame, 4, 3, 82, 1, UiV2StripeLight);
  fill_rect(frame, 85, 3, 1, 9, UiV2StripeLight);
  fill_rect(frame, 85, 11, 42, 1, UiV2Stripe);
  fill_rect(frame, 152, 3, 62, 1, UiV2Stripe);
  fill_rect(frame, 214, 3, 1, 13, UiV2Stripe);
  fill_rect(frame, 214, 15, 20, 1, UiV2StripeLight);
  fill_rect(frame, 5, 10, 1, 35, UiV2Stripe);
  fill_rect(frame, 5, 44, 13, 1, UiV2Stripe);
  fill_rect(frame, 17, 44, 1, 18, UiV2StripeLight);
  fill_rect(frame, 222, 24, 1, 32, UiV2Stripe);
  fill_rect(frame, 222, 55, 12, 1, UiV2StripeLight);
  fill_rect(frame, 233, 55, 1, 27, UiV2StripeLight);
  fill_rect(frame, 6, bottom - 12, 1, 10, UiV2StripeLight);
  fill_rect(frame, 6, bottom - 12, 18, 1, UiV2StripeLight);
  fill_rect(frame, 23, bottom - 12, 1, 7, UiV2Stripe);
  fill_rect(frame, 23, bottom - 6, 58, 1, UiV2Stripe);
  fill_rect(frame, 81, bottom - 6, 1, 4, UiV2Stripe);
  fill_rect(frame, 81, bottom - 3, 50, 1, UiV2StripeLight);
  fill_rect(frame, 151, bottom - 3, 45, 1, UiV2Stripe);
  fill_rect(frame, 195, bottom - 10, 1, 8, UiV2Stripe);
  fill_rect(frame, 195, bottom - 10, 38, 1, UiV2StripeLight);
  fill_rect(frame, 232, bottom - 25, 1, 16, UiV2StripeLight);
  for (unsigned x = 11; x < 75; x += 9)
    fill_rect(frame, x, 7, 4, 2, UiV2AccentDark);
  for (unsigned x = 163; x < 224; x += 10)
    fill_rect(frame, x, bottom - 7, 4, 2, UiV2AccentDark);
  set_pixel(frame, 17, 62, UiV2AccentDark);
  set_pixel(frame, 233, 82, UiV2AccentDark);
}

static void draw_background(volatile uint8_t *frame) {
  fill_rect(frame, 0, 0, SCREEN_WIDTH, UI_BROWSER_V2_DOCK_TOP,
            UiV2Background);
  switch (ui_wallpaper) {
  case UiWallpaperWeave:
    for (unsigned y = 1; y < UI_BROWSER_V2_DOCK_TOP; y += 4)
      fill_rect(frame, 0, y, SCREEN_WIDTH, 1, UiV2Stripe);
    for (unsigned x = 2; x < SCREEN_WIDTH; x += 8)
      for (unsigned y = 3; y < UI_BROWSER_V2_DOCK_TOP; y += 8)
        set_pixel(frame, x, y, UiV2StripeLight);
    break;
  case UiWallpaperGrid:
    for (unsigned y = 0; y < UI_BROWSER_V2_DOCK_TOP; y += 8)
      fill_rect(frame, 0, y, SCREEN_WIDTH, 1, UiV2Stripe);
    for (unsigned x = 0; x < SCREEN_WIDTH; x += 8)
      fill_rect(frame, x, 0, 1, UI_BROWSER_V2_DOCK_TOP, UiV2Stripe);
    break;
  case UiWallpaperCircuit:
    for (unsigned y = 6; y < UI_BROWSER_V2_DOCK_TOP; y += 16) {
      unsigned bend = (y * 5) % 37 + 12;
      fill_rect(frame, 0, y, bend, 1, UiV2Stripe);
      fill_rect(frame, bend, y, 1, 7, UiV2Stripe);
      fill_rect(frame, bend, y + 6, 30, 1, UiV2StripeLight);
      set_pixel(frame, bend + 30, y + 6, UiV2AccentDark);
    }
    break;
  case UiWallpaperTechFrame:
    draw_tech_frame(frame, UI_BROWSER_V2_DOCK_TOP);
    break;
  default:
    break;
  }
}

static void draw_text_clipped(const char *text, volatile uint8_t *frame,
                              unsigned x, unsigned y, unsigned max_width,
                              uint8_t color) {
  if (!text)
    return;
  if (font_width(text) <= max_width) {
    draw_text_idx8_bus16(text, (uint8_t *)&frame[y * SCREEN_WIDTH + x],
                         SCREEN_WIDTH, color);
    return;
  }

  char clipped[128];
  unsigned count = font_width_cap(text, max_width - 12);
  if (count >= sizeof(clipped) - 4)
    count = sizeof(clipped) - 4;
  memcpy(clipped, text, count);
  memcpy(&clipped[count], "...", 4);
  draw_text_idx8_bus16(clipped, (uint8_t *)&frame[y * SCREEN_WIDTH + x],
                       SCREEN_WIDTH, color);
}

static void draw_text_selected(const char *text, volatile uint8_t *frame,
                               unsigned x, unsigned y, unsigned max_width,
                               uint8_t color, unsigned anim_state) {
  if (!text)
    return;
  if (font_width(text) <= max_width) {
    draw_text_idx8_bus16(text, (uint8_t *)&frame[y * SCREEN_WIDTH + x],
                         SCREEN_WIDTH, color);
    return;
  }

  char scrolling[540];
  unsigned anim = anim_state > 128 ? (anim_state - 128) >> 4 : 0;
  strcpy(scrolling, text);
  strcat(scrolling, "      ");
  unsigned loop_width = font_width(scrolling);
  if (anim > loop_width)
    anim %= loop_width;
  strcat(scrolling, text);
  draw_text_idx8_bus16_range(scrolling,
                             (uint8_t *)&frame[y * SCREEN_WIDTH + x],
                             anim, max_width, SCREEN_WIDTH, color);
}

static void draw_row(volatile uint8_t *frame,
                     const t_ui_browser_v2_model *model, unsigned i,
                     unsigned left, unsigned top, unsigned width,
                     unsigned height) {
    bool selected = i == model->selected_row;
    draw_box(frame, left, top, width, height,
             selected ? UiV2SelectedCard : UiV2Card,
             selected ? UiV2GlowEdge : UiV2CardEdge,
             selected ? UiV2GlowShadow : UiV2CardShadow, selected);
    uint8_t text_color = (model->entries[i].hidden ||
                          model->entries[i].kind == UiBrowserV2Unsupported) ?
                           UiV2Disabled :
                         (model->entries[i].kind == UiBrowserV2Folder ||
                          model->entries[i].kind == UiBrowserV2Parent) ?
                           UiV2Folder : UiV2Text;
    unsigned text_left = left + 6;
    unsigned text_width = width - 12;
    if (model->entries[i].value) {
      unsigned value_width = font_width(model->entries[i].value);
      unsigned value_left = left + width - 6 - MIN(value_width, width / 2);
      draw_text_clipped(model->entries[i].value, frame, value_left, top + 1,
                        left + width - 6 - value_left,
                        selected ? UiV2White : UiV2Muted);
      text_width = value_left > text_left + 8 ? value_left - text_left - 8 : 1;
    }
    if (model->entries[i].centered) {
      unsigned name_width = MIN(font_width(model->entries[i].name),
                                width - 12);
      unsigned name_left = left + (width - name_width) / 2;
      draw_text_clipped(model->entries[i].name, frame, name_left, top + 1,
                        name_width, selected ? UiV2White : text_color);
    } else if (selected)
      draw_text_selected(model->entries[i].name, frame, text_left, top + 1,
                         text_width,
                         text_color, model->anim_state);
    else
      draw_text_clipped(model->entries[i].name, frame, text_left, top + 1,
                        text_width,
                        text_color);
}

static void draw_rows(volatile uint8_t *frame,
                      const t_ui_browser_v2_model *model) {
  if (model->layout == UiBrowserV2LayoutLaunch) {
    static const uint8_t tops[] = {4, 27, 55, 83, 111};
    static const uint8_t heights[] = {17, 23, 23, 23, 23};
    draw_row(frame, model, 0, 3, tops[0], 234, heights[0]);
    for (unsigned i = 1; i < model->entry_count && i < 5; i++)
      draw_row(frame, model, i, 85, tops[i], 152, heights[i]);
    return;
  }

  const unsigned left = model->wide ? 3 : 85;
  const unsigned width = model->wide ? 234 : 152;
  for (unsigned i = 0; i < model->entry_count && i < UI_BROWSER_V2_ROWS; i++)
    draw_row(frame, model, i, left, model->row_top + 2 + i * 20,
             width, 17);
}

static void draw_centered_cover_message(volatile uint8_t *frame,
                                        const char *message, uint8_t color) {
  unsigned width = font_width(message);
  unsigned x = 42 - MIN(width, 68) / 2;
  draw_text_clipped(message, frame, x, 62, 68, color);
}

static unsigned dock_text_width(const char *text);
static void draw_dock_text(volatile uint8_t *frame, const char *text,
                           unsigned x, unsigned y, uint8_t color);

static unsigned page_pill_inset(unsigned edge) {
  return edge == 0 ? 6 : edge == 1 ? 3 : edge == 2 ? 2 :
         edge == 3 ? 1 : 0;
}

static void draw_page_pill(volatile uint8_t *frame, const char *page_text) {
  if (!page_text)
    return;

  unsigned text_width = dock_text_width(page_text);
  const unsigned width = MIN(74, MAX(31, text_width + 14));
  const unsigned left = 42 - width / 2;
  const unsigned top = 121;
  const unsigned height = 15;
  for (unsigned row = 0; row < height; row++) {
    unsigned edge = MIN(row, height - row - 1);
    unsigned inset = page_pill_inset(edge);
    unsigned span = width - inset * 2;
    uint8_t color = (row == 0 || row == height - 1) ? UiV2GlowEdge :
                                                     UiV2BackgroundDeep;
    fill_rect(frame, left + inset, top + row, span, 1, color);
    if (row && row + 1 < height) {
      unsigned outer_inset = page_pill_inset(edge - 1);
      for (unsigned connector = inset; connector <= outer_inset; connector++) {
        set_pixel(frame, left + connector, top + row, UiV2GlowEdge);
        set_pixel(frame, left + width - connector - 1, top + row,
                  UiV2GlowEdge);
      }
    }
  }

  draw_dock_text(frame, page_text, left + (width - text_width) / 2, top + 4,
                 UiV2White);
}

static void draw_cover(volatile uint8_t *frame,
                       const t_ui_browser_v2_model *model) {
  draw_box(frame, 3, 32, UI_BROWSER_V2_COVER_FRAME_SIZE,
           UI_BROWSER_V2_COVER_FRAME_SIZE, UiV2SelectedCard, UiV2GlowEdge,
           UiV2GlowShadow, true);

  if (model->cover_state == CoverReady && model->cover_pixels) {
    for (unsigned row = 0; row < COVER_HEIGHT; row++)
      dma_memcpy16(&frame[(UI_BROWSER_V2_COVER_TOP + row) * SCREEN_WIDTH +
                          UI_BROWSER_V2_COVER_LEFT],
                   &model->cover_pixels[row * COVER_WIDTH], COVER_WIDTH / 2);
    return;
  }

  fill_rect(frame, UI_BROWSER_V2_COVER_LEFT, UI_BROWSER_V2_COVER_TOP,
            COVER_WIDTH, COVER_HEIGHT, UiV2BackgroundDeep);

  uint8_t kind = model->entry_count ? model->entries[model->selected_row].kind :
                                      UiBrowserV2Unsupported;
  if (kind == UiBrowserV2Folder || kind == UiBrowserV2Parent) {
    fill_rect(frame, 20, 58, 44, 32, UiV2Folder);
    fill_rect(frame, 24, 54, 18, 6, UiV2Folder);
    fill_rect(frame, 24, 64, 36, 20, UiV2FolderInset);
    fill_rect(frame, 26, 66, 32, 16, UiV2Folder);
    return;
  }

  const char *message = model->cover_state == CoverPending ? "LOADING" :
                        model->cover_state == CoverMissing ? "NO COVER" :
                        model->cover_state == CoverInvalid ? "INVALID" :
                        model->cover_state == CoverIoError ? "SD ERROR" : "";
  uint8_t color = (model->cover_state == CoverInvalid ||
                   model->cover_state == CoverIoError) ? UiV2Danger : UiV2Muted;
  if (*message)
    draw_centered_cover_message(frame, message, color);
}

static void draw_dock_icon(volatile uint8_t *frame, unsigned icon,
                           unsigned x, unsigned y, uint8_t color) {
  for (unsigned row = 0; row < 8; row++)
    for (unsigned col = 0; col < 8; col++)
      if (dock_icons[icon][row] & (0x80 >> col))
        set_pixel(frame, x + col, y + row, color);
}

static unsigned dock_text_width(const char *text) {
  unsigned length = strlen(text);
  return length ? length * 6 - 1 : 0;
}

static void draw_dock_text(volatile uint8_t *frame, const char *text,
                           unsigned x, unsigned y, uint8_t color) {
  while (*text) {
    const uint8_t *glyph = NULL;
    if (*text >= 'A' && *text <= 'Z')
      glyph = dock_font[*text - 'A'];
    else if (*text >= '0' && *text <= '9')
      glyph = dock_number_font[*text - '0'];
    else if (*text == '/')
      glyph = dock_number_font[10];
    if (glyph)
      for (unsigned row = 0; row < 7; row++)
        for (unsigned col = 0; col < 5; col++)
          if (glyph[row] & (0x10 >> col))
            set_pixel(frame, x + col, y + row, color);
    x += 6;
    text++;
  }
}

static void draw_dock(volatile uint8_t *frame, unsigned selected) {
  fill_rect(frame, 0, UI_BROWSER_V2_DOCK_TOP, SCREEN_WIDTH,
            SCREEN_HEIGHT - UI_BROWSER_V2_DOCK_TOP, UiV2BackgroundDeep);
  fill_rect(frame, 0, UI_BROWSER_V2_DOCK_TOP, SCREEN_WIDTH, 1, UiV2Accent);

  for (unsigned i = 0; i < 4; i++) {
    unsigned left = i * 60;
    bool active = i == selected;
    if (active) {
      fill_rect(frame, left + 2, UI_BROWSER_V2_DOCK_TOP + 1, 56, 14,
                UiV2AccentDark);
      fill_rect(frame, left + 2, UI_BROWSER_V2_DOCK_TOP + 1, 56, 1,
                UiV2GlowEdge);
      fill_rect(frame, left + 2, UI_BROWSER_V2_DOCK_TOP + 14, 56, 1,
                UiV2GlowEdge);
    }
    if (i)
      fill_rect(frame, left, UI_BROWSER_V2_DOCK_TOP + 2, 2, 12,
                UiV2StripeLight);
    uint8_t color = active ? UiV2White : UiV2DockText;
    unsigned group_width = 10 + dock_text_width(dock_labels[i]);
    unsigned group_left = left + (60 - group_width) / 2;
    draw_dock_icon(frame, i, group_left, UI_BROWSER_V2_DOCK_TOP + 4, color);
    draw_dock_text(frame, dock_labels[i], group_left + 10,
                   UI_BROWSER_V2_DOCK_TOP + 4, color);
  }
}

static void draw_footer(volatile uint8_t *frame, const char *left,
                        const char *right) {
  fill_rect(frame, 0, UI_BROWSER_V2_DOCK_TOP, SCREEN_WIDTH,
            SCREEN_HEIGHT - UI_BROWSER_V2_DOCK_TOP, UiV2BackgroundDeep);
  fill_rect(frame, 0, UI_BROWSER_V2_DOCK_TOP, SCREEN_WIDTH, 1, UiV2Accent);
  if (left)
    draw_text_clipped(left, frame, 6, UI_BROWSER_V2_DOCK_TOP + 3,
                      SCREEN_WIDTH / 2 - 6, UiV2White);
  if (right) {
    unsigned width = MIN(font_width(right), SCREEN_WIDTH / 2 - 6);
    draw_text_clipped(right, frame, SCREEN_WIDTH - 6 - width,
                      UI_BROWSER_V2_DOCK_TOP + 3, width, UiV2White);
  }
}

void ui_browser_v2_load_palette(volatile uint16_t *palette) {
  ui_theme_apply_palette(palette);
}

void ui_browser_v2_render(volatile uint8_t *frame,
                          const t_ui_browser_v2_model *model) {
  draw_background(frame);
  if (model->show_cover) {
    draw_cover(frame, model);
    draw_page_pill(frame, model->page_text);
  }
  draw_rows(frame, model);
  if (!model->entry_count) {
    if (model->show_cover)
      draw_centered_cover_message(frame, "EMPTY", UiV2Muted);
    else {
      const char *empty = "EMPTY";
      draw_text_clipped(empty, frame, (SCREEN_WIDTH - font_width(empty)) / 2,
                        65, 80, UiV2Muted);
    }
  }
  if (model->hide_dock)
    draw_footer(frame, model->footer_left, model->footer_right);
  else
    draw_dock(frame, MIN(model->selected_dock, 3));
}

void ui_browser_v2_render_loading(volatile uint8_t *frame, const char *title,
                                  const uint8_t *cover_pixels,
                                  uint8_t cover_state,
                                  unsigned done, unsigned total) {
  while (total > 0xFFFF) {
    done >>= 1;
    total >>= 1;
  }
  unsigned percent = total ? MIN(100, done * 100 / total) : 0;
  char percent_text[5];
  if (percent == 100) {
    strcpy(percent_text, "100%");
  } else {
    percent_text[0] = percent >= 10 ? '0' + percent / 10 : ' ';
    percent_text[1] = '0' + percent % 10;
    percent_text[2] = '%';
    percent_text[3] = 0;
  }

  t_ui_browser_v2_model model;
  memset(&model, 0, sizeof(model));
  model.cover_pixels = cover_pixels;
  model.cover_state = cover_state;
  model.entry_count = 1;
  model.entries[0].kind = UiBrowserV2Game;
  model.entries[0].name = title;
  model.entries[0].centered = true;

  draw_background(frame);
  draw_row(frame, &model, 0, 3, 4, 234, 17);
  draw_cover(frame, &model);

  const unsigned right_center = 161;
  unsigned width = MIN(font_width("LOADING GAME"), 140);
  draw_text_clipped("LOADING GAME", frame, right_center - width / 2, 38,
                    width, UiV2Text);
  width = font_width(percent_text);
  draw_text_clipped(percent_text, frame, right_center - width / 2, 55,
                    width, UiV2White);

  draw_box(frame, 88, 73, 146, 18, UiV2Card, UiV2GlowEdge,
           UiV2GlowShadow, true);
  unsigned fill = 140 * percent / 100;
  if (fill)
    fill_rect(frame, 91, 76, fill, 12, UiV2Accent);
  if (fill && fill < 140)
    fill_rect(frame, 90 + fill, 76, 2, 12, UiV2White);

  width = MIN(font_width("PLEASE WAIT"), 140);
  draw_text_clipped("PLEASE WAIT", frame, right_center - width / 2, 101,
                    width, UiV2Muted);

  draw_footer(frame, "LOADING", NULL);
}

#endif
