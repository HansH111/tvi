#include "tvi.h"

/*** row operations ***/

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

int editorRowRxToCx(erow *row, int rx) {
  int cx = 0;
  int runtime_rx = 0;
  while (cx < row->size) {
    if (row->chars[cx] == '\t')
      runtime_rx += (KILO_TAB_STOP - 1) - (runtime_rx % KILO_TAB_STOP);
    runtime_rx++;

    if (runtime_rx > rx) return cx;
    cx++;
  }
  return cx;
}

void editorUpdateRow(erow *row) {
  unsigned int tabs = 0, nonprint = 0;
  int j, idx;

  /* Create a version of the row we can directly print on the screen,
     * respecting tabs, substituting non printable characters with '?'. */
  free(row->render);
  for (j = 0; j < row->size; j++)
    if (row->chars[j] == '\t') tabs++;

  unsigned long long allocsize =
      (unsigned long long) row->size + tabs*(KILO_TAB_STOP - 1) + nonprint*9 + 1;
  if (allocsize > UINT32_MAX) {
      printf("Some line of the edited file is too long for tvi\n");
      exit(1);
  }

  row->render = xmalloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

  idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  free(row->hl);
  row->hl = xmalloc(row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);

  editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;

  editorTrackInsertLine(at);   
  E.row = xrealloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

  E.row[at].idx = at;

  E.row[at].size = len;
  E.row[at].chars = xmalloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  E.row[at].hl_open_comment = 0;
  editorUpdateRow(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows) return;

  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;

  if (E.mode != INSERT) editorTrackInsertChar(E.cy, at, &row->chars[at],1);
  row->chars = xrealloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = xrealloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowUpdate(erow *row, int at, char *s, size_t len) {
  if (at < 0 || (at+(int)len) >= row->size) return;
  editorTrackModifyChar(E.cy, at, &row->chars[at],len);
  memcpy(&row->chars[at], s, len);
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDelChar(erow *row, int at) {                                       
  if (at < 0 || at >= row->size) return;                                         
                                                                                 
  editorTrackDeleteChar(E.cy, at, &row->chars[at], 1);                           
#if DEBUG                                                                        
  fprintf(stderr," size=%d at=%d\n",row->size,at);                               
#endif                                                                           
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);                 
  row->size--;                                                                   
  if (row->size == at) row->chars[at + 1] = '\0';                                
                                                                                 
  editorUpdateRow(row);                                                          
  E.dirty++;                                                                     
}                   



void editorTrimTrailingSpaces(erow *row) {
  if (row == NULL || row->size == 0) return;

  while (row->size > 0) {
    char c = row->chars[row->size - 1];
    if (c == ' ' || c == '\t') {
      row->size--;
    } else {
      break;
    }
  }
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}
