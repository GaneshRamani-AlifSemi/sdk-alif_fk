# MSPI / Alif DesignWare SSI / IS25 Flash Mindmap

This document is a working map for the MSPI flash stack used by the Alif
DesignWare SSI controller and the ISSI IS25xX0xx MSPI flash driver.

The goal is to keep the generic DesignWare MSPI driver upstream-friendly, keep
Alif-specific controller wrapper behavior isolated, and make the flash
initialization/read/write/erase flow easy to review.

## 1. Big picture

```text
MSPI flash stack
|
+-- Devicetree controller node
|   |
|   +-- alif,designware-ssi.yaml
|   |   `-- Alif wrapper properties around the Synopsys SSI block
|   |
|   `-- snps,designware-ssi.yaml
|       `-- Generic DesignWare SSI/MSPI controller properties
|
+-- Devicetree flash child node
|   |
|   +-- mspi-is25xX0xx.yaml
|   |   `-- ISSI flash-specific properties
|   |
|   +-- mspi-device.yaml
|   |   `-- MSPI transaction/device properties
|   |
|   `-- jedec,jesd216.yaml
|       `-- JEDEC/SFDP flash geometry properties
|
+-- zephyr/drivers/mspi/mspi_dw.c
|   `-- Generic Synopsys DesignWare MSPI controller driver
|
+-- zephyr/drivers/mspi/mspi_dw_vendor_specific.h
|   `-- Vendor hook dispatch layer
|
+-- zephyr/drivers/mspi/mspi_dw_alif.h
|   `-- Alif wrapper timing and XiP implementation
|
`-- zephyr/drivers/flash/flash_mspi_is25xX0xx.c
    `-- ISSI IS25 flash command/init/read/write/erase driver
```

## 2. Source ownership map

| Area | File | Owns |
| --- | --- | --- |
| Generic MSPI controller | `zephyr/drivers/mspi/mspi_dw.c` | DesignWare register programming, normal transceive path, XiP control register setup, timing hook calls |
| Vendor dispatch | `zephyr/drivers/mspi/mspi_dw_vendor_specific.h` | Selection between Nordic, Alif, Ambiq/generic no-op hooks |
| Alif wrapper | `zephyr/drivers/mspi/mspi_dw_alif.h` | Alif AES/wrapper registers, RXDS delay, BAUD2 delay, DDR drive edge, XiP wrapper enable/disable |
| Flash protocol | `zephyr/drivers/flash/flash_mspi_is25xX0xx.c` | Flash reset, JEDEC ID, IO-mode switch, wait-cycle programming, read/write/erase operations |
| Alif controller binding | `zephyr/dts/bindings/mspi/alif,designware-ssi.yaml` | Alif-only controller properties |
| Generic controller binding | `zephyr/dts/bindings/mspi/snps,designware-ssi.yaml` | Synopsys SSI controller properties |
| MSPI device binding | `zephyr/dts/bindings/mspi/mspi-device.yaml` | Flash child transaction properties: IO mode, data rate, DQS, commands, dummy cycles |
| IS25 flash binding | `zephyr/dts/bindings/mtd/mspi-is25xX0xx.yaml` | Flash-specific properties such as reset GPIO and write block size |

## 3. Controller binding mindmap

```text
Controller node
|
+-- compatible
|   |
|   +-- "alif,designware-ssi"
|   |   `-- Selects Alif wrapper binding and validates Alif properties
|   |
|   `-- "snps,designware-ssi"
|       `-- Lets the generic DesignWare driver bind to the node
|
+-- Generic DesignWare properties
|   |
|   +-- reg / interrupts / clocks
|   +-- fifo-depth / dfs-offset / max-xfer-size style controller details
|   +-- dma-related generic controller settings, if enabled
|   `-- other SSI controller capabilities
|
`-- Alif wrapper properties
    |
    +-- aes-reg
    |   `-- Register space for Alif wrapper/AES/XiP controls
    |
    +-- ddr-drive-edge
    |   `-- TXD drive edge setting used for DDR operation
    |
    +-- rx-ds-delay
    |   `-- Alif RXDS/DQS strobe delay in the wrapper
    |
    +-- baud2-delay
    |   `-- BAUD2 delay control in the Alif wrapper
    |
    +-- xip-wait-cycles
    |   `-- Wait cycles used for Alif XiP transactions
    |
    +-- xip-rxds-vl-en
    |   `-- Enables RXDS variable latency for XiP
    |
    `-- xip-base-address
        `-- Memory mapped XiP address range, for example OSPI1 on E7
