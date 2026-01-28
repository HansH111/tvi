#include "tvi.h"

/*** init ***/

void initEditor(int ttymode) {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.mode = NORMAL;
  E.size = 0;
  E.bold = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.syntax = NULL;
  E.op_pending = 0;
  E.op_count = 0;
  E.yank_buffer = NULL;
  E.yank_len = 0;
  E.input_file = NULL;
  E.input_fp = NULL;
  E.input_buffer = NULL;
  E.input_buflen = 0;
  E.input_bufpos = 0;
  E.output_enabled = ttymode;
  E.last_cmd = NULL;
  E.last_len = 0;
  E.last_pos = 0;
  E.visual_start_y = 0;
  E.search_highlight_buffer = NULL;
  E.search_highlight_size = 0;
  E.last_search_query = NULL;
  E.last_search_direction = 1;
  E.last_search_match_line = -1;

  // Initialize undo system
  E.undo_stack = xmalloc(sizeof(struct undoEntry) * MAX_UNDO);
  E.redo_stack = xmalloc(sizeof(struct undoEntry) * MAX_UNDO);
  E.undo_count = 0;
  E.undo_pos = 0;
  E.redo_count = 0;
  E.redo_pos = 0;
  E.in_undo = 0;
  E.in_redo = 0;

  if (ttymode) {
     updateWindowSize();
     signal(SIGWINCH, handleSigWinCh);
  } else {
     // do dummy tty rows/cols for batchmode
     E.screenrows = 25;
     E.screencols = 80;
  }
}

void updateWindowSize(void) {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        perror("Unable to query the screen for size (columns / rows)");
        exit(1);
    }
    E.screenrows -= 1; /* Get room for status bar. */
}

void handleSigWinCh(int unused __attribute__((unused))) {
    updateWindowSize();
    if (E.cy > E.screenrows) E.cy = E.screenrows - 1;
    if (E.cx > E.screencols) E.cx = E.screencols - 1;
    editorRefreshScreen();
}

void initRedirectInput(const char *filename) {
  size_t read;
  char *p;
  if (strcmp(filename,"-") != 0) {
     FILE *fp = fopen(filename, "r");
     if (!fp) die("Opening input file not succeeded !");

     fseek(fp, 0, SEEK_END);
     long fsize = ftell(fp);
     fseek(fp, 0, SEEK_SET);

     E.input_buffer = xmalloc(fsize + 1);
     read = fread(E.input_buffer, 1, fsize, fp);
     fclose(fp);
  } else {
     E.input_buffer = xmalloc(256);
     read = fread(E.input_buffer, 1, 255, stdin);
  }
  while ((p=strstr(E.input_buffer,"0x"))!= NULL) { *p='\\'; } 
  E.input_buffer[read] = '\0';
  E.input_buflen = read;
  E.input_bufpos = 0;
}
