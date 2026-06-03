CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
HEADERS  := $(wildcard src/*.hpp) $(wildcard ../core/*.hpp)

# Outil kidfem : ./kidfem <cas.txt>  (maillage triangulaire gmsh)
kidfem: kidfem.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. -Isrc -I../core $< -o $@
	@echo "✓ $@"

clean:
	@rm -f kidfem
	@echo "✓ Cleaned"

.PHONY: clean
