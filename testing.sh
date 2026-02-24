#!/bin/bash

# Identify device and ensure stats directory exists
DEVICE_NAME=$(hostname)
STATS_DIR="stats/$DEVICE_NAME"
mkdir -p "$STATS_DIR"

# Find the first unused filename in the format testing_N.doc within the device folder
COUNT=1
while [[ -f "$STATS_DIR/testing_$COUNT.doc" ]]; do
  ((COUNT++))
done
OUTPUT_FILE="$STATS_DIR/testing_$COUNT.doc"

for j in {1..10}; do
  # Inner loop: runs ./output for each matrix size
  for i in 200 400 600 800 1000 1200 1400 1600 1800 2000 2200 2400 2600 2800 3000 3200 3400 ; do # For the time just this to test, i don't want to explode my pc
    ./output $i $i >> "$OUTPUT_FILE"
  done

  echo "" >> "$OUTPUT_FILE" # Add a space after each i loop
done

echo "Listo! Los resultados se han guardado en $OUTPUT_FILE"
