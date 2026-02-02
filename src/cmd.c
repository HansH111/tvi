#include "tvi.h"

/*** command parsing and execution ***/

void editorCommandSubstitute(int from, int until, char *cmd) {
#if DEBUG
    fprintf(stderr,"# substitute from:%d until:%d  s=%s",from,until,cmd);
#endif
    if (*cmd != '/') return;

    cmd++;
    const char *pattern_start = cmd;
    // Substitute: s/pattern/replacement/[g]
    const char *pattern_end = strchr(pattern_start, '/');
    if (!pattern_end) {
        editorSetStatusMessage("Invalid substitute command");
        return;
    }

    char pattern[256] = {0};
    int pattern_len = pattern_end - pattern_start;
    if (pattern_len >= 256) pattern_len = 255;
    strncpy(pattern, pattern_start, pattern_len);
    pattern[pattern_len] = '\0';

    const char *replacement_start = pattern_end + 1;
    const char *replacement_end = strrchr(replacement_start, '/');
#if DEBUG
    fprintf(stderr,"  substitute pattern=%s start=%s\n",pattern,replacement_start);
#endif
    if (!replacement_end) {
        editorSetStatusMessage("Invalid substitute command");
        return;
    }

    char replacement[256] = {0};
    int replacement_len = replacement_end - replacement_start;
    if (replacement_len >= 256) replacement_len = 255;
    strncpy(replacement, replacement_start, replacement_len);
    replacement[replacement_len] = '\0';

    int global  = (strchr(&replacement_end[1],'g')!=NULL) ? 1 : 0;
#if REGEX
    int igncase = (strchr(&replacement_end[1],'i')!=NULL) ? 1 : 0;
    editorSearchInit(pattern,igncase);
#endif
#if DEBUG
    fprintf(stderr,"  substitute pattern=%s replace=%s\n",pattern,replacement);
#endif
    int substitutions = 0;
    int len=strlen(pattern);
    for (int i = from; i < until; i++) {
        if (i >= E.numrows) continue;
        char *line = E.row[i].chars;
        char *new_line = NULL;
        char *new_p = NULL;

        char *p = line;
        int line_changed = 0;
        while (*p) {
            #if REGEX
                int m = editorSearch(p, &len, 1);
                char *match = m < 0 ? NULL : &p[m];
                    #if DEBUG
 if (match) fprintf(stderr,"- subst peak recursion=%d  backtrack=%d\n",tre_peak_recursion,tre_peak_backtrack);
                    #endif
            #else
                char *match=strstr(p, pattern);
            #endif
            if (match && new_line==NULL) {
                new_line = malloc(E.row[i].size * 2 + 1); // Safe buffer
                if (!new_line) break;
                new_p = new_line;
            }
            if (!match) {
                if (new_line) {
                   strcpy(new_p, p);
                   new_p += strlen(p);
                }
                break;
            }
            // Copy text before match
            int before_len = match - p;
            strncpy(new_p, p, before_len);
            new_p += before_len;

            // Copy replacement
            strcpy(new_p, replacement);
            new_p += replacement_len;
            p = match + len;

            substitutions++;
            line_changed = 1;
            if (!global) break; // Only first match per line
        }
        if (line_changed) {
            editorTrackModifyLine(i, 0, E.row[i].chars, E.row[i].size);
            free(E.row[i].chars);
            free(E.row[i].render);
            free(E.row[i].hl);

            E.row[i].chars = strdup(new_line);
            E.row[i].size = strlen(new_line);
            E.row[i].render = NULL;
            E.row[i].hl = NULL;
            E.row[i].hl_open_comment = 0;
            editorUpdateRow(&E.row[i]);
        }
        if (new_line) free(new_line);
    }
    editorSetStatusMessage("Substituted %d occurrence(s)", substitutions);
}

