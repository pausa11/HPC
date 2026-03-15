#!/bin/bash

# Identify device and ensure stats directory exists
DEVICE_NAME=$(hostname)
STATS_DIR="./stats/$DEVICE_NAME"

# Ensure output directory exists for the executable (though it should already exist)
mkdir -p ./output
mkdir -p "$STATS_DIR"

sizes=(200 400 800 1000)
num_proccesses=(2 4) # Please check if the machine has 16 cores to run this

SECUENTIAL_FILE="$STATS_DIR/secuential.csv"

MEMORY_FILE="$STATS_DIR/memory.csv"

THREAD_FILE="$STATS_DIR/threads" # Dont add csv to this ones beacuse they will have a number at the end to indicate the num of process

MULTIPROCESSING_FILE="$STATS_DIR/mutiprocessing"

# SECUENTIAL TESTING LOOP
echo "Secuential testing in process ..."

for j in {1..10}; do
  # Inner loop: runs ./output for each matrix size
  for i in "${sizes[@]}"; do
    ./output/secuential $i >>"$SECUENTIAL_FILE"
  done
  echo "" >>"$SECUENTIAL_FILE" # Add a space after each i loop
done

# MEMORY TESTING LOOP
echo "Memory testing in process ..."

for j in {1..10}; do
  for i in "${sizes[@]}"; do
    ./output/memory $i >>"$MEMORY_FILE"
  done
  echo "" >>"$MEMORY_FILE" # Add a space after each i loop
done

# THREAD TESTING LOOP
echo "Threads testing in process ..."

COUNT=0
for n in "${num_proccesses[@]}"; do
  echo "Threads for $n testing in process ..."
  for j in {1..10}; do
    for i in "${sizes[@]}"; do
      ./output/threads $i $n >>"$THREAD_FILE${num_proccesses[$COUNT]}.csv"
    done
    echo "" >>"$THREAD_FILE${num_proccesses[$COUNT]}.csv" # Add a space after each i loop
  done
  ((COUNT++))
done

# MULTIPROCESSING TESTING LOOP
echo "Multiprocessing testing in process ..."

COUNT=0
for n in "${num_proccesses[@]}"; do
  echo "Multiprocessing for $n testing in process ..."
  for j in {1..10}; do
    for i in "${sizes[@]}"; do
      ./output/multiprocessing $i $n >>"$MULTIPROCESSING_FILE${num_proccesses[$COUNT]}.csv"
    done
    echo "" >>"$MULTIPROCESSING_FILE${num_proccesses[$COUNT]}.csv" # Add a space after each i loop
  done
  ((COUNT++))
done

echo "Listo! Los resultados se han guardado exitosamente (creo, tenga fe parcero)"
