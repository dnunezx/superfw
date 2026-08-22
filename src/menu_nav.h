/*
 * Copyright (C) 2026 Danny Nunez (dnunezx)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 */

#ifndef _MENU_NAV_H__
#define _MENU_NAV_H__

#include <stdbool.h>
#include <stdint.h>

#define MENU_PAGE_REPEAT_DELAY_MS       400
#define MENU_PAGE_REPEAT_FAST_AFTER_MS  1300
#define MENU_PAGE_REPEAT_SLOW_MS        133
#define MENU_PAGE_REPEAT_FAST_MS         67

typedef struct {
  int direction;
  uint32_t started_at;
  uint32_t next_at;
} t_menu_page_repeat;

static inline int menu_nav_clamp(int value, int low, int high) {
  return value < low ? low : value > high ? high : value;
}

static inline unsigned menu_page_count(int maxentries, int rows) {
  return maxentries > 0 && rows > 0 ?
         (unsigned)(maxentries + rows - 1) / (unsigned)rows : 0;
}

static inline unsigned menu_page_number(int selector, int maxentries,
                                        int rows) {
  if (maxentries <= 0 || rows <= 0)
    return 0;
  selector = menu_nav_clamp(selector, 0, maxentries - 1);
  return (unsigned)selector / (unsigned)rows + 1;
}

/*
 * Arm on a new direction, then return one page step at each repeat deadline.
 * Rescheduling from now avoids a burst of catch-up steps after blocking I/O.
 */
static inline int menu_page_repeat_step(t_menu_page_repeat *repeat,
                                        int direction, uint32_t now) {
  if (direction != -1 && direction != 1) {
    repeat->direction = 0;
    return 0;
  }

  if (direction != repeat->direction) {
    repeat->direction = direction;
    repeat->started_at = now;
    repeat->next_at = now + MENU_PAGE_REPEAT_DELAY_MS;
    return 0;
  }

  if ((int32_t)(now - repeat->next_at) < 0)
    return 0;

  uint32_t held_for = now - repeat->started_at;
  repeat->next_at = now +
                    (held_for >= MENU_PAGE_REPEAT_FAST_AFTER_MS ?
                     MENU_PAGE_REPEAT_FAST_MS : MENU_PAGE_REPEAT_SLOW_MS);
  return direction;
}

/*
 * Keep list selection on fixed pages. Item movement crosses page boundaries,
 * while page movement selects the first item of the previous or next page.
 * The final page may contain fewer than rows entries.
 */
static inline void menu_list_navigate(int *selector, int *seloff,
                                      int maxentries, int rows,
                                      int item_delta, int page_delta) {
  if (maxentries <= 0 || rows <= 0) {
    *selector = 0;
    *seloff = 0;
    return;
  }

  *selector = menu_nav_clamp(*selector, 0, maxentries - 1);

  if (page_delta) {
    int page_count = menu_page_count(maxentries, rows);
    int current_page = *selector / rows;
    int target_page = menu_nav_clamp(current_page + page_delta,
                                     0, page_count - 1);
    if (target_page != current_page)
      *selector = target_page * rows;
  } else {
    *selector = menu_nav_clamp(*selector + item_delta, 0, maxentries - 1);
  }

  *seloff = (*selector / rows) * rows;
}

/*
 * Persist an empty list before committing its cleared UI state. If the write
 * fails, restore the previous position and count so the menu stays truthful.
 */
static inline bool menu_list_reset_flush(int *selector, int *seloff,
                                         int *maxentries,
                                         bool (*flush)(void)) {
  int old_selector = *selector;
  int old_seloff = *seloff;
  int old_maxentries = *maxentries;

  *selector = 0;
  *seloff = 0;
  *maxentries = 0;
  if (flush())
    return true;

  *selector = old_selector;
  *seloff = old_seloff;
  *maxentries = old_maxentries;
  return false;
}

static inline int menu_browser_sort_compare(int comparison,
                                            bool descending) {
  return descending ? -comparison : comparison;
}

/* Game families use 1=GBA, 2=GB, and 3=GBC. */
static inline bool menu_browser_entry_visible(bool is_directory,
                                              unsigned game_family,
                                              bool recognized,
                                              unsigned game_filter,
                                              bool hide_folders,
                                              bool hide_unknown) {
  if (is_directory)
    return !hide_folders;
  if (game_family)
    return !game_filter || game_filter == 1 ||
           game_filter == game_family + 1;
  if (!game_filter)
    return recognized || !hide_unknown;
  return !recognized && !hide_unknown;
}

#endif
