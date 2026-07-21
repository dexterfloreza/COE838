# Multi-Engine Hardware-Accelerated MD5 Hash Generator (SoC Implementation)

> **Course:** COE838 – Systems-on-Chip (SoC) Design  
> **Repository Subdirectory:** [`/soc_project`](./)  
> **Author:** Dexter Ryan Floreza  

---

## 📌 System Overview

This project implements a high-throughput **Multi-Engine MD5 Cryptographic Hash Accelerator** on an Intel Cyclone V SoC platform. Hashing computations are offloaded from the dual-core **ARM Cortex-A9 Hard Processor System (HPS)** running Yocto Linux to a custom **32-engine parallel MD5 hardware array (`md5_group`)** instantiated in the FPGA fabric.

The HPS driver maps the FPGA peripherals into user space via `/dev/mem` and the **Lightweight HPS-to-FPGA Bridge (`lwh2f`)**, enabling high-speed message dispatch, execution triggering, status polling, and digest retrieval over 32-bit Avalon-MM registers.

---

## 🛠️ Hardware & Software Co-Design Architecture

```
+-------------------------------------------------------------------------------+
|                            Cyclone V SoC System                               |
|                                                                               |
|   +--------------------------+                 +--------------------------+   |
|   |   ARM Cortex-A9 (HPS)    |   Lightweight   |       FPGA Fabric        |   |
|   |   Running Yocto Linux    |  HPS-to-FPGA    |   (32 x MD5 Engines)     |   |
|   |                          |     Bridge      |                          |   |
|   |  - Pre-padded Messages   |<===============>|  +--------------------+  |   |
|   |  - mmap() /dev/mem       |   (Avalon-MM)   |  |     md5_group      |  |   |
|   |  - Serial/Parallel Exec  |                 |  | (Engines 0 to 31)  |  |   |
|   |  - Benchmark Analytics   |                 |  +--------------------+  |   |
|   +--------------------------+                 +--------------------------+   |
+-------------------------------------------------------------------------------+
```

### Memory-Mapped Register Mapping (lwh2f Bridge)

Communication is split into two distinct Avalon-MM slaves mapped to the FPGA:

* **Data Slave (`MD5_DATA_BASE` @ `0x00000000`):**
  * `DATA_REG_DATA`: 32-bit data register for writing message words and reading hash digests.
  * `DATA_REG_ADDR`: 32-bit address register encoding the target engine (0–31) and internal word offset (message word 0–15 or digest word 0–3).
* **Control Slave (`MD5_CTRL_BASE` @ `0x00000800`):**
  * `CTRL_REG_START`: Pulse bit mask to trigger MD5 processing on selected engines.
  * `CTRL_REG_RESET`: Active-high pulse bit mask to reset internal engine FSMs.
  * `CTRL_REG_WEN` / `CTRL_REG_DONE`: Multi-bit write-enable mask and execution status register (bit 0 = Serial Engine 0, `0xFFFFFFFF` = All 32 Parallel Engines complete).

---

## ⚡ Execution Modes & Performance Analytics

The C controller driver evaluates hardware execution efficiency across two modes:

1. **Serial Mode:** Harnesses only Engine 0 to establish baseline single-core hardware throughput.
2. **Parallel Mode:** Dispatches message blocks across all 32 hardware engines concurrently, measuring multi-engine speedup, memory-mapped I/O overhead, and parallel scaling efficiency.

### Monitored Metrics
* **Throughput:** Overall Hash Rate (H/s) vs. Hardware Compute-only Rate (H/s).
* **Time Breakdown:** Pure hardware execution time vs. Linux user-space memory-mapped I/O transfer overhead.
* **Accuracy:** Hardware digest verification against official RFC 1321 reference values (`MD5("abc") = 900150983cd24fb0d6963f7d28e17f72`).
* **Scaling:** Parallel speedup factor ($S = \text{Rate}_{\text{Parallel}} / \text{Rate}_{\text{Serial}}$) and multi-core efficiency ($E = S / 32$).

---

## 📁 Repository Structure

```
soc_project/
├── MD5.vhd                          # Top-level entity instantiating soc_system and md5_group
├── main.c                           # Linux HPS driver application for MMIO control & benchmarking
├── COE838_FINAL_PROJECT_REPORT-2.pdf # Technical paper detailing hardware results & analysis
└── ...                              # Platform Designer (Qsys) IP files, constraints & scripts
```

---

## 🚀 Build & Execution Guide

### 1. Hardware Compilation (Quartus Prime)
1. Generate the Qsys system (`soc_system`) containing the HPS component and Avalon-MM master bridges.
2. Compile the top-level VHDL wrapper (`MD5.vhd`) in Intel Quartus Prime.
3. Program the FPGA bitstream (`.sof`) onto the Cyclone V Development Board.

### 2. Software Compilation & Running on HPS
1. Boot Yocto Linux on the ARM Hard Processor System.
2. Cross-compile or natively compile the controller code:
   ```bash
   gcc -O2 main.c -o md5_controller
   ```
3. Run the application with root privileges (required for `/dev/mem` MMIO access):
   ```bash
   # Run serial mode for 5 seconds:
   ./md5_controller s 5

   # Run parallel mode for 5 seconds:
   ./md5_controller p 5

   # Run full benchmark comparison for 5 seconds:
   ./md5_controller b 5
   ```

