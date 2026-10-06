.. _snippet-ospi-psram:

OSPI PSRAM Snippet
###################

Overview
********

This snippet selects the appropriate overlay fragment to enable OSPI0 and the
connected AP Memory PSRAM on E8 boards or ISSI HyperRAM on B1/E7 DevKit.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_psram
   :board: alif_e8_ak/ae822fa0e5597xx0/rtss_he
   :goals: build
   :gen-args: -S ospi-psram
   :compact:

B1 DevKit HyperRAM
******************

The ``b1_dk_ospi0.overlay`` fragment enables ISSI IS66WVH HyperRAM on
OSPI0 for the B1 DevKit ``ab1c1f4m51820ph0/rtss_he`` target at 80 MHz.

.. code-block:: console

   west build -p always -b alif_b1_dk/ab1c1f4m51820ph0/rtss_he -S ospi-psram ../alif/samples/drivers/spi_psram

E7 DevKit HyperRAM
******************

The ``e7_dk_ospi0.overlay`` fragment enables ISSI IS66WVH HyperRAM using the
Alif HAL OSPI driver on E7 DevKit HE and HP targets. It configures OSPI0 at
100 MHz with six wait cycles and a 64 MiB XiP window at ``0xA0000000``.

.. code-block:: console

   west build -p always -b alif_e7_dk/ae722f80f55d5xx/rtss_he -S ospi-psram ../alif/samples/drivers/spi_psram

Replace ``rtss_he`` with ``rtss_hp`` for the HP core.

Engineering Board HyperRAM
*************************

The ``e8_ek_ospi1.overlay`` fragment configures Infineon S80KS HyperRAM on
the E8 engineering board. This board uses the E8 DevKit build target, but
the HyperRAM is connected to OSPI1. Select this fragment explicitly rather
than the DevKit AP Memory fragment chosen by ``-S ospi-psram``:

.. code-block:: console

   west build -p always -b alif_e8_dk/ae822fa0e5597xx0/rtss_he ../alif/samples/drivers/spi_psram -- '-DDTC_OVERLAY_FILE=../../../snippets/ospi-psram/e8_ek_ospi1.overlay'

Application Output
******************

.. code-block:: console

   PSRAM XIP mode demo app started
   Writing data to the XIP region:
   Reading back:
   Done, total errors = 0
