
CC = gcc
CFLAGS = -O2
SRC = src
OUT = output

all: $(OUT)/secuential $(OUT)/memory $(OUT)/threads $(OUT)/multiprocessing $(OUT)/secuentialFlags

$(OUT):
	mkdir -p $(OUT)

$(OUT)/secuential: $(SRC)/JacobiSecuential.c | $(OUT)
	$(CC) -Wall -O0 -o $@ $< -lm

$(OUT)/secuentialFlags: $(SRC)/JacobiSecuentialFlags.c | $(OUT)
	$(CC) $(CFLAGS) -Wall -o $@ $< -lm

$(OUT)/memory: $(SRC)/JacobiMemory.c | $(OUT)
	$(CC) -Wall -O0 -o $@ $< -lm

$(OUT)/threads: $(SRC)/JacobiThreads.c | $(OUT)
	$(CC) -Wall -O0 -o $@ $< -lpthread -lm

$(OUT)/multiprocessing: $(SRC)/JacobiMultiprocessing.c | $(OUT)
	$(CC) -Wall -O0 -o $@ $< -lm

clean:
	rm -rf $(OUT)
