#!/bin/bash

OUTPUT_FILE=testing.doc
# Clear the output file before starting (so we don't append to old results)
>OUTPUT_FILE

for j in {1..10}; do
  # Inner loop: runs ./output for each matrix size
  for i in 200 400 600 800 1000 1200 1400 1600 1800 2000 2200 2400 2600 2800 3000 3200 3400; do # FOr the time just this to test, i dont want to explote my pc
    ./output $i $i >>$OUTPUT_FILE
  done

  echo "" >>$OUTPUT_FILE #Add and space after each i loop

done

echo "Listo xd"