```

Notes:

- Alif-specific properties should stay in `alif,designware-ssi.yaml`.
- Generic Synopsys properties should stay in `snps,designware-ssi.yaml`.
- Runtime bus speed is not an Alif controller wrapper property. It belongs to
  the flash child device as `mspi-max-frequency`.
- Chip select selection is also a flash child property, normally
  `mspi-hardware-ce-num`.

## 4. Flash child binding mindmap

```text
Flash child node: compatible = "mspi-is25xX0xx"
|
+-- mspi-is25xX0xx.yaml
|   |
|   +-- write-block-size
|   |   `-- Minimum access/program alignment exposed through flash parameters
|   |
|   +-- reset-gpios
|   +-- t-reset-pulse
|   `-- t-reset-recovery
|
+-- mspi-device.yaml
|   |
|   +-- mspi-max-frequency
|   +-- mspi-io-mode
|   +-- mspi-data-rate
|   +-- mspi-hardware-ce-num
|   +-- mspi-cpp-mode
|   +-- mspi-endian
|   +-- mspi-dqs-enable
|   +-- rx-dummy / tx-dummy
|   +-- read-command / write-command
|   +-- command-length / address-length
|   +-- xip-config
|   +-- scramble-config
|   `-- ce-break-config
|
`-- jedec,jesd216.yaml
    |
    +-- jedec-id
    +-- sfdp-bfp
    +-- size
    `-- page-size
```

For the current Alif Octal DDR path, the important flash child properties are:

```text
mspi-max-frequency     = 100 MHz target, after init
mspi-io-mode           = MSPI_IO_MODE_OCTAL
mspi-data-rate         = MSPI_DATA_RATE_S_D_D
mspi-dqs-enable        = enabled for Octal DDR
mspi-endian            = MSPI_BIG_ENDIAN for DesignWare/Alif data packing
read-command           = Octal DDR read command
write-command          = Octal DDR page program command
command-length         = INSTR_1_BYTE
address-length         = ADDR_4_BYTE
rx-dummy               = flash wait cycles for read path
write-block-size       = 2 for Octal DDR paired-byte access
xip-config             = enables controller memory mapped access
```

## 5. IO mode versus data rate

IO mode describes how many data lines are used in each transaction phase.
Data rate describes whether each phase transfers on one edge or both edges.

```text
IO mode examples
|
+-- MSPI_IO_MODE_SINGLE
|   `-- 1-1-1: instruction/address/data all on one IO line
|
+-- MSPI_IO_MODE_OCTAL_1_8_8
|   `-- 1-8-8: instruction on one line, address/data on eight lines
|
`-- MSPI_IO_MODE_OCTAL
    `-- 8-8-8 style: instruction/address/data on eight lines

Data rate examples
|
+-- MSPI_DATA_RATE_SINGLE
|   `-- SDR on all enabled phases
|
+-- MSPI_DATA_RATE_S_D_D
|   `-- single-rate instruction, DDR address, DDR data
|
`-- MSPI_DATA_RATE_DUAL
    `-- DDR-style transfer on applicable phases
```

Current Alif target mode:

```text
MSPI_IO_MODE_OCTAL + MSPI_DATA_RATE_S_D_D
|
`-- 8S-8D-8D
    |
    +-- instruction: 8-bit bus, single data rate
    +-- address:     8-bit bus, double data rate
    `-- data:        8-bit bus, double data rate
```

That is Octal DDR operation, but instruction DDR is not enabled for the normal
command R/W/E path. XiP may need its own instruction-DDR control bit depending
on the controller/XiP register path.

## 6. Driver data flow

