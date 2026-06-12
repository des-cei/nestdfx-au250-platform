# Developer Notes

This file collects implementation details, assumptions, known gotchas and
recommended fixes for maintainers of this template.

## Reconfiguration flow (static / A1)

- `alveo_reconfigure()` implements the static/A1-region flow used by this
  template. It asserts a small set of shell-to-A1 gate registers, programs
  HBICAP control words, streams the `.bin` into the HBICAP data address via
  the dedicated XDMA reconfiguration device, polls for completion, and then
  releases the gates.
- Gate addresses are defined in `alveo.h` (`ALVEO_DFX_SHUTDOWN_*` and
  `ALVEO_DFX_DECOUPLER_0_ADDR`). These registers are part of the stable
  shell interface and must match the hardware wrapper (`reconfig_base_inst.vhd`).

## HBICAP control mapping and `mmap()` alignment

- Current implementation maps a fixed `0x10000` control window at the
  `ALVEO_HBICAP_CTRL_ADDR` user offset and indexes words by fixed offsets.
  This works when the control base and offsets are page-aligned (current
  constants are page-aligned).
- Recommended robustness improvement: align the `mmap()` offset to the system
  page boundary and compute a pointer delta for non-page-aligned control bases.
  This avoids assumptions about control-base alignment if constants change.

Suggested mapping pattern (conceptual):

```c
long page = sysconf(_SC_PAGESIZE);
uint64_t user_offset = ALVEO_AXIL_USER_OFFSET(ALVEO_HBICAP_CTRL_ADDR);
uint64_t map_off = user_offset & ~(page - 1);
uint64_t delta = user_offset - map_off; /* pointer delta into mapped region */
void *map = mmap(NULL, map_size + delta, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)map_off);
volatile uint32_t *hbicap = (volatile uint32_t *)((unsigned char *)map + delta);
```

Notes:
- Use `map_size + delta` for the `mmap()` length to ensure the control range is fully mapped.
- Unmap using the original mapping pointer and total mapped size.

## XDMA transfer behavior

- `xdma_transfer()` uses repeated `read()`/`write()` calls limited by
  `ALVEO_RW_MAX_SIZE` and checks for short transfers and zero-length EOFs.
  If you observe short transfers, inspect kernel logs and device permissions
  first. Short transfers may indicate driver/firmware issues or resource
  constraints on the host.

## `posix_memalign` warning in `main.c`

- During a plain `make` the compiler may warn about an implicit declaration
  of `posix_memalign` on some toolchains because feature-test macros are not
  set before including headers.
- Fix options (choose one):
  - Add `#define _POSIX_C_SOURCE 200112L` at the top of `main.c` (before
    standard includes) so `posix_memalign` is exposed.
  - Use C11 `aligned_alloc()` where available (remember to free with `free()`),
    but `aligned_alloc` requires the allocation size to be a multiple of the
    alignment.
  - Keep `posix_memalign()` and add a small compatibility wrapper with a
    feature-check in a platform-specific manner.

Example quick fix (add at top of `main.c`):

```c
#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
```

## Error reporting and errno policy

- The helper `report_errno()` in `alveo.c` returns negative errno-style
  values and prints a short message to `stderr`. This is a simple and
  consistent policy for a small template but consider returning richer error
  contexts or logging to a structured log for larger projects.

## Tests and CI

- Recommended: add a CI job that runs `make` to verify build integrity on PRs.
- If you can include a licensable test bitstream, add a small runtime test
  that executes the `tutorial` (or a headless smoke test) in CI. If you
  cannot include a bitstream in the repo, provide a small test that checks
  compile and static analysis only.

## Files of interest (hardware repo cross-check)

- `reconfig_base_inst.vhd` — the stable shell-facing wrapper (hardware repo).
- `script/project_gen_reconfig.tcl`, `script/syn_reconfig.tcl` — hardware
  build flow scripts (hardware repo).