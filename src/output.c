#include "tvi.h"

/*** append buffer ***/

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }

  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "tinyVi editor -- version %s", VERSION);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
      }
      abAppend(ab, "\x1b[0;39m", 7);
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

int editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  abAppend(ab, "\x1b[m", 4);

  char msg[255];
  int len, retlen=0;
  int msglen = strlen(E.statusmsg);
  int maxlen = E.screencols > 252 ? 250 : E.screencols - 2;
  if (E.statusmsg[0] == ':' || E.statusmsg[0] == '/') {
     len=snprintf(msg,maxlen,"%s",E.statusmsg);
     retlen=len;
  } else {
      int perc=100;
      if (E.numrows) perc=((E.cy+1)*100) / E.numrows;
      len=snprintf(msg,maxlen,"%s %s%s %d/%d %d%%%s%s",
                  E.mode == INSERT ? "I": "-",
                  E.filename?E.filename:"no file",E.dirty ? " [Modified]":"",
                  E.cy+1,E.numrows,perc, msglen ? " : " : "", E.statusmsg);
  }
  abAppend(ab, msg, len);
  strcpy(E.statusmsg,"");
  return retlen;
}

void editorRefreshScreen() {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  int rl=editorDrawMessageBar(&ab);

  if (rl == 0) {
     char buf[32];
     snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
                                            (E.rx - E.coloff) + 1);
     abAppend(&ab, buf, strlen(buf));
  } else {
     char buf[32];
     snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.screenrows+1,rl+1);
     abAppend(&ab, buf, strlen(buf));
  }
  abAppend(&ab, "\x1b[?25h", 6);

  write2screen(ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
}

int write2screen(const char *buf, size_t len) {
  if (E.output_enabled) {
    return write(STDOUT_FILENO, buf, len);
  }
  return len;
}
