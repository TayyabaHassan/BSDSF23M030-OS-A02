# =========================
# Makefile for ls project
# =========================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Target names
TARGET = $(BIN_DIR)/ls
SRC = $(SRC_DIR)/lsv1.2.0.c
OBJ = $(OBJ_DIR)/lsv1.2.0.o

# Default rule (build everything)
all: $(TARGET)

# Link object file into final binary
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source file into object file
$(OBJ): $(SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

# Phony targets (not real files)
.PHONY: all clean

