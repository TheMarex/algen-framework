# g++ >= 4.9 or clang++ >= 3.4 is required to build this.
# clang++ 3.4 and 3.5 cannot build the compare target with debug information.
# Either remove "-g" from the flags or use clang++ >= 3.6 (or g++).
CX ?= clang++-3.6
CC ?= gcc

SANITIZER ?= address

COMMONFLAGS = -std=c++1y -Wall -Werror
CFLAGS = ${COMMONFLAGS} -Ofast -g -DNDEBUG
DEBUGFLAGS = ${COMMONFLAGS} -O0 -ggdb3
LDFLAGS = -lpapi -lboost_serialization
MALLOC_LDFLAGS = -ldl

all: bench_tmp

clean:
	rm -f *.o bench_tmp bench_tmp_malloc debug sanitize

malloc_count.o: malloc_count/malloc_count.c  malloc_count/malloc_count.h
	$(CC) -O2 -Wall -Werror -g -c -o $@ $<

bench_tmp: bench_tmp.cpp */*.h
	$(CX) $(CFLAGS) -o $@ $< $(LDFLAGS)

bench_tmp_malloc: bench_tmp.cpp */*.h malloc_count.o
	$(CX) $(CFLAGS) -DMALLOC_INSTR -o $@ $< malloc_count.o $(LDFLAGS) $(MALLOC_LDFLAGS)

debug: bench_tmp.cpp */*.h
	$(CX) $(DEBUGFLAGS) -o $@ $< $(LDFLAGS)

debug_malloc: bench_tmp.cpp */*.h malloc_count.o
	$(CX) $(DEBUGFLAGS) -DMALLOC_INSTR -o $@ $< malloc_count.o $(LDFLAGS) $(MALLOC_LDFLAGS)

sanitize: bench_tmp.cpp */*.h
	$(CX) $(CFLAGS) -fsanitize=${SANITIZER} -o $@ $< $(LDFLAGS)
	./$@

compare: compare.cpp */*.h
	$(CX) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: bench_tmp
	./bench_tmp

run_malloc: bench_tmp_malloc
	./bench_tmp_malloc
