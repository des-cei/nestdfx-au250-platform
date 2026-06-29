# Alveo XDMA + HBICAP — Minimal Reconfigurable SW Template

This folder provides a compact C userspace template for working with Alveo boards that use XDMA for data movement and HBICAP for static/A1-region reconfiguration. It is the software companion to the hardware DFX shell in [`../hw`](../hw/README.md).

The goal is to offer a small, easy-to-adapt starting point that demonstrates the common software patterns needed to control and validate a reconfigurable hardware design:

- load an A1 partial bitstream,
- access AXI-Lite control/status registers,
- move data through full-AXI XDMA transfers,
- optionally collect power-rail samples from the Alveo CMS while a workload is running.

The examples are intentionally simple. They are meant to be copied and adapted when a new reconfigurable module is added on the hardware side.

## Key features

- Small helper library, [`alveo.c`](alveo.c) / [`alveo.h`](alveo.h), implementing:
  - `alveo_reconfigure(const char *bitstream_path)` — stream a raw `.bin` bitstream into HBICAP and wait for completion.
  - `alveo_reg_read32()` / `alveo_reg_write32()` — simple 32-bit AXI-Lite register access through the XDMA user BAR.
  - `alveo_write()` / `alveo_read()` — full-AXI XDMA H2C/C2H bulk transfers.
  - `alveo_power_start()` / `alveo_power_stop()` — optional CMS-based power sampling around a measured code section.
- Example applications under [`accelerators/`](accelerators/), each targeting a specific reconfigurable module.
- Simple application selection with `make ACCEL=<name>`.
- Same accelerator-name convention as the hardware side, so the selected software example matches the selected A1 reconfigurable module.

## Repository layout

```text
sw/
├── Makefile
├── README.md
├── alveo.c
├── alveo.h
└── accelerators/
    ├── timer_bram.c
    └── ddr_scale_hls.c
```

- [`alveo.h`](alveo.h): configuration macros, address-map constants and public API.
- [`alveo.c`](alveo.c): implementation of reconfiguration, register access, DMA transfers and CMS power-rail sampling.
- [`accelerators/`](accelerators/): example programs for specific reconfigurable modules.
- [`Makefile`](Makefile): build rules for `libalveo.a` and the selected accelerator.

## Quick start

1. Prepare an Alveo Linux system with the XDMA kernel driver and user character devices:

   ```bash
   /dev/xdma0_user
   /dev/xdma0_h2c_0
   /dev/xdma0_c2h_0
   ```

   Ensure your user has permission to access those device nodes, either through udev rules or by running the example as root.

2. List the available accelerator examples:

   ```bash
   make list_accelerators
   ```

3. Build one accelerator example:

   ```bash
   make ACCEL=timer_bram
   ```

   This produces `libalveo.a` and an executable with the selected accelerator name.

4. Run the example with the matching A1 partial bitstream:

   ```bash
   sudo ./timer_bram path/to/partial_region.bin
   ```

Notes:

- The bitstream argument must be a raw `.bin` file whose size is a multiple of 4 bytes. The loader checks this before streaming the file to HBICAP.
- HBICAP completion only tells you that the bitstream stream completed; it does not prove that the loaded module is the one expected by the software example.
- The selected software example must match the reconfigurable module loaded into A1.

## Build and run examples

Build the default example:

```bash
make
```

Build a specific example:

```bash
make ACCEL=timer_bram
make ACCEL=ddr_scale_hls
```

Run it with the generated partial bitstream:

```bash
sudo ./timer_bram path/to/top_A1_reconfig.bin
sudo ./ddr_scale_hls path/to/top_A1_reconfig.bin
```

Clean generated objects and executables:

```bash
make clean
```

## How the software maps to the hardware paths

The library hides the low-level Linux/XDMA details, but the address map still comes from the hardware shell.

### AXI-Lite register access

AXI-Lite register accesses use the XDMA user BAR:

```text
/dev/xdma0_user
```

The public API expects Vivado AXI-Lite addresses. For example, an accelerator control register at `0x42000000` should be passed as `0x42000000`. The library internally translates it to the corresponding `mmap()` offset using `ALVEO_AXIL_BASE_ADDR`.

Example:

```c
uint32_t value;
alveo_reg_read32(0x42000000ULL, &value);
alveo_reg_write32(0x42000000ULL, 0x1);
```

### Full-AXI XDMA transfers

Full-AXI bulk transfers use the XDMA H2C/C2H character devices:

```text
/dev/xdma0_h2c_0
/dev/xdma0_c2h_0
```

For `alveo_write()` and `alveo_read()`, pass the Vivado full-AXI address directly. The address is used as the XDMA file offset.

