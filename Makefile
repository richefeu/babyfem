# Simple Makefile for babyFEM C++17 FEM Solver
# Compile: make
# Run:     make run
# Clean:   make clean

CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCDIR   := src
EXDIR    := examples
BINDIR   := bin

# Target executable
TARGET   := $(BINDIR)/test_glace

# Source files
SOURCES  := $(EXDIR)/test_glace.cpp
HEADERS  := $(SRCDIR)/fem_2d.hpp $(SRCDIR)/matrix.hpp $(SRCDIR)/solver.hpp $(SRCDIR)/svg_generator.hpp

# Default target
all: $(TARGET)

# Create bin directory if needed
$(BINDIR):
	@mkdir -p $(BINDIR)

# Compile
$(TARGET): $(SOURCES) $(HEADERS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(SOURCES) -o $(TARGET)
	@echo "✓ Compilation successful: $(TARGET)"

# Run the example
run: $(TARGET)
	@echo "Running FEM solver..."
	@./$(TARGET)

# Clean build artifacts
clean:
	@rm -rf $(BINDIR)
	@echo "✓ Cleaned"

# Rebuild from scratch
rebuild: clean all

.PHONY: all run clean rebuild
