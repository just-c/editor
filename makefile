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

# Release build settings
RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELDEPS = $(addprefix $(RELDIR)/, $(DEPS))
RELCFLAGS = -O2 -DNDEBUG

# Debug build settings
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGDEPS = $(addprefix $(DBGDIR)/, $(DEPS))
# -fsanitize=address,undefined
DBGCFLAGS = -Og -g3 -D_DEBUG

# Default target
all: prep release

$(BUNDLER) : $(BUNDLER_SRC)
	$(CC) -s -o $@ $<

$(BUNDLE) : $(BUNDLER)
	$(BUNDLER) $(BUNDLE) $(SYNTAX_FILES)

# Release build
release: $(RELEXE)
$(RELEXE): $(RELOBJS)
	$(CC) $(CFLAGS) $(RELCFLAGS) -s -o $(RELEXE) $^
$(RELDIR)/%.o: $(SRCDIR)/%.c $(BUNDLE)
	$(CC) -c -MMD $(CFLAGS) $(RELCFLAGS) -o $@ $<

# Debug build
debug: $(DBGEXE)
$(DBGEXE): $(DBGOBJS)
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $^
$(DBGDIR)/%.o: $(SRCDIR)/%.c $(BUNDLE)
	$(CC) -c -MMD $(CFLAGS) $(DBGCFLAGS) -o $@ $<

-include $(RELDEPS) $(DBGDEPS)

# Prepare
prep:
	@$(call mkdir, $(RELDIR))
	@$(call mkdir, $(DBGDIR))

# Clean target
clean:
	$(call rm, $(RELEXE) $(RELDEPS) $(RELOBJS) $(DBGEXE) $(DBGDEPS) $(DBGOBJS) $(BUNDLER) $(BUNDLE))

# Format all files
format:
	clang-format -i $(SRCDIR)/*.h $(SRCDIR)/*.c

# Install target
install:
	$(INSTALL) $(RELEXE) $(DESTDIR)$(bindir)/$(EXE)