Example:

```c
alveo_write(0x80000000ULL, buffer, size_bytes);
alveo_read(0x80000000ULL, buffer, size_bytes);
```

### HBICAP reconfiguration

A1 reconfiguration is performed through the HBICAP block in the static shell. The helper function:

```c
alveo_reconfigure("path/to/partial_region.bin");
```

streams the raw `.bin` file to the HBICAP data path and polls the HBICAP status/control registers until the transfer completes or times out.

### CMS power sampling

The library also includes optional access to the Alveo Card Management Subsystem (CMS) register window exposed through the XDMA user BAR. This is used by the `alveo_power_*` API to sample a selected power rail while a workload section is running. The default rail is `VCCINT`, which is useful as a proxy for FPGA core-logic activity. The library can also select input rails such as `12V_PEX`, `12V_AUX`, `3V3_PEX`, `3V3_AUX`, or the summed `CARD_TOTAL` value.

The CMS address-map details are centralized in [`alveo.h`](alveo.h):

```c
#define ALVEO_CMS_CTRL_ADDR                  0x41000000ULL
#define ALVEO_CMS_CTRL_MAP_SIZE              0x40000U
#define ALVEO_CMS_RESET_BYTE_OFFSET          0x20000U
#define ALVEO_CMS_SENSOR_BYTE_OFFSET         0x28000U
#define ALVEO_CMS_VCCINT_VOLTAGE_WORD_OFFSET 58U
#define ALVEO_CMS_VCCINT_CURRENT_WORD_OFFSET 61U
#define ALVEO_POWER_SAMPLE_PERIOD_US         120000U
#define ALVEO_POWER_MAX_SAMPLES              4096U
```

Application examples should not access those CMS registers directly. They should only use the public API:

```c
/* Optional. The default is ALVEO_POWER_RAIL_VCCINT. */
alveo_power_set_rail(ALVEO_POWER_RAIL_VCCINT);

alveo_power_start();

/* Run the section to be measured. */

alveo_power_stop();

printf("rail   : %s\n",
       alveo_power_get_rail_name(alveo_power_get_rail()));
printf("samples: %zu\n", alveo_power_get_num_samples());
printf("avg    : %.3f W\n",
       (double)alveo_power_get_average() / (double)ALVEO_POWER_MW_PER_W);
printf("max    : %.3f W\n",
       (double)alveo_power_get_max() / (double)ALVEO_POWER_MW_PER_W);
```

The collected samples are CMS-derived power values stored in milliwatts. The CMS voltage/current product used by the library corresponds to microwatts for the current shell scaling, so `alveo.c` converts it to mW before storing each sample. The examples divide by `ALVEO_POWER_MW_PER_W` when printing watts.

The default rail is `ALVEO_POWER_RAIL_VCCINT`. Use `alveo_power_set_rail()` before `alveo_power_start()` to select another rail. `ALVEO_POWER_RAIL_CARD_TOTAL` is computed as the sum of the 12 V and 3.3 V card input rails exposed by the CMS. If the CMS register scaling changes in the shell or driver setup, update the constants and conversion logic in the library, not in the examples.

The sampling period is `ALVEO_POWER_SAMPLE_PERIOD_US` and defaults to 120 ms, matching the coarse CMS telemetry update rate. `alveo_power_stop()` wakes the sampling thread immediately, so stopping the monitor does not wait for the full sampling period.

## API summary

### Reconfiguration

```c
int alveo_reconfigure(const char *bitstream_path);
```

Streams a raw `.bin` bitstream into HBICAP and waits for completion. Returns `0` on success or a negative errno-style value on failure.

### AXI-Lite register access

```c
int alveo_reg_write32(uint64_t axil_addr, uint32_t value);
int alveo_reg_read32(uint64_t axil_addr, uint32_t *value);
```

Read/write single 32-bit AXI-Lite registers. Pass Vivado AXI-Lite addresses; the library handles the XDMA user BAR mapping.

### Full-AXI XDMA transfers

```c
ssize_t alveo_write(uint64_t hw_addr, const void *buffer, uint64_t size);
ssize_t alveo_read(uint64_t hw_addr, void *buffer, uint64_t size);
```

Move data over the full-AXI XDMA path. `hw_addr` is the Vivado full-AXI address and is used directly as the XDMA offset.

### Power sampling

```c
int alveo_power_start(void);
int alveo_power_stop(void);

int alveo_power_set_rail(alveo_power_rail_t rail);
alveo_power_rail_t alveo_power_get_rail(void);
const char *alveo_power_get_rail_name(alveo_power_rail_t rail);

size_t alveo_power_get_num_samples(void);
const uint32_t *alveo_power_get_samples(void);

uint32_t alveo_power_get_average(void);
uint32_t alveo_power_get_max(void);
```

