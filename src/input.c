#include "tvi.h"
/*** input ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = xmalloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

    int c = editorReadKey();
    if (c == '\x1b' || c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen == 0 || c == '\x1b') {
         editorSetStatusMessage("");
         if (callback) callback(buf, c);
         free(buf);
         return NULL;
      }
      buf[--buflen] = '\0';
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (c == ':') {
      return NULL;  //continue;
    } else if (!iscntrl(c) && c < 128) {
      if (buflen >= bufsize - 1) {
        bufsize *= 2;
        buf = xrealloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
    if (callback) callback(buf, c);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      } else if (E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARROW_RIGHT:
      if (row && E.cx < row->size) {
        E.cx++;
      } else if (row && E.cx == row->size) {
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;
    case ARROW_DOWN:
      if (E.cy < E.numrows) {
        E.cy++;
      }
      break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

// return 1 if handled
int editorHandleNavigation(int c) {
  int stat=0;
  int count = E.op_count > 0 ? E.op_count : 1;

  switch (c) {
    case 'k': c=ARROW_UP;    break;
    case 'j': c=ARROW_DOWN;  break;
    case 'h': c=ARROW_LEFT;  break;
    case 'l': c=ARROW_RIGHT; break;

    case CTRL_KEY('B'): c=PAGE_UP;   break;
    case CTRL_KEY('F'): c=PAGE_DOWN; break;
  }
  switch (c) {
    case HOME_KEY:
      stat=1;
      E.cx = 0;
      break;
    case END_KEY:
      stat=1;
      if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
      break;

    case PAGE_UP:
    case PAGE_DOWN: {
        stat=1;
        if (c == PAGE_UP) {
          E.cy = E.rowoff;
        } else if (c == PAGE_DOWN) {
          E.cy = E.rowoff + E.screenrows - 1;
          if (E.cy > E.numrows) E.cy = E.numrows;
        }
        int times = E.screenrows;
        while (times--)
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
      break;

    case ARROW_UP    :
    case ARROW_DOWN  :
    case ARROW_LEFT  :
    case ARROW_RIGHT :
      stat=1;
      while (count-- > 0)  editorMoveCursor(c);
      break;
    case '0':
      stat=1;
      editorMoveLineStart();
      break;
    case '$':
      stat=1;
      editorMoveLineEnd();
      break;
    case 'w':
      stat=1;
      while (count-- > 0) editorMoveWordForward();
      break;
    case 'b':
      stat=1;
      while (count-- > 0) editorMoveWordBackward();
      break;
    case 'G':
      stat=1;
      if (E.op_count > 0 && E.op_count < E.numrows) {
         E.cy=E.op_count;
         E.cx=0;
      } else {
        editorMoveFileBottom();
      }
      break;
  }
  if (stat) editorRecordKey('\x1b');
  return stat;
}

void editorProcessDoubleKeypress(int op, int secop) {
  int count = E.op_count > 0 ? E.op_count : 1;

  if (op == ' ') return;  // to prevent a compiler warning

  if (op == 'r') {
     erow *row = &E.row[E.cy];
     count = editorCountChar(E.cx,count);

     char *buf=xmalloc(count+1);
     for (int i=0; i<count; i++) buf[i]=secop;
     buf[count]='\0';
     editorRowUpdate(row,E.cx,buf,count);
     free(buf);
  } else {
      switch (secop) {
         case 'g' : editorMoveFileTop();
                    break;
         case 'd' : editorDeleteLines(count);
                    break;
         case 'y' : editorYankLines(count);
                    break;
         case 'w':
             if (op == 'c') editorChangeWord(count);
             if (op == 'd') editorDeleteWord(count,0);
             if (op == 'y') editorYankWord(count);
             break;
         case 'l':
             if (op == 'c') editorChangeChar(count);
             if (op == 'd') editorDeleteChar(count);
             if (op == 'y') editorYankChar(count);
             break;
         case '$':
             if (op == 'c') editorChangeEndOfLine();
             if (op == 'd') editorDeleteEndOfLine();
             if (op == 'y') editorYankEndOfLine();
             break;
       }
  }
}

void editorProcessSingleKeypress(int c) {
  int count = E.op_count > 0 ? E.op_count : 1;

  switch (c) {
    case 'r':
      E.op_pending = 'r';
      break;
    case 'y':
      E.op_pending = 'y';
      break;
    case 'd':
      E.op_pending = 'd';
      break;
    case 'c':
      E.op_pending = 'c';
      break;
    case 'g':
      E.op_pending = 'g';
      break;
    case 'p':
      editorPut(1);
      break;
    case 'P':
      editorPut(0);
      break;
    case '/':
      editorFind();
      break;
    case 'n':
      editorFindNext();
      break;
    case 'N':
      editorFindPrev();
      break;
    case CTRL_KEY('g'):
      editorSetStatusMessage("x=%d len=%d size=%d",E.cx+1, E.row[E.cy].size, E.row[E.cy].rsize);
      break;
    case CTRL_KEY('r'):
      editorRedo();
      break;
    case 'u':
      editorUndo();
      if (E.undo_pos == 0 &&  E.undo_count != (MAX_UNDO -1)) E.dirty=0;
      break;
    case 'A':
      E.cx = E.row[E.cy].size - 1;
      // fall through
    case 'a':
      E.cx++;
      // fall through
    case 'i':
      if (E.numrows > 0) editorTrackModifyLine(E.cy, E.cx, E.row[E.cy].chars, E.row[E.cy].size);
      E.mode = INSERT;
      E.bold = 1;
      break;
    case 'o':
      editorInsertRow(E.cy + 1, "", 0);
      editorTrimTrailingSpaces(&E.row[E.cy + 1]);
      E.cy++;
      E.cx = 0;
      editorTrackModifyLine(E.cy, E.cx, E.row[E.cy].chars, E.row[E.cy].size);
      E.mode = INSERT;
      E.bold = 1;
      break;
    case 'O':
      editorInsertRow(E.cy, "", 0);
      editorTrimTrailingSpaces(&E.row[E.cy]);
      E.cx = 0;
      editorTrackModifyLine(E.cy, E.cx, E.row[E.cy].chars, E.row[E.cy].size);
      E.mode = INSERT;
      E.bold = 1;
      break;
    case '.':
      if (E.last_cmd) {
         size_t len = strlen(E.last_cmd);
#if DEBUG
         fprintf(stderr,"- lastcmd %ld [%s]\n",len,E.last_cmd);
#endif
         if (len < 250) {
            E.cmd_buf = strdup(E.last_cmd);
            E.cmd_pos = 0;
            E.cmd_len = len;
            E.last_cmd[0]='\0';
            E.last_pos=0;
         }
      }
      break;
    case 'x':
      editorDeleteChar(count);
      break;
    case ':':
      editorSetStatusMessage(":");
      editorRefreshScreen();
      editorProcessDefaultCommand();
      break;
  }
}

void editorProcessNormalKeypress(int c) {
  if (E.search_highlight_buffer && strchr("nN",c)==0)   editorFreeSearchHighlight();
  if (E.op_count == 0 && c == '0') {
      editorMoveLineStart();
      E.op_pending = 0;
      return;
  }
  if (c >= '0' && c <= '9') {
      E.op_count = E.op_count * 10 + (c - '0');
      E.op_pending = 0;
      return;
  }

  if (E.op_pending) {
    editorProcessDoubleKeypress(E.op_pending, c);
    E.op_pending = 0;
    if (E.op_pending == 0) E.op_count = 0;
    return;
  }

  if (editorHandleNavigation(c)) {
      E.op_count = 0;
      return;
  }

  editorProcessSingleKeypress(c);
  if (E.op_pending == 0) E.op_count = 0;
}

// in INSERT mode
void editorProcessInsertKeypress(int c) {
  switch (c) {
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      for (int i = 0; i < E.numrows; i++) editorUpdateRow(&E.row[i]);
      break;
    case '\r':
      editorInsertNewline();
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
      editorDelChar();
      break;
    case DEL_KEY:
      editorDelForward();
      break;
    case '\x1b':
      editorSetStatusMessage("");
      if (E.cx > 0) E.cx--;
      E.mode = NORMAL;
      E.bold = 0;
      for (int i = 0; i < E.numrows; i++) editorUpdateRow(&E.row[i]);
      break;
    default:
      editorInsertChar(c);
      break;
  }
  E.op_pending = 0;
  E.op_count=0;
}

void editorProcessKeypress() {
  int c = editorReadKey();

  switch (E.mode) {
    case NORMAL:
      editorProcessNormalKeypress(c);
      break;
    case INSERT:
      editorProcessInsertKeypress(c);
      break;
    case OPERATOR_PENDING:
      break;
  }
}
