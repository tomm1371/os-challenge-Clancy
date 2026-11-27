# Makefile for the OS Challenge server


# Change this variable to build a different experiment. 
# For example, to build the 01_FIFO experiment,
# run: EXP ?= 01_FIFO
EXP ?= 01_FIFO


# Name of the final executable
TARGET := server

# Source files and header files. All .c and .h files in the src/$(EXP) directory will be compiled.
SOURCES := $(wildcard src/$(EXP)/*.c)
HEADERS := $(wildcard src/$(EXP)/*.h)

# Compiler and flags. Add include directories with, for example,
#   CPPFLAGS += -Iinclude
# Add libraries with, for example, LDFLAGS += -lpthread
CC := gcc
CPPFLAGS :=
CFLAGS += -O3  
LDFLAGS += -lpthread
LDLIBS :=

OBJECTS := $(SOURCES:.c=.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# This generic rule builds one object file for each source file. Header
# dependencies are generated automatically by -MMD -MP and included below.
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJECTS): $(HEADERS)

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)

rebuild: clean all
