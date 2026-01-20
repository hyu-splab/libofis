CXX = g++
CC = gcc

# Compiler flags
CXXFLAGS = -std=c++1y -g -pthread 
# CXXFLAGS += -fopenmp
CFLAGS = -std=gnu11 -Wall -Wextra -pthread
CFLAGS += -g
# CFLAGS += -fopenmp

# Source files
SRCS_CPP = make_BUF.cpp
# SRCS_C = main_single.c main_role.c
SRCS_C = CPU_baseline.c

# Object files
OBJS_CPP = $(SRCS_CPP:.cpp=.o)
OBJS_C = $(SRCS_C:.c=.o)

COMMON_OBJ = make_BUF.o

# Targets
TARGETS = CPU_baseline

.PHONY: all clean re single mutex mutex_time spin atomic role role_nolock mutex_text

# Default target
all: $(TARGETS)

CPU_baseline: CPU_baseline.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(OBJS_CPP) CPU_baseline.o -o $@ -lnuma

# Generic rule to compile C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generic rule to compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS_CPP) $(OBJS_C) $(TARGETS)

# Rebuild target
re: clean all
