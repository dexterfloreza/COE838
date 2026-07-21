# Hardware-Accelerated MD5 Hash Generator (SoC Implementation)

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