```text
Devicetree
|
+-- Controller node properties
|   |
|   +-- parsed into mspi_dw_config
|   `-- Alif wrapper properties parsed into alif_mspi_vendor_data
|
+-- Flash child node properties
|   |
|   +-- parsed by MSPI_DEVICE_CONFIG_DT_INST(n)
|   |   `-- cfg->tar_dev_cfg
|   |
|   +-- parsed by MSPI_DEVICE_CONFIG_SERIAL(n)
|   |   `-- cfg->serial_cfg
|   |
|   +-- xip-config
|   |   `-- cfg->tar_xip_cfg
|   |
|   `-- size / page-size / write-block-size
|       `-- cfg->mem_size / cfg->page_layout / cfg->flash_param
|
+-- Flash init calls mspi_dev_config()
|   |
|   +-- first with serial_cfg
|   `-- later with tar_dev_cfg
|
`-- mspi_dw.c stores active settings in mspi_dw_data
```

## 7. Generic versus Alif-specific MSPI driver split

```text
mspi_dw.c
|
+-- writes generic DesignWare registers
+-- computes transfer mode, frame count, FIFO behavior
+-- handles transceive, command mode, target mode
+-- configures generic XiP registers
|
`-- calls vendor-specific hooks
    |
    +-- vendor_specific_apply_timing_config()
    +-- vendor_specific_ddr_drive_edge()
    +-- vendor_specific_xip_update_ctrl()
    +-- vendor_specific_xip_prepare_registers()
    +-- vendor_specific_xip_enable()
    `-- vendor_specific_xip_disable()

mspi_dw_vendor_specific.h
|
+-- Nordic EXMIF branch
+-- Nordic QSPI v2 branch
+-- Alif DesignWare SSI branch
|   |
|   +-- calls Alif hooks only for Alif-compatible nodes
|   `-- otherwise keeps no-op behavior
|
`-- generic fallback branch
    `-- grouped no-op implementations

mspi_dw_alif.h
|
+-- owns Alif wrapper register layout
+-- owns Alif vendor data structure
+-- programs rx-ds-delay and baud2-delay
+-- supplies DDR drive edge
+-- updates Alif XiP control bits
`-- enables/disables Alif XiP wrapper region
```

This split keeps `mspi_dw.c` generic and minimizes upstream review friction.

## 8. Timing mindmap

```text
Timing parameters
|
+-- rx_sample_dly
|   |
|   +-- Generic DesignWare RX sample delay register
|   +-- Stored in mspi_dw_data
|   +-- Written through write_rx_sample_dly()
|   `-- For Alif Octal DDR with RXDS/DQS, current working value is 0
|
+-- rx-ds-delay
|   |
|   +-- Alif wrapper RXDS/DQS delay
|   +-- Comes from alif,designware-ssi.yaml
|   +-- Applied by alif_apply_timing_config()
|   `-- Distinct from generic rx_sample_dly
|
+-- ddr-drive-edge
|   |
|   +-- Alif TXD drive edge
|   `-- Applied through vendor_specific_ddr_drive_edge()
|
+-- baud2-delay
|   |
|   +-- Alif wrapper BAUD2 delay control
|   +-- Valid values are binding-limited
|   `-- Keep SoC-specific behavior inside Alif vendor code
|
`-- mspi-dqs-enable
    |
    +-- Flash child property
    +-- Enables DQS/RXDS-capable operation
    `-- Required for reliable Octal DDR, not useful for plain single IO mode
```

Important distinction:

- `rx_sample_dly` is a DesignWare controller sample-delay register.
- `rx-ds-delay` is the Alif wrapper delay for the receive data strobe path.
- They are not the same knob, even if early experiments used similar numbers.

## 9. Flash init sequence

