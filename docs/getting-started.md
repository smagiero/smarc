---
layout: default
title: Getting Started
---

# Getting Started

This page shows the shortest path to build and run the `smile` and `smicro` simulators.

# Get It and Build It
```bash
# Get the repo
$ git clone https://github.com/emilitronic/smarc.git
# Go in the repo
$ cd smarc
# Make a place for the executables
smarc $ mkdir build
# Configure (set CEDAR_DIR to your Cascade build/install)
smarc $ cmake -S . -B build -DCEDAR_DIR=/Users/seb/Research/Cascade/cedar -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# Build it
smarc $ cmake --build build -j
# See it
smarc $ ls build/smile build/smicro
```

# Run It
```bash
# Minimal Tile1 run
smarc $ ./build/smile/tb_tile1 -prog=smile/progs/prog.bin -steps=200 -trace "Tile1"

# Enter the Tile1 debugger REPL
smarc $ ./build/smile/tb_tile1 -prog=smile/progs/prog.bin

# SoC interactive (step manually)
smarc $ ./build/smicro/smicro -suite=proto_core -trace "SoC"

# Batched SoC run
smarc $ ./build/smicro/smicro -suite=proto_core -trace "SoC.RvCore;SoC.mem" -steps=20 -mem_latency=2
```

Next: see the [Examples Guide](./examples.md).
