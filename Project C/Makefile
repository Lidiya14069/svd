CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11

COMMON_SRC = image/image.c \
             JPEG/dct.c \
             JPEG/jpeg.c \
             matrix/matrix.c \
             metrics/metrics.c \
             SVD/jacobi.c \
             SVD/svd.c \
             utils/utils.c

PROGRAM_SRC = main.c $(COMMON_SRC)
TEST_SRC = tests/test_svd_project.c $(COMMON_SRC)

all: program

program: $(PROGRAM_SRC)
	$(CC) $(CFLAGS) $(PROGRAM_SRC) -o $@ -lm

test_program: $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $@ -lm

test: test_program
	./test_program

run: program
	./program

clean:
	rm -f program test_program
	rm -f output/*.bmp
	rm -f output/*.csv
