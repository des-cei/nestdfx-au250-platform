# TCL hardware flow

This folder contains the common TCL scripts used by the automated DFX hardware flow.

Accelerator-specific Vivado project-generation TCL files are not kept here. They are located under:

```text
accelerators/<ACCEL>/project_gen_reconfig.tcl
```

For details on how those accelerator project-generation scripts should be written or adapted from a Vivado export, see:

```text
accelerators/README.md
```

## Files

- [`generate_all_bitstreams.tcl`](generate_all_bitstreams.tcl): helper script for bitstream generation.
- [`implement_A1_level1.tcl`](implement_A1_level1.tcl): implementation TCL for the A1 reconfigurable design.
- [`syn_reconfig.tcl`](syn_reconfig.tcl): synthesis orchestration for the selected reconfigurable module.

## General workflow

The recommended workflow is:

1. Select or create an accelerator under `accelerators/<ACCEL>/`.
2. Make sure that accelerator provides its own `project_gen_reconfig.tcl`.
3. Run the flow from the repository root:

```bash
make ACCEL=<ACCEL> -j8
```

The scripts in this folder are shared by all accelerators. In normal use, researchers should modify files inside `accelerators/<ACCEL>/`, not the common flow scripts.

## Practical rules

- Keep this folder for shared flow scripts only.
- Put accelerator-specific Vivado project generation in `accelerators/<ACCEL>/project_gen_reconfig.tcl`.
- Do not modify the common scripts unless you are changing the shared DFX flow for all accelerators.
- See [accelerators/README.md](../accelerators/README.md) for the DFX-specific edits required after exporting a Vivado TCL.
