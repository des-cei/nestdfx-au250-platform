# Accelerators

This folder contains the reconfigurable module examples for the A1 region.

Each accelerator is placed in its own folder and provides its own Vivado project-generation TCL:

```text
accelerators/<ACCEL>/project_gen_reconfig.tcl
```

The root Makefile selects the accelerator using:

```bash
make ACCEL=<ACCEL>
```

## Current examples

```text
accelerators/
├── timer_bram/
└── ddr_scale_hls/
```

- [`timer_bram`](timer_bram/README.md): simple AXI Timer + BRAM reference design.
- [`ddr_scale_hls`](ddr_scale_hls/README.md): HLS accelerator with AXI-Lite control and an `m_axi` DDR master.

## Creating a new accelerator

A simple way to start is to copy one of the existing examples:

```bash
cp -r accelerators/timer_bram accelerators/my_accel
```

or, for an HLS-based accelerator:

```bash
cp -r accelerators/ddr_scale_hls accelerators/my_hls_accel
```

Then modify:

```text
accelerators/my_accel/project_gen_reconfig.tcl
```

and build from the repository root:

```bash
make ACCEL=my_accel -j8
```

## General project-generation workflow

The recommended workflow for creating or modifying an accelerator is:

1. Generate or edit the Vivado project for the reconfigurable hardware.
2. Open the project in Vivado if graphical editing is needed.
3. Modify the block design.
4. Export the block design/project TCL from Vivado.
5. Apply the mandatory DFX edits described below before replacing the accelerator's `project_gen_reconfig.tcl`.

Vivado-generated TCL scripts are useful as a starting point, but they cannot be used directly in this DFX flow without a few manual changes. The reason is that the static shell expects a fixed reconfigurable-region top entity called `reconfig_base_inst`, while Vivado normally generates the block design wrapper as the top-level module. In this repository, the generated block design must be placed under a fixed VHDL wrapper.

The final hierarchy must be:

```text
reconfig_base_inst
    └── reconfig_base_inst_bd_wrapper
            └── user block design
```

where:

- `reconfig_base_inst` is the fixed shell-facing VHDL wrapper.
- `reconfig_base_inst_bd_wrapper` is the wrapper generated automatically from the Vivado block design.
- The user hardware is placed inside the block design.

## Mandatory edits after exporting TCL from Vivado

After exporting a TCL script from Vivado, a series of changes need to be applied.

One can compare a fresh Vivado-generated version with a modified one at:

```text
project_generation_script_changes/
```

For example:

```bash
cd accelerators/project_generation_script_changes
diff -u vivado_version.tcl edited_dfx_flow.tcl
```

Those changes are described below.

### 1. Use the repository project name and path

Vivado may export something like:

```tcl
create_project project_1 myproj -part xcu250-figd2104-2L-e
```

For the automated flow, use the repository project name and output path:

```tcl
create_project reconfig ./reconfig_prj -part xcu250-figd2104-2L-e
```

This keeps the generated project in the location expected by the rest of the scripts.

### 2. Rename the block design

Vivado normally exports the block design with the same name as the reconfigurable module, for example:

```tcl
set design_name reconfig_base_inst
```

Change it to:

```tcl
set design_name reconfig_base_inst_bd
```

This is required because `reconfig_base_inst` must remain the fixed DFX top-level VHDL entity. The block design must be internal to it.

After this change, Vivado will generate:

```text
reconfig_base_inst_bd.bd
reconfig_base_inst_bd_wrapper
```

### 3. Normalize the internal `M00_AXI_0` interface

The shell-facing top-level wrapper includes the AXI USER sideband ports:

```text
M00_AXI_0_aruser[7:0]
M00_AXI_0_awuser[7:0]
```

However, not all internal hardware exposes those optional AXI signals. For example, an HLS IP may not expose them. To make all designs compatible, the internal block design must use a normalized `M00_AXI_0` interface without USER sideband ports.

