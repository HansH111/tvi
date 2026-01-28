#include "tvi.h"

/*** vi movement commands ***/

int is_word_char(int c) {
  return isalnum(c) || c == '_';
}

void editorMoveLeft() {
  if (E.cx > 0) {
    E.cx--;
  } else if (E.cy > 0) {
    E.cy--;
    E.cx = E.row[E.cy].size;
  }
}

void editorMoveRight() {
  if (E.cy < E.numrows && E.cx < E.row[E.cy].size) {
    E.cx++;
  } else if (E.cy < E.numrows - 1) {
    E.cy++;
    E.cx = 0;
  }
}

void editorMoveWordForward() {
  if (E.cy >= E.numrows) return;

  erow *row = &E.row[E.cy];                                                                                  
  if (E.cx >= row->size && E.cy < E.numrows) {
     E.cx=0;
     E.cy++;
     return;
  }
  int end_cx = E.cx;                                                                                         

  // move end_cx to next word                                                                                
  if (end_cx < row->size) {                                                                                
    while (end_cx < row->size && is_word_char(row->chars[end_cx])) {                                       
       end_cx++;                                                                                            
    }                                                                                                      
  }                                                                                                        
  while (end_cx < row->size && !isspace(E.row[E.cy].chars[end_cx + 1]) &&
         !is_word_char(E.row[E.cy].chars[end_cx + 1])) {
     end_cx++;
  }
  while (end_cx < row->size && isspace(row->chars[end_cx])) {                                              
     end_cx++;                                                                                              
  }                                                                                                        
  if (end_cx >= row->size && E.cy < E.numrows) {
     E.cy++;
     end_cx = 0;
     row = &E.row[E.cy];                                                                                  
     while (end_cx < row->size && isspace(row->chars[end_cx])) {                                              
        end_cx++;                                                                                              
     }                                                                                                        
  }
  E.cx = end_cx;
}

void editorMoveWordBackward() {
  if (E.cy == 0 && E.cx == 0) return;

  if (E.cx > 0) {
    int c = E.row[E.cy].chars[E.cx - 1];
    if (isspace(c)) {
      while (E.cx > 0 && isspace(E.row[E.cy].chars[E.cx - 1])) {
        E.cx--;
      }
      while (E.cx > 0 && is_word_char(E.row[E.cy].chars[E.cx - 1])) {
        E.cx--;
      }
    } else if (is_word_char(c)) {
      while (E.cx > 0 && is_word_char(E.row[E.cy].chars[E.cx - 1])) {
        E.cx--;
      }
    } else {
      while (E.cx > 0 && !isspace(E.row[E.cy].chars[E.cx - 1]) &&
             !is_word_char(E.row[E.cy].chars[E.cx - 1])) {
        E.cx--;
      }
    }
  } else if (E.cy > 0) {
    E.cy--;
    E.cx = E.row[E.cy].size;
    editorMoveWordBackward();
  }
}

void editorMoveLineStart() {
  E.cx = 0;
}

void editorMoveLineEnd() {
  if (E.cy < E.numrows) {
    E.cx = E.row[E.cy].size-1;
  }
}

void editorMoveFileTop() {
  E.cy = 0;
  E.cx = 0;
}

void editorMoveFileBottom() {
  if (E.numrows > 0) {
    E.cy = E.numrows - 1;
    E.cx = 0;
  }
}
