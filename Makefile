CXX = clang++
CXXFLAGS = -std=c++11 -Wall -O2
targets = main
.PHONY = clean

all: $(targets)

main: $(addprefix src/, main.cpp Parsers.hpp Dictionary.hpp)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(targets)