void editorCommandAction(int from, int until, char act, int force, char *args) {
    if (act == 'x') act='W';

    if (tolower(act) == 'w') {
        if (args && *args) {
            // Write to specific filename
            char *old_filename = E.filename;
            E.filename = strdup(args);

            // Check if file exists and we're not forcing overwrite
            if (!force) {
                FILE *check_fp = fopen(args, "r");
                if (check_fp) {
                    fclose(check_fp);
                    editorSetStatusMessage("File %s exists. Use :w! to overwrite", args);
                    free(E.filename);
                    E.filename = old_filename;
                    return;
                }
            }
            editorSave(from, until - from + 1);
            free(old_filename);
        } else {
            editorSave(from, until - from + 1);
        }
        if (act == 'W') {
            write2screen("\x1b[2J", 4);
            write2screen("\x1b[H", 3);
            exit(0);
        }
    } else if (act == 'q') { 
        if (E.dirty && force == 0) {
            editorSetStatusMessage("file contains unsaved changes. Use q! to override");
        } else {
            write2screen("\x1b[2J", 4);
            write2screen("\x1b[H", 3);
            exit(0);
        }
    } else if (act == 'r') {
        if (args && *args) {
            // Read file at current position
            FILE *fp = fopen(args, "r");
            if (!fp) {
                editorSetStatusMessage("Error: cannot open file %s", args);
                return;
            }
            char line[1024];
            while (fgets(line, sizeof(line), fp)) {
                size_t len = strlen(line);
                if (len > 0 && line[len-1] == '\n') {
                    line[len-1] = '\0';
                    len--;
                }
                editorInsertRow(E.cy + 1, line, len);
                E.cy++;
            }
            fclose(fp);
            editorSetStatusMessage("Read %d lines from %s", E.cy + 1, args);
        } else {
            editorSetStatusMessage("Usage: :r <filename>");
        }
    } else if (act == 's') {
        // Check if this is a "set" command (not substitute)
        if (args && strncmp(args, "et", 2) == 0) {
            editorProcessSetCommand(args + 2); // Skip "et"
        } else {
            editorCommandSubstitute(from,until,args);
        }
    } else if (act == 'h') {
        editorSetStatusMessage("Commands: w,q,q!,wq,x,r,h,set | Range: :from:until w[!] filename");
    } else {
        editorSetStatusMessage("Unknown command: %c %s. Type :h for available commands",act,args);
    }
}

void editorProcessDefaultCommand() {
    char *cmd=editorPrompt(":%s", NULL);
    if (cmd == NULL) return;

    if (!cmd || !*cmd) return;
    /* trim line */
    for (int i=strlen(cmd) -1; (i>0 && cmd[i] <= 32); i--) cmd[i]='\0';

    // can be: <number> or % . $ 
    //         <number>,<number>cmd   
    //         <cmd>
    int  p, no=0, range=0, force=0, number[2]={1, E.numrows};
    char act=0;

    for (p=0; cmd[p]; p++) {
        if (isalpha(cmd[p])) {
            act=tolower(cmd[p]);
            cmd[p++]='\0';
            if (no==1 && range) number[1]=atoi(&cmd[range]);
            break;
        } else if (no==0 && cmd[p] == '%') {
            number[0] = 1;
            number[1] = E.numrows;
            no=2;
            range=0;
        } else if (cmd[p] == '.') {
            number[no++] = E.cy+1;
            if (cmd[p+1] == ',') {
                cmd[++p]='\0';
                range=p+1;
            }
        } else if (no<2 && cmd[p] == '$') {
            number[no++] = E.numrows;
        } else if (no==0 && cmd[p] == ',') {
            cmd[p]='\0';
            number[0]=atoi(&cmd[0]);
            range=p+1;
            no++;
        } else if (!isdigit(cmd[p])) {  // single number 
            number[0]=atoi(cmd);
        }
    }
    for (int i=0; i<2; i++) {  // sanatize the numbers
         if (number[i] < 1)          number[i]=1;
         if (number[i] > E.numrows)  number[i]=E.numrows;
         number[i]--;
    }
    if (!act) {			          // only rownumber is specified
       E.cy = number[0];
       E.cx=0;
       return;
    }
    if (act == 'w' && cmd[p] && cmd[p] == 'q') {
        act='W'; 
        p++;
    }
    if (cmd[p] && cmd[p] == '!') {        // check if action is forced
        force=1;
        p++;
    }
    while (cmd[p] && cmd[p] == ' ') p++;  // skip any spaces

    editorCommandAction(number[0],number[1],act,force,&cmd[p]);
    free(cmd);
}