```text
flash_mspi_is25xX0xx_init()
|
+-- 1. Check controller device readiness
|
+-- 2. Decide whether target config is Octal DDR
|   |
|   `-- is25xX0xx_is_octal_ddr_cfg(cfg->tar_dev_cfg)
|
+-- 3. Validate supported IO mode
|   |
|   +-- single mode
|   +-- octal mode
|   +-- octal 1-1-8
|   `-- octal 1-8-8
|
+-- 4. Select flash IO mode register value
|   |
|   +-- Octal DDR + DQS
|   +-- Octal DDR without DQS
|   +-- Extended SPI + DQS
|   `-- Extended SPI without DQS
|
+-- 5. Configure controller in safe serial mode
|   |
|   `-- mspi_dev_config(..., cfg->serial_cfg)
|
+-- 6. Reset flash and read vendor ID
|
+-- 7. Compute wait-cycle register value from rx_dummy
|
+-- 8. Optional 4-byte address mode for non-Octal-DDR path
|   |
|   +-- write enable
|   `-- SPI_NOR_CMD_4BA
|
+-- 9. Write enable before changing flash volatile registers
|
+-- 10. Program flash mode while controller is still serial
|    |
|    +-- Octal DDR:
|    |   `-- write IO mode register to switch flash into Octal DDR
|    |
|    `-- non-Octal-DDR:
|        `-- write wait-cycle register in serial mode
|
+-- 11. Configure controller to target mode
|    |
|    `-- mspi_dev_config(..., cfg->tar_dev_cfg)
|
+-- 12. If Octal DDR, program wait cycles again in Octal DDR mode
|    |
|    +-- write enable
|    `-- write two identical wait-cycle bytes
|
+-- 13. Apply MSPI timing configuration
|
+-- 14. Enable XiP, if requested
|
+-- 15. Enable scrambling, if requested
|
`-- 16. Release flash lock
```

Why wait cycles are written after the IO mode switch in Octal DDR:

- Before the switch, the controller and flash can still speak serial command
  mode safely.
- After switching the flash to Octal DDR, the controller must also be moved to
  the Octal DDR target config.
- Some DDR register writes require paired data bytes, so the wait-cycle value is
  written as two identical bytes.

## 10. Read sequence

```text
flash_mspi_is25xX0xx_read(offset, buffer, len)
|
+-- 1. Validate alignment
|   |
|   +-- offset must align to write-block-size
|   `-- len must align to write-block-size
|
+-- 2. Acquire flash lock
|
+-- 3. If XiP is enabled
|   |
|   +-- compute mapped address:
|   |   xip_base_addr + address_offset + offset
|   |
|   `-- memcpy() directly from mapped flash window
|
`-- 4. If XiP is not enabled
    |
    +-- fill MSPI RX packet
    +-- use read command / address / dummy cycles from active dev_cfg
    +-- call mspi_transceive()
    `-- release flash lock
```

Octal DDR read alignment:

- `write-block-size = <2>` intentionally rejects odd-length/single-byte reads.
- This avoids accidental byte reads in a DDR path where the flash/controller
  exchange data as paired bytes.
- If true byte reads are required, use SDR/single-rate configuration and
  `write-block-size = <1>`.

## 11. Write sequence

```text
flash_mspi_is25xX0xx_write(offset, buffer, len)
|
+-- 1. Validate alignment
|   |
|   +-- offset aligned to write-block-size
|   `-- len aligned to write-block-size
|
+-- 2. Acquire flash lock
|
+-- 3. Clean data cache for source buffer, if needed
|
+-- 4. Program page chunks
|   |
|   +-- compute chunk size not crossing page boundary
|   +-- write enable
|   +-- enter target/program mode as required
|   +-- send page program command
|   +-- busy wait until ready
|   `-- advance offset/buffer
|
+-- 5. Write disable
|
+-- 6. If XiP is enabled
|   |
|   `-- invalidate mapped XiP cache range for programmed area
|
`-- 7. Release flash lock
```

Upstream-friendly buffer handling:

- Do not use an Alif-only padded buffer in the flash driver.
- Keep the normal upstream-style chunked write loop.
- Alignment checks should catch invalid DDR access size before transaction
  setup.

## 12. Erase sequence

```text
flash_mspi_is25xX0xx_erase(offset, size)
|
+-- 1. Acquire flash lock
|
+-- 2. Validate offset and size
|
+-- 3. Full chip erase path
|   |
|   +-- offset == 0
|   +-- size == cfg->mem_size
|   +-- write enable
|   +-- erase chip
|   `-- busy wait
|
+-- 4. Block/sector erase path
|   |
|   +-- prefer largest valid erase unit
|   +-- write enable before each erase command
|   +-- send 64K / 32K / 4K erase command as applicable
|   `-- busy wait after each command
|
`-- 5. Release flash lock
```

