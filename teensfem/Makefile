CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
HEADERS  := $(wildcard src/*.hpp) $(wildcard ../core/*.hpp)

# Outil teensfem : ./teensfem <cas.txt>  (structures à barres/poutres)
teensfem: teensfem.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. -Isrc -I../core $< -o $@
	@echo "✓ $@"

clean:
	@rm -f teensfem
	@echo "✓ Cleaned"

.PHONY: clean
