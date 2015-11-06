CC = clang++
CFLAGS = -std=c++11 -Wall
.PHONY = all clean

all: main

main: main.cpp Parsers.hpp Dictionary.hpp
	$(CC) $(CFLAGS) -o main main.cpp

clean:
	rm main