Start/stop CMS-based power-rail sampling and inspect the samples collected during the last measured section. The sampling thread and CMS register mapping are managed internally by the library. The returned samples/statistics are expressed in milliwatts; examples convert them to watts when printing. The selected rail must be changed before `alveo_power_start()`; the library rejects rail changes while the sampling thread is running.

## Adapting this template to your design

Create a new example under [`accelerators/`](accelerators/), or copy an existing one:

```bash
cp accelerators/timer_bram.c accelerators/my_accel.c
```

Change the user-design-specific addresses near the top of the file. For example, a timer/BRAM design may use:

```c
#define TIMER_BASE_ADDR 0x42000000ULL
#define BRAM_BASE_ADDR  0x80000000ULL
```

Build the new example with:

```bash
make ACCEL=my_accel
```

The recommended convention is to use the same accelerator name as in the hardware folder:

```bash
# Hardware folder
make ACCEL=my_accel

# Software folder
make ACCEL=my_accel
```

When adapting an example, update only the parts that belong to the reconfigurable module:

- AXI-Lite base addresses and register offsets for your accelerator,
- full-AXI buffer addresses,
- input/output validation logic,
- optional measured section wrapped by `alveo_power_start()` and `alveo_power_stop()`, with `alveo_power_set_rail()` if the default `VCCINT` rail is not the desired measurement.

Do not duplicate XDMA, HBICAP or CMS access code in the example. Put reusable board-level operations in `alveo.c/.h`.

## Hardware contract with the shell

This software folder is intended to be used with a fixed, pre-built Alveo shell that defines the stable wrapper boundary and address map. See the hardware documentation in [`../hw`](../hw/README.md).

Keep these rules in mind when adapting the software to a new reconfigurable design:

- The shell is the source of truth for device address windows, register offsets, DMA windows and IRQ wiring.
- Do not invent new offsets. Use the shell-provided address map, block diagrams, generated headers or exported address reports.
- The reconfigurable region A1 may change contents, such as timer/BRAM locations, accelerator control registers or DDR buffer usage, but the shell-facing wrapper boundary must remain compatible.
- The shell-facing wrapper in the hardware repo is `reconfig_base_inst.vhd`. Treat it as the canonical interface between the static shell and the user logic.
- If the hardware build changes the CMS, HBICAP, AXI-Lite or XDMA address windows, update the corresponding constants in [`alveo.h`](alveo.h).
- When the user modifies the reconfigurable logic, they must update the matching software example.

## Example integration workflow

1. Create or select a reconfigurable module in the hardware folder.

2. Build the hardware partial bitstream:

   ```bash
   cd ../hw
   make ACCEL=my_accel
   ```

3. Build the matching software example:

   ```bash
   cd ../sw
   make ACCEL=my_accel
   ```

4. Run the software with the generated `.bin`:

   ```bash
   sudo ./my_accel path/to/top_A1_reconfig.bin
   ```

5. Use the example output to validate that:

   - the partial bitstream was accepted by HBICAP,
   - the AXI-Lite control path works,
   - the full-AXI data path works, if used by the accelerator,
   - optional CMS power-rail samples were collected, if the example enables power measurement.

## Troubleshooting

- **Device files are not present**: ensure the XDMA kernel module is loaded and the Alveo drivers are installed on the system.
- **Permission denied when opening `/dev/xdma0_*`**: run as root or configure udev rules for the XDMA character devices.
- **Reconfiguration times out**: verify the HBICAP control/data addresses in [`alveo.h`](alveo.h) and inspect `dmesg` or system logs for HBICAP/driver errors.
- **HBICAP reports completion but the example fails**: the bitstream may not match the expected reconfigurable module. Reconfiguration success does not validate the accelerator interface.
- **AXI-Lite access fails**: check that the Vivado AXI-Lite address belongs to the shell address map and that the loaded A1 module exposes the expected registers.
- **XDMA read/write fails**: check the full-AXI address, buffer size and whether the loaded hardware actually exposes that memory/data path.
- **Power sampling returns no samples**: ensure the example calls `alveo_power_start()` before the measured section and `alveo_power_stop()` after it. Very short workloads may only produce one sample because the CMS sampling period is 120 ms by default.
- **Power values look wrong**: verify the CMS register offsets and scaling in [`alveo.h`](alveo.h) and the conversion in [`alveo.c`](alveo.c) against the shell/CMS address map. The current library stores CMS-derived power as milliwatts and the examples print watts.
