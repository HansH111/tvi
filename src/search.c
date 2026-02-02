#include "tvi.h"
#include "tre.c"

/* ==== Search ==== */

static char *regex=NULL;
static int  regex_case = 0;
static int  regex_stat = 0;

void editorSearchExit() {
    if (regex_stat) {
        free(regex);
        regex_stat=0;
    }
}

int editorSearchInit(char *pattern, int igncase __attribute__((unused))) {
    if (regex_stat) editorSearchExit();
    regex_case = E.ignore_case;

    regex=strdup(pattern);
    int len=strlen(regex);
    if (len > 2  && regex[len-2] == '\\' && strchr("Cc",regex[len-1])!=NULL) {
        regex[len-2]='\0';
        regex_case = regex[len-1]=='c' ? 1 : 0;
    } 
    regex_stat = 1;
#if DEBUG
    fprintf(stderr,"# search pattern=%s  ingcase=%d\n",regex,regex_case);
#endif
    return 0;      
}

int editorSearch(char *text, int *len, int direction) {
    int matchlen;
    char *found=match(regex, text, &matchlen, regex_case, direction);
    if (found) {
        int idx=(int)(found - text);
        if (len) *len=matchlen;
#if DEBUG
        fprintf(stderr,"- search peak recursion=%d backtrack=%d\n",tre_peak_recursion,tre_peak_backtrack);
#endif
        return idx;
    }
    return -1;
}

