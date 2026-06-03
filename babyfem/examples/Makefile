CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCDIR   := ../src

SRCS    := $(wildcard *.cpp)
TARGETS := $(patsubst %.cpp,%,$(SRCS))

all: $(TARGETS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -I../../core $< -o $@
	@echo "✓ $@"

clean:
	@rm -f $(TARGETS)
	@echo "✓ Cleaned"

clean+: clean
	@rm -f *.svg
	@echo "✓ SVG files removed"

.PHONY: all clean clean+
