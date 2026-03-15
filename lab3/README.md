# COE838 Lab 3 — DE1-SoC Tutorial

This lab focuses on hardware-software integration on the DE1-SoC platform using the ARM Cortex-A9 HPS and Cyclone V FPGA. The project demonstrates how FPGA peripherals can be accessed and controlled through memory-mapped I/O over the lightweight HPS-to-FPGA AXI bridge.

## Overview

The goal of this lab was to build and test a simple SoC system in Quartus and Platform Designer, then control the connected FPGA peripherals using embedded C code running on the HPS. The completed design allowed communication between software and hardware through peripherals such as LEDs, switches, and seven-segment displays.

## Objectives

- Build a DE1-SoC hardware system in Quartus and Platform Designer
- Connect FPGA peripherals to the HPS using the lightweight AXI bridge
- Use memory-mapped I/O for hardware-software communication
- Write embedded C code to control the FPGA peripherals
- Program and test the design on the DE1-SoC board

## Tools and Technologies

- DE1-SoC development board
- Cyclone V FPGA
- ARM Cortex-A9 HPS
- Intel Quartus
- Platform Designer (Qsys)
- VHDL
- Embedded C
- Intel SoC Embedded Development Suite

## System Architecture

The system uses the lightweight HPS-to-FPGA bridge to allow the processor to access FPGA peripherals through assigned memory addresses. The embedded C program maps these hardware addresses into user space and reads or writes to the peripheral registers directly.

## Implementation

The lab followed a standard SoC design flow:

1. Create the hardware system in Quartus and Platform Designer
2. Add FPGA peripherals and assign their base addresses
3. Generate the system files and compile the FPGA design
4. Program the board with the hardware configuration
5. Develop embedded C software for peripheral control
6. Access the peripherals through memory-mapped I/O
7. Verify the system using board outputs and terminal messages

## Results

The final system successfully demonstrated communication between the HPS and FPGA peripherals. The software was able to control board outputs and interact with the mapped hardware registers correctly. LEDs and seven-segment displays responded as expected, confirming proper hardware-software integration.

## Demo Video

Demo of the final hardware implementation on the DE1-SoC board:

🔗 [Lab 3 Video Demonstration](https://drive.google.com/file/d/18_HiTohm4m10vXItO44ZEy9HtVhn1zn1/view)

## Challenges

One of the main issues encountered during development was the persistence of outdated generated files after hardware changes were made in Quartus and Platform Designer. This caused mismatches between the current hardware design and the generated software support files, especially header files with peripheral address definitions.

To resolve this, old generated artifacts had to be removed and the system files regenerated to ensure that the software matched the updated hardware configuration.

## Key Takeaways

- SoC design requires careful coordination between hardware and software
- Memory-mapped I/O is a practical method for controlling FPGA peripherals from software
- The HPS-to-FPGA bridge is essential for processor-peripheral communication on the DE1-SoC
- Keeping generated hardware files and software headers synchronized is critical

## Repository Structure

```text
.
├── README.md
├── report/
│   └── COE838_Lab3_Floreza_DexterRyan_500946679.pdf
├── src/
│   └── main.c
├── hardware/
│   ├── qsys/
│   ├── quartus/
│   └── vhdl/
└── images/
