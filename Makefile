CFLAGS=-O3 -g -std=gnu99 -Wno-sign-compare -Wextra

.PHONY: all clean
all: sudoku

sudoku: sudoku.o

clean:
	$(RM) sudoku.o sudoku
