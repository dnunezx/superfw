# SuperR7 hardware validation

This record identifies the exact firmware images that passed physical
SuperCard SD testing. SHA-256 is the authority when similarly named local
files exist.

Generated images and test captures live in the ignored local `artifacts/`
workspace. Public downloads belong on versioned GitHub Releases, not in the
source tree.

## Accepted lineage

| Date | State | Local image | Size | SHA-256 |
| --- | --- | --- | ---: | --- |
| 2026-09-13 | Current: Luna 1.2 RC1 Chis; hardware-tested on SuperChis | `SuperR7-Chis-Luna1.2-RC1.gba` | 1,671,680 | `791D6FB76F05BC0CD7EF3B67EFCAABB6AB5B8AB9A164240CE0D7EE071B70D3D0` |
| 2026-08-22 | Candidate: Luna 1.1 SD fingerprint; exact chain-load pending | `SuperR7-SD-Luna1.1-hardware-test.gba` | Pending final tagged build | Pending final tagged build |
| 2026-08-22 | Current: Browse controls, list resets, and clean logo-to-UI handoff | `superr7-logo-to-ui-no-white-flash-hardware-test.gba` | 521,728 | `E9AA581CB3D8401D3663646F1D72183B674C753CE0266C692A50BA056D853204` |
| 2026-08-22 | Rollback: connected dynamic Browse page pill | `superr7-connected-page-pill-test.gba` | 520,704 | `136C0726E4758460BBE1E7988F4647F8429946F839B0E556A472FC0230482C36` |
| 2026-08-21 | Rollback: dynamic Browse page pill and held paging | `superr7-dynamic-page-pill-hardware-test.gba` | 520,704 | `0A64F0A7A528B5469B83EC25CA35795608D022ACC344E849E8B39ED9F565CCB2` |
| 2026-08-14 | Rollback: fixed-page library navigation | `superr7-page-navigation-hardware-test.gba` | 520,192 | `DCD599CD17745FB350A7176C24257377AB9B301E6C5B661B25594E3D53E5C940` |
| 2026-08-13 | Rollback: Tech Frame wallpaper | `superr7.gba` | 520,192 | `63EE181F90C0FCACB0014D6F819B6994C26E2677A589C9DCC355718C9F1FAA4F` |
| 2026-08-12 | Rollback: stacked boot logo | `superr7-boot-logo-v2.gba` | 519,168 | `6DCDEA075A8CF04C8A4FF523F20628D5FF74AFF7126E3234F59A7B9E93A26BFC` |
| 2026-08-11 | Rollback: Launch Back footer | `superr7-phase13-launch-back-8ca8aaf2.gba` | 519,168 | `8CA8AAF27941BAE9A1DF35D3C3E88C863CEF51B4AA5C35F1F25D780F0C808A2F` |
| 2026-08-10 | Rollback: persistent Favorites | `superr7-phase11-favorites-cbdae08b.gba` | 520,704 | `CBDAE08B529566E37517776CA336B1FF070AEF4296ED09503E961577972C68B3` |
| 2026-08-07 | Historical: Gothic boot logo | `superr7-phase9-gothic-boot-1eec14f.gba` | 518,144 | `93F774D81C6DEF17125587A66B121A8CF126D87256F7F547389ACD482F49A1E5` |
| 2026-08-06 | Historical: initial SuperR7 branding | `superr7-initial-branded.gba` | 518,144 | `72FCA4B89E329B9D2A4E21D5E4BB6C083E997A214C2BB0AE8373FCC0367AF61B` |
| 2026-08-06 | Historical: pre-branding Phase 5 baseline | `superr7-phase5-baseline.gba` | 520,192 | `15A88B4F0F25B057ED4B93B4B0D855E7F3CFE67C0E7D0B7ADBA01261A6667A92` |

The latest confirmed SuperChis image is Luna 1.2 RC1. The exact image above
passed hardware testing, including the SuperChis-specific release gate, and
its matching `.gba` and `.fw` files are byte-identical. Its recorded size and
SHA-256 identify the approved release candidate.

The latest confirmed SuperCard SD image preserves fixed seven-item pages,
accelerated held Browse paging, and the connected dynamic page pill. It adds
Browse-only sorting and GBA/GB/GBC filtering, confirmed Reset Favorites and
Reset Recent tools, and a VBlank handoff that keeps the boot logo visible until
the first complete UI framebuffer is ready. The user confirmed that this exact
image works great on physical SuperCard SD hardware. Its GBA header checksum,
embedded size, and firmware digest are valid, and it remains 2,560 bytes below
the 512 KiB limit.

The Luna 1.1 SD source adds the visible `Luna 1.1 SD` System Information build
fingerprint and persists all four Browse View & Sort choices across reboot.
Its popup advertises and accepts A as the value-change control; L and R remain
reserved for navigation outside it. These changes produce a new binary, so the
final tagged image must complete one exact-hash chain-load before it replaces
the current image in the accepted lineage or is attached to a public release.

The three oldest historical packages remain tracked under
[`releases/archive`](../releases/README.md). New public binaries should be
attached to GitHub Releases instead of committed to this repository.

## Release gates

Before publishing a firmware image for either board:

1. Build `superr7.gba` from the intended clean source tag with `BOARD=sd` or
   `BOARD=chis` as appropriate.
2. Verify the embedded flavour and confirm the final image remains below its
   board limit: 512 KiB for SD or 2 MiB for Chis.
3. Run the host and native mGBA regression suites.
4. Chain-load that exact image on matching physical hardware. For Chis, also
   complete the [SuperChis NOR and save checklist](superchis-hardware-test.md).
5. Record its byte size and SHA-256 here.
6. Publish it as `SuperR7-SD-LunaX.Y.gba` or
   `SuperR7-Chis-LunaX.Y.gba` with the matching `LunaX.Y` source tag.

Internal phase names may remain in development history, but public downloads
use normal semantic release versions.
