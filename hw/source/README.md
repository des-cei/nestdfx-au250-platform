# Source files

This folder contains the fixed source files used by the reconfigurable hardware flow.

The most important file is:

```text
hdl/reconfig_base_inst.vhd
```

This VHDL file is the fixed shell-facing wrapper for the A1 reconfigurable region. It defines the interface expected by the static shell.

## How it is used

Each accelerator creates its own internal Vivado block design through:

```text
accelerators/<ACCEL>/project_gen_reconfig.tcl
```

That block design is generated as:

```text
reconfig_base_inst_bd_wrapper
```

and is instantiated under the fixed wrapper:

```text
reconfig_base_inst
└── reconfig_base_inst_bd_wrapper
```

So the accelerator-specific logic goes inside the block design, while the shell-facing wrapper remains stable.

## Practical rules

- Do not modify `reconfig_base_inst.vhd` when creating a normal accelerator.
- Put accelerator-specific RTL, IP, HLS IP and block-design changes under `accelerators/<ACCEL>/`.
- Keep the wrapper ports compatible with the static shell.
- Only modify this folder if you are intentionally changing the shell-facing contract.
- If this interface changes, the hardware flow, address maps and software examples must be updated consistently.
