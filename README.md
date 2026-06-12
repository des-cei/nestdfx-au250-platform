# Nested DFX Alveo U250 Platform

This repository contains the hardware and software components for a reconfigurable Alveo U250 lab platform based on a fixed DFX shell and user-defined A1 reconfigurable modules.

The repository is intentionally kept simple. The hardware and software sides are separated into two folders:

```text
.
├── hw/
└── sw/
```

A root Makefile is also provided to build the hardware and software sides from the top-level repository.

## Repository layout

- [hw/](hw/README.md): hardware DFX shell, static A0 shell, A1 reconfigurable module flow, accelerator examples and Vivado/TCL build scripts.
- [sw/](sw/README.md): C userspace runtime for XDMA, AXI-Lite and HBICAP, plus software examples matching the hardware accelerators.
- [Makefile](Makefile): top-level helper Makefile that delegates to `hw/` and `sw/`.

## Basic idea

The hardware side provides a reusable static shell and a set of reconfigurable module examples. Researchers can add new accelerators under the hardware flow while keeping the shell infrastructure stable.

The software side provides a small userspace runtime to:

- load A1 partial bitstreams through HBICAP,
- access AXI-Lite registers through XDMA,
- move data through full-AXI XDMA transfers.

The recommended convention is to use the same accelerator name on both sides.

## Quick start from the repository root

Build the selected hardware accelerator:

```bash
make hw ACCEL=timer_bram -j8
```

Build the matching software example:

```bash
make sw ACCEL=timer_bram
```

Run the software with the generated A1 partial bitstream:

```bash
cd sw
sudo ./timer_bram <a1_region_bitstream.bin>
```

For another accelerator, use the same name:

```bash
make hw ACCEL=my_accel -j8
make sw ACCEL=my_accel
```

By default, the top-level Makefile uses:

```text
ACCEL=timer_bram
JOBS=8
```

so running:

```bash
make
```

is equivalent to building the default hardware accelerator.

## Alternative: build from each folder

You can also enter each folder and use its own Makefile directly.

Hardware side:

```bash
cd hw
make ACCEL=timer_bram -j8
```

Software side:

```bash
cd ../sw
make ACCEL=timer_bram
```

Both approaches are equivalent. The root Makefile is only a convenience wrapper.

## Useful top-level targets

```bash
make hw ACCEL=<name>      # Build hardware for the selected accelerator
make sw ACCEL=<name>      # Build matching software example
make clean                # Clean hardware and software outputs
make hw_clean             # Clean only hardware outputs
make sw_clean             # Clean only software outputs
```

## Documentation

For detailed instructions, see the README files inside each folder:

- [Hardware README](hw/README.md)
- [Software README](sw/README.md)

The hardware README explains the DFX shell, accelerator structure, project generation and hardware contract.

The software README explains the userspace API, XDMA/HBICAP access paths, software examples and how to adapt the runtime to a new accelerator.
