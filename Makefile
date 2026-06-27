# Simple build. Run `make` to compile, `make run` to build+run, `make clean` to remove output.
CXX      = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRC      = $(wildcard src/*.cpp)

poker: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o poker

run: poker
	./poker

clean:
	rm -f poker
