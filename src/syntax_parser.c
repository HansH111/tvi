#include "tvi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Syntax configuration structure is now defined in tvi.h

// Forward declarations
//static char*  trim_whitespace(char *str);
static char** split_space_separated(char *str);
static int    parse_colors(char *str, int *colors, int max_colors);
static char*  join_strings(char *keywords, char *str);


int editorSyntaxLine(void *c, int sep, char *key, char *value)
{
  struct editorSyntax *config = (struct editorSyntax *)c;
#if DEBUG
  fprintf(stderr,"- %s : %s\n",key,value?value:"");
#endif
  if (sep == 0) {
      if (!strcmp(key, "numbers"    )) config->flags |= HL_HIGHLIGHT_NUMBERS;
      if (!strcmp(key, "strings"    )) config->flags |= HL_HIGHLIGHT_STRINGS;
      if (!strcmp(key, "searchmatch")) config->flags |= HL_MATCH;
  } else {
      if (!strcmp(key, "colors256")) {
          parse_colors(value, config->colors, 8);
      } else if (!strcmp(key, "keywords")) {
          config->keywstr = join_strings(config->keywstr, value);
      } else if (!strcmp(key, "remark")) {
          config->singleline_comment_start = strdup(value);
      } else if (!strcmp(key, "remarkstart")) {
          config->multiline_comment_start = strdup(value);
      } else if (!strcmp(key, "remarkend")) {
          config->multiline_comment_end = strdup(value);
      }
  }
  return 1;
}

// Parse a syntax file and return a SyntaxConfig
struct editorSyntax* parseSyntaxFile(char* filename) {

    struct editorSyntax *config = calloc(1, sizeof(struct editorSyntax));
    if (!config) return NULL;

    // Initialize defaults
    config->flags = 0;
    config->keywstr = NULL;
    for (int i=0; i<8; i++) config->colors[i]=15;

#if DEBUG
  fprintf(stderr,"# syntax load %s\n",filename);
#endif
    int rc=editorReadFile(filename,':',editorSyntaxLine,(void *)config);
    if (rc == 0) {
       free(config);
       return NULL;
    }
    config->keywords = split_space_separated(config->keywstr);
    if (config->keywstr) free(config->keywstr);
    return config;
}

static char** split_space_separated(char *str) {
    if (!str || !*str) return NULL;
    // Count tokens
    const char *p = str;
    int num_tokens = 0;
    while (*p) {
        while (*p && isspace(*p)) p++;
        if (*p) {
            num_tokens++;
            while (*p && !isspace(*p)) p++;
        }
    }
    if (num_tokens == 0) return NULL;
#if DEBUG
    fprintf(stderr,"- found %d tokens\n",num_tokens);
#endif
    char **result = malloc(sizeof(char*) * (num_tokens + 1));
    if (!result) return NULL;

    int idx = 0;
    p = str;
    while (*p && idx < num_tokens) {
        while (*p && isspace(*p)) p++;
        if (*p) {
            const char *start = p;
            while (*p && !isspace(*p)) p++;
            size_t len = p - start;
            result[idx] = malloc(len + 1);
            if (!result[idx]) {
                // Free already allocated strings
                for (int i = 0; i < idx; i++) free(result[i]);
                free(result);
                return NULL;
            }
            memcpy(result[idx], start, len);
            result[idx][len] = '\0';
            idx++;
        }
    }
    result[idx] = NULL;
    return result;
}

static char* join_strings(char *keywords, char *str) {
    if (!str || !*str) return keywords;
    if (keywords) {
        size_t len=strlen(keywords);
        keywords=realloc(keywords,len+strlen(str)+2);
        if (keywords) {
           strcat(&keywords[len++]," ");
           strcat(&keywords[len]  ,str);
        }
    } else {
        keywords=malloc(strlen(str)+1);
        if (keywords) strcpy(keywords,str);
    }
    return keywords;
}

// Parse space-separated color codes
static int parse_colors(char *str, int *colors, int max_colors) {
    if (!str) return 0;

    // -- set to default colors
    for (int i=0; i<max_colors; i++)  colors[i] = 249;

    char *copy = strdup(str);
    if (!copy) return 0;

    int count = 0;
    char *token = strtok(copy, " \t");
    while (token && count < max_colors) {
        char *endptr;
        long val = strtol(token, &endptr, 10);
        if (*endptr != '\0' || val < 0 || val > 255) {
            count++;        // skip value, keep default color
        } else {
            colors[count++] = (int)val;
        }
        token = strtok(NULL, " \t");
    }
    free(copy);
    return count == max_colors;  // Should have exactly max_colors
}