---

## 📜 Specifications & References

- **RFC 1321:** [The MD5 Message-Digest Algorithm](https://datatracker.ietf.org/doc/html/rfc1321)
- **Intel Cyclone V Hard Processor System Technical Reference Manual**# Hardware-Accelerated MD5 Hash Generator (SoC Implementation)

> **Course:** COE838 – Systems-on-Chip (SoC) Design  
> **Repository Subdirectory:** [`/soc_project`](./)  

---

## 📌 Project Overview

This project presents a complete **System-on-Chip (SoC) hardware accelerator for the MD5 Cryptographic Hash Algorithm**, implemented in **VHDL** and integrated within an FPGA-based SoC platform (Intel/Altera Quartus & Qsys/Platform Designer). 

By offloading the computationally intensive bitwise operations, non-linear functions (F, G, H, I), modular additions, and variable left-rotations of the MD5 algorithm from software to dedicated hardware, this system achieves significant speedups compared to traditional software-only implementations running on embedded soft-core processors (e.g., Nios II).

---

## 🛠️ Architecture & System Design

```
+-----------------------------------------------------------------------+
|                               SoC System                              |
|                                                                       |
|   +-------------------+                     +---------------------+   |
|   |   Nios II CPU /   |   Avalon-MM / AXI   |    MD5 Hardware     |   |
|   |  Host Controller  |<===================>|     Accelerator     |   |
|   +-------------------+      Memory Bus     +---------------------+   |
|            |                                           |              |
|            v                                           v              |
|    Software Driver                           Datapath & FSM Control   |
|  (Message Padding &                        (64 Rounds, Functions F-I) |
|   Memory Management)                                   |              |
+--------------------------------------------------------|--------------+
                                                         v
                                                  128-bit Digest Output
```

### Key Components
1. **MD5 Hardware Accelerator (`MD5.vhd`)**:
   - Custom hardware block executing the core 64-round MD5 hashing process.
   - Dedicated logic for non-linear round functions:
     - `F(X,Y,Z) = (X AND Y) OR (NOT(X) AND Z)`
     - `G(X,Y,Z) = (X AND Z) OR (Y AND NOT(Z))`
     - `H(X,Y,Z) = X XOR Y XOR Z`
     - `I(X,Y,Z) = Y XOR (X OR NOT(Z))`
   - Internal state registers for hash accumulators (A, B, C, D) initialized with standard IV constants.
   - Finite State Machine (FSM) managing message processing, round iteration, and handshaking signals.

2. **SoC Bus & System Integration**:
   - Integrated as a memory-mapped peripheral via Avalon-MM / AXI interconnect.
   - Enables control, status monitoring, 512-bit message block feeding, and 128-bit hash output retrieval via memory address offsets.

3. **Software Control Driver**:
   - Handles preprocessing (RFC 1321 message padding to 512-bit boundaries).
   - Manages register read/write cycles to trigger hardware hashing and extract output digests.

---

## 📁 Repository Structure

```
soc_project/
├── MD5.vhd                          # Core VHDL hardware design for the MD5 engine
├── COE838_FINAL_PROJECT_REPORT-2.pdf # Full technical report, performance analysis & design spec
└── ...                              # Supporting source files, Qsys/Platform Designer modules & testbenches
```

---

## ⚡ Key Features & Performance Highlights

- **Hardware/Software Co-Design:** Combines software flexibility (padding and bus communication) with hardware speed (parallel 64-round hashing pipeline).
- **Optimized Datapath:** Hardwired constant tables (`K[i]`) and circular shifts (`S_i`) to eliminate dynamic lookup overhead.
- **Resource Efficiency:** Designed with low logic block utilization, making it suitable for compact FPGA fabrics.
- **Verification:** Simulation-verified using ModelSim against official RFC 1321 test vectors.

---

## 🚀 How to Build & Run

### Prerequisites
- **EDA Tools:** Intel Quartus Prime
- **Simulation:** ModelSim 
- **Target Hardware:** Altera/Intel FPGA development board (e.g., DE2-115, DE10-Lite, or Cyclone IV/V equivalent)

### Hardware Synthesis & Integration
1. Open **Quartus Prime** and load the project file.
2. Launch **Platform Designer (Qsys)**:
   - Import `MD5.vhd` as a custom component with an Avalon-MM / AXI slave interface.
   - Map control, input message, and output digest registers to standard base addresses.
   - Connect clock and reset signals to the system interconnect.
3. Generate the Qsys system and synthesize the top-level entity in Quartus.
4. Perform Pin Planner assignments and program the FPGA (`.sof` file).

---

## 📊 Results Summary

| **Report Reference** | Detailed benchmarks & timing diagrams available in [`COE838_FINAL_PROJECT_REPORT-2.pdf`](./COE838_FINAL_PROJECT_REPORT-2.pdf) |

---

## 📜 References & Specifications

- [RFC 1321 - The MD5 Message-Digest Algorithm](https://datatracker.ietf.org/doc/html/rfc1321)
- Intel FPGA Platform Designer / Avalon Interconnect Architecture Specifications
