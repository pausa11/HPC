
# TO compile all the files

## To compile secuential

```
gcc src/SecuentialMatrixSolver.c -o output/secuentail
```

## To compile memory

```
gcc src/MemoryMatrixSolver.c -O3 -o output/memory
```

## To compile threads

```
gcc src/ThreadsMatrixSolver.c -o output/threads -lpthread
```

#### Command to see the threads of the process in Linux
```
top -H -p $(pgrep output)
```
## To compile multiprocessing

```
gcc src/MultiprocessingMatrixSolver.c -o output/multiprocessing
```

# TO run the script
```
./scripts/RunAll.sh
```