EXTRA_CXXFLAGS=
CXXFLAGS=-O3 -Wall -std=c++20 $(EXTRA_CXXFLAGS)

all: uvid_compress uvid_decompress

uvid_compress: uvid_compress.o
	g++ -o $@ $^

uvid_decompress: uvid_decompress.o
	g++ -o $@ $^



clean:
	rm -f uvid_compress uvid_decompress *.o
