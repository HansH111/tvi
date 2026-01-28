#include "tvi.h"

/*** find ***/

void editorFreeSearchHighlight() {
  if (E.search_highlight_buffer) {
      if (E.last_search_match_line >= 0 && E.last_search_match_line < E.numrows) {
          memcpy(E.row[E.last_search_match_line].hl, E.search_highlight_buffer,
                 E.row[E.last_search_match_line].rsize);
      }
      free(E.search_highlight_buffer);
      E.search_highlight_buffer = NULL;
      E.search_highlight_size = 0;
  }
}

void editorFindNext() {
  if (E.last_search_query == NULL || E.last_search_query[0] == '\0') {
    return;
  }
  editorFreeSearchHighlight();
  int current = E.cy;
  //int current = E.last_search_match_line;
  int direction = E.last_search_direction;
  int i;

  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;

    erow *row = &E.row[current];
    char *match = strstr(row->render, E.last_search_query);
    if (match) {
      E.last_search_match_line = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;

      E.search_highlight_size = row->rsize;
      E.search_highlight_buffer = malloc(row->rsize);
      memcpy(E.search_highlight_buffer, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(E.last_search_query));
      break;
    }
  }
}

void editorFindPrev() {
  if (E.last_search_query == NULL || E.last_search_query[0] == '\0') {
    return;
  }

  E.last_search_direction = -E.last_search_direction;
  editorFindNext();
  E.last_search_direction = -E.last_search_direction;
}

void editorFindCallback(char *query, int key) {
  if (key == '\r' || key == '\x1b') {
    editorFreeSearchHighlight();
    if (key == '\r' && query && query[0] != '\0') {
      if (E.last_search_query) free(E.last_search_query);
      E.last_search_query = strdup(query);
      E.last_search_match_line = -1;
      E.last_search_direction = 1;
    }
    return;
  }
  editorFreeSearchHighlight();

  if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    E.last_search_direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    E.last_search_direction = -1;
  } else {
    E.last_search_match_line = -1;
    E.last_search_direction = 1;
  }

  if (E.last_search_match_line == -1) E.last_search_direction = 1;
  int current = E.last_search_match_line;
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += E.last_search_direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;

    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      E.last_search_match_line = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;

      E.search_highlight_size = row->rsize;
      E.search_highlight_buffer = malloc(row->rsize);
      memcpy(E.search_highlight_buffer, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;

  char *query = editorPrompt("/%s", editorFindCallback);
  if (query) {
     free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}
