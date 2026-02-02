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
  int i;
  editorFreeSearchHighlight();
  int direction = E.last_search_direction;
  int len=strlen(E.last_search_query);

  int current = E.cy;
  int found = 0;

  for (i = 0; i < E.numrows; i++) {
    current += direction;
    
    // Wrap search support
    if (current == -1) {
      if (E.wrap_search) {
        current = E.numrows - 1;
      } else {
        break;  // Don't wrap
      }
    } else if (current == E.numrows) {
      if (E.wrap_search) {
        current = 0;
      } else {
        break;  // Don't wrap
      }
    }
    
    erow *row = &E.row[current];
#if REGEX
    int m = editorSearch(row->render, &len, direction);
    char *match = m < 0 ? NULL : &row->render[m];
#else
    char *match=strstr(row->render, E.last_search_query); 
#endif
    if (match) {
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.last_search_match_line = current;
      E.rowoff = E.numrows;

      // Search highlighting support
      if (E.highlight_search) {
        E.search_highlight_size = row->rsize;
        E.search_highlight_buffer = malloc(row->rsize);
        memcpy(E.search_highlight_buffer, row->hl, row->rsize);
        memset(&row->hl[match - row->render], HL_MATCH, len);
      }
      found = 1;
      break;
    }
  }
  
  // If not found and wrap is disabled, show message
  if (!found && !E.wrap_search) {
    editorSetStatusMessage("Pattern not found");
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
}

void editorFindStart(char *query) {
  editorFreeSearchHighlight();
  int i;

  if (E.last_search_match_line == -1) E.last_search_direction = 1;
  int current = E.last_search_match_line;
  int len=strlen(query);
  
#if REGEX
  if (editorSearchInit(query,0) != 0) return;
#endif
  int wrapped = 0;
  for (i = 0; i < E.numrows; i++) {
    current += E.last_search_direction;
    
    // Wrap search support
    if (current == -1) {
      if (E.wrap_search) {
        current = E.numrows - 1;
        wrapped = 1;
      } else {
        break;  // Don't wrap
      }
    } else if (current == E.numrows) {
      if (E.wrap_search) {
        current = 0;
        wrapped = 1;
      } else {
        break;  // Don't wrap
      }
    }

    erow *row = &E.row[current];
#if REGEX
    int m = editorSearch(row->render, &len, 0);
    char *match = m < 0 ? NULL : &row->render[m];
#else
    char *match=strstr(row->render, E.last_search_query);
#endif
    if (match) {
      E.last_search_match_line = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;

      // Search highlighting support
      if (E.highlight_search) {
        E.search_highlight_size = row->rsize;
        E.search_highlight_buffer = malloc(row->rsize);
        memcpy(E.search_highlight_buffer, row->hl, row->rsize);
        memset(&row->hl[match - row->render], HL_MATCH, len);
      }
      break;
    }
  }
  
  // Show wrap notification if search wrapped
  if (wrapped && !E.wrap_search) {
    editorSetStatusMessage("Search hit BOTTOM without match");
  }
}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;

  char *query = editorPrompt("/%s", editorFindCallback);
  if (query) {
    editorFindStart(query);
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}
