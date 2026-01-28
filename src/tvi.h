#ifndef TVI_H
#define TVI_H

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295U
#endif

#define TVI_VERSION  "1.0.4"
#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 4
#define CTRL_KEY(k) ((k) & 0x1f)

#define MAX_UNDO 1000

#define XMALLOC(size) xmalloc(size)
#define XREALLOC(ptr, size) xrealloc(ptr, size)

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

/*** enums ***/

enum pasteMode {  // for yank
  LINE,
  CHAR,
};

enum editorMode {
  NORMAL,
  INSERT,
  OPERATOR_PENDING,
};

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

enum editorHighlight {
  HL_NORMAL = 0,
  HL_NONPRINT = 1,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

enum undoType {
  UNDO_INSERT_LINE,
  UNDO_DELETE_LINE,
  UNDO_INSERT_CHAR,
  UNDO_DELETE_CHAR,
  UNDO_MODIFY_CHAR,
  UNDO_MODIFY_LINE
};

struct undoEntry {
  int type;
  int cy;
  int cx;
  char *text;
  size_t text_len;
  int line_count;
};

/*** forward declarations ***/

typedef struct erow erow;

struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
};

struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  int mode;
  off_t size;
  int bold;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  struct editorSyntax *syntax;
  struct termios orig_termios;
  int op_pending;
  int op_count;
  int   yank_mode;
  char *yank_buffer;
  size_t yank_len;
  char *input_file;
  FILE *input_fp;
  char *input_buffer;
  size_t input_buflen;
  size_t input_bufpos;
  int output_enabled;
  int visual_start_y;
  char *last_cmd;
  size_t  last_len;
  size_t  last_pos;
  char *cmd_buf;
  size_t cmd_len;
  size_t cmd_pos;
  char *search_highlight_buffer;
  size_t search_highlight_size;
  char *last_search_query;
  int last_search_direction;
  int last_search_match_line;
  struct undoEntry *undo_stack;
  struct undoEntry *redo_stack;
  int undo_count;
  int undo_pos;
  int redo_count;
  int redo_pos;
  int in_undo;
  int in_redo;
};

extern struct editorConfig E;

/*** terminal ***/

void die(const char *s);
void disableRawMode();
void enableRawMode();
int getWindowSize(int *rows, int *cols);
int editorReadKey();
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
int editorLoadInputFile(const char *filename);
void editorFreeInput(void);
int editorRecordKey(int c);

/*** prototypes ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));
int write2screen(const char *buf, size_t len);

/*** row operations ***/

void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowUpdate(erow *row, int at, char *s, size_t len); 
void editorRowDelChar(erow *row, int at);
void editorTrimTrailingSpaces(erow *row);
int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);

/*** syntax highlighting ***/

void editorSelectSyntaxHighlight();
void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);

/*** movement ***/

int is_word_char(int c);
void editorMoveLeft();
void editorMoveRight();
void editorMoveWordForward();
void editorMoveWordBackward();
void editorMoveLineStart();
void editorMoveLineEnd();
void editorMoveFileTop();
void editorMoveFileBottom();

/*** yank/delete/change ***/

int  editorCountChar(int at, int count);
void editorYankLines(int count);
void editorDeleteLines(int count);
void editorChangeLines(int count);
void editorChangeToEndOfLine();
void editorChangeWord(int count);
void editorDeleteWord(int count,int trim);
void editorYankWord(int count);
void editorChangeChar(int count);
void editorDeleteChar(int count);
void editorYankChar(int count);
void editorChangeEndOfLine(void);
void editorDeleteEndOfLine(void);
void editorYankEndOfLine(void);
void editorPut(int after);
void editorEnterVisualLine();
void editorExitVisual();

/*** editor operations ***/

void editorInsertChar(int c);
void editorInsertNewline();
void editorDelChar();
void editorDelForward();

/*** file i/o ***/

char *editorRowsToString(int from, int norows, int *buflen);
void editorOpen(char *filename);
void editorSave(int start, int rows);

/*** find ***/

void editorFindNext();
void editorFindPrev();
void editorFindCallback(char *query, int key);
void editorFreeSearchHighlight();
void editorFind();

/*** undo ***/

void editorUndo();
void editorRedo();
void editorPushUndo(int type, int cy, int cx, char *text, size_t text_len, int line_count);
void editorPushUndoEntry(struct undoEntry *entry);
void editorClearUndoStack();
void editorClearRedoStack();
void editorFreeUndoEntry(struct undoEntry *entry);
void editorTrackInsertChar(int cy, int cx, char *text,size_t len);
void editorTrackDeleteChar(int cy, int cx, char *text,size_t len);
void editorTrackModifyChar(int cy, int cx, char *text,size_t len);
void editorTrackInsertLine(int cy);
void editorTrackDeleteLine(int cy, char *text, size_t len, int line_count);
void editorTrackModifyLine(int cy, int cx, char *old_text, size_t old_len);

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

/*** input ***/

void editorMoveCursor(int key);
int  editorHandleNavigation(int c);
void editorProcessDefaultCommand();
void editorProcessCommand(const char *cmd_line);
void editorProcessKeypress();
void editorProcessDefaultKeypress(int c);
void editorProcessInsertKeypress(int c);
void editorProcessVisualKeypress(int c);
void editorExecuteOperator(int op, int count);
void editorApplyOperatorMotion(int op, int motion, int count);

/*** init ***/

void initEditor(int ttymode);
void initRedirectInput(const char *filename);
void updateWindowSize(void);
void handleSigWinCh(int unused);

/*** syntax ***/

struct editorSyntax {
  int  colors[9];         // 8 colors: normal, kw1, kw2, comment, mlcomment, string, number, match, non
  char **keywords;               
  char *singleline_comment_start;
  char *multiline_comment_start; 
  char *multiline_comment_end;   
  int  flags;                                                               
};                                                                         
                
#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

struct editorSyntax* parseSyntaxFile(const char* filename);

#endif