Erase command address length:

```text
data->dev_cfg.addr_length == 4
|
+-- true
|   `-- use 4-byte erase opcode, for example SPI_NOR_CMD_SE_4B
|
`-- false
    `-- use 3-byte erase opcode, for example SPI_NOR_CMD_SE
```

Full erase sizing:

- The driver exposes `.get_size`.
- Applications should use `flash_get_size()` for full-chip erase size.
- Do not rely on removed local geometry fields such as `num_of_sector *
  sector_size` in the application.

## 13. Status / ready sequence

```text
flash_mspi_is25xX0xx_is_ready()
|
+-- choose dummy cycles
|   |
|   +-- Octal DDR: use DDR status dummy value
|   `-- serial/non-DDR: use normal status dummy
|
+-- issue flag/status register read
|
`-- return ready/busy state
```

Earlier symptom:

```text
logic analyzer showed 0x80, software read 0x40
```

That points toward sampling/phase/packing issues, not the flash status value
itself. For Alif Octal DDR the eventual stable direction was:

- DQS enabled for Octal DDR.
- Generic `rx_sample_dly` set to 0.
- Alif `rx-ds-delay` used for RXDS/DQS strobe tuning.

## 14. JEDEC ID and SFDP behavior

```text
read_jedec_id()
|
`-- returns cached 3-byte ID copied from init-time read
```

Reason:

- JEDEC ID is a standard three-byte identity: manufacturer, memory type, and
  capacity.
- After switching to Octal DDR, a simple serial `RDID` command may no longer be
  valid unless the driver temporarily returns to serial mode.
- Returning the cached init-time ID keeps the API stable without disturbing the
  active flash mode.

SFDP read remains a separate API path and should be reviewed based on whether
the flash is currently in serial or Octal DDR mode.

## 15. XiP mindmap

```text
XiP enable path
|
+-- Flash child DTS
|   |
|   `-- xip-config = <enable, address_offset, size, permission>
|
+-- Flash init
|   |
|   `-- mspi_xip_config(cfg->bus, cfg->dev_id, cfg->tar_xip_cfg)
|
+-- mspi_dw.c
|   |
|   +-- copies stored device config into active XiP params
|   +-- builds generic XIP_CTRL read/write values
|   +-- writes XIP read/write instructions
|   +-- writes XIP control registers
|   `-- calls vendor hooks
|
`-- Alif vendor hooks
    |
    +-- update Alif-specific XIP_CTRL bits
    +-- prepare RX sample delay / RXDS delay / drive edge
    +-- select mapped device region
    `-- enable Alif wrapper/AES XiP path
```

XiP read path:

```text
flash_read()
|
+-- if XiP enabled
|   `-- memcpy() from mapped address window
|
`-- otherwise
    `-- normal mspi_transceive() read
```

For E7 OSPI1, the mapped window is expected from the SoC memory map, for
example `0xC0000000`, and should be represented by the Alif controller
`xip-base-address`/flash `xip-config` combination.

## 16. 4-byte address mode notes

```text
Two possible models
|
+-- Enter global 4-byte address mode
|   |
|   +-- SPI_NOR_CMD_4BA
|   +-- changes flash global address interpretation
|   `-- currently safer for selected non-Octal-DDR/Ambiq-style path
|
`-- Use explicit 4-byte opcodes/address phase
    |
    +-- address-length = ADDR_4_BYTE
    +-- read/write/erase use 4-byte commands where required
    `-- preferred for current Alif Octal DDR evaluation path
