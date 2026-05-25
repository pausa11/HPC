#!/bin/bash

# Resolve the project root directory (one level up from scripts/)
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Identify device and find or create a run directory
DEVICE_NAME=$(hostname)
STATS_BASE="$ROOT_DIR/stats/$DEVICE_NAME"

# Resume the last incomplete run if it exists, otherwise start a new one
INCOMPLETE_DIR=$(find "$STATS_BASE" -maxdepth 1 -mindepth 1 -type d ! -name "*.done" 2>/dev/null | sort | tail -n 1)

if [ -n "$INCOMPLETE_DIR" ]; then
  STATS_DIR="$INCOMPLETE_DIR"
  echo "[RESUME] Reanudando corrida incompleta: $STATS_DIR"
else
  TIMESTAMP=$(date +%Y%m%d_%H%M%S)
  STATS_DIR="$STATS_BASE/$TIMESTAMP"
  echo "[NEW] Iniciando nueva corrida: $STATS_DIR"
fi

mkdir -p "$ROOT_DIR/output"
mkdir -p "$STATS_DIR"

sizes=(504 1008 1296 1608 2004 2304 2604 3000 3300 3600 4008)
num_proccesses=(2 4 6)

POINT_TO_POINT_FILE="$STATS_DIR/point_to_point_"
COLLECTIVE_FILE="$STATS_DIR/collective_"
SECUENTIAL_FILE="$STATS_DIR/secuential.csv"
CHECKPOINT_FILE="$STATS_DIR/checkpoint.log"

# Helper: check if a combination was already completed successfully
already_done() {
  local key="$1"
  grep -qF "$key" "$CHECKPOINT_FILE" 2>/dev/null
}

# Helper: mark a combination as completed
mark_done() {
  local key="$1"
  echo "$key" >> "$CHECKPOINT_FILE"
}

# Helper: run a benchmark safely, skipping if it crashes (non-zero exit or signal)
run_safe() {
  local key="$1"
  local output_file="$2"
  shift 2
  local cmd=("$@")

  if already_done "$key"; then
    echo "  [SKIP] $key (ya completado)"
    return
  fi

  # Run the command and capture exit code
  "${cmd[@]}" >> "$output_file"
  local exit_code=$?

  if [ $exit_code -eq 0 ]; then
    mark_done "$key"
  else
    echo "  [WARN] $key fallo con codigo $exit_code (segfault u otro error), continuando..."
  fi
}

# ─── COLLECTIVE ─────────────────────────────────────────────────────────
echo "Collective testing in process ..."

COUNT=0
for n in "${num_proccesses[@]}"; do
  echo "Collective for $n testing in process ..."
  for j in $(seq 1 10); do
    for i in "${sizes[@]}"; do
      key="collective,${i},n${n},run${j}"
      # OPT: Wrapped with mpirun -n "$n" so the MPI binary runs in parallel
      run_safe "$key" "${COLLECTIVE_FILE}${num_proccesses[$COUNT]}.csv" \
               mpirun -n "$n" "$ROOT_DIR/output/collective" "$i" "$n"
    done
    echo "" >> "${COLLECTIVE_FILE}${num_proccesses[$COUNT]}.csv"
  done
  ((COUNT++))
done

# Mark this run as fully completed so it won't be resumed again
mv "$STATS_DIR" "${STATS_DIR}.done"

echo "Listo! Los resultados se han guardado en: ${STATS_DIR}.done"