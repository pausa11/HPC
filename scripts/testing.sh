#!/bin/bash

# Identify device and ensure stats directory exists
DEVICE_NAME=$(hostname)
STATS_DIR="../stats/$DEVICE_NAME"
mkdir -p "$STATS_DIR"

# Find the first unused filename in the format testing_Threads_N.csv within the device folder
COUNT=1
while [[ -f "$STATS_DIR/secuential_$COUNT.csv" ]]; do
  ((COUNT++))
done
OUTPUT_FILE="$STATS_DIR/secuential_$COUNT.csv"

# Ensure output directory exists for the executable (though it should already exist)
mkdir -p ../output

for j in {1..10}; do
  # Inner loop: runs ./output for each matrix size
  for i in 100 200 400; do 
    ../output/secuential $i $i >>"$OUTPUT_FILE"
  done

  echo "" >>"$OUTPUT_FILE" # Add a space after each i loop
done

echo "Listo! Los resultados se han guardado en $OUTPUT_FILE"

