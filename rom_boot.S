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

@ Wrapper/Bootloader for firmware.ewram
@ Wraps the EWRAM payload around a ROM (linked at 0x08000000) and boots it by
@ copying the payload into the EWRAM and executing it from there.

@ The layout is as follows:
@
@ The EWRAM payload is copied at 0x027C0000 (abusing ewram mirrors!).
@ Assets are copied at 0x09E00000 (last 1MB of ROM/SDRAM), compressed
@ Built-in patch DB is copied at 0x09D00000, uncompressed.
@ No IWRAM is initialized (that is the ewram-payload responsibility).

@ Incorporates NDS support, which similarly boots firmware.ewram but using
@ the ARM9 CPU. The ARM7 is left behind waiting for instructions to boot an
@ NDS file. In this mode only basic NDS boot functionality is used.

#define GBA_SWIDTH    240
#define GBA_SHEIGHT   160
#define NDS_SWIDTH    256
#define NDS_SHEIGHT   192

#define BOOT_LOGO_WIDTH      56
#define BOOT_LOGO_HEIGHT     42
#define BOOT_LOGO_ROW_BYTES  (BOOT_LOGO_WIDTH / 8)
#define BOOT_SPLASH_HEIGHT   (BOOT_LOGO_HEIGHT * 2)

#define NDS_WRAM_MIRROR  0x06000000     /* Temp WRAM storage area (using VRAM-D) */
#define BOOTMIRROR_ADDR  0x023FEC00     /* Bootloader relocation (NDS mode) */
#define BOOTMIRROR_ITCM  0x00000000     /* Bootloader relocation for the ARM9 */

#define BOOTNDS_SP7_SVC  0x023FFE00     /* From 0x023FFD80 to 0x023FFE00 */
#define BOOTNDS_SP7_SYS  0x023FFD80     /* From 0x023FFD00 to 0x023FFD80 */
#define BOOTNDS_SP9_SVC  0x023FFD00     /* From 0x023FFC80 to 0x023FFD00 */
#define BOOTNDS_SP9_IRQ  0x023FFC80     /* From 0x023FFC00 to 0x023FFC80 */
#define BOOTNDS_SP9_SYS  0x023FFC80     /* From 0x023FF800 to 0x023FFC00 */

#define FWPAYLOAD_ADDR   0x027C0000

#define NDS_HEADER_ADDR  0x027FFE00
#define WRAM_BOOTM_ADDR  0x03800000     /* WRAM were boot code is copied */
#define GAMEPAK_BASEADDR 0x08000000

#define FONT_BASE_ADDR   0x08E80000     @ Unpacked font glyph database (1.5MiB)
#define HISCRATCH_ADDR   0x09000000     @ Place here any temporary data
#define PATCH_DB_ADDR    0x09D00000     @ Where patch db lives
#define ASSETS_ADDR      0x09E00000     @ Where assets (emu/menu) live

#define REG_DISPCNT      0x04000000
#define REG_BG2CNT       0x0400000C
#define REG_BG2PA        0x04000020
#define REG_BG2PB        0x04000022
#define REG_BG2PC        0x04000024
#define REG_BG2PD        0x04000026
#define REG_BG2X         0x04000028
#define REG_BG2Y         0x0400002C

#define REG_WRAMCNT      0x04000247
#define REG_EXMEMCNT     0x04000204
#define REG_DISPCNT_A    0x04000000
#define REG_DISPCNT_B    0x04001000
#define REG_POWERCNT     0x04000304
#define REG_VRAMCNT_ABCD 0x04000240

#define REG_IPCSYNC      0x04000180

#define ROUND_UP2(x, a) ( (((x) + (a) - 1) & (~(a - 1))) )

@ Waits on IPCSYNC and sleeps a few microseconds every time before checking.
#define WAIT_SYNC(value, wcnt)  \
  1:                            \
    mov r0, $(wcnt);            \
    swi 0x00030000;             \
                                \
    ldr r0, =REG_IPCSYNC;       \
    ldr r1, [r0];               \
    and r1, $0xF;               \
    cmp r1, $(value);           \
    bne 1b

#define SET_SYNC(value)         \
  ldr r0, =REG_IPCSYNC;         \
  mov r1, $(value << 8);        \
  str r1, [r0];


@ This is mainly to allow for easy ROM manipulation without having to worry
@ about mappings having custom handlers in IWRAM.