Find the exported `M00_AXI_0` interface creation code:

```tcl
set M00_AXI_0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M00_AXI_0 ]
set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.PROTOCOL {AXI4} \
   ] $M00_AXI_0
```

Replace it with:

```tcl
set M00_AXI_0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M00_AXI_0 ]
set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.WUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   ] $M00_AXI_0
```

The fixed outer wrapper still exposes `M00_AXI_0_aruser` and `M00_AXI_0_awuser` to the shell, but ties them to zero. This keeps the DFX boundary stable for different internal accelerators.

### 4. Generate the BD wrapper, but do not use it as the DFX top

At the end of the exported TCL, make sure the script generates the wrapper for `reconfig_base_inst_bd.bd`:

```tcl
set bd_file [get_files reconfig_base_inst_bd.bd]

set_property REGISTERED_WITH_MANAGER "1" $bd_file
set_property SYNTH_CHECKPOINT_MODE "Hierarchical" $bd_file

set bd_wrapper_path [make_wrapper -fileset sources_1 -files $bd_file -top]
add_files -norecurse -fileset sources_1 $bd_wrapper_path
```

This creates and adds `reconfig_base_inst_bd_wrapper` to the project.

### 5. Add the fixed DFX top-level wrapper

After generating the BD wrapper, add the fixed VHDL wrapper from the repository:

```tcl
add_files -norecurse -fileset sources_1 \
    [file normalize [file join $script_folder ../source/hdl/reconfig_base_inst.vhd]]
```

If the accelerator TCL is inside `accelerators/<ACCEL>/`, check that the relative path is correct from the script location.

### 6. Set the fixed wrapper as the top-level module

Finally, force the actual synthesis top to be the fixed wrapper:

```tcl
set_property top reconfig_base_inst [get_filesets sources_1]
```

Do not set `reconfig_base_inst_bd_wrapper` as the top. The shell expects `reconfig_base_inst`.

## Complete end-of-script template

The end of `project_gen_reconfig.tcl` should look like this:

```tcl
create_root_design ""

set bd_file [get_files reconfig_base_inst_bd.bd]

set_property REGISTERED_WITH_MANAGER "1" $bd_file
set_property SYNTH_CHECKPOINT_MODE "Hierarchical" $bd_file

set bd_wrapper_path [make_wrapper -fileset sources_1 -files $bd_file -top]
add_files -norecurse -fileset sources_1 $bd_wrapper_path

# Add the fixed DFX top-level wrapper.
add_files -norecurse -fileset sources_1 \
    [file normalize [file join $script_folder ../source/hdl/reconfig_base_inst.vhd]]

# Make the fixed wrapper the actual top.
set_property top reconfig_base_inst [get_filesets sources_1]
```

## Custom IPs and HLS IPs

If the block design uses custom IPs, such as HLS-generated IPs, the corresponding IP repository must be added to the Vivado IP catalog before the IP-check or IP-instantiation section of the generated TCL script.

For an HLS accelerator, this usually means adding something like this in the accelerator-specific `project_gen_reconfig.tcl`:

```tcl
set_property ip_repo_paths [list [file normalize "$script_folder/hls/export"]] [current_project]
update_ip_catalog
```

Adjust the path if the exported HLS IP is stored somewhere else.

Without this step, Vivado will not find the HLS IP when the block design is recreated.

## Practical rules

- Do not modify the shell-facing DFX interface.
- Do not let the Vivado-generated block design wrapper become the DFX top.
- Always keep `reconfig_base_inst` as the fixed top-level wrapper.
- Always place the user block design under `reconfig_base_inst_bd_wrapper`.
- Normalize the internal `M00_AXI_0` interface so that optional AXI USER ports do not depend on the selected internal IP.
- If a shell-facing AXI port is unused by the accelerator, connect it to a safe placeholder rather than leaving it floating.
