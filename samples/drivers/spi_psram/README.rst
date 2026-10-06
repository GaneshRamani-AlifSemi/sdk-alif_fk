
.. _psram-test:

PS RAM Test
###############

Overview
********

This is the test application to test and verify PSRAM/HyperRAM devices over
the OSPI interface.
The sample supports AP Memory PSRAM on E8 and HyperRAM on the configured
E8 engineering board and B1/E7 DevKit targets.


Building and Running
********************

The application will build only for a target that has a devicetree entry with
*:dt compatible:`alif,apmemory-aps512xxn`* or
*:dt compatible:`alif,infineon-s80ks2564`* or
*:dt compatible:`issi,is66wvhxx`* as a compatible.
Use the ``ospi-psram`` snippet for AP Memory PSRAM or B1/E7 ISSI HyperRAM,
or select the engineering board overlay explicitly for S80KS HyperRAM.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_psram
   :board: alif_e8_ak/ae822fa0e5597xx0/rtss_he
   :goals: build
   :gen-args: -S ospi-psram
   :compact:

S80KS HyperRAM Test
===================

The ``snippets/ospi-psram/e8_ek_ospi1.overlay`` file provides the devicetree
configuration for testing the Infineon S80KS HyperRAM with this sample.

This overlay was tested on the Alif E8 CSP engineering board. On the Alif E8
engineering board, the S80KS HyperRAM is connected over OSPI1, but the sample is
still built using the Alif E8 DevKit board target.

Select the engineering board overlay explicitly, without ``-S ospi-psram``:

.. code-block:: console

   west build -p always -b alif_e8_dk/ae822fa0e5597xx0/rtss_he ../alif/samples/drivers/spi_psram -- '-DDTC_OVERLAY_FILE=../../../snippets/ospi-psram/e8_ek_ospi1.overlay'

The overlay path is relative to the sample directory. Despite the filename,
the engineering board overlay configures OSPI1.

Also make sure the MPU entry for the OSPI1 XiP region is configured as
read/write with SRAM attributes.

B1 ISSI HyperRAM Test
====================

The snippet selects ``snippets/ospi-psram/b1_dk_ospi0.overlay`` for
the B1 DevKit ``ab1c1f4m51820ph0/rtss_he`` target. It enables ISSI HyperRAM
on OSPI0 at 80 MHz.

.. code-block:: console

   west build -p always -b alif_b1_dk/ab1c1f4m51820ph0/rtss_he -S ospi-psram ../alif/samples/drivers/spi_psram

E7 ISSI HyperRAM Test
====================

The snippet selects ``snippets/ospi-psram/e7_dk_ospi0.overlay`` for E7
DevKit HE and HP targets, using the Alif HAL OSPI driver at 100 MHz.

.. code-block:: console

   west build -p always -b alif_e7_dk/ae722f80f55d5xx/rtss_he -S ospi-psram ../alif/samples/drivers/spi_psram

Replace ``rtss_he`` with ``rtss_hp`` for the HP core.

Sample Output
=============

.. code-block:: console


PSRAM XIP mode demo app started
Writing data to the XIP region:
Reading back:
Done, total errors = 0
