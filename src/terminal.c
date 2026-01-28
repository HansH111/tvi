#include "tvi.h"
#include <stdio.h>

/*** terminal ***/

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) die("Unable to allocate memory with malloc");
  return ptr;
}

void *xrealloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (new_ptr == NULL) die("Unable to increase memory with realloc");
  return new_ptr;
}

void die(const char *s) {
  write2screen("\x1b[2J", 4);
  write2screen("\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
  if (E.output_enabled) {
    write2screen("\x1b[?1049l", 8); /* Switch back to main screen */
  }
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cflag |= (CS8);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");

  if (E.output_enabled) {
    write2screen("\x1b[?1049h", 8); /* Switch to alternate screen */
  }
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

static int parseEscapeSequence(const char *seq) {
  if (seq[0] == 'x' && seq[1]) {
    int val = 0;
    for (int i = 1; seq[i] && i < 3; i++) {
      char h = seq[i];
      if (h >= '0' && h <= '9') val = val * 16 + (h - '0');
      else if (h >= 'a' && h <= 'f') val = val * 16 + (h - 'a' + 10);
      else if (h >= 'A' && h <= 'F') val = val * 16 + (h - 'A' + 10);
      else break;
    }
    return val;
  }
  if (seq[0] == 'n') return '\n';
  if (seq[0] == 't') return '\t';
  if (seq[0] == 'r') return '\r';
  return seq[0];
}

int editorRecordKey(int c) {
  if (E.last_cmd==NULL) { 
      E.last_cmd=xmalloc(255);
      E.last_len=254;
  }
  if (c == '\x1b') {
     E.last_pos=0;
  } else if (c != CTRL_KEY('r') && c != 'u') {
     if (E.last_pos < E.last_len && (c!='.' || E.mode == INSERT) ) {
         E.last_cmd[E.last_pos++] = c;
         E.last_cmd[E.last_pos]   = '\0';
     }
  }
  #if DEBUG
  if (E.last_pos > 0 && E.last_pos < 20) {
     editorSetStatusMessage("-- LASTCMD [%s] --",E.last_cmd);
  }
  #endif
  return c;
}

int editorReadKey() {
  if (E.cmd_buf && E.cmd_pos < E.cmd_len) {
      int c = E.cmd_buf[E.cmd_pos++];
      if (E.cmd_pos >= E.cmd_len) {
          free(E.cmd_buf);
          E.cmd_buf=NULL;
          E.cmd_len=0;
          E.cmd_pos=0;
      }
      return editorRecordKey(c);
  }
  if (E.input_buffer) {     // stdin redirection
    if (E.input_bufpos >= E.input_buflen) {
        editorSave(0,E.numrows);
        free(E.input_buffer);
        write2screen("\x1b[2J", 4);
        write2screen("\x1b[H", 3);
        exit(0);
    }
    int c = E.input_buffer[E.input_bufpos++];
    if (c == '\\') {
        char seq[5];
        size_t remaining = E.input_buflen - E.input_bufpos;
        size_t to_read = remaining < 4 ? remaining : 3;
        memcpy(seq, &E.input_buffer[E.input_bufpos], to_read);
        seq[to_read] = '\0';
        c = parseEscapeSequence(seq);
        E.input_bufpos += to_read;
    }
    return editorRecordKey(c);
  }

  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  if (c == '\x1b') {
    char seq[3];

    editorRecordKey('\x1b');
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
    return '\x1b';
  } else {
    return editorRecordKey(c);
  }
}
