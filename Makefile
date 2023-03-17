# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

.PHONY = all clean

CC = gcc # compiler to use

FLAGS := -O2 -Wall -fopenmp -lm -lstdc++ --std=c++17 -D_GLIBCXX_PARALLEL 

SRCS := main.cpp tensor.cpp reduction.cpp util.cpp
BINS := featen

EXEC_NAME := featen

all: featen

test: featen
	cd testing && \
	python3 test.py

featen: $(SRCS)
	@echo "Compiling.."
	$(CC) $(FLAGS) $(SRCS) -o $(EXEC_NAME) -lm -lstdc++

debug: 
	@echo "Compiling in debug mode.."
	$(CC) $(FLAGS) $(SRCS) -o $(EXEC_NAME) -g -lm -lstdc++

clean:
		@echo "Cleaning up..."
		rm -rvf $(EXEC_NAME)