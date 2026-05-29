# Simple Makefile for babyFEM C++17 FEM Solver
# Compile: make          (builds all examples)
# Run:     make run      (runs test_glace, default)
#          make run-beam (runs three_span_beam)
# Clean:   make clean

CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCDIR   := src
EXDIR    := examples
BINDIR   := bin

# Target executables
TARGET_GLACE := $(BINDIR)/test_glace
TARGET_BEAM  := $(BINDIR)/three_span_beam

# Source files and headers
HEADERS  := $(SRCDIR)/fem_2d.hpp $(SRCDIR)/matrix.hpp $(SRCDIR)/solver.hpp $(SRCDIR)/svg_generator.hpp

# Default target: build all examples
all: $(TARGET_GLACE) $(TARGET_BEAM)

# Create bin directory if needed
$(BINDIR):
	@mkdir -p $(BINDIR)

# Compile test_glace
$(TARGET_GLACE): $(EXDIR)/test_glace.cpp $(HEADERS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(EXDIR)/test_glace.cpp -o $(TARGET_GLACE)
	@echo "✓ Compilation successful: $(TARGET_GLACE)"

# Compile three_span_beam
$(TARGET_BEAM): $(EXDIR)/three_span_beam.cpp $(HEADERS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(EXDIR)/three_span_beam.cpp -o $(TARGET_BEAM)
	@echo "✓ Compilation successful: $(TARGET_BEAM)"

# Run the default example (test_glace)
run: $(TARGET_GLACE)
	@echo "Running test_glace..."
	@./$(TARGET_GLACE)

# Run the three_span_beam example
run-beam: $(TARGET_BEAM)
	@echo "Running three_span_beam..."
	@./$(TARGET_BEAM)

# Clean build artifacts
clean:
	@rm -rf $(BINDIR)
	@echo "✓ Cleaned"

# Rebuild from scratch
rebuild: clean all

.PHONY: all run run-beam clean rebuild
