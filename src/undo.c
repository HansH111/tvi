#include "tvi.h"

/*** undo/redo system ***/

void editorFreeUndoEntry(struct undoEntry *entry) {
  if (entry) {
    if (entry->text) free(entry->text);
    entry->text = NULL;
  }
}

void editorClearUndoStack() {
  int i;
  for (i = 0; i < E.undo_count; i++) {
    editorFreeUndoEntry(&E.undo_stack[i]);
  }
  E.undo_count = 0;
  E.undo_pos = 0;
}

void editorClearRedoStack() {
  int i;
  for (i = 0; i < E.redo_count; i++) {
    editorFreeUndoEntry(&E.redo_stack[i]);
  }
  E.redo_count = 0;
  E.redo_pos = 0;
}

void editorPushUndoEntry(struct undoEntry *entry) {
  if (E.in_undo || E.in_redo) return;

  editorClearRedoStack();
  if (E.undo_count >= MAX_UNDO) {
    editorFreeUndoEntry(&E.undo_stack[0]);
    memmove(&E.undo_stack[0], &E.undo_stack[1],
            sizeof(struct undoEntry) * (E.undo_count - 1));
    E.undo_count--;
  }

#if DEBUG
   fprintf(stderr,"- push undo %d-%d t=%d y=%d x=%d l=%ld\n",
           E.undo_pos,E.undo_count,entry->type,entry->cy,entry->cx,entry->text_len);
#endif

  struct undoEntry *new_entry = &E.undo_stack[E.undo_count];
  new_entry->type = entry->type;
  new_entry->cy = entry->cy;
  new_entry->cx = entry->cx;
  new_entry->text = entry->text ? strdup(entry->text) : NULL;
  new_entry->text_len = entry->text_len;
  new_entry->line_count = entry->line_count;
  E.undo_count++;
  E.undo_pos=E.undo_count;
}

void editorPushUndo(int type, int cy, int cx, char *text, size_t text_len, int line_count) {
  if (E.in_undo || E.in_redo) return;
  struct undoEntry entry;
  entry.type = type;
  entry.cy = cy;
  entry.cx = cx;
  entry.text = text ? strdup(text) : NULL;
  entry.text_len = text_len;
  entry.line_count = line_count;
  editorPushUndoEntry(&entry);
  if (entry.text) free(entry.text);
}

