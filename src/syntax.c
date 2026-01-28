#include "tvi.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>

/*** syntax highlighting ***/

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);

  if (E.syntax == NULL) return;

  char **keywords = E.syntax->keywords;
  char *scs = E.syntax->singleline_comment_start;
  char *mcs = E.syntax->multiline_comment_start;
  char *mce = E.syntax->multiline_comment_end;

  int scs_len = scs ? strlen(scs) : 0;
  int mcs_len = mcs ? strlen(mcs) : 0;
  int mce_len = mce ? strlen(mce) : 0;

  int in_string = 0;
  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

  int i = 0;
  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char hl = row->hl[i];

    if (hl == HL_COMMENT) {
      row->hl[i] = HL_COMMENT;
      i++;
      continue;
    }

    if (hl == HL_MLCOMMENT) {
      row->hl[i] = HL_MLCOMMENT;
      if (mce_len && !strncmp(row->render + i, mce, mce_len)) {
        memset(row->hl + i, HL_NORMAL, mce_len);
        i += mce_len;
        row->hl_open_comment = 0;
        continue;
      } else {
        i++;
        continue;
      }
    }

    if (scs_len && !in_comment && !in_string &&
        !strncmp(row->render + i, scs, scs_len)) {
      memset(row->hl + i, HL_COMMENT, scs_len);
      memset(row->hl + i + scs_len, HL_COMMENT, row->rsize - i - scs_len);
      break;
    }

    if (mcs_len && !in_string && !in_comment &&
        !strncmp(row->render + i, mcs, mcs_len)) {
      memset(row->hl + i, HL_MLCOMMENT, row->rsize - i);
      i += mcs_len;
      row->hl_open_comment = 1;
      in_comment = 1;
      continue;
    }

    if (in_comment) {
      row->hl[i] = HL_MLCOMMENT;
      i++;
      continue;
    }

    if (in_string) {
      row->hl[i] = HL_STRING;
      if (c == '\\' && i + 1 < row->rsize) {
        row->hl[i + 1] = HL_STRING;
        i += 2;
        continue;
      }
      if (c == in_string) in_string = 0;
      i++;
      continue;
    }

    if (c == '"' || c == '\'') {
      in_string = c;
      row->hl[i] = HL_STRING;
      i++;
      continue;
    }

    if ((E.syntax->flags & HL_HIGHLIGHT_NUMBERS) &&
        (isdigit(c) || (c == '.' && i + 1 < row->rsize && isdigit(row->render[i + 1])))) {
      row->hl[i] = HL_NUMBER;
      while (i < row->rsize && !is_separator(row->render[i])) {
        if (row->hl[i] == HL_NUMBER) row->hl[i] = HL_NUMBER;
        i++;
      }
      continue;
    }

    if (hl != HL_STRING && hl != HL_COMMENT) {
      int j;
      for (j = 0; keywords[j]; j++) {
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2) klen--;

        if (i + klen <= row->rsize &&
            !strncmp(row->render + i, keywords[j], klen) &&
            (i + klen == row->rsize || is_separator(row->render[i + klen]))) {
          memset(row->hl + i, kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] == NULL) {
        i++;
      }
      continue;
    }

    i++;
  }

  row->hl_open_comment = in_comment;
}

int editorSyntaxToColor(int hl) {
  if (E.syntax) {
    // Use colors from syntax file
    switch (hl) {
      case HL_NORMAL: return E.syntax->colors[0];
      case HL_KEYWORD1: return E.syntax->colors[1];
      case HL_KEYWORD2: return E.syntax->colors[2];
      case HL_COMMENT:
      case HL_MLCOMMENT: return E.syntax->colors[3];
      case HL_STRING: return E.syntax->colors[4];
      case HL_NUMBER: return E.syntax->colors[5];
      case HL_MATCH: return E.syntax->colors[6];
      case HL_NONPRINT: return E.syntax->colors[7];
      default: return E.syntax->colors[0];
    }
  }

  // Fallback to hardcoded colors
  switch (hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 73;
    case HL_KEYWORD1: return 117;
    case HL_KEYWORD2: return 48;
    case HL_STRING: return 135;
    case HL_NUMBER: return 134;
    case HL_MATCH: return 195;
    case HL_NONPRINT: return 196; // Reddish for non-printable chars
    default: return 249;
  }
}

