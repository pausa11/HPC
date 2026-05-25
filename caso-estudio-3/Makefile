MPI = mpicc
CC = gcc
SRC = src
OUT = output

all: $(OUT)/point_to_point $(OUT)/secuential

$(OUT):
	mkdir -p $(OUT)

$(OUT)/secuential: $(SRC)/SecuentialMatrixSolver.c | $(OUT)
	$(CC) -o $@ $<

$(OUT)/point_to_point: $(SRC)/PointToPoint.c | $(OUT)
	$(MPI) -o $@ -O2 $<

clean:
	rm -rf $(OUT)
