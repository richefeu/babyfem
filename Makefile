CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCDIR   := src
HEADERS  := $(wildcard $(SRCDIR)/*.hpp)

# Outil principal : ./babyfem <cas.txt>
babyfem: babyfem.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. $< -o $@
	@echo "✓ $@"

clean:
	@rm -f babyfem
	@echo "✓ Cleaned"

.PHONY: clean
