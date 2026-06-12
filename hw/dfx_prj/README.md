# Vivado DFX project generation

This folder contains the Vivado project generation step for the selected reconfigurable module.

The project-generation TCL is selected from:

```text
../accelerators/<ACCEL>/project_gen_reconfig.tcl
```

## How to run

From the hw root folder:

```bash
make ACCEL=timer_bram vivado_dfx_proj -j8
```

or:

```bash
make ACCEL=ddr_scale_hls vivado_dfx_proj -j8
```

From this folder:

```bash
make ACCEL=timer_bram
```

The `ACCEL` variable must match one of the folders under `accelerators/`.

## What it does

The flow runs the selected accelerator TCL and creates the Vivado project in:

```text
dfx_prj/reconfig_prj/
```

That generated project can be opened in Vivado to inspect or modify the reconfigurable hardware.

If you modify the block design in Vivado and want the change to be reproducible, export the updated TCL and place it back in the corresponding accelerator folder:

```text
accelerators/<ACCEL>/project_gen_reconfig.tcl
```

Do not replace the common DFX scripts unless you are changing the shared shell flow.
