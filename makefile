.PHONY: all clean install debug

# Compiler and flags
COMPILER = gcc
COMPILER_FLAGS = -pedantic -std=c11 -Wall -Wextra
RELEASE_FLAGS = -O2 -DNDEBUG
DEBUG_FLAGS = -Og -g3 -D_DEBUG

# Directories and files
SOURCE_FILES = $(wildcard src/*.c)
OBJECT_FILES = $(SOURCE_FILES:src/%.c=%.o)
SYNTAX_FILES = $(wildcard resources/syntax/*.json)

# Default target
all: release/nino

# Release build
release/nino: resources/bundle.h $(addprefix release/, $(OBJECT_FILES))
	$(COMPILER) $(COMPILER_FLAGS) $(RELEASE_FLAGS) -o $@ $^

release/%.o: src/%.c
	mkdir -p release
	$(COMPILER) -c $(COMPILER_FLAGS) $(RELEASE_FLAGS) -o $@ $<

# Debug build
debug: debug/nino

debug/nino: resources/bundle.h $(addprefix debug/, $(OBJECT_FILES))
	$(COMPILER) $(COMPILER_FLAGS) $(DEBUG_FLAGS) -o $@ $^

debug/%.o: src/%.c
	mkdir -p debug
	$(COMPILER) -c $(COMPILER_FLAGS) $(DEBUG_FLAGS) -o $@ $<

# Bundle generation
resources/bundle.h: resources/bundler
	resources/bundler resources/bundle.h $(SYNTAX_FILES)

resources/bundler: resources/bundler.c
	$(COMPILER) -o $@ $<

# Clean target
clean:
	rm -rf release debug resources/bundler resources/bundle.h

# Install target
install: release/nino
	install -D release/nino /usr/local/bin/nino
