---
layout: default
title: smicro
---

# smicro - Small SoC with Memory Subsystem

`smicro` is a SoC topology sandbox, a test harness with (more) detailed interaction dynamics specified between the SoC's constituent parts.  It is meant to flesh out things like interconnect/memory-subsystem correctness, multi-client behaviour (a core is just one client), topology choices, etc.  In contrast, `smile` is meant to contain more detailed descriptions of cores in terms of instruction sets and micro-architectural behaviour, it treats ancillary components (memory and accelerators) as black boxes that, at most, are modeled with simple latency.  No attempt is made to account for interactions between the core and other SoC parts in much detail.  Thus, `smile` is meant to ensure that the core/tile itself is doing what we expect (tile correctness), and to check basic interactions with memory and accelerators at a high level (small mu-arch experiments).  This does include memory timing, but as seen from the core.  In distilled terms, `smile` asks how does the core behave when the SoC behaves like X.

Conversely, `smicro` is meant to check how those components actually work and interact with each other and with the core.  In distilled terms, `smicro` asks how does the SoC behave when clients behave like Y.

`smicro` includes a more detailed DRAM model, a memory controller that implements a simple protocol, and a test suite that runs real instructions on the core and checks how the memory system responds.  It also includes a simple accelerator port and an example accelerator that performs an array sum by issuing memory requests through the memory controller.

The test suite in `smicro` is a layered testing model that's split into two main categories: those at the hardware abstraction layer (HAL) that directly poke at the DRAM array, and those that run through the protocol and timing of the core and memory controller. The former are `hal_*` suites and the latter are `proto_*` suites.

The `hal_*` tests poke at DRAM.  A sketch of the setup for these tests is shown below.  Note that there is no Sim::run() loop for these tests; they just execute once at t=0.
```
(tb_smicro.cpp)  hal_* suite
    |
    |  Dram HAL: read/write bytes at physical addresses
    v
+-------------------+
|      Dram         |
|  (storage array)  |
|  HAL: read/write  |
+-------------------+
```
Notes:
- No Sim::run() loop required for the test itself (runs at t=0).
- Addresses are PHYSICAL: dram_base + offset.
- Core + MemCtrl may exist as objects, but they are not part of the test path.

The `proto_*` tests drive MemCtrl timing/protocol over cycles.
```
(proto_core)   Driver = core     (MemCtrl path unused)

Sim::run() cycles:
    |
    v
+-------------------+        MemoryPort::read32/write32      +-------------------+
|   Tile1 (RV32)    |--------------------------------------->| Tile1Core::       |
|   tick()          |                                        | DramMemoryPort    |
|   executes instr  |<---------------------------------------| (adapter)         |
+-------------------+                                        +-------------------+
                                                                     |
                                                                     | Dram HAL read/write
                                                                     v
                                                             +-------------------+
                                                             |       Dram        |
                                                             | (storage array)   |
                                                             +-------------------+

```
Meanwhile:
- MemCtrl core ports are neutralized (bit-bucket / zero).
- MemTester is disabled.
- Accelerator may be attached, but *its memory traffic* (if any) is separate.


```
(proto_accel_sum*)   Driver = core
- CPU memory: direct to Dram via MemoryPort adapter
- Accel memory: MemCtrl path via AccelMemBridge

A) CPU instruction execution + mailbox store
-------------------------------------------
Sim::run() cycles:
    |
    v
+-------------------+        MemoryPort::read32/write32      +-------------------+
|   Tile1 (RV32)    |--------------------------------------->| Tile1Core::       |
|   executes program|                                        | DramMemoryPort    |
|   issues CUSTOM-0 |<---------------------------------------| (adapter)         |
+-------------------+                                        +-------------------+
                                                                     |
                                                                     | Dram HAL read/write
                                                                     v
                                                             +-------------------+
                                                             |       Dram        |
                                                             | (storage array)   |
                                                             +-------------------+

B) Accelerator execution + timed memory loads (sum array)
---------------------------------------------------------
Tile1 CUSTOM-0
    |
    v
+-------------------+      host API (start_load32/resp_*)    +-------------------+
| AccelArraySumSoc  |--------------------------------------->|  AccelMemBridge   |
| (AccelPort impl)  |<---------------------------------------|  (MemCtrl client) |
+-------------------+                                        +-------------------+
                                                                     |
                                                                     | MemReq/MemResp FIFOs
                                                                     v
                     +-------------------+   s_req/s_resp    +-------------------+
                     |      MemCtrl      |------------------>|       Dram        |
                     | (latency/backprs) |<------------------| (storage array)   |
                     +-------------------+                   +-------------------+

```
Notes:
- CPU passes rs1 = *CPU address* (e.g. 0x4000), rs2 = len.
- AccelMemBridge adds addr_base_ (dram_base) to form physical addresses for MemCtrl.
- Mailbox is written by the CPU program (not by the accel).
- Suites:
  - altaddr: same but array at 0x6000 to prove translation isn’t hard-coded.
  - badarg: misaligned rs1 => returns ACCEL_E_BADARG to mailbox.
  - unsupported: funct3!=0 => returns ACCEL_E_UNSUPPORTED to mailbox.
  - twice: two CUSTOM-0 ops back-to-back, results to mailbox0+mailbox1.


## Components and Roles (smicro + smile)

