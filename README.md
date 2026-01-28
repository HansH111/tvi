# TVI - Tiny Vim-like Editor

TVI is a small, fast terminal-based text editor inspired by Vim. It's based on the [kilo](https://github.com/antirez/kilo) text editor, but implements basic Vi/Vim-like functionality instead of Emacs-style editing.

## Features

### Core Editing
- **Vi-compatible modes**: Normal, Insert and Command-line modes
- **Basic Vi commands**: Navigation (h,j,k,l,w,b,0,$), editing (i,a,o,O,x,d,c,y,p), search (/), etc.
- **Line-based operations**: dd, yy, p, etc.
- **Word movements**: w, b, e for word navigation
- **Undo/Redo**: Full undo (`u`) and redo (`Ctrl+R`) support with logical operation tracking

### Syntax Highlighting
- **Dynamic syntax loading**: Syntax files loaded from `~/.config/tvi/syntax.{language ext}`
- **Supported languages**: C, C++, Python, JavaScript, Rust, Perl
- **256-color support**: Full terminal color support
- **Customizable**: Easy to add new languages or modify existing syntax

### Terminal Integration
- **Raw mode handling**: Proper terminal raw mode setup/cleanup
- **Window resizing**: Handles terminal resize events
- **Alternate screen**: Uses alternate screen buffer for clean UI
- **Escape sequences**: Full ANSI escape sequence support

### File Handling
- **File operations**: Open, save, save-as
- **Multiple file types**: Automatic syntax detection by extension
- **File status**: Shows modified status, file name, cursor position

## Installation

### Build Requirements
- C compiler (gcc recommended)
- Make
- POSIX-compliant system

### Building
```bash
make all        # Build optimized binary
make debug      # Build with debug symbols
make static     # Build static binary
make help       # show help screen
```

### Installation
```bash
# copy tvi binary to your PATH manually
```

## Usage

### Basic Usage
```bash
tvi                    # Open new file
tvi filename.c         # Open existing file
```

### Vi Modes
- **Normal mode** (default): Navigation and commands
- **Insert mode** (`i`, `a`, `o`): Text insertion
- **Command mode** (`:`): File operations and settings

### Key Bindings

#### Navigation (Normal Mode)
- `h` `j` `k` `l` - Move cursor left/down/up/right
- `w` / `b` - Next/previous word
- `0` / `$` - Beginning/end of line
- `gg` / `G` - Beginning/end of file
- `Ctrl+U` / `Ctrl+D` - Page up/down

#### Editing (Normal Mode)
- `i` / `a` - Enter insert mode before/after cursor
- `o` / `O` - Open new line below/above
- `x` - Delete character under cursor
- `d` + motion - Delete text (dd for whole line)
- `c` + motion - Change text (cc for whole line)
- `y` + motion - Yank (copy) text (yy for whole line)
- `p` / `P` - Paste after/before cursor
- `u` - Undo last change
- `Ctrl+R` - Redo last undone change

#### File Operations
- `:w` - Write file
- `:wq` - Write and quit
- `:q` - Quit
- `:q!` - Force quit without saving

#### Search
- `/pattern` - Search forward
- `?pattern` - Search backward
- `n` / `N` - Next/previous match

## Configuration

### Syntax Files

TVI loads syntax highlighting from files in `~/.config/tvi/syntax{.ext}{.ext}`:
example:  syntax.c.h

```bash
# C syntax highlighting example
# --- 256-color codes for: normal, kw1, kw2, comment/mlcomment, string, number, match, notprintable
colors256  : 249 117 48 73 73 246 246 195 196
# --- keywords can be repeated, color with end on | = kw2 and without =kw1
keywords  : switch if while for break continue return else case default
keywords  : int| long| double| float| char| unsigned| signed| void| short| struct|
# --- single-line remark, and/or multiline remarks
remark     : //
remarkstart: /*
remarkend  : */
# --- highlighting number, strings and searchmatch
numbers
strings
searchmatch
```

### Color Codes
The `colors256` field uses 256-color ANSI codes:
- Index 0: Normal text
- Index 1: Keywords1 (control flow)
- Index 2: Keywords2 (types)
- Index 3: Comments/Multiline comments
- Index 4: Strings
- Index 5: Numbers
- Index 6: Search matches
- Index 7: not printable
if named in file all default colors are gray

## Architecture

TVI is built as a single binary with modular components:

- **terminal.c**: Terminal mode handling and I/O
- **syntax.c**: Dynamic syntax loading and highlighting
- **syntax_parser.c**: Syntax file parsing
- **editor.c**: Core editing operations
- **row.c**: Line-based text operations
- **input.c**: Key handling and Vi mode logic
- **file.c**: File I/O operations

## Testing

TVI includes a comprehensive test suite using CRC validation:

```bash
cd t
./runtests.sh patterns.txt       # Run basic operation tests (59 tests)
./runtests.sh pattern_undo.txt   # Run undo operation tests (20 tests)
./runtests.sh pattern_redo.txt   # Run redo operation tests (24 tests)
./runtests.sh all                # Run all tests
./runtests.sh custom.txt         # Run with custom data
```

### Test Coverage
- **Basic Operations**: Character/line deletion, yank/put, movement commands
- **Undo Operations**: Undo functionality for all logical operations
- **Redo Operations**: Redo functionality and boundary conditions
- **Logical Operations**: `dl`, `dw`, `cw` properly tracked as single undo units

## Differences from Kilo

While based on kilo, TVI includes several enhancements:

- **Vi command mode**: Full Vi-style command system
- **Dynamic syntax loading**: Runtime syntax file loading vs hardcoded
- **Window resize handling**: Proper SIGWINCH signal handling
- **Memory safety**: Bounds checking and overflow protection
- **256-color support**: Enhanced color capabilities
- **DEL key fix**: Correct forward delete behavior
- **Undo/Redo system**: Logical operation tracking for proper undo/redo behavior

## Contributing

### Code Style
- 2-space indentation
- CamelCase function names
- Lowercase variables with underscores
- Comprehensive error checking
- Memory safety first

### Testing
- All changes must pass the test suite
- Add tests for new features
- Maintain backward compatibility

### Pull Requests
- Clear commit messages
- Document significant changes
- Update this README if needed

## License

TVI is released under the same license as kilo (BSD-like, permissive).

## Credits

- **Original kilo**: Salvatore Sanfilippo (antirez)
- **TVI enhancements**: Improved Vi compatibility and modern features
- **Community**: Various bug fixes and improvements

## Related Projects

- [kilo](https://github.com/antirez/kilo) - Original text editor
- [vim](https://www.vim.org/) - The classic Vi/Vim editor
- [neovim](https://neovim.io/) - Modern Vim fork

