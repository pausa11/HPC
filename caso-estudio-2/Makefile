CC = gcc
CFLAGS = -O2
SRC = src
OUT = output

# Detect OpenMP flags. On Linux/gcc, plain -fopenmp works. On macOS with Apple
# clang (which is invoked when 'gcc' is the system stub), -fopenmp is unsupported
# and we need -Xpreprocessor -fopenmp -lomp plus the include/lib paths from
# Homebrew's libomp keg-only formula.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LIBOMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
  OMP_FLAGS := -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include -L$(LIBOMP_PREFIX)/lib -lomp
else
  OMP_FLAGS := -fopenmp
endif

all: $(OUT)/secuential $(OUT)/secuentialFlags $(OUT)/memory $(OUT)/threads \
     $(OUT)/multiprocessing $(OUT)/openmp $(OUT)/openmp_memory

$(OUT):
	mkdir -p $(OUT)

$(OUT)/secuential: $(SRC)/SecuentialMatrixSolver.c | $(OUT)
	$(CC) -o $@ $<

$(OUT)/secuentialFlags: $(SRC)/SecuentialMatrixSolver.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/memory: $(SRC)/MemoryMatrixSolver.c | $(OUT)
	$(CC) -o $@ $<

$(OUT)/threads: $(SRC)/ThreadsMatrixSolver.c | $(OUT)
	$(CC) -pthread -o $@ $<

$(OUT)/multiprocessing: $(SRC)/MultiprocessingMatrixSolver.c | $(OUT)
	$(CC) -o $@ $<

$(OUT)/openmp: $(SRC)/OpenMPMatrixSolver.c | $(OUT)
	$(CC) $(OMP_FLAGS) -o $@ $<

$(OUT)/openmp_memory: $(SRC)/OpenMPMemoryMatrixSolver.c | $(OUT)
	$(CC) $(OMP_FLAGS) -o $@ $<

clean:
	rm -rf $(OUT)
