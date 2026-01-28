#include "tvi.h"

/*** file i/o ***/

char *editorRowsToString(int from, int norows, int *buflen) {
  *buflen = 0;
  if ((from + norows) > E.numrows) norows = E.numrows - from;
  if (norows <= 0) return NULL;

  int totlen = 0;
  for (int j = 0; j < norows; j++) totlen += E.row[from+j].size + 1;
  char *buf = xmalloc(totlen+1);
  if (buf) {
     char *p = buf;
     for (int j = 0; j < norows; j++) {
       memcpy(p, E.row[from+j].chars, E.row[from+j].size);
       p += E.row[from+j].size;
       *p = '\n';
       p++;
     }
     *p = '\0';
     *buflen = totlen+1;
  }
  return buf;
}

void editorOpen(char *filename) {

  if (E.filename) free(E.filename);
  E.filename = strdup(filename);

  #ifdef DEBUG                          
  fprintf(stderr, "# file open %s\n", filename);
  #endif     
  if (E.output_enabled) editorSelectSyntaxHighlight();

  FILE *fp = fopen(filename, "r");
  if (!fp) die("Opening file did not succeed !");

  struct stat buf;
  fstat(fileno(fp), &buf);
  E.size = buf.st_size;
  E.in_undo = 1;
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
    editorInsertRow(E.numrows, line, linelen);
    editorTrimTrailingSpaces(&E.row[E.numrows - 1]);
  }
  free(line);
  fclose(fp);
  E.in_undo = 0;
  E.dirty = 0;
}

void editorSave(int start, int rows) {
  int len;
  char *buf = editorRowsToString(start, rows, &len);

  if (E.output_enabled == 0) {
    printf("%s",buf);
    return;
  }
  if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
  }
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        E.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        if (E.output_enabled) editorSelectSyntaxHighlight();
        return;
      }
    }
    close(fd);
  }

  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}
