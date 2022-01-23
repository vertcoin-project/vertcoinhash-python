CFLAGS=-O3 -Wall -Wextra

all: h1 h2

h1:
	gcc $(CFLAGS) h1.c tiny_sha3/sha3.c -o create-verthash-datafile

h2:
	gcc $(CFLAGS) h2.c tiny_sha3/sha3.c -o verify-verthash-datafile

clean:
	@echo "Cleaning up..."
	rm -f create-verthash-datafile* verify-verthash-datafile* *.res
