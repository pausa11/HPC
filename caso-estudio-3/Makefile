MPI = mpicc
SRC = src
OUT = output

all: $(OUT)/point_to_point

$(OUT):
	mkdir -p $(OUT)

$(OUT)/point_to_point: $(SRC)/PointToPoint.c | $(OUT)
	$(MPI) -o $@ $<

clean:
	rm -rf $(OUT)