void editorRedo() {
  if (E.redo_pos <= 0) {
    editorSetStatusMessage("Nothing to redo");
    return;
  }

  E.redo_pos--;
  E.redo_count--;
  struct undoEntry *entry = &E.redo_stack[E.redo_pos];
#if DEBUG
   fprintf(stderr,"- pop  redo %d-%d t=%d y=%d x=%d l=%ld\n",
                E.redo_pos,E.redo_count,entry->type,entry->cy,entry->cx,entry->text_len);
#endif
  E.in_redo = 1;
  switch (entry->type) {
    case UNDO_INSERT_LINE:
      // Add bounds checking for cursor position handling
      if (entry->cy >= 0 && entry->cy <= E.numrows) {
        editorInsertRow(entry->cy, "", 0);
        if (entry->text && entry->text_len > 0) {
          // Additional bounds checking before accessing row
          if (entry->cy < E.numrows) {
            free(E.row[entry->cy].chars);
            free(E.row[entry->cy].render);
            free(E.row[entry->cy].hl);
            E.row[entry->cy].chars = strdup(entry->text);
            E.row[entry->cy].size = entry->text_len;
            E.row[entry->cy].render = NULL;
            E.row[entry->cy].hl = NULL;
            E.row[entry->cy].hl_open_comment = 0;
            editorUpdateRow(&E.row[entry->cy]);
          }
        }
        // Fixed cursor position handling - ensure cursor is within bounds
        E.cy = entry->cy;
        if (E.cy >= E.numrows) E.cy = E.numrows - 1;
        E.cx = entry->cx;
        if (E.cy < E.numrows && E.cx > E.row[E.cy].size) {
          E.cx = E.row[E.cy].size;
        }
      }
      break;

    case UNDO_DELETE_LINE:
      // Enhanced bounds checking for delete line operations
      if (E.numrows > 0 && entry->cy >= 0 && entry->cy < E.numrows) {
        editorFreeRow(&E.row[entry->cy]);
        memmove(&E.row[entry->cy], &E.row[entry->cy + 1],
                sizeof(erow) * (E.numrows - entry->cy - 1));
        E.numrows--;
        int i;
        for (i = entry->cy; i < E.numrows; i++) {
          E.row[i].idx = i;
        }
        // Fixed cursor position handling
        E.cy = entry->cy;
        if (E.cy >= E.numrows && E.numrows > 0) E.cy = E.numrows - 1;
        E.cx = entry->cx;
        if (E.cy < E.numrows && E.cx > E.row[E.cy].size) {
          E.cx = E.row[E.cy].size;
        }
      }
      break;

    case UNDO_INSERT_CHAR:
      // Enhanced bounds checking for character insertion
      if (entry->cy >= 0 && entry->cy < E.numrows && entry->cx >= 0) {
        // Ensure cursor position is valid
        int safe_cx = entry->cx;
        if (safe_cx > E.row[entry->cy].size) safe_cx = E.row[entry->cy].size;
          for (int i = 0; i < (int)entry->text_len; i++) {
             editorRowInsertChar(&E.row[entry->cy], safe_cx+i, entry->text[i]);
          }
          E.cy = entry->cy;
          E.cx = entry->cx;
          // Ensure cursor doesn't exceed line length
          if (E.cx > E.row[E.cy].size) E.cx = E.row[E.cy].size;
      }
      break;

    case UNDO_DELETE_CHAR:
      // Enhanced bounds checking for character deletion
      if (entry->cy >= 0 && entry->cy < E.numrows && 
          entry->cx >= 0 && entry->cx < E.row[entry->cy].size) {
          for (int i = 0; i < (int)entry->text_len; i++) {
            editorRowDelChar(&E.row[entry->cy], entry->cx);
          }
          E.cy = entry->cy;
          E.cx = entry->cx;
          // Ensure cursor stays within bounds
          if (E.cx > E.row[E.cy].size) E.cx = E.row[E.cy].size;
      }
      break;

    case UNDO_MODIFY_CHAR:
      if (entry->cy < E.numrows && entry->text && entry->text_len > 0) {
        editorRowUpdate(&E.row[entry->cy], entry->cx, entry->text,entry->text_len);
        E.cy = entry->cy;
        E.cx = entry->cx - entry->text_len;
        if (E.cx < 0) E.cx = 0;
        editorUpdateRow(&E.row[E.cy]);
      }
      break;

    case UNDO_MODIFY_LINE:
      if (E.cy < E.numrows && entry->text) {
        free(E.row[E.cy].chars);
        free(E.row[E.cy].render);
        free(E.row[E.cy].hl);
        E.row[E.cy].chars = strdup(entry->text);
        E.row[E.cy].size = entry->text_len;
        E.row[E.cy].render = NULL;
        E.row[E.cy].hl = NULL;
        E.row[E.cy].hl_open_comment = 0;
        editorUpdateRow(&E.row[E.cy]);
        E.cx = entry->cx;
      }
      break;
  }

  E.in_redo = 0;
//  E.redo_pos++;
  E.dirty++;

  // Push the redone entry to undo stack
  if (E.undo_count >= MAX_UNDO) {
    editorFreeUndoEntry(&E.undo_stack[0]);
    memmove(&E.undo_stack[0], &E.undo_stack[1],
            sizeof(struct undoEntry) * (E.undo_count - 1));
    E.undo_count--;
  }

  struct undoEntry *undo_entry = &E.undo_stack[E.undo_count];
  undo_entry->type = entry->type;
  undo_entry->cy = entry->cy;
  undo_entry->cx = entry->cx;
  undo_entry->text = entry->text ? strdup(entry->text) : NULL;
  undo_entry->text_len = entry->text_len;
  undo_entry->line_count = entry->line_count;
#if DEBUG
   fprintf(stderr,"  push undo %d-%d t=%d y=%d x=%d l=%ld\n",
                E.undo_pos,E.undo_count,entry->type,entry->cy,entry->cx,entry->text_len);
#endif
  E.undo_count++;
  E.undo_pos = E.undo_count;
}