.section    .text, "ax"
.globl _start
.globl _main_end
.globl _end_bootloader
.cpu        arm7tdmi

.arm

_start:
  b entry_point

  @ Nintendo Logo Character Data
  .byte 0x24,0xFF,0xAE,0x51,0x69,0x9A,0xA2,0x21,0x3D,0x84,0x82,0x0A,0x84,0xE4,0x09,0xAD
  .byte 0x11,0x24,0x8B,0x98,0xC0,0x81,0x7F,0x21,0xA3,0x52,0xBE,0x19,0x93,0x09,0xCE,0x20
  .byte 0x10,0x46,0x4A,0x4A,0xF8,0x27,0x31,0xEC,0x58,0xC7,0xE8,0x33,0x82,0xE3,0xCE,0xBF
  .byte 0x85,0xF4,0xDF,0x94,0xCE,0x4B,0x09,0xC1,0x94,0x56,0x8A,0xC0,0x13,0x72,0xA7,0xFC
  .byte 0x9F,0x84,0x4D,0x73,0xA3,0xCA,0x9A,0x61,0x58,0x97,0xA3,0x27,0xFC,0x03,0x98,0x76
  .byte 0x23,0x1D,0xC7,0x61,0x03,0x04,0xAE,0x56,0xBF,0x38,0x84,0x00,0x40,0xA7,0x0E,0xFD
  .byte 0xFF,0x52,0xFE,0x03,0x6F,0x95,0x30,0xF1,0x97,0xFB,0xC0,0x85,0x60,0xD6,0x80,0x25
  .byte 0xA9,0x63,0xBE,0x03,0x01,0x4E,0x38,0xE2,0xF9,0xA2,0x34,0xFF,0xBB,0x3E,0x03,0x44
  .byte 0x78,0x00,0x90,0xCB,0x88,0x11,0x3A,0x94,0x65,0xC0,0x7C,0x63,0x87,0xF0,0x3C,0xAF
  .byte 0xD6,0x25,0xE4,0x8B,0x38,0x0A,0xAC,0x72,0x21,0xD4,0xF8,0x07

  @ Game Title (80000A0h): SUPERR7
  .byte   0x53,0x55,0x50,0x45,0x52,0x52,0x37,0x00,0x00,0x00,0x00,0x00

  @ Game Code (80000ACh)
  .byte   0x50,0x41,0x53,0x53

  @ Maker Code (80000B0h)
  .byte   0x44,0x44

  @ Fixed Value (80000B2h)
  .byte   0x96

  @ Main Unit Code (80000B3h)
  .byte   0x00

  @ Device Type (80000B4h)
  .byte   0x00

  @ Unused Data (7Byte) (80000B5h)
  .byte   0x00,0x00,0x00,0x00,0x00,0x00,0x00

  @ Software Version No (80000BCh)
  .byte   0x5F

  @ Complement Check (80000BDh)
  .byte   0x00

  @ Checksum (80000BEh)
  .byte   0x00,0x00

entry_point:
  b main                       @ Must be valid code (for passme...)
  .word VERSION_WORD           @ Version (as a 32 bit word)
  .word VERSION_SLUG_WORD      @ Commit ID as 32 bits
  .word 0                      @ 32 bit firmware file size (patched)
  .ascii FW_FLAVOUR; .balign 4 @ Device variant
  .ascii "WRLD"                @ Firmware variant (no variants atm.)
  .word 0                      @ Unused
  .word 0                      @ Unused
  .fill 16, 1, 0               @ 16 byte hash (patched)
  @ Retained as the inherited firmware-format signature so SuperR7 remains
  @ compatible with the proven SuperFW update validator.
  .asciz "SUPERFW~DAVIDGF"

