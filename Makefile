CXX = clang++
CXXFLAGS = -std=c++11 -Wall -O2
targets = ULC
objs = src/ExpressionParser.o
.PHONY = clean

all: $(targets)

ULC: $(addprefix src/, main.cpp Dictionary.hpp) $(objs)
	$(CXX) $(CXXFLAGS) $< $(objs) -o $@

src/ExpressionParser.o: $(addprefix src/, ExpressionParser.cpp ExpressionParser.hpp Parsers.hpp)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(targets) $(objs)

