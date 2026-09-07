# SuperChis hardware test

Use this sequence for a board-labelled SuperR7 Chis candidate. The `.gba` and
`.fw` files in a candidate package are byte-identical; the different extension
exists to keep chain-loading and internal flashing explicit.

## Prepare and recover

1. Confirm the cart is SuperChis. In the existing firmware's System
   Information screen, the hardware flavour should be `Chis`.
2. Back up the current internal firmware and copy that backup off the SD card.
   Keep a known recovery method available before changing internal flash.
3. On the computer, verify the candidate `.gba` against the package's SHA-256
   file. Do not test any file whose hash or board label differs.
4. Copy only the `.gba` test candidate to the SD card initially. Keep the `.fw`
   file off the card until chain-load testing is complete.

## Chain-load first

Launch the `.gba` from the known-working firmware. Do not flash it yet. Record
pass/fail and any visible error for each item:

- System Information shows `Luna 1.2 RC1 Chis`.
- Cold boot, menu navigation, SD browsing, covers, Favorites, Recent, View &
  Sort, and settings persistence work.
- A known-good GBA game launches; its normal save survives a reboot.
- Direct Saving and the in-game menu work on games already known to support
  them.
- One known-good GB/GBC game and any bundled emulator you use launch normally.

## Exercise SuperChis NOR safely

Do not format NOR during the first test. Back up anything irreplaceable first.

1. Open the NOR game list and confirm existing entries are displayed.
2. Write one small, known-good GBA game with NOR verification enabled.
3. Launch it from NOR, create or load a save, and return to the menu.
4. Confirm the NOR launch appears in Recent and still launches after a cold
   boot.
5. Delete only the test entry and confirm the remaining NOR list is intact.

Stop immediately if the cart identifies as the wrong flavour, NOR contents
change unexpectedly, verification fails, saves disappear, or the chain-loaded
menu is unstable. Preserve the exact error and do not flash.

## Internal flash gate

Only after every required chain-load test passes for the exact recorded
SHA-256 should the matching `.fw` be copied to the card. Reconfirm that its
hash matches the `.gba`, use the firmware's normal update flow, then cold boot
and repeat the identity, SD, save, and NOR launch checks. Record the tested
size, SHA-256, cart revision, and result in
[`hardware-validation.md`](hardware-validation.md).
