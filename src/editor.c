#include "tvi.h"

/*** editor operations ***/

static void copyLinesToYankBuffer(int start, int end) {
  if (E.yank_buffer) free(E.yank_buffer);

  int total_len = 0;
  int i;
  for (i = start; i <= end && i < E.numrows; i++) {
    total_len += E.row[i].size + 1;
  }
  E.yank_mode=LINE;
  E.yank_buffer = xmalloc(total_len + 1);
  E.yank_len = total_len;

  char *p = E.yank_buffer;
  for (i = start; i <= end && i < E.numrows; i++) {
    memcpy(p, E.row[i].chars, E.row[i].size);
    p += E.row[i].size;
    *p = '\n';
    p++;
  }
  *p = '\0';
}

void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    editorTrimTrailingSpaces(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorDelChar() {
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  } else {
    // Track line deletion (merging with previous line)
    char *old_line = strdup(row->chars);
    editorTrackDeleteLine(E.cy - 1, old_line, row->size + E.row[E.cy - 1].size + 1, 1);
    free(old_line);
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    editorTrimTrailingSpaces(&E.row[E.cy - 1]);
    E.cy--;
  }
}

void editorDelForward() {
  if (E.cy == E.numrows) return;

  erow *row = &E.row[E.cy];
  if (E.cx < row->size) {
    editorRowDelChar(row, E.cx);
  } else if (E.cx == row->size && E.cy + 1 < E.numrows) {
    // At end of line, join with next line
    char *next_line = strdup(E.row[E.cy + 1].chars);
    editorTrackDeleteLine(E.cy, next_line, E.row[E.cy + 1].size, 1);
    free(next_line);
    editorRowAppendString(row, E.row[E.cy + 1].chars, E.row[E.cy + 1].size);
    editorDelRow(E.cy + 1);
    editorTrimTrailingSpaces(row);
  }
  // If at end of file, do nothing
}

void editorYankLines(int count) {
  if (E.cy >= E.numrows) return;

  int start = E.cy;
  int end = start + count - 1;
  if (end >= E.numrows) end = E.numrows - 1;

  copyLinesToYankBuffer(start, end);
  editorSetStatusMessage("Yanked %d line(s)", end - start + 1);
}

void editorDeleteLines(int count) {
  if (E.cy >= E.numrows) return;

  int start = E.cy;
  int end = start + count - 1;
  while (end >= E.numrows) {
      end--;
      count--;
  }
  copyLinesToYankBuffer(start, end);

  int totsize;
  char *lines=editorRowsToString(start,count,&totsize);
  if (lines) {
     editorTrackDeleteLine(start, lines, totsize, count);
     free(lines);
  }

  for (int i = start; i <= end; i++) {
    editorFreeRow(&E.row[start]);
    memmove(&E.row[start], &E.row[start + 1],
            sizeof(erow) * (E.numrows - start - 1));
    E.numrows--;
  }

  for (int i = start; i < E.numrows; i++) {
    E.row[i].idx = i;
  }

  if (E.cy >= E.numrows && E.numrows > 0) {
    E.cy = E.numrows - 1;
  }
  if (E.cy > 0 && E.cx > E.row[E.cy].size) {
    E.cx = E.row[E.cy].size;
  }

  E.dirty++;
  editorSetStatusMessage("Deleted %d line(s)", end - start + 1);
}

void editorChangeLines(int count) {
  if (E.cy >= E.numrows) return;

  editorDeleteLines(count);
  editorInsertRow(E.cy, "", 0);
  E.mode = INSERT;
  E.cx = 0;
}

void editorChangeToEndOfLine() {
  if (E.cy >= E.numrows) return;

  erow *row = &E.row[E.cy];
  if (E.cx < row->size) {
    editorTrackDeleteChar(E.cy, E.cx, &row->chars[E.cx], row->size - E.cx);
    E.in_undo++;
    for (int i = E.cx; i < row->size; i++) {
      editorRowDelChar(row, E.cx);
    }
    E.in_undo--;
  }
  editorTrackModifyLine(E.cy, E.cx, row->chars, row->size);
  E.mode = INSERT;
}

void editorDeleteWord(int count, int trim) {
  int i;
  int end_cx = E.cx;

  erow *row = &E.row[E.cy];
  if (E.cy >= E.numrows || E.cx >= row->size) return;

  // move end_cx to next word
  for (i = 0; i < count; i++) {
    if (end_cx < row->size) {
      while (end_cx < row->size && is_word_char(row->chars[end_cx])) {
        end_cx++;
      }
    }
    while (end_cx < row->size && isspace(row->chars[end_cx])) {
      end_cx++;
    }
  }
  while (trim && end_cx > E.cx) {
     if (!isspace(row->chars[end_cx-1])) break;
     end_cx--;
  } 
  editorTrackDeleteChar(E.cy, E.cx, &row->chars[E.cx], end_cx - E.cx);
  E.in_undo++;
  while (E.cx < end_cx) {
    editorRowDelChar(row, E.cx);
    end_cx--;
  }
  E.in_undo--;
  editorSetStatusMessage("Deleted %d word(s)", i);
}

void editorChangeWord(int count) {
  editorDeleteWord(count,1);
  erow *row = &E.row[E.cy];
  editorTrackModifyLine(E.cy, E.cx, row->chars, row->size);
  E.mode = INSERT;
}

