
# Name of the final executable. Change this if your program should have a
# different name.
TARGET := server

# Add every .c file used by the program to this list. For example, when you
# split server.c into server.c, socket.c, and client.c, use:
#   SOURCES := server.c socket.c client.c
SOURCES := server.c 
HEADERS := lonesha256.h messages.h

# Compiler and flags. Add include directories with, for example,
#   CPPFLAGS += -Iinclude
# Add libraries with, for example, LDFLAGS += -lpthread
CC := gcc
CPPFLAGS :=
CFLAGS :=
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
