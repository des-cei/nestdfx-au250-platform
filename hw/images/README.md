# Images and address maps

This folder contains the reference diagrams for the static shell and the example reconfigurable region.

## Files

- [shell_address_map.png](shell_address_map.png)
- [shell_block_diagram.png](shell_block_diagram.png)
- [reconfig_region_address_map.png](reconfig_region_address_map.png)
- [reconfig_region_block_diagram.png](reconfig_region_block_diagram.png)

## Why they matter

- Use the shell diagrams to confirm the fixed shell-facing interface.
- Use the address maps to verify the reserved control, data and interrupt windows.
- Use the reconfigurable-region diagrams as a reference example, not as a mandatory structure for every accelerator.

## Address map reference

| Area | Base / window | Notes |
| --- | --- | --- |
| Shell control path | `0x8000_0000`, 32 MB window | Shell-facing control/register space. |
| Shell data / master path | `0x4000_0000` | Shell-side master/data mapping shown in the example shell map. |
| Reference timer block | `axi_timer_0` at `0x4200_0000`, 64 KB window | Example control-side block inside the `timer_bram` reconfigurable module. |
| Reference BRAM block | `axi_bram_ctrl_0` at `0x8000_0000`, 1 MB window | Example data-side block inside the `timer_bram` reconfigurable module. |

## Diagram notes

The shell diagram shows the static logic, DFX decoupler, shutdown managers, XDMA, DDR4, AXI HBICAP and CMS subsystem.

The reconfigurable-region diagram shows the reference `timer_bram` user block structure. Other accelerators may use different internal logic, for example an HLS IP with AXI-Lite control and an `m_axi` DDR master, as long as the shell-facing interface remains compatible.

## Practical rule

Keep the shell-facing interface contract stable. Adapt the reconfigurable project internals for each accelerator, but do not change the reserved shell maps unless you are intentionally redesigning the shell.