void editorYankWord(int count) {
  if (E.cy >= E.numrows) return;

  int total_len = 0;
  char *yank_text = NULL;
  int i;

  for (i = 0; i < count; i++) {
    if (E.cy >= E.numrows) break;

    erow *row = &E.row[E.cy];
    if (E.cx >= row->size) break;

    int start_cx = E.cx;
    int end_cx = E.cx;

    while (end_cx < row->size && isspace(row->chars[end_cx])) {
      end_cx++;
    }
    if (end_cx < row->size) {
      while (end_cx < row->size && is_word_char(row->chars[end_cx])) {
        end_cx++;
      }
    }

    int word_len = end_cx - start_cx;
    if (word_len > 0) {
      char *word = xmalloc(word_len + 2);
      memcpy(word, &row->chars[start_cx], word_len);
      word[word_len] = (i < count - 1 && E.cy < E.numrows - 1) ? ' ' : '\0';
      word[word_len + 1] = '\0';

      char *new_text = xmalloc(total_len + word_len + 1);
      if (yank_text) {
        memcpy(new_text, yank_text, total_len);
        free(yank_text);
      }
      memcpy(new_text + total_len, word, word_len + 1);
      total_len += word_len;
      yank_text = new_text;
      free(word);
    }
  }

  if (E.yank_buffer) free(E.yank_buffer);
  E.yank_mode = CHAR;
  E.yank_buffer = yank_text;
  E.yank_len = total_len;
  editorSetStatusMessage("Yanked %d word(s)", i);
}

int editorCountChar(int at, int count) {
  int max=0;
  erow *row = &E.row[E.cy];
  while (at < 0) {
     at++;
     count--; 
  }
  while (max < count && (at+max) < row->size) max++;
  return max;
}

void editorDeleteChar(int count) {
  if (E.cy >= E.numrows) return;
  int max=editorCountChar(E.cx,count);

  erow *row = &E.row[E.cy];
  editorTrackDeleteChar(E.cy, E.cx, &row->chars[E.cx], max);
  E.in_undo++;
  for (int i = 0; i < max; i++) {
    editorRowDelChar(row, E.cx);
    if (row->size == E.cx) E.cx--;
  }
  E.in_undo--;
}

void editorChangeChar(int count) {
  if (E.cy >= E.numrows) return;
  editorDeleteChar(count);
  erow *row = &E.row[E.cy];
  editorTrackModifyLine(E.cy, E.cx, row->chars, row->size);
  E.mode = INSERT;
}

void editorYankChar(int count) {
  if (E.cy >= E.numrows) return;

  int len=editorCountChar(E.cx, count);
  erow *row = &E.row[E.cy];

  if (E.yank_buffer) free(E.yank_buffer);
  E.yank_buffer = xmalloc(len + 1);
  memcpy(E.yank_buffer, &row->chars[E.cx], len);
  E.yank_buffer[len] = '\0';
  E.yank_len  = len;
  E.yank_mode = CHAR;
  editorSetStatusMessage("Yanked %d char(s)", len);
}

void editorDeleteEndOfLine(void) {
  if (E.cy >= E.numrows) return;

  erow *row = &E.row[E.cy];
  if (E.cx < row->size) {
    int i;
    i=row->size - E.cx;
    editorTrackDeleteChar(E.cy, E.cx, &row->chars[E.cx], i);
    E.in_undo++;
    while (i-- > 0) {
      editorRowDelChar(row, E.cx);
    }
    E.in_undo--;
  }
}

void editorChangeEndOfLine(void) {
  if (E.cy >= E.numrows) return;

  editorDeleteEndOfLine();
  erow *row = &E.row[E.cy];
  editorTrackModifyLine(E.cy, E.cx, row->chars, row->size);
  E.mode = INSERT;
}

void editorYankEndOfLine(void) {
  if (E.cy >= E.numrows) return;

  erow *row = &E.row[E.cy];
  if (E.cx >= row->size) return;

  int len = row->size - E.cx;
  if (E.yank_buffer) free(E.yank_buffer);
  E.yank_buffer = xmalloc(len + 1);
  memcpy(E.yank_buffer, &row->chars[E.cx], len);
  E.yank_buffer[len] = '\0';
  E.yank_len  = len;
  E.yank_mode = CHAR;
  editorSetStatusMessage("Yanked to EOL");
}

void editorPutChar(int after) {                            
  int len = E.yank_len;                                 
  if (after) E.cx++;

  after=E.cx;                                                 
  char *p = E.yank_buffer;                       
  while (*p && len > 0) {                               
    editorInsertChar(*p);
    p++;
    len--;
  }                                                  
  E.cx = after;               
  editorSetStatusMessage("Put %d char(s)", E.yank_len);
}

void editorPut(int after) {
  if (E.yank_buffer == NULL || E.yank_len == 0) {
    editorSetStatusMessage("Nothing to put");
    return;
  }

  if (E.yank_mode == CHAR) {
      editorPutChar(after);
      return;
  }
  int insert_at;
  if (after) {
    insert_at = E.cy + 1;
  } else {
    insert_at = E.cy;
  }

  char *p = E.yank_buffer;
  while (*p) {
    char *line_start = p;
    while (*p && *p != '\n') p++;
    size_t line_len = p - line_start;
    if (*p == '\n') p++;

    editorInsertRow(insert_at, line_start, line_len);
    editorTrimTrailingSpaces(&E.row[insert_at]);
    insert_at++;
  }

  if (after) {
    if (E.cy + 1 < E.numrows) {
      E.cy = E.cy + 1;
    }
  }
  E.cx = 0;

  editorSetStatusMessage("Put %d line(s)", insert_at - (after ? E.cy + 1 : E.cy));
}

