.PHONY: all clean install debug

# Compiler and flags
COMPILER = gcc
COMPILER_FLAGS = -pedantic -std=c11 -Wall -Wextra
RELEASE_FLAGS = -O2 -DNDEBUG
DEBUG_FLAGS = -Og -g3 -D_DEBUG

# Directories and files
SOURCE_FILES = $(wildcard src/*.c)
OBJECT_FILES = $(SOURCE_FILES:src/%.c=%.o)

# Default target
all: release/nino

# Release build
release/nino: $(addprefix release/, $(OBJECT_FILES))
	@mkdir -p release
	$(COMPILER) $(COMPILER_FLAGS) $(RELEASE_FLAGS) -o $@ $^

release/%.o: src/%.c
	@mkdir -p release
	$(COMPILER) -c $(COMPILER_FLAGS) $(RELEASE_FLAGS) -o $@ $<

# Debug build
debug: debug/nino

debug/nino: $(addprefix debug/, $(OBJECT_FILES))
	@mkdir -p debug
	$(COMPILER) $(COMPILER_FLAGS) $(DEBUG_FLAGS) -o $@ $^

debug/%.o: src/%.c
	@mkdir -p debug
	$(COMPILER) -c $(COMPILER_FLAGS) $(DEBUG_FLAGS) -o $@ $<

# Clean target
clean:
	rm -rf release debug

# Install target
install: release/nino
	install -D release/nino /usr/local/bin/nino
