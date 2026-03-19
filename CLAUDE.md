# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Benchmarking framework comparing four C implementations of square matrix multiplication to measure CPU performance across different parallelization strategies. All programs output a single float (user CPU time in seconds, 6 decimal places) to stdout.

## Build Commands

```bash
make              # Build all 5 binaries into output/
make clean        # Remove output/ directory
```

Individual targets: `output/secuential`, `output/secuentialFlags`, `output/memory`, `output/threads`, `output/multiprocessing`

The `secuentialFlags` target compiles the same source as `secuential` but with `-O2` to measure compiler optimization impact.

## Running the Programs

```bash
# Sequential and memory-optimized: <rows> <cols>
./output/secuential 500 500
./output/memory 500 500

# Threads and multiprocessing: <rows> <num_workers>
./output/threads 1000 4
./output/multiprocessing 1000 8
```

## Running Benchmarks

```bash
./scripts/RunAll.sh    # Full benchmark suite with resume support
./scripts/testing.sh   # Simpler legacy script for smaller sizes
```

`RunAll.sh` saves results to `stats/<hostname>/<timestamp>/` as CSV files and supports resuming interrupted runs via `checkpoint.log`.

## Architecture

All four solvers share a common pattern: allocate matrices, fill with random ints (0–99), multiply, measure CPU time via `getrusage(RUSAGE_SELF, ...)` using `ru_utime`, free memory, print time.

| Binary | Source | Strategy |
|--------|--------|----------|
| `secuential` | `SecuentialMatrixSolver.c` | Naive O(n³), no flags |
| `secuentialFlags` | `SecuentialMatrixSolver.c` | Same, compiled with `-O2` |
| `memory` | `MemoryMatrixSolver.c` | Transposes B for cache locality |
| `threads` | `ThreadsMatrixSolver.c` | POSIX threads (`-pthread`), divides rows across threads |
| `multiprocessing` | `MultiprocessingMatrixSolver.c` | `fork()` + `mmap(MAP_SHARED\|MAP_ANONYMOUS)` for result matrix C; measures `RUSAGE_SELF + RUSAGE_CHILDREN` |

The memory solver accesses `B[j][k]` instead of `B[k][j]` to avoid column-major cache misses.

The multiprocessing solver uses shared memory only for the result matrix C; A and B are copied into each child via fork's copy-on-write semantics.

## Stats Directory

Results are organized as `stats/<hostname>/<timestamp>/` with CSV files per implementation variant. Completed runs have a `.done` suffix on the directory name. The `checkpoint.log` inside each run tracks completed `<type>,<size>,<run>` combinations for resume support.
