# `timer_bram` accelerator example

This is the basic reference reconfigurable module for the A1 region.

It contains a simple design with:

- AXI Timer on the AXI-Lite control path.
- AXI BRAM Controller on the full-AXI data path.

It is useful as a first sanity check because it validates the main runtime paths used by the shell:

- HBICAP partial reconfiguration.
- AXI-Lite register access through XDMA.
- Full-AXI XDMA write/read transfers.

## Files

- `project_gen_reconfig.tcl`: Vivado project-generation script for this accelerator.

## Build

From the repository root:

```bash
make ACCEL=timer_bram -j8
```

This generates the Vivado project, synthesizes the RM, implements it against the static shell and produces the partial bitstream/bin.

## Software

The matching software example is expected to be built from the companion software repository with:

```bash
make APP=timer_bram
```

The software should use the timer and BRAM addresses assigned in this reconfigurable design.
