# Makefile for tinsh - Tiny Shell
# Compiles the shell with debugging symbols and warnings

CC = clang
CFLAGS = -Wall -Wextra -g -std=c99
TARGET = tinsh
SOURCES = main.c shell.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files into object files
%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Build and run the shell
run: $(TARGET)
	./$(TARGET)

# Install target (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall target (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Debug build with additional flags
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

.PHONY: all clean run install uninstall debug
