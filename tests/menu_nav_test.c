/* Copyright (C) 2026 Danny Nunez (dnunezx) */

#include <assert.h>
#include <string.h>

#include "menu_nav.h"
#include "superr7.h"

static bool flush_ok(void) {
  return true;
}

static bool flush_fail(void) {
  return false;
}

static void move(int *selector, int *seloff, int count,
                 int item_delta, int page_delta) {
  menu_list_navigate(selector, seloff, count, 7, item_delta, page_delta);
}

int main(void) {
  assert(strcmp(SUPERR7_BUILD_FINGERPRINT, "Luna 1.1 SD") == 0);

  int selector = 0;
  int seloff = 0;

  /* A seven-entry list has no next page, so paging is a no-op. */
  move(&selector, &seloff, 7, 0, 1);
  assert(selector == 0);
  assert(seloff == 0);

  /* The final page may be partial and begins on a fixed boundary. */
  selector = 0;
  seloff = 0;
  move(&selector, &seloff, 8, 0, 1);
  assert(selector == 7);
  assert(seloff == 7);

  /* Item movement crosses fixed page boundaries in either direction. */
  selector = 6;
  seloff = 0;
  move(&selector, &seloff, 15, 1, 0);
  assert(selector == 7);
  assert(seloff == 7);
  move(&selector, &seloff, 15, -1, 0);
  assert(selector == 6);
  assert(seloff == 0);

  /* Page movement selects the first item on the adjacent fixed page. */
  selector = 3;
  seloff = 0;
  move(&selector, &seloff, 15, 0, 1);
  assert(selector == 7);
  assert(seloff == 7);

  /* The final partial page is stable at both navigation boundaries. */
  selector = 7;
  seloff = 7;
  move(&selector, &seloff, 15, 0, 1);
  assert(selector == 14);
  assert(seloff == 14);
  move(&selector, &seloff, 15, 0, 1);
  assert(selector == 14);
  assert(seloff == 14);
  move(&selector, &seloff, 15, 0, -1);
  assert(selector == 7);
  assert(seloff == 7);

  selector = 0;
  seloff = 0;
  move(&selector, &seloff, 15, 0, -1);
  assert(selector == 0);
  assert(seloff == 0);

  /* Page movement takes priority over a simultaneous item delta. */
  move(&selector, &seloff, 15, 1, 1);
  assert(selector == 7);
  assert(seloff == 7);

  /* Existing selections normalize to their fixed page without moving. */
  selector = 14;
  seloff = 8;
  move(&selector, &seloff, 15, 0, 0);
  assert(selector == 14);
  assert(seloff == 14);

  /* Empty lists always have a safe zero position. */
  selector = 12;
  seloff = 9;
  move(&selector, &seloff, 0, 0, 1);
  assert(selector == 0);
  assert(seloff == 0);

  /* Page labels derive from the same fixed seven-row boundaries. */
  assert(menu_page_count(0, 7) == 0);
  assert(menu_page_number(0, 0, 7) == 0);
  assert(menu_page_count(7, 7) == 1);
  assert(menu_page_count(8, 7) == 2);
  assert(menu_page_number(14, 126, 7) == 3);
  assert(menu_page_count(126, 7) == 18);

  /* A hold waits, repeats, accelerates, and rearms on release/reversal. */
  t_menu_page_repeat repeat = {0};
  assert(menu_page_repeat_step(&repeat, 1, 1000) == 0);
  assert(menu_page_repeat_step(&repeat, 1, 1399) == 0);
  assert(menu_page_repeat_step(&repeat, 1, 1400) == 1);
  assert(menu_page_repeat_step(&repeat, 1, 1532) == 0);
  assert(menu_page_repeat_step(&repeat, 1, 1533) == 1);
  assert(menu_page_repeat_step(&repeat, 1, 2300) == 1);
  assert(menu_page_repeat_step(&repeat, 1, 2366) == 0);
  assert(menu_page_repeat_step(&repeat, 1, 2367) == 1);
  assert(menu_page_repeat_step(&repeat, 0, 2400) == 0);
  assert(menu_page_repeat_step(&repeat, -1, 2500) == 0);
  assert(menu_page_repeat_step(&repeat, -1, 2900) == -1);
  assert(menu_page_repeat_step(&repeat, 1, 3000) == 0);
  assert(menu_page_repeat_step(&repeat, 1, 3400) == 1);

  /* A successful reset commits zero state; a failed write rolls it back. */
  selector = 9;
  seloff = 7;
  int maxentries = 12;
  assert(menu_list_reset_flush(&selector, &seloff, &maxentries, flush_ok));
  assert(selector == 0);
  assert(seloff == 0);
  assert(maxentries == 0);

  selector = 9;
  seloff = 7;
  maxentries = 12;
  assert(!menu_list_reset_flush(&selector, &seloff, &maxentries, flush_fail));
  assert(selector == 9);
  assert(seloff == 7);
  assert(maxentries == 12);

  /* Browse ordering reverses without changing directory grouping. */
  assert(menu_browser_sort_compare(-7, false) == -7);
  assert(menu_browser_sort_compare(-7, true) == 7);
  assert(menu_browser_sort_compare(0, true) == 0);

  /* Game family, folder, and unknown-file filters remain independent. */
  assert(menu_browser_entry_visible(true, 0, false, 0, false, true));
  assert(!menu_browser_entry_visible(true, 0, false, 0, true, false));
  assert(menu_browser_entry_visible(false, 1, true, 0, false, true));
  assert(menu_browser_entry_visible(false, 2, true, 1, false, true));
  assert(menu_browser_entry_visible(false, 2, true, 3, false, true));
  assert(!menu_browser_entry_visible(false, 2, true, 2, false, false));
  assert(menu_browser_entry_visible(false, 0, true, 0, false, true));
  assert(!menu_browser_entry_visible(false, 0, false, 0, false, true));
  assert(menu_browser_entry_visible(false, 0, false, 0, false, false));
  assert(!menu_browser_entry_visible(false, 0, true, 1, false, false));
  assert(menu_browser_entry_visible(false, 0, false, 1, false, false));
  assert(!menu_browser_entry_visible(false, 0, false, 1, false, true));

  return 0;
}