```

Current caution:

- Do not force global 4-byte address mode for all Alif Octal DDR paths until
  all read/write/erase/status paths are verified.
- Erase command selection should follow the active `data->dev_cfg.addr_length`.

## 17. Endian notes

```text
MSPI endian
|
+-- Comes from flash child DTS: mspi-endian
|
+-- Flash serial config
|   |
|   `-- DT value if present, otherwise LITTLE_ENDIAN fallback
|
`-- Alif E7 path
    |
    `-- MSPI_BIG_ENDIAN needed for DesignWare/Alif FIFO data packing
```

This avoids hardcoding little endian in the serial phase while still preserving
the default behavior for boards that do not specify `mspi-endian`.

## 18. Review checklist before upstreaming

```text
Upstream cleanup checklist
|
+-- Generic driver separation
|   |
|   +-- mspi_dw.c has only generic logic and vendor hook calls
|   +-- Alif wrapper details stay in mspi_dw_alif.h
|   `-- generic no-op vendor hooks are grouped in one fallback branch
|
+-- Binding separation
|   |
|   +-- alif,designware-ssi.yaml contains only Alif controller properties
|   +-- snps,designware-ssi.yaml contains generic controller properties
|   +-- mspi-device.yaml remains device/transaction oriented
|   `-- mspi-is25xX0xx.yaml owns flash-specific write-block-size
|
+-- Flash driver behavior
|   |
|   +-- init sequence starts serial, then switches flash/controller to target mode
|   +-- Octal DDR wait cycles are written after target mode is active
|   +-- read/write validate write-block-size alignment
|   +-- no Alif-only padded buffer in write path
|   +-- get_size returns DTS/configured memory size
|   `-- cached JEDEC ID remains 3 bytes
|
+-- Alif timing
|   |
|   +-- rx_sample_dly and rx-ds-delay are treated as different controls
|   +-- DQS/RXDS is enabled only where the target mode needs it
|   `-- BAUD2 behavior stays behind Alif/SoC-specific hooks
|
+-- XiP
|   |
|   +-- mspi_xip_config() remains the MSPI API entry point
|   +-- Alif wrapper enable/disable stays in vendor hooks
|   `-- mapped address comes from DTS, not hardcoded in the driver
|
`-- Application
    |
    +-- full erase uses flash_get_size()
    +-- DDR tests use even read/write lengths
    `-- byte access tests use SDR/single-rate configuration
```

## 19. Common debugging map

```text
Symptom
|
+-- Read data shifted or specific bits wrong at high speed
|   |
|   +-- Check DQS enabled for Octal DDR
|   +-- Check rx_sample_dly is not fighting RXDS sampling
|   +-- Tune Alif rx-ds-delay
|   `-- Confirm endian/packing
|
+-- Every 64th byte missing
|   |
|   +-- Check FIFO frame count
|   +-- Check data_frames calculation
|   +-- Check bytes-per-frame handling
|   `-- Compare against Alif HAL OSPI FIFO handling
|
+-- flash_get_parameters() reports write_block_size = 1
|   |
|   +-- Ensure write-block-size is on actual flash node
|   `-- Partition child properties do not replace parent flash parameters
|
+-- Full erase does not erase full device
|   |
|   +-- Use flash_get_size()
|   +-- Confirm cfg->mem_size from DTS size
|   `-- Avoid app-local num_of_sector/sector_size assumptions
|
+-- XiP mapped read works but flash_read path differs
|   |
|   +-- Check cfg->tar_xip_cfg.enable
|   +-- Check address_offset and xip_base_addr
|   +-- Check cache invalidation after writes/erase
|   `-- Check Alif vendor XiP enable path
```

## 20. Short mental model

```text
DTS describes two things:
|
+-- controller capability/wrapper registers
`-- flash target transaction protocol

Flash driver owns flash protocol:
|
+-- reset
+-- mode switch
+-- wait cycles
+-- status
+-- read/write/erase
`-- flash API metadata

MSPI DW driver owns controller mechanics:
|
+-- SSI registers
+-- FIFO/frames
+-- transceive
+-- generic XiP registers
`-- timing hook points

Alif vendor layer owns only Alif wrapper extras:
|
+-- RXDS delay
+-- BAUD2 delay
+-- DDR drive edge
`-- Alif XiP wrapper/AES enable
```

