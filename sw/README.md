# Alveo XDMA + HBICAP — Minimal Reconfigurable SW Template

This folder provides a compact C userspace template for working with
Alveo boards that use XDMA for data movement and HBICAP for static/A1-region
reconfiguration. It is a companion software template for the hardware side. The goal is to offer a small, easy-to-adapt software starting
point that demonstrates the common patterns you will need to control and
validate your own reconfigurable hardware design.

## Key features

* Small helper library implementing:

  * `alveo_reconfigure(const char *bitstream_path)` — stream a raw `.bin`
    bitstream into HBICAP (static/A1 flow) and wait for completion.
  * `alveo_reg_read32()` / `alveo_reg_write32()` — simple AXI-Lite register
    access through the XDMA user BAR.
  * `alveo_write()` / `alveo_read()` — full-AXI XDMA H2C/C2H bulk transfers.
* Example applications under `accelerators/`, each targeting a specific
  reconfigurable module.
* Simple application selection using `make ACCEL=<name>`.

## Repository layout

* [alveo.h](alveo.h): configuration macros and public API.
* [alveo.c](alveo.c): implementation of register, DMA and reconfiguration
  helpers.
* [accelerators/](accelerators/): example programs for specific reconfigurable
  modules.
* [Makefile](Makefile): build rules for `libalveo.a` and the selected accelerator.

## Quick start

1. Prepare an Alveo Linux system with the XDMA kernel driver and user
   character devices (`/dev/xdma0_*`). Ensure your user has permission to
   access those device nodes (udev rules or root).

2. List the available accelerator:

```bash
make list_accelerators
```

3. Build one accelerators:

```bash
make ACCEL=timer_bram
```

This produces `libalveo.a` and an executable with the selected accelerator name.

4. Run the accelerator:

```bash
./timer_bram <a1_region_bitstream.bin>
```


**Notes**

* `<a1_region_bitstream.bin>` must be a raw `.bin` file where size is a
  multiple of 4 bytes (the loader checks this). The accelerator program
  performs a validation sequence but does not validate the full contents of
  the bitstream beyond a successful HBICAP completion.
* The selected software accelerator must match the reconfigurable module loaded
  into A1.

## How the code maps to hardware paths

* AXI-Lite register accesses use the XDMA user BAR: `/dev/xdma0_user`.
  The API expects Vivado AXI-Lite addresses (for example `0x42000000`) and
  internally translates them to an `mmap()` offset using `ALVEO_AXIL_BASE_ADDR`.
* Full-AXI bulk transfers use the XDMA H2C/C2H character devices, for example
  `/dev/xdma0_h2c_0` and `/dev/xdma0_c2h_0`. For these calls pass the Vivado
  full-AXI address directly (no translation).

## Build and run accelerator

```bash
make ACCEL=timer_bram
sudo ./timer_bram partial_region.bin
```

## Adapting this template to your design

* Create a new example under [accelerators](accelerators), or copy an existing one:

```bash
cp accelerators/timer_bram.c accelerators/my_accel.c
```

* Change the user-design-specific addresses near the top of the file. For
  example, a timer/BRAM design may use:

```c
#define TIMER_BASE_ADDR  0x42000000ULL
#define BRAM_BASE_ADDR   0x80000000ULL
```

* Build the new accelerator with:

```bash
make ACCEL=my_accel
```

The recommended convention is to use the same name as in the hardware
folder:

```bash
# Hardware folder
make ACCEL=my_accel

# Software folder
make ACCEL=my_accel
```

## Hardware contract (shell)

This software folder is intended to be used with a fixed, pre-built
Alveo shell that defines the stable wrapper boundary and address map ([check the hw](../hw/README.md)). Keep
these rules in mind when you adapt the software to a new reconfigurable
design:

* The shell is the source of truth for device address windows, register
  offsets, DMA windows and IRQ wiring. Do not invent new offsets — use the
  shell-provided address map and diagrams.
* The reconfigurable region (A1) may change contents (timer/BRAM locations,
  accelerator control registers, DDR buffers, etc.), but the shell-facing
  wrapper boundary must remain compatible. The shell-facing wrapper in the
  hardware repo is `reconfig_base_inst.vhd` and represents the contract
  between shell and user logic. Treat it as the canonical interface.
* The hardware build flow is driven by the Vivado TCL scripts in the related
  hardware repository. The selected reconfigurable module is chosen with
  `make ACCEL=<name>` in the hardware repository.
* Software must use the address map and IRQ map exported by the hardware
  repository (block diagrams, CSV maps, or generated header files). When the
  user modifies their reconfigurable logic, they must update the matching
  software example.

## Example integration workflow

1. User creates or selects a reconfigurable module in the hardware repository.
2. Run synthesis/implementation to produce a partial bitstream/bin (in the hw side):

```bash
make ACCEL=<name>
```

3. Build the matching software example (in this sw side):

```bash
make ACCEL=<name>
```

4. Run the software example with the generated `.bin`:

```bash
sudo ./<name> <partial.bin>
```

Note: the accelerators use sample addresses for demonstration and may need to be
updated to match addresses chosen inside the user's reconfigurable region. The
shell's address map is authoritative.

## API summary (what to call from your application)

* `int alveo_reconfigure(const char *bitstream_path)`

  * Streams the `.bin` into HBICAP and waits for completion. Returns 0 on
    success or a negative errno-style value on failure.
* `int alveo_reg_write32(uint64_t axil_addr, uint32_t value)` /
  `int alveo_reg_read32(uint64_t axil_addr, uint32_t *value)`

  * Simple read/write wrappers for single 32-bit AXI-Lite registers. Pass
    Vivado AXI-Lite addresses; the library handles the mmap translation.
* `ssize_t alveo_write(uint64_t hw_addr, const void *buffer, uint64_t size)` /
  `ssize_t alveo_read(uint64_t hw_addr, void *buffer, uint64_t size)`

  * Bulk transfers over the full AXI XDMA path. `hw_addr` is the Vivado
    full-AXI address and is used directly as the XDMA offset.

## Troubleshooting

* Device files not present: ensure the XDMA kernel module is loaded and the
  Alveo drivers are installed on your system.
* Reconfiguration times out: verify the HBICAP control/data addresses in
  [alveo.h](alveo.h) and inspect `dmesg`/system logs for HBICAP or driver
  errors.
* AXI-Lite or XDMA access fails: check that the software example matches the
  loaded reconfigurable module and that the addresses are correct.
