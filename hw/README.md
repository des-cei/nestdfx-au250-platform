# Alveo XDMA + HBICAP — Reconfigurable HW Template

This folder provides the hardware half of a reconfigurable Alveo U250 design. It combines a fixed static shell with a user-defined A1 reconfigurable region, and the Makefiles/TCL scripts automate project generation, synthesis, implementation and bitstream/bin creation.

It is intended as a simple lab platform where researchers can create new reconfigurable modules while reusing the same static shell infrastructure.

## Key features

* Pre-built static shell DCP at [implementation_A0/top_static/top_A0_locked.dcp](implementation_A0/top_static/top_A0_locked.dcp).
* Reconfigurable module examples under [accelerators/](accelerators/README.md).
* Accelerator selection with `make ACCEL=<name>`.
* Automated synthesis and implementation flow driven by the root [Makefile](Makefile).
* Example shell and reference-region address maps and block diagrams in [images/](images/README.md).
* Designed to pair with the companion software template that talks to XDMA, AXI-Lite and HBICAP.

## Repository layout

* [accelerators/](accelerators/README.md): reconfigurable module examples and instructions for creating new accelerators.
* [implementation_A0/](implementation_A0/README.md): static shell and locked DCP.
* [dfx_prj/](dfx_prj/README.md): Vivado project generation flow.
* [scripts/](scripts/README.md): common TCL scripts for synthesis, implementation and bitstream generation.
* [source/](source/README.md): fixed wrapper and constraint sources for the user design.
* [images/](images/README.md): shell and reference reconfigurable-region diagrams plus address maps.

## Quick start

Open the repo in a Linux environment with Vivado installed and build one of the available accelerators:

```bash
make ACCEL=timer_bram -j8
```

This runs the project-generation, synthesis, implementation and bitstream-generation flow for the selected reconfigurable module.

Available accelerators are placed under:

```text
accelerators/
```

Each accelerator provides its own Vivado project-generation TCL:

```text
accelerators/<ACCEL>/project_gen_reconfig.tcl
```

For details on the provided examples, how to create a new accelerator, and how to adapt a Vivado-exported TCL script to this DFX flow, see [accelerators/README.md](accelerators/README.md).

## Hardware contract

The shell expects a stable interface boundary with AXI-Lite control, AXI-full DMA/data, a master port and interrupts.

The wrapper that defines this boundary is:

```text
source/hdl/reconfig_base_inst.vhd
```

The shell address windows are predefined, so software and hardware must use the same map. Start from the diagrams in [images/](images/README.md) and keep those ranges fixed unless you are intentionally redesigning the shell itself.

## Companion software

There is a separate software template for the userspace side at [../sw/README.md](../sw/README.md).

The recommended convention is to use matching names in both sides:

```bash
# Hardware folder
make ACCEL=my_accel

# Software folder
make ACCEL=my_accel
```
