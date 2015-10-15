CC = clang++
CFLAGS = -std=c++11 -lreadline -Wall
.PHONY = all clean

all: main

main: main.cpp Parsers.hpp
	$(CC) $(CFLAGS) -o main main.cpp

clean:
	rm main
