# TVI - Tiny Vi-like Editor

TVI is a small, fast terminal-based text editor inspired by Vi. It's based on the [kilo](https://github.com/antirez/kilo) text editor, but implements basic Vi/Vim-like functionality instead of Emacs-style editing.

## Features

### Core Editing
- **Vi-compatible modes**: Normal, Insert and Command-line modes
- **Basic Vi commands**: Navigation (h,j,k,l,w,b,0,$), editing (i,a,o,O,x,d,c,y,p), search (/), etc.
- **Line-based operations**: dd, yy, p, etc.
- **Word movements**: w, b, e for word navigation
- **Regexpr search and replace**: search/replace can include regular expressions (see TinyRE project)
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
- `n` / `N`  - Next/previous match

## Configuration

### :set Command

TVI includes a configuration system using the `:set` command. All settings use simple integer values:

#### Basic Settings
```bash
:set                   # Show all current settings
:set tabstop=8         # Set tab width (1-16, default: 4)
:set noautoindent      # Enable auto-indent (default: 0)
:set noignorecase      # Case-sensitive search (default: 0)
:set wrapsearch        # Wrap around when searching (default: 1)
:set highlightsearch   # Highlight search matches (default: 1)
```

#### Short Form Commands
```bash
:set ts=8             # tabstop
:set noai             # autoindent
:set noic             # ignorecase
:set ws               # wrapsearch
:set hl               # highlightsearch
```

#### Setting Validation
- **tabstop**: Must be between 1-16
- **All other settings**: Must be 0 (disabled) or 1 (enabled)
- Invalid values show helpful error messages

#### Testing the :set Command
Configuration changes can be tested using the test suite:
```bash
cd t && ./runtests.sh pattern_config.txt  # Test :set command functionality
```

#### Case Sensitivity
TVI provides explicit control over case sensitivity through the `ignorecase` setting:
- **ignorecase=0**: Always case-sensitive search
- **ignorecase=1**: Always case-insensitive search
- Use regex escapes for explicit control: `\C` (force case-sensitive), `\c` (force case-insensitive)

### Syntax Files

TVI loads syntax highlighting from files in `~/.config/tvi/` or `/usr/local/share/tvi/` directories.

#### Directory Priority
1. **User config**: `~/.config/tvi/` (checked first)
2. **System config**: `/usr/local/share/tvi/` (fallback)

#### Syntax File Naming
Syntax files use the format: `syntax{extension1}{extension2}`

Examples:
- `syntax.c.h` - for C files (.c and .h extensions)
- `syntax.cpp.hpp.cc` - for C++ files (.cpp, .hpp, .cc)
- `syntax.js.jsx` - for JavaScript files (.js, .jsx)
- `syntax.py` - for Python files (.py)
- `syntax.pl.pm`- for Perl files (.pl, .pm)

```bash
# Example C syntax highlighting file (~/.config/tvi/syntax.c.h)
# --- 256-color codes for: normal, kw1, kw2, comment, string, number, match, notprintable
colors256  : 249 117 48 73 73 246 246 195 196
# --- keywords (control flow)
keywords  : if else do while for break continue return goto switch case default
# --- keywords (types with | at end)
keywords   : int| long| double| float| char| unsigned| signed| void| short|
# --- single-line comment
remark     : //
# --- multi-line comment start/end
remarkstart: /*
remarkend  : */
# --- highlighting
numbers
strings
searchmatch
```

## Configuration Files

TVI supports configuration through the `:set` command and also loads settings from `tvi.conf` files.

### Configuration File Locations
1. **User config**: `~/.config/tvi/tvi.conf` (checked first)
2. **System config**: `/usr/local/share/tvi/tvi.conf` (fallback)

### Sample tvi.conf
Create `~/.config/tvi/tvi.conf` with your preferred settings:

```bash
# TVI Configuration File
# Lines starting with # are comments
# Format: setting=value
#
# Tab settings
tabstop=4
autoindent=1
#
# Search settings
ignorecase=0
wrapsearch=1
highlightsearch=1
#
```

### Settings Available
- **tabstop**   : Tab width (1-16, default: 4)
- **autoindent**: Enable auto-indent (0/1, default: 1)
- **ignorecase**: Case-sensitive search (0/1, default: 0)
- **wrapsearch**: Wrap around when searching (0/1, default: 1)
- **highlightsearch**: Highlight search matches (0/1, default: 1)

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
- **config.c**: Configuration management and tvi.conf loading

## Testing

TVI includes a comprehensive test suite using CRC validation:

```bash
cd t
./runtests.sh patterns.txt       # Run basic operation tests (57 tests)
./runtests.sh pattern_undo.txt   # Run undo operation tests (20 tests)
./runtests.sh pattern_redo.txt   # Run redo operation tests (24 tests)
./runtests.sh pattern_config.txt # Run configuration tests (:set commands)
./runtests.sh all                # Run all tests
./runtests.sh custom.txt         # Run with custom data
```

### Test Coverage
- **Basic Operations**: Character/line deletion, yank/put, movement commands
- **Undo Operations**: Undo functionality for all logical operations
- **Redo Operations**: Redo functionality and boundary conditions
- **Configuration**: `:set` command functionality and parameter validation
- **Logical Operations**: `dl`, `dw`, `cw` properly tracked as single undo units

## Differences from Kilo

While based on kilo, TVI includes several enhancements:

- **Vi command mode**: Full Vi-style command system
- **Dynamic syntax loading**: Runtime syntax file loading vs hardcoded
- **Configuration files**: Support for tvi.conf configuration files with persistent settings
- **Window resize handling**: Proper SIGWINCH signal handling
- **Memory safety**: Bounds checking and overflow protection
- **256-color support**: Enhanced color capabilities
- **DEL key fix**: Correct forward delete behavior
- **Regexpr search and replace system**: search can include regular expressions
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