int findSyntaxDirFile(char *dirnm, char *ext, char *fn, size_t maxlen) {
    DIR *folder;
    struct dirent *entry;
    char *ptr1,*ptr2;

    #ifdef DEBUG                                         
    fprintf(stderr, "# syntax dirnm %s ext=%s\n", dirnm,ext);    
    #endif        

    folder = opendir(dirnm);
    if (folder == NULL) return 0;
    while ((entry = readdir(folder)) != NULL) {
        if (strncmp(entry->d_name,"syntax.",7)==0 
                && (ptr1=strstr(entry->d_name,ext))!=NULL) {
            if (strlen(entry->d_name) < maxlen) {
                strcpy(fn,entry->d_name);
                ptr2=strchr(&ptr1[1],'.');
                if (ptr2) *ptr2='\0';

                #ifdef DEBUG                                         
                fprintf(stderr, "- found %s [%s] = [%s]\n", fn,ptr1,ext);    
                #endif        
                if (strcmp(ptr1,ext)==0) {
                    closedir(folder);
                    return 1;
                } 
            } 
        }
    }
    closedir(folder);
    strcpy(fn,"");
    return 0;
}

// Find syntax file for an extension 
static char* findSyntaxFile(char *ext) {
    char path[PATH_MAX];
    char fn[31];

    // Check user config directory first
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, PATH_MAX, "%s/.config/tvi", home);
        if (findSyntaxDirFile(path, ext, fn, 30) == 1) {
            strcat(path,"/");
            strcat(path,fn);
            return strdup(path);
        }
    }
    // Check system directory
    strcpy(path,"/usr/local/share/tvi");
    if (findSyntaxDirFile(path, ext, fn, 30) == 1) {
        strcat(path,"/");
        strcat(path,fn);
        return strdup(path);
    }
    if (access(path, R_OK) == 0) return strdup(path);

    return NULL;
}

// Load syntax for a file
struct editorSyntax *loadSyntaxForFile(const char* filename) {
    if (!filename) return NULL;

    char *ext = strrchr(filename, '.');
    if (!ext) return NULL;

    char *syntax_fn=findSyntaxFile(ext);
    if (!syntax_fn) return NULL;

    struct editorSyntax* syntax = parseSyntaxFile(syntax_fn);                      
    if (syntax) {
        #ifdef DEBUG
        fprintf(stderr, "- Loaded syntax for %s from %s\n", ext, syntax_fn);
        #endif
        free(syntax_fn);
    }
    return syntax;
}

// Auto-create config directory
static void ensureConfigDirectory() {
    const char* home = getenv("HOME");
    if (!home) return;

    char config_dir[PATH_MAX];
    snprintf(config_dir, PATH_MAX, "%s/.config", home);

    // Create ~/.config if it doesn't exist
    struct stat st;
    if (stat(config_dir, &st) != 0) {
        mkdir(config_dir, 0755);
    }

    // Create ~/.config/tvi if it doesn't exist
    snprintf(config_dir, PATH_MAX, "%s/.config/tvi", home);
    if (stat(config_dir, &st) != 0) {
        mkdir(config_dir, 0755);
    }
}

void editorSelectSyntaxHighlight() {
  E.syntax = NULL;

  if (E.filename == NULL) return;

  // Ensure config directory exists
  ensureConfigDirectory();

  // Try to load syntax dynamically
  E.syntax = loadSyntaxForFile(E.filename);

// Update syntax highlighting for all rows
  int filerow;
  for (filerow = 0; filerow < E.numrows; filerow++) {
    editorUpdateSyntax(&E.row[filerow]);
  }
}
