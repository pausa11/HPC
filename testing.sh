#!/bin/bash

OUTPUT_FILE=testing.doc
# Clear the output file before starting (so we don't append to old results)
>OUTPUT_FILE

for j in {1..10}; do
  # Inner loop: runs ./mul for each matrix size
  for i in 10 100; do # FOr the time just this to test, i dont want to explote my pc
    ./output $i $i >>times2.doc
  done

done

echo "Listo xd"
