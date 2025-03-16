.PHONY: all prep release debug clean format install

# Commands
mkdir = mkdir -p $(1)
rm = rm $(1) > /dev/null 2>&1 || true

# Compiler flags
CC = gcc
CFLAGS = -pedantic -std=c11 -Wall -Wextra

# Project files
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, %.o, $(SOURCES))
DEPS = $(patsubst $(SRCDIR)/%.c, %.d, $(SOURCES))
EXE = nino

# Resources
RESOURCE_DIR = resources
SYNTAX_FILES= $(wildcard $(RESOURCE_DIR)/syntax/*.json)
BUNDLER_SRC = $(RESOURCE_DIR)/bundler.c
BUNDLER = $(RESOURCE_DIR)/bundler
BUNDLE = $(RESOURCE_DIR)/bundle.h

# Install settings
prefix ?= /usr/local
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin

INSTALL = install

# Build settings
RELDIR = release
DBGDIR = debug
RELEXE = $(RELDIR)/$(EXE)
DBGEXE = $(DBGDIR)/$(EXE)
RELCFLAGS = -O2 -DNDEBUG
DBGCFLAGS = -Og -g3 -D_DEBUG

# Default target
all: prep release

$(BUNDLER) : $(BUNDLER_SRC)
	$(CC) -s -o $@ $<

$(BUNDLE) : $(BUNDLER)
	$(BUNDLER) $(BUNDLE) $(SYNTAX_FILES)

# Release build
release: $(RELEXE)
$(RELEXE): $(addprefix $(RELDIR)/, $(OBJS))
	$(CC) $(CFLAGS) $(RELCFLAGS) -s -o $@ $^
$(RELDIR)/%.o: $(SRCDIR)/%.c $(BUNDLE)
	$(CC) -c -MMD $(CFLAGS) $(RELCFLAGS) -o $@ $<

# Debug build
debug: $(DBGEXE)
$(DBGEXE): $(addprefix $(DBGDIR)/, $(OBJS))
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $@ $^
$(DBGDIR)/%.o: $(SRCDIR)/%.c $(BUNDLE)
	$(CC) -c -MMD $(CFLAGS) $(DBGCFLAGS) -o $@ $<

-include $(addprefix $(RELDIR)/, $(DEPS)) $(addprefix $(DBGDIR)/, $(DEPS))

# Prepare
prep:
	@$(call mkdir, $(RELDIR))
	@$(call mkdir, $(DBGDIR))

# Clean target
clean:
	$(call rm, $(RELEXE) $(DBGEXE) $(addprefix $(RELDIR)/, $(OBJS) $(DEPS)) $(addprefix $(DBGDIR)/, $(OBJS) $(DEPS)) $(BUNDLER) $(BUNDLE))

# Format all files
format:
	clang-format -i $(SRCDIR)/*.h $(SRCDIR)/*.c

# Install target
install:
	$(INSTALL) $(RELEXE) $(DESTDIR)$(bindir)/$(EXE)

