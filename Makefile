CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic
LDFLAGS ?=
TARGET = tvi
DEBUG = tvi_debug
STATIC = tvi_static

SRC = src/terminal.c src/row.c src/syntax.c \
       src/syntax_parser.c src/movement.c src/editor.c \
       src/file.c src/find.c src/output.c src/input.c \
       src/undo.c src/init.c src/main.c
OBJS = $(SRC:.c=.o)

STATIC_CFLAGS ?= -Wall -Wextra -pedantic -static -no-pie
STATIC_LDFLAGS ?=

.PHONY: all clean debug static help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -s -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)
	$(CC) -s -o t/docrc t/docrc.c

%.o: src/%.c src/tvi.h
	$(CC) -c -o $@ $< $(CFLAGS)

debug: $(DEBUG)

$(DEBUG): $(SRC) src/tvi.h
	$(CC) -DDEBUG -g -O0 -o $@ $(SRC)
	$(CC) -s -o t/docrc t/docrc.c

static: $(STATIC)

$(STATIC): $(SRC) tvi.h
	$(CC) -s -o $@ $(SRC) $(STATIC_CFLAGS) $(STATIC_LDFLAGS)
	$(CC) -s -static -o t/docrc t/docrc.c

musl-static:
	@echo "Building static binary with musl..."
	@if command -v musl-gcc >/dev/null 2>&1; then \
		musl-gcc -o $(STATIC) $(SRC) -s -static -Wall -Wextra -pedantic; \
		musl-gcc -s -static -o t/docrc t/docrc.c; \
		echo "Built $(STATIC) with musl-gcc"; \
	else \
		echo "musl-gcc not found. Using standard gcc with -static -no-pie"; \
		$(CC) -o $(STATIC) $(SRC) -s -static -no-pie -Wall -Wextra -pedantic; \
		$(CC) -s -static -no-pie -o t/docrc t/docrc.c; \
	fi

clean:
	rm -f $(TARGET) $(DEBUG) $(STATIC) $(OBJS)

help:
	@echo "tvi - tiny Vim-like Editor"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build standard binary (default)"
	@echo "  debug         - Build debug binary with symbols"
	@echo "  static        - Build static binary"
	@echo "  alpine-static - Build static binary for Alpine Linux"
	@echo "  musl-static   - Build static binary with musl libc"
	@echo "  clean         - Remove built binaries and test files"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CC=$(CC)               - C compiler (default: gcc)"
	@echo "  CFLAGS=$(CFLAGS)       - Compiler flags"
	@echo "  STATIC_CFLAGS=$(STATIC_CFLAGS) - Static build flags"
	@echo ""
	@echo "Examples:"
	@echo "  make                           # Build standard binary"
	@echo "  make debug                     # Build with debug symbols"
	@echo "  make static                    # Build static binary"
	@echo "  make musl-static               # Build with musl libc"
	@echo "  make static CC=clang           # With clang"
	@echo "  make clean                     # Clean up"

