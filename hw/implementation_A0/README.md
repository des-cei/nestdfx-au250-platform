# Static shell implementation

This folder contains the pre-built static shell artifacts used by the DFX flow.

The main artifact is the locked static design checkpoint:

```text
top_static/top_A0_locked.dcp
```

This DCP represents the static A0 shell. The A1 reconfigurable modules are implemented against this fixed shell.

## How it is used

When building a reconfigurable module with:

```bash
make ACCEL=<name>
```

the flow reuses the static shell DCP from this folder and implements the selected A1 reconfigurable design against it.

## Practical rules

- Researchers creating new accelerators should not normally modify this folder.
- New accelerators should be added under `accelerators/<ACCEL>/`.
- The static shell should only be regenerated when intentionally changing the platform infrastructure.
- If the static shell changes, the address maps, wrapper contract and software assumptions must be checked again.