void editorUndo() {
  if (E.undo_pos <= 0) {
    editorSetStatusMessage("Nothing to undo");
    if (E.undo_count != (MAX_UNDO -1)) E.dirty=0;
    return;
  }

  E.undo_pos--;
  E.undo_count--;
  struct undoEntry *entry = &E.undo_stack[E.undo_pos];

  E.in_undo = 1;
#if DEBUG
   fprintf(stderr,"- pop  undo %d-%d t=%d y=%d x=%d l=%ld\n",
                E.undo_pos,E.undo_count,entry->type,entry->cy,entry->cx,entry->text_len);
#endif

  switch (entry->type) {
    case UNDO_INSERT_LINE:
      if (E.numrows > 0 && entry->cy < E.numrows) {
        editorFreeRow(&E.row[entry->cy]);
        memmove(&E.row[entry->cy], &E.row[entry->cy + 1],
                sizeof(erow) * (E.numrows - entry->cy - 1));
        E.numrows--;
        int i;
        for (i = entry->cy; i < E.numrows; i++) {
          E.row[i].idx = i;
        }
        E.cy = entry->cy;
        E.cx = entry->cx;
      }
      break;

    case UNDO_DELETE_LINE:
      if (entry->text && entry->line_count > 0) {
        char *p = entry->text;
        int inserted = 0;
        while (*p && inserted < entry->line_count) {
          size_t line_len = 0;
          while (p[line_len] && p[line_len] != '\n') line_len++;
          editorInsertRow(entry->cy + inserted, p, line_len);
          if (p[line_len] == '\n') line_len++;
          p += line_len;
          inserted++;
        }
        E.cy = entry->cy;
        E.cx = entry->cx;
      }
      break;

    case UNDO_INSERT_CHAR:
      if (entry->cy < E.numrows && entry->text_len > 0) {
        for (int i = 0; i < (int)entry->text_len; i++) {
            editorRowDelChar(&E.row[entry->cy], entry->cx);
        }
        E.cy = entry->cy;
        E.cx = entry->cx + entry->text_len;
      }
      break;

    case UNDO_DELETE_CHAR:
      if (entry->cy < E.numrows && entry->text && entry->text_len > 0) {
        for (int i = 0; i < (int)entry->text_len; i++) {
            editorRowInsertChar(&E.row[entry->cy], entry->cx + i, entry->text[i]);
        }
        E.cy = entry->cy;
        E.cx = entry->cx;
      }
      break;

    case UNDO_MODIFY_CHAR:
      if (entry->cy < E.numrows && entry->text && entry->text_len > 0) {
        editorRowUpdate(&E.row[entry->cy], entry->cx, entry->text, entry->text_len);
        E.cy = entry->cy;
        E.cx = entry->cx;
      }
      break;

    case UNDO_MODIFY_LINE:
      if (E.cy < E.numrows && entry->text) {
        free(E.row[E.cy].chars);
        free(E.row[E.cy].render);
        free(E.row[E.cy].hl);
        E.row[E.cy].chars = strdup(entry->text);
        E.row[E.cy].size = entry->text_len;
        E.row[E.cy].render = NULL;
        E.row[E.cy].hl = NULL;
        E.row[E.cy].hl_open_comment = 0;
        editorUpdateRow(&E.row[E.cy]);
        E.cx = entry->cx;
      }
      break;
  }

  E.in_undo = 0;
  E.dirty++;

  // Push the undone entry to redo stack
  if (E.redo_count >= MAX_UNDO) {
    editorFreeUndoEntry(&E.redo_stack[0]);
    memmove(&E.redo_stack[0], &E.redo_stack[1],
            sizeof(struct undoEntry) * (E.redo_count - 1));
    E.redo_count--;
  }

  struct undoEntry *redo_entry = &E.redo_stack[E.redo_count];
  redo_entry->type = entry->type;
  redo_entry->cy = entry->cy;
  redo_entry->cx = entry->cx;
  redo_entry->text = entry->text ? strdup(entry->text) : NULL;
  redo_entry->text_len = entry->text_len;
  redo_entry->line_count = entry->line_count;
#if DEBUG
   fprintf(stderr,"  push redo %d-%d t=%d y=%d x=%d l=%ld\n",
                E.redo_pos,E.redo_count,entry->type,entry->cy,entry->cx,entry->text_len);
#endif
  E.redo_count++;
  E.redo_pos = E.redo_count;
}

void editorTrackInsertLine(int cy) {
  editorPushUndo(UNDO_INSERT_LINE, cy, 0, NULL, 0, 0);
}

void editorTrackDeleteLine(int cy, char *text, size_t len, int line_count) {
  editorPushUndo(UNDO_DELETE_LINE, cy, 0, text, len, line_count);
}

void editorTrackInsertChar(int cy, int cx, char *text, size_t len) {
  editorPushUndo(UNDO_INSERT_CHAR, cy, cx, text, len, 0);
}

void editorTrackDeleteChar(int cy, int cx, char *text, size_t len) {
  editorPushUndo(UNDO_DELETE_CHAR, cy, cx, text, len, 0);
}

void editorTrackModifyChar(int cy, int cx, char *old_text, size_t old_len) {
  editorPushUndo(UNDO_MODIFY_CHAR, cy, cx, old_text, old_len, 0);
}

void editorTrackModifyLine(int cy, int cx, char *old_text, size_t old_len) {
  editorPushUndo(UNDO_MODIFY_LINE, cy, cx, old_text, old_len, 0);
}

