CC = gcc
CFLAGS = -O2
SRC = src
OUT = output

all: $(OUT)/secuential $(OUT)/memory $(OUT)/threads $(OUT)/multiprocessing $(OUT)/secuentialFlags

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

clean:
	rm -rf $(OUT)
