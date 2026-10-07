# ---------------------------------------------------------------------------
#   make          build everything
#   make run      build + run the demo           (./poker)
#   make test     build + run the evaluator test (./test_evaluator)
#   make clean    delete the built programs
# ---------------------------------------------------------------------------
CXX      = c++
# -Isrc lets files in apps/ and tests/ write #include "cards.h" without a path.
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc

LIB_SRC  = $(wildcard src/*.cpp)   # the engine, shared by every program
HEADERS  = $(wildcard src/*.h)     # listed as dependencies so header edits trigger a rebuild

all: poker test_evaluator

poker: apps/main.cpp $(LIB_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) apps/main.cpp $(LIB_SRC) -o $@

test_evaluator: tests/test_evaluator.cpp $(LIB_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/test_evaluator.cpp $(LIB_SRC) -o $@

run: poker
	./poker

test: test_evaluator
	./test_evaluator

clean:
	rm -f poker test_evaluator

# These targets are commands, not files, so make shouldn't look for files with these names.
.PHONY: all run test clean
