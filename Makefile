
# Name of the final executable
TARGET := server

# aource files and header files. Add new source files here.
SOURCES := src/server.c src/brute.c
HEADERS := src/lonesha256.h src/messages.h src/brute.h

# Compiler and flags. Add include directories with, for example,
#   CPPFLAGS += -Iinclude
# Add libraries with, for example, LDFLAGS += -lpthread
CC := gcc
CPPFLAGS :=
CFLAGS += -O3
LDFLAGS :=
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
