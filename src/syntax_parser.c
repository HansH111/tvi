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
static char*  trim_whitespace(char *str);
static char** split_space_separated(const char *str);
static int    parse_colors(const char *str, int *colors, int max_colors);
static char*  join_strings(char *keywords, char *str);


// Parse a syntax file and return a SyntaxConfig
struct editorSyntax* parseSyntaxFile(const char* filename) {
    char *keywords = NULL;

    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    struct editorSyntax *config = calloc(1, sizeof(struct editorSyntax));
    if (!config) {
        fclose(fp);
        return NULL;
    }

    // Initialize defaults
    config->flags = 0;
    char *line = NULL;                                  
    size_t linecap = 0;                                 
    ssize_t linelen;                                    
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
       while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
        // Skip empty lines and comments
        if (!line[0] || line[0] == '#') continue;

        // Find the colon separator
        char *colon = strchr(line, ':');
        if (!colon) {
             trim_whitespace(line);
             if (!strcmp(line, "numbers"    )) config->flags |= HL_HIGHLIGHT_NUMBERS;
             if (!strcmp(line, "strings"    )) config->flags |= HL_HIGHLIGHT_STRINGS;
             if (!strcmp(line, "searchmatch")) config->flags |= HL_MATCH;
             continue;
        }
        *colon = '\0';
        char *key = trim_whitespace(line);
        char *value = trim_whitespace(colon + 1);

        if (!strcmp(key, "colors256")) {
            parse_colors(value, config->colors, 8);
        } else if (!strcmp(key, "keywords")) {
            keywords = join_strings(keywords, value);
        } else if (!strcmp(key, "remark")) {
            config->singleline_comment_start = strdup(value);
        } else if (!strcmp(key, "remarkstart")) {
            config->multiline_comment_start = strdup(value);
        } else if (!strcmp(key, "remarkend")) {
            config->multiline_comment_end = strdup(value);
        }
    }
    free(line);                                                                                 
    fclose(fp);          

    config->keywords = split_space_separated(keywords);
    if (config->keywords == NULL) {
        if (keywords) free(keywords);                                                                                 
        free(config);
        return NULL;
    }
    if (keywords) free(keywords);                                                                                 
    return config;
}

// Trim leading and trailing whitespace
static char* trim_whitespace(char *str) {
    if (!str) return NULL;

    // Trim leading whitespace
    while (isspace(*str)) str++;

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        *end = '\0';
        end--;
    }

    return str;
}

static char** split_space_separated(const char *str) {
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
        #if DEBUG
        fprintf(stderr,"- append keywords: %s\n",str);
        #endif
        keywords=realloc(keywords,len+strlen(str)+2);
        if (keywords) {
           strcat(&keywords[len++]," ");
           strcat(&keywords[len]  ,str);
        }
    } else {
        #if DEBUG
        fprintf(stderr,"- new keywords: %s\n",str);
        #endif
        keywords=malloc(strlen(str)+1);
        if (keywords) strcpy(keywords,str);
    }
    return keywords; 
}    

// Parse space-separated color codes
static int parse_colors(const char *str, int *colors, int max_colors) {
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
