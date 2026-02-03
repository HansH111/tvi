# version, cflags and ldflags can be set from external
VERSION ?= 1.0.2
CFLAGS ?= -s -Wall -Wextra -pedantic
LDFLAGS ?= -s
#
# Compiler
CC ?= gcc
#
# include directory
INCL = -Iinclude

# Source and Object Files
SRC = src/terminal.c src/row.c src/syntax.c \
	src/syntax_parser.c src/movement.c src/editor.c \
	src/file.c src/find.c src/output.c src/input.c \
	src/undo.c src/init.c src/main.c src/cmd.c src/search.c \
	src/config.c
OBJ = $(SRC:.c=.o)

# Target Executable
TARGET = tvi

# Make Rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -Wl,--build-id=none -o $@ $^ $(LDFLAGS)
	$(CC) -s -o t/docrc t/docrc.c

%.o: %.c
	$(CC) $(CFLAGS) $(INCL) -DVERSION=\"$(VERSION)\" -c $< -o $@

# Run tests
check: t/docrc
	cd t && ./runtests.sh all

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all tvi check clean

