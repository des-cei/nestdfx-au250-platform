# `ddr_scale_hls` accelerator example

This is an HLS-based reconfigurable module for the A1 region.

The accelerator implements a simple DDR scaler. The host writes input data to DDR through XDMA, the HLS accelerator reads it through its `m_axi` interface, scales the data, writes the result back to DDR, and the host reads the output through XDMA.

## Architecture

```text
Host
↓ XDMA
DDR
↓
HLS Accelerator inside A1
├─ AXI-Lite control
└─ m_axi DDR master
↓
DDR
↓ XDMA
Host
```

## Files

- `project_gen_reconfig.tcl`: Vivado project-generation script for this accelerator.
- `hls/`: HLS source, testbench, scripts and exported IP.

## HLS IP catalog

This accelerator uses an HLS-generated IP. The exported HLS IP must be added to the Vivado IP catalog inside `project_gen_reconfig.tcl` before the block design checks or IP instantiation steps.

The project-generation script should include the HLS export directory as an IP repository, for example:

```tcl
set_property ip_repo_paths [list [file normalize "$script_folder/hls/export"]] [current_project]
update_ip_catalog
```

Adjust the path if the exported IP is stored somewhere else.

Without adding the HLS IP repository to the catalog, Vivado will not be able to instantiate the accelerator IP in the generated block design.

## Build

From the repository root:

```bash
make ACCEL=ddr_scale_hls -j8
```

If the HLS source has changed, regenerate the exported HLS IP before running the DFX build.

## Software

The matching software example is expected to be built from the companion software repository with:

```bash
make APP=ddr_scale_hls
```

The software should configure the HLS control registers through AXI-Lite and use the DDR input/output addresses expected by this design.
