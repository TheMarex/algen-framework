CXX ?= g++

CFLAGS = -std=c++11 -g -Wall -Wextra -Werror -I..

# This is where the test files go
#SRC = maybe.cpp \
#      unordered_map.cpp \
#      pairing_heap.cpp
SRC = pairing_heap.cpp
HEADER = ../pq/addressable_pairing_heap.hpp

BUILDDIR ?= build

OBJ = $(patsubst %.cpp,${BUILDDIR}/%.o,$(SRC))

.PHONY: directories

all: compile

compile: directories tests

directories: ${BUILDDIR}

build:
	mkdir -p ${BUILDDIR}

${BUILDDIR}/%.o: %.cpp build $(HEADER)
	$(CXX) $(CFLAGS) -c $< -o $@

tests: $(OBJ)
	$(CXX) $(CFLAGS) -o tests tests.cpp $^

clean:
	rm -rf ${BUILDDIR} tests

run: compile
	./tests

gdb: compile
	gdb --args ./tests --break
