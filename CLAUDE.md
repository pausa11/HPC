# CLAUDE.md

Purpose: onboard the coding agent quickly so it can make correct changes in this HPC repo.

## WHY

This repository compares implementation strategies for three HPC problems and measures CPU time:

- `caso-estudio-1/`: matrix multiplication benchmark.
- `caso-estudio-2/`: additional case study (see `docs/enunciado.md` for scope).
- `Reto-1/`: 1D Jacobi solver benchmark.

Primary goal in most tasks: preserve correctness while improving or analyzing performance.

## WHAT

Top-level projects:

- `caso-estudio-1/`
- `caso-estudio-2/`
- `Reto-1/`

Each project typically contains:

- `src/`: C implementations (`secuential`, `secuentialFlags`, `memory`, `threads`, `multiprocessing`).
- `Makefile`: builds binaries into `output/`.
- `scripts/`: benchmark automation.
- `stats/`: CSV outputs from benchmark runs.
- `docs/`: reports, analysis, and helper scripts.

Common behavior across binaries: print only execution time in seconds to stdout.

## HOW (DEFAULT WORKFLOW)

When editing code:

1. Enter the relevant project folder (`caso-estudio-1/`, `caso-estudio-2/`, or `Reto-1/`).
2. Build with `make`.
3. Run only the variant(s) affected by the change.
4. If behavior or performance tooling changed, run the corresponding benchmark script.

Useful commands:

```bash
make
make clean
./output/secuential <args>
./output/memory <args>
./output/threads <args>
./output/multiprocessing <args>
```

Benchmark automation:

```bash
./scripts/RunAll.sh
```

## PROGRESSIVE DISCLOSURE (READ ON DEMAND)

Read only what is relevant to the current task:

- `caso-estudio-1/CLAUDE.md`: project-specific details for matrix multiplication.
- `caso-estudio-2/docs/enunciado.md`: problem statement and scope for the new case study.
- `Reto-1/docs/md/How2Run.md`: execution and usage details for Jacobi variants.
- `Reto-1/docs/md/Memory.md`: memory-optimized Jacobi strategy notes.
- `Reto-1/docs/md/Threads.md`: threading behavior and limitations in Jacobi case.

If a task needs deeper context, inspect source files in `src/` and the latest CSVs in `stats/` for the target project.

## GUARDRAILS

- Keep changes minimal and consistent with the existing C style in each project.
- Do not change CLI arguments or stdout format unless explicitly requested.
- Prefer deterministic verification (build + targeted run) over broad, expensive runs.