main:
  @ Basic GBA+NDS common HW initialization
  mov r1, $0x04000000
  strb r1, [r1, #0x208]        @ Disable interrupts!

  @ Copy the boot code to IWRAM so we can properly page flash and sdram as required.
  @ We copy 1KB first (since the mapping can be borked after that).
  @ On NDS it is copied to the ARM7-WRAM (on GBA we just abuse mirroring)
  @ Do not use pool loads in this first KiB!
  mov r0, $GAMEPAK_BASEADDR
  mov r1, $WRAM_BOOTM_ADDR
  mov r2, $(1024 / 4 / 4)
  1:
    ldmia r0!, {r4, r5, r6, r7}
    stmia r1!, {r4, r5, r6, r7}
    subs r2, $1
    bne 1b

  @ Resume execution at IWRAM, do the re-offset trick.
  mov r1, $(GAMEPAK_BASEADDR - WRAM_BOOTM_ADDR)
  sub pc, pc, r1
  nop; nop;

  @ When using bigger flashes, map the flash mirrored N times.
  #ifdef SUPERCHIS_IO
  mov r0, $0x0A000000
  mov r1, $0x5A
  orr r1, $0xA500
  mov r2, $8
  mov r3, $0x100
1:
    strh r1, [r0, #-2]
    strh r1, [r0, #-2]
    strh r3, [r0, #-2]
    subs r2, $1
    bne 1b
  #endif

  @ Figure out whether this payload is running on FW flash or on SDRAM.
  @ Place the mode (0x4 for flash or 0x5 for sdram) in r12
  mov r0, $GAMEPAK_BASEADDR
  mov r1, $(32 / 4 - 1)  @ Number of blocks to check
  mov r12, $0x4          @ Assume booting from flash
  ldmia r0, {r4-r7}

  1:
    add r0, $(4*1024*1024)   @ Use 4MiB blocks (compatible with bigger flash)
    ldmia r0, {r8-r11}   @ Compare the first 16 bytes of each block block.
    cmp   r4, r8
    cmpeq r5, r9
    cmpeq r6, r10
    cmpeq r7, r11
    movne r12, $0x5      @ Running from SDRAM, set flag, break loop.
    bne 2f

    subs r1, $1
    bne 1b
  2:

  @ Unconditionally set the guessed mode, to ensure the map is not bonkers
  @ (like in the Flash case when WE=0, I suspect a CPLD bug)
  mov r0, $0x0A000000
  mov r1, $0x5A
  orr r1, $0xA500
  strh r1, [r0, #-2]
  strh r1, [r0, #-2]
  strh r12, [r0, #-2]
  strh r12, [r0, #-2]

  @ Now proceed to copy the full bootloader into IWRAM, not just the first KB.
  mov r0, $GAMEPAK_BASEADDR
  mov r1, $WRAM_BOOTM_ADDR
  mov r2, $(2048 / 4 / 4)     @ Fixed 3KB bootloader size.
  @ Skip the already copied 1024 bytes
  add r0, $1024
  add r1, $1024
  1:
    ldmia r0!, {r4, r5, r6, r7}
    stmia r1!, {r4, r5, r6, r7}
    subs r2, $1
    bne 1b

_main_end:

  @ Check if we are running on GBA or NDS, since the startup changes significantly.
  @ EWRAM (at 0x02000000) is 256KiB on GBA but 4MB on DS, (ab)use mirroring.
  @ Check if mem[0x02000000] == mem[0x02040000].
  mov r0, $0x02000000
  add r1, r0, $(256*1024)
  ldr r2, =0xDEADBEEF
  ldr r3, =0xBADC0DE1
  str r2, [r0]           @ these two positions should be the same in GBA
  str r3, [r1]           @ on NDS these are two different memory positions tho.

  ldr r2, [r0]
  ldr r3, [r1]
  cmp r2, r3
  bne nds_boot_detected  @ They are different, therefore we have an NDS device


  @ Initialize the GBA screen and display the logo while we boot/unpack

  @ Disable GBA screen for now
  mov r8, $REG_DISPCNT
  mov r3, $0x80
  strh r3, [r8]
  @ Clear the screen to zero (background color)
  mov r0, $0x8
  swi 0x10000
  @ Copy palette
  adr r0, boot_logo_pal
  mov r1, $0x05000000
  ldr r2, [r0]
  str r2, [r1]
  @ Render the centered SuperR7 logo.
  adr r0, boot_logo
  ldr r1, =(0x06000000 + (GBA_SWIDTH-BOOT_LOGO_WIDTH*2)/2 + ((GBA_SHEIGHT-BOOT_SPLASH_HEIGHT)/2) * GBA_SWIDTH)
  mov r6, $GBA_SWIDTH
  bl draw_boot_logo
  mov r8, $REG_DISPCNT

  @ Setup BG2 regs, ensure it has some sensible state
  ldr r0, =REG_BG2CNT
  mov r1, $0      @ Base block, palette mode, etc
  strh r1, [r0]

  @ Setup bitmap offset regs
  mov r1, $0
  mov r2, $0x0100
  ldr r0, =REG_BG2PA; strh r2, [r0]
  ldr r0, =REG_BG2PB; strh r1, [r0]
  ldr r0, =REG_BG2PC; strh r1, [r0]
  ldr r0, =REG_BG2PD; strh r2, [r0]
  ldr r0, =REG_BG2X ; str r1, [r0]
  ldr r0, =REG_BG2Y ; str r1, [r0]

  @ Enable screen again. Bitmap 8bit mode
  ldr r1, =0x0404    // Mode 4, BG2 enabled
  strh r1, [r8]

  @ Uncompress/extract the font glyphs. This supports any size.
  ldr r0, =font_start
  #if defined(COMPRESS_FONTS)
    mov r2, $((font_end - font_start + 2047) / 2048)
    mov r1, $HISCRATCH_ADDR
    bl sdram_copy                      @ Copy compressed fonts to scratch area.
    mov r0, $0x5
    bl set_sd_mode                     @ Map SDRAM so we can unpack it.
    mov r0, $HISCRATCH_ADDR
    ldr r1, =FONT_BASE_ADDR
    bl apunpack16                      @ Unpack font assets
  #else
    mov r2, $(((font_end - font_start + 8191) / 8192) * 4)
    ldr r1, =FONT_BASE_ADDR
    bl sdram_copy                      @ Copy fonts directly (no unpacking)
  #endif

  @ Uncompress the patch database. The uncompressed payload must be < 256KiB!
  #if defined(COMPRESS_PATCHES)
    mov r0, r12
    bl set_sd_mode                     @ Switch to source map
    ldr r1, =patches_start
    mov r0, $0x02000000
    bl upkr_unpack                     @ Decompress to EWRAM
    add r4, r0, $3
    mov r0, $0x5
    bl set_sd_mode                     @ Switch to SDRAM map
    mov r0, $0x02000000
    mov r1, $PATCH_DB_ADDR
    lsr r2, r4, $2                     @ Number of words to copy
    swi 0x0C0000                       @ Copy decompressed data to SDRAM
  #else
    ldr r0, =patches_start
    mov r1, $PATCH_DB_ADDR
    mov r2, $((patches_end - patches_start + 2047) / 2048)
    bl sdram_copy                      @ Copy patches directly
  #endif

  @ Go ahead and copy assets to SDRAM locations.
  ldr r0, =assets_start
  mov r1, $ASSETS_ADDR
  mov r2, $((assets_end - assets_start + 2047) / 2048)
  bl sdram_copy

  @ Now we can proceed to unpack the firmware to EWRAM
  mov r0, r12
  bl set_sd_mode                       @ Switch to source map

  #if defined(COMPRESS_FIRMWARE)
    ldr r0, =FWPAYLOAD_ADDR
    ldr r1, =payload_ewram
    bl upkr_unpack
  #else
    ldr r0, =payload_ewram
    ldr r1, =FWPAYLOAD_ADDR
    mov r2, $(ROUND_UP2(((payload_ewram_end - payload_ewram) / 4), 256))
    swi 0x0C0000
  #endif

  @ Proceed to launch/boot the actual firmware!
  @ Reset to EWRAM using SoftReset
  mov r0, $0x04000000
  mov r1, $1
  strb r1, [r0, #-6]   @ Flag stored at 0x03007FFA
  swi 0x0              @ Reset to RAM

  .balign 4
boot_logo_expand:
  @ Two 1bpp source pixels expanded into four 8bpp Mode 4 pixels.
  .byte 0,0,0,0
  .byte 0,0,1,1
  .byte 1,1,0,0
  .byte 1,1,1,1

boot_logo:
  @ Artwork selected by Danny Nunez (dnunezx). This 1bpp mask is a compact adaptation
  @ of the supplied stacked Super R7 wordmark; it is rendered at 2x scale.
  .byte 0x00,0x00,0x0e,0x00,0x00,0x00,0x00
  .byte 0x00,0x00,0x7a,0x08,0x00,0x00,0x00
  .byte 0x00,0x03,0xc2,0x18,0x00,0x00,0x00
  .byte 0x00,0x0e,0x37,0x28,0x03,0xff,0x80
  .byte 0x00,0x19,0xf7,0xcf,0xbf,0x80,0xc0
  .byte 0x00,0x67,0xf5,0x8c,0xfc,0xff,0x60
  .byte 0x00,0xdf,0xe0,0x63,0x10,0xff,0xa0
  .byte 0x00,0xbf,0x6c,0xff,0xdf,0xbb,0xa0
  .byte 0x01,0x7c,0x4c,0xdf,0xfe,0x73,0xa0
  .byte 0x01,0x78,0x1c,0xdd,0xf8,0x77,0xa0
  .byte 0x03,0x7f,0x19,0xdf,0xfe,0x7f,0x60
  .byte 0x01,0x7f,0xd9,0xdf,0xfe,0x7e,0xc0
  .byte 0x01,0x8f,0xf9,0xfe,0x70,0xfe,0x80
  .byte 0x00,0xe3,0xfb,0xb8,0x77,0xee,0x40
  .byte 0x00,0x37,0xbf,0xb0,0x7e,0xe7,0x60
  .byte 0x00,0x4f,0x1f,0x30,0x78,0xc7,0xa0
  .byte 0x00,0x9e,0x0e,0x20,0xe1,0x83,0xb0
  .byte 0x01,0x3c,0x00,0x60,0x01,0x8b,0xd0
  .byte 0x06,0xf8,0xff,0xb7,0xfe,0xed,0xc8
  .byte 0x0d,0xe7,0x3f,0xfb,0xfe,0xee,0xe8
  .byte 0x1b,0xdd,0xbf,0xfb,0xff,0xdb,0x34
  .byte 0x36,0x30,0xbc,0x7b,0x87,0xd1,0x96
  .byte 0x61,0xc0,0xbc,0x3b,0x67,0xb0,0x62
  .byte 0x4f,0x00,0xb8,0x78,0xef,0xa0,0x1b
  .byte 0xf8,0x01,0xbf,0xfb,0xaf,0x40,0x07
  .byte 0x80,0x01,0x7f,0xf7,0x5e,0x40,0x01
  .byte 0x00,0x01,0x7f,0xcc,0xde,0x80,0x00
  .byte 0x00,0x01,0x7f,0xd8,0xbd,0x80,0x00
  .byte 0x00,0x01,0x73,0xe9,0x3d,0x00,0x00
  .byte 0x00,0x03,0x71,0xed,0x7b,0x00,0x00
  .byte 0x00,0x02,0xf4,0xf6,0x7a,0x00,0x00
  .byte 0x00,0x02,0xf6,0xfa,0xf6,0x00,0x00
  .byte 0x00,0x02,0xef,0x79,0xf4,0x00,0x00
  .byte 0x00,0x06,0xe9,0x3d,0xec,0x00,0x00
  .byte 0x00,0x06,0xd8,0x9c,0xe8,0x00,0x00
  .byte 0x00,0x05,0xb0,0x66,0xe8,0x00,0x00
  .byte 0x00,0x05,0x60,0x33,0x58,0x00,0x00
  .byte 0x00,0x04,0xc0,0x0c,0x50,0x00,0x00
  .byte 0x00,0x05,0x80,0x07,0x10,0x00,0x00
  .byte 0x00,0x0f,0x00,0x01,0x90,0x00,0x00
  .byte 0x00,0x0e,0x00,0x00,0x60,0x00,0x00
  .byte 0x00,0x0c,0x00,0x00,0x20,0x00,0x00
  .balign 4

boot_logo_pal:
  .hword 0x0000,0x7fff
  .balign 4
  @ Keep the later NDS-path ADR displacement ARM-immediate encodable after
  @ relocating this bootloader into shared WRAM/ITCM.
  .fill 8, 1, 0

@ Draws the 1bpp logo at 2x scale.
@ r0: logo source, r1: destination, r6: screen width in bytes
draw_boot_logo:
  adr r9, boot_logo_expand
  mov r3, $BOOT_LOGO_HEIGHT
  1:
    add r7, r1, r6
    mov r2, $BOOT_LOGO_ROW_BYTES
    2:
      ldrb r4, [r0], #1
      mov r5, $4
      3:
        and r10, r4, $0xc0
        ldr r10, [r9, r10, lsr #4]
        str r10, [r1], #4
        str r10, [r7], #4
        lsl r4, $2
        subs r5, $1
        bne 3b
      subs r2, $1
      bne 2b
    add r1, r6, lsl #1
    sub r1, $(BOOT_LOGO_WIDTH*2)
    subs r3, $1
    bne 1b

  bx lr
@ Copies data from the SDRAM/Flash to the desired SDRAM address
@ Uses IWRAM as a buffer (2KiB chunks)
@ r0: source address
@ r1: dest address
@ r2: number of 2KiB blocks to copy
sdram_copy:
  @ Go ahead and copy assets to SDRAM locations. We use IWRAM as tmp buffer.
  mov r4, r1
  mov r5, r2
  mov r6, r0
  ldr r8, =0x03004000   @ Use offset 16KiB-18KiB
  mov r9, lr
  1:
    mov r0, r12
    bl set_sd_mode           @ Map source (FW/SDRAM)

    @ Copy 2KiB to IWRAM.
    mov r0, r6               @ Source addr
    mov r1, r8               @ Dest addr
    mov r2, $(2048 / 4)      @ Word count
    swi 0x0C0000
    add r6, $2048

    mov r0, $0x5
    bl set_sd_mode           @ Map SDRAM to copy data to.

    @ Copy 2KiB from IWRAM.
    mov r0, r8               @ Source addr
    mov r1, r4               @ Dest addr
    mov r2, $(2048 / 4)      @ Word count
    swi 0x0C0000
    add r4, $2048

    subs r5, $1
    bne 1b

  @ Map source again (FW/SDRAM)
  mov r0, r12
  bl set_sd_mode
  bx r9

@ Sets the Supercard map mode
@ r0: mode
set_sd_mode:
  mov r1, $0x0A000000
  ldr r2, =0xA55A
  strh r2, [r1, #-2]
  strh r2, [r1, #-2]
  strh r0, [r1, #-2]
  strh r0, [r1, #-2]
  bx lr


@ NDS boot codepath
nds_boot_detected:
  ldr r0, =REG_IPCSYNC       @ Clear SYNC reg
  mov r1, $0
  str r1, [r0]

  @ We do not use the stack in this codepath, except for the BIOS calls
  msr cpsr, #0x13          @ SVC Mode
  ldr sp, =(BOOTNDS_SP7_SVC)
  msr cpsr, #0x1F          @ System Mode
  ldr sp, =(BOOTNDS_SP7_SYS)

  @ We are currently running on the ARM7 on ARM7-WRAM, prepare to enable ARM9.
  @ Copy the boot rom to main ram (only place where ARM9 can boot).
  mov r0, $GAMEPAK_BASEADDR
  ldr r1, =BOOTMIRROR_ADDR
  mov r2, $(((_end_bootloader - _start + 15) / 16) * 4)   @ Round it up to fit
  swi 0x0C0000

  @ Move ARM7 execution back to main RAM as well, since the payload will use WRAM.
  adr r0, arm7_bootaddr
  ldr r1, =(WRAM_BOOTM_ADDR - BOOTMIRROR_ADDR)
  sub r0, r0, r1
  bx r0

arm7_bootaddr:
  @ Unblock the ARM9 by writing the main-ram mapped address for arm9_bootaddr
  adr r0, arm9_bootaddr
  ldr r1, =0x027FFE24        @ Boot address, overwrite with any value
  str r0, [r1]

  mov r0, $0x4000            @ Wait a few ms before checking on the ARM9
  swi 0x00030000

  @ The arm9 should be up and running, we block here until we hear from it.
  WAIT_SYNC(0x5, 4096)       @ Slow wait, do not starve the ARM9

  SET_SYNC(0x6)              @ Reply with flag 0x6

  WAIT_SYNC(0x7, 512)        @ Waits for ARM9 to be mostly ready to launch

  @ Get ready to launch binary. All WRAM was mapped back to ARM7 at this point.

  @ Check the NDS header, see if we need to copy data to WRAM
  ldr r0, =NDS_HEADER_ADDR
  ldr r1, [r0, #56]            @ Load addr
  lsr r2, r1, $24              @ Get top address byte
  cmp r2, $3                   @ Check if it's in WRAM
  bne 1f

    ldr r2, [r0, #60]            @ Load size
    ldr r3, =NDS_WRAM_MIRROR     @ ARM9 placed the data here!
    2:
      ldm r3!, {r4-r11}
      stm r1!, {r4-r11}          @ Copy the data into the final addr
      subs r2, $32
      bgt 2b
  1:

  @ We are ready with the setup, signal so
  SET_SYNC(0xF)              @ Reply we are ready.

  @ Final wait before reset. Make it a tight loop to avoid missing it.
  WAIT_SYNC(0xF, 4)

  @ Reset now!
  swi 0x0


arm9_bootaddr:
  @ Here is where the arm9 execution begins! Running from main RAM.

  @ Disable interrupts
  mov r1, $0x04000000
  strb r1, [r1, #0x208]

  ldr r0, =REG_IPCSYNC         @ Clear SYNC reg
  mov r1, $0
  str r1, [r0]

  @ Setup ITCM as 16KB memory at 0x0
  mov r0, $0x0A
  mcr p15, 0, r0, c9, c1, 1
  @ Disable DTCM
  mov r0, $0
  mcr 15, 0, r0, cr9, cr1, 0

  @ Disable MMU and DTCM, enable ITCM
  ldr r0, =0x00042070
  mcr 15, 0, r0, cr1, cr0, 0

  // Setup IRQ, SVC and System stack pointers
  // We use stack, and the passme setup is quite inconsistent.
  msr cpsr, #0x13          @ SVC Mode
  ldr sp, =(BOOTNDS_SP9_SVC)
  msr cpsr, #0x12          @ IRQ Mode
  ldr sp, =(BOOTNDS_SP9_IRQ)
  msr cpsr, #0x1F          @ System Mode
  ldr sp, =(BOOTNDS_SP9_SYS)

  @ Copy and resume from ITCM
  ldr r0, =BOOTMIRROR_ADDR
  mov r1, $BOOTMIRROR_ITCM
  mov r2, $(((_end_bootloader - _start + 15) / 16) * 4)   @ Round it up to fit
  swi 0x0C0000

  adr r0, arm9_bootaddr_itcm
  ldr r1, =(BOOTMIRROR_ADDR - BOOTMIRROR_ITCM)
  sub r0, r0, r1
  bx r0

arm9_bootaddr_itcm:
  ldr r0, =REG_WRAMCNT     @ Ensure we map all the shared-WRAM to ARM9
  mov r1, $0
  strb r1, [r0]
  ldr r0, =REG_EXMEMCNT    @ Map slot2 to ARM9
  strh r1, [r0]

  ldr r0, =REG_DISPCNT_B   @ Disable screen B
  str r1, [r0]
  ldr r0, =REG_POWERCNT    @ Enable screens (power bits)
  ldr r1, =0x8003          @ Move splash to upper screen
  strh r1, [r0]

  @ Enable top LCD with the right bits/config to display the logo
  ldr r0, =REG_DISPCNT_A   @ Enable top screen (engine A)
  ldr r1, =0x00010405      @ Enable mode 5, enable BG2
  str r1, [r0]

  @ Setup BG2 mode, 8-bit indexed color mode
  ldr r0, =REG_BG2CNT
  ldr r1, =0x4080
  strh r1, [r0]

  @ Setup bitmap offset regs
  mov r1, $0
  mov r2, $0x0100
  ldr r0, =REG_BG2PA; strh r2, [r0]
  ldr r0, =REG_BG2PB; strh r1, [r0]
  ldr r0, =REG_BG2PC; strh r1, [r0]
  ldr r0, =REG_BG2PD; strh r2, [r0]
  ldr r0, =REG_BG2X ; str r1, [r0]
  ldr r0, =REG_BG2Y ; str r1, [r0]

  @ Enable VRAM-A and allow CPU access
  @ Enable VRAM-D and map it to the ARM9
  @ Disable other VRAM banks
  ldr r0, =REG_VRAMCNT_ABCD
  ldr r1, =0x80000081
  str r1, [r0]

  @ Init LCD logo with the logo on the top screen, using 8 bit bitmap mode.
  mov r0, $0x06000000
  mov r1, $(NDS_SWIDTH * NDS_SHEIGHT)    @ Clear the first 48KB or so.
  mov r2, $0
  1:
    str r2, [r0], #4; str r2, [r0], #4;
    str r2, [r0], #4; str r2, [r0], #4;
    subs r1, $16
    bne 1b
  @ Copy palette
  adr r0, boot_logo_pal
  mov r1, $0x05000000
  ldr r2, [r0]
  str r2, [r1]
  @ Draw logo
  adr r0, boot_logo
  ldr r1, =(0x06000000 + (NDS_SWIDTH-BOOT_LOGO_WIDTH*2)/2 + ((NDS_SHEIGHT-BOOT_SPLASH_HEIGHT)/2) * NDS_SWIDTH)
  mov r6, $NDS_SWIDTH
  bl draw_boot_logo

  @ In NDS mode, we only need the actual payload (includes DLDI).
  @ Only the base ASCII font is ever used (on error)

  @ Proceed to unpack the EWRAM payload to main-ram.
  #if defined(COMPRESS_FIRMWARE)
    ldr r0, =FWPAYLOAD_ADDR
    ldr r1, =payload_ewram
    bl upkr_unpack
  #else
    ldr r0, =payload_ewram
    ldr r1, =FWPAYLOAD_ADDR
    mov r2, $(ROUND_UP2(((payload_ewram_end - payload_ewram) / 4), 256))
    swi 0x0C0000
  #endif

  @ NDS soft-reset to run EWRAM payload
  ldr r0, =FWPAYLOAD_ADDR
  ldr r1, =0x027FFE24        @ Boot address, start of main ram
  str r0, [r1]
  swi 0x0                    @ Reset to RAM (ewram payload)


@ Include decompression routines here

#define _APPACK_HWORD_VERSION
#include "src/unpack.S"

#include "src/upkrunpack.S"

@ Place constants here.
.pool

_end_bootloader:

@ apack compressed assets!
payload_ewram:
  #ifndef FW_EWRAM_PAYLOAD
  #if defined(COMPRESS_FIRMWARE)
    #define FW_EWRAM_PAYLOAD "firmware.ewram.gba.comp"
  #else
    #define FW_EWRAM_PAYLOAD "firmware.ewram.gba"
  #endif
  #endif
  .incbin FW_EWRAM_PAYLOAD
  .balign 4
payload_ewram_end:

patches_start:
  #if defined(COMPRESS_PATCHES)
    .incbin "res/patches.db.comp"
  #else
    .incbin "res/patches.db"
  #endif
  .balign 4
patches_end:

font_start:
  #if defined(COMPRESS_FONTS)
    .incbin "res/fonts.pack.comp"
  #elif defined(FONTS_EXT)
    .incbin "res/fonts-ext.pack"
  #else
    .incbin "res/fonts.pack"
  #endif
  .balign 4
font_end:

assets_start:
  #ifdef BUNDLE_GBC_EMULATOR
  .ascii "GCEM"                                        @ GB/C emulator (GoombaColor)
  .word (goomba_emu_end - goomba_emu_start)            @ Payload size
  goomba_emu_start:
    .incbin "emu/jagoombacolor_v0.5.gba.comp"
  goomba_emu_end:
  .balign 4
  #endif

  #ifdef BUNDLE_OTHER_EMULATOR
  .ascii "PNES"
  .word (pocketnes_emu_end - pocketnes_emu_start)
  pocketnes_emu_start:
    .incbin "emu/pocketnes_20130701.gba.comp"
  pocketnes_emu_end:
  .balign 4

  .ascii "WASA"
  .word (wasabi_emu_end - wasabi_emu_start)
  wasabi_emu_start:
    .incbin "emu/wasabigba_v0.2.4.gba.comp"
  wasabi_emu_end:
  .balign 4

  .ascii "NGPG"
  .word (ngpgba_emu_end - ngpgba_emu_start)
  ngpgba_emu_start:
    .incbin "emu/NGPGBA_v0.5.7.gba.comp"
  ngpgba_emu_end:
  .balign 4

  .ascii "PCEA"
  .word (pceadv_emu_end - pceadv_emu_start)
  pceadv_emu_start:
    .incbin "emu/pceadvance-v7.5-scptch.gba.comp"
  pceadv_emu_end:
  .balign 4
  #endif

  .word 0               @ End of asset list
  .word 0
assets_end:

#if defined(SUPPORT_NORGAMES)
  // We place a mirror copy of the DirSave payload so it can be used by NOR games.
  .org 0x00190000
  .incbin "directsave.payload"

  // The IGM trampoline handles IRQs and switches to SDRAM-IGM
  .org 0x00198000
  .incbin "ingame_trampoline.payload"
#endif