- `Tile1` (in `smile/`):
  - The actual RV32 CPU model.
  - Knows:
    - a clock (`clk`).
    - a `MemoryPort*` and optional accelerator port.
  - Does **not** know about:
    - `Dram`, `MemCtrl`, FIFOs, `SoC`, or testbenches.

- `Tile1Core` (in `smicro/`):
  - A thin wrapper to host `Tile1` inside smicro.
  - Responsibilities:
    - owns a `Tile1 tile_;`
    - for now, creates a `DramMemoryPort` adapter and calls `tile_.attach_memory(...)`.
    - implements `update()` (one `tile_.tick()` per cycle).
    - optionally implements `reset()` later.

- `Dram`:
  - Simulated DRAM + HAL helpers.
  - Two personalities:
    - **Protocol side**: `s_req/s_resp` FIFOs (MemCtrl talks to these).
    - **HAL side**: raw `read(addr, void*, bytes)` / `write(...)` for tests and adapters.

- `MemCtrl`:
  - Speaks `MemReq/MemResp` on the core side and `MemReq/MemResp` on the DRAM side.
  - Only in the path for `proto_*` suites that use the tester or (later) core via `m_req/m_resp`.

- `AccelMemBridge`:
  - A tiny MemCtrl client shim for accelerators
  - host facing: `start_load32()/start_store32()` (among others)
  - MemCtrl facing: `MemReq/MemResp` FIFOs

- `RvCore`:
  - A tiny FSM that **only** exists to exercise `MemCtrl` protocol (store then load).
  - Not a real CPU; it just pushes/consumes `MemReq/MemResp`.

- `SoC`:
  - The top-level composition: instantiates `Tile1Core`, `MemTester`, `MemCtrl`, `Dram`, etc.
  - Decides who is actually driving memory based on `use_test_driver` (derived from `-suite`).

## Memory Interfaces (3 Layers)

There are three different “views” of memory:

1. **CPU view – `MemoryPort`**  
   - API: `read32(addr)`, `write32(addr, value)`  
   - Used by: `Tile1`  
   - Goal: Simple, word-oriented, no FIFOs, no timing details.

2. **HAL / test view – `Dram::read/write`**  
   - API: `read(uint64_t addr, void* dst, uint64_t bytes)`, `write(...)`  
   - Used by: testbenches and adapters (`DramMemoryPort`, HAL tests in `tb_smicro`, `tb_tile1`)  
   - Goal: “Let me poke bytes into the DRAM array from the outside.”

3. **Protocol view – `MemReq/MemResp` FIFOs**  
   - API: `FifoOutput<MemReq>`, `FifoInput<MemResp>` with `push/pop/full/empty`  
   - Used by: `RvCore`, `MemTester`, `MemCtrl`, and (later) `Tile1Core` when it grows a proper LSU  
   - Goal: Exercise and model an on-chip memory protocol with latency/backpressure.

Current setup for `-suite=proto_core`:

- `Tile1` uses **(1)** via `MemoryPort`.
- `Tile1Core::DramMemoryPort` bridges **(1) → (2)**.
- The `MemReq/MemResp` (**3**) path is effectively idle (core’s `m_req/m_resp` are bit-bucketed).

### Call chain for an instruction fetch (proto_core, topo=dram)

Tile1::tick()
  → mem_port_->read32(pc_)
    → Tile1Core::DramMemoryPort::read32(pc_)
      → Dram::read(dram_base + pc_, &value, 4)

## Suites (`-suite`) and Drivers

We distinguish between:

- `hal_*` suites:
  - Only test DRAM via HAL at t=0 (no cycles, no driver).
  - `is_hal = true`, `use_tester = false`.

- `proto_*` suites:
  - Exercise protocol / timing over cycles.
  - `is_proto = true`.
  - `use_tester = (suite != "proto_core")`.

Mapping:

- `proto_core`:
  - `use_tester = false`.
  - SoC wiring:
    - `Tile1Core` is wired directly to `Dram` via `attach_dram()` and `DramMemoryPort`.
    - `MemTester` is disabled.
    - `MemCtrl` core-side ports are neutralized.
  - Effect: “Let the real CPU run against DRAM; MemCtrl is not in the path yet.”

- `proto_raw`, `proto_no_raw`, `proto_rar`, `proto_lat`:
  - `use_tester = true`.
  - SoC wiring:
    - `MemTester` ↔ `MemCtrl` ↔ `Dram`.
    - `Tile1Core` is present but its `m_req/m_resp` are sent to bit bucket / zero.
  - Effect: “Drive MemCtrl with scripted MemTester traffic and check timing/latency.”

  ## Unknown / Garbage Instructions

- Tile1 will interpret **any 32-bit word** as an instruction.
- The decode logic (`Instruction decoded(instr)`) always succeeds in producing a struct.
- The main `switch (decoded.category)` has a `default:` that just `break`s (no trap, no assert).
- Result:
  - Many garbage encodings effectively become NOPs or weird-but-legal ALU ops.
  - The core only “explodes” if:
    - an `assert_always` fires, or
    - some path calls `request_illegal_instruction()` and the trap handling leads to a halt.

This is why seeding DRAM with `0x1122334455667788` results in instructions like `0x55667788` and `0x11223344` that execute without immediate failure.