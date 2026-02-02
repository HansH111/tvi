#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "tre.h"

// Global configuration variables (initialized to safe defaults)
int tre_max_pattern_length  = TRE_DEFAULT_MAX_PATTERN_LENGTH;
int tre_max_depth           = TRE_DEFAULT_MAX_RECURSION_DEPTH;
int tre_max_backtrack_steps = TRE_DEFAULT_MAX_BACKTRACK_STEPS;

int tre_last_error = 0;

int tre_peak_backtrack = 0;
int tre_peak_recursion = 0;

// Internal backtracking step counter (reset for each match call)
static int tre_backtrack_steps = 0;
static int tre_igncase = 0;                    // Case-sensitive by default

void tre_reset_peaks(void) {
    tre_peak_backtrack = 0;
    tre_peak_recursion = 0;
}

static int matchcompare(char a, char b) {
    return tre_igncase ? (tolower((unsigned char)a) == tolower((unsigned char)b)) : (a == b);
}

/* Helper: check if ch is in [class] or [^class], supports a-z ranges */
static int matchinclass(char ch, const char *cls) {
    int negate = 0;
    if (*cls == '^') { negate = 1; cls++; }

    int matched = 0;
    while (*cls && *cls != ']') {
        if (cls[1] == '-' && cls[2] && cls[2] != ']') {
            char low = *cls;
            char high = cls[2];
            if (tre_igncase) {
                low  = tolower((unsigned char)low);
                high = tolower((unsigned char)high);
                ch   = tolower((unsigned char)ch);
            }
            if (ch >= low && ch <= high) matched = 1;
            cls += 2;
        } else {
            if (matchcompare(ch, *cls)) matched = 1;
            cls++;
        }
    }
    return negate ? !matched : matched;
}

// Returns: number of chars matched in text (0 or 1)
// Advances *regexp_ptr past the atom AND any following {n} quantifier
// Sets *repeat_count = n if {n} was found, otherwise *repeat_count = 1
static int matchoneatom(char **regexp_ptr, char *text, int *repeat_count)
{
    char *re = *regexp_ptr;
    *repeat_count = 1;
    if (*text == '\0') return 0;

    char *start_re = re;
    // Match one unit (same as before)
    if (re[0] == '\\' && re[1] != '\0') {
        if (matchcompare(*text, re[1])) re += 2;
        else return 0;
    } else if (re[0] == '[') {
        char *close = strchr(re, ']');
        if (!close || !matchinclass(*text, re+1)) return 0;
        re = close + 1;
    } else if (re[0] == '.') {
        re++;
    } else if (matchcompare(*text, re[0])) {
        re++;
    } else return 0;

    // Now check for {n}
    if (*re == '{') {
        re++;  // skip {
        int n = 0;
        while (*re >= '0' && *re <= '9') {
            n = n * 10 + (*re - '0');
            re++;
        }
        if (n == 0 || *re != '}') { // invalid → treat as literal or fail
            if (tre_last_error == TRE_OK) tre_last_error = TRE_ERROR_MALFORMED_PATTERN;
            *regexp_ptr = start_re;
            return 0;
        }
        re++;           // skip }
        *repeat_count = n;
    }
    *regexp_ptr = re;
    return 1;  // matched one character
}

/* matchhere: match regexp at the beginning of text */
static char* matchhere(char *regexp, char *text, int *outlen, int depth)
{
    if (outlen) *outlen = 0;
    if (depth > tre_peak_recursion)   tre_peak_recursion = depth;
    if (depth > tre_max_depth) {
        if (tre_last_error == TRE_OK) tre_last_error = TRE_ERROR_RECURSION_DEPTH;
        return NULL;
    }
    if (regexp[0] == '\0') return text;

    // $ : end of string (only when it's the last thing in the pattern)
    if (regexp[0] == '$' && regexp[1] == '\0') {
        if (*text == '\0') return text;
        return NULL;
    }

    char *next_re = regexp;
    int repeat_count;
    int consumed = matchoneatom(&next_re, text, &repeat_count);
    if (consumed == 0) return NULL;  // atom didn't match

    // ─────────────────────────────────────────────────────
    // Now handle repetition — either {n} or * / + / ?
    // ─────────────────────────────────────────────────────
    char *after_quant = next_re;
    int min_rep = repeat_count;  // usually 1, or n from {n}
    int max_rep = repeat_count;  // same for exact count

    char q = next_re[0];
    if (q == '*' || q == '+' || q == '?') {
        if (q == '*') { min_rep = 0; max_rep = -1; }
        if (q == '+') { min_rep = 1; max_rep = -1; }
        if (q == '?') { min_rep = 0; max_rep =  1; }
        after_quant++;
    } else if (q == '{') {
        // We already parsed {n} in matchoneatom → nothing more to do here
        // repeat_count is already set to n
        after_quant++;  // skip past the {n} we already consumed
    }

    // ─────────────────────────────────────────────────────
    // Greedy repetition (eat as many as possible)
    // ─────────────────────────────────────────────────────
    char *start = text;
    text += consumed;
    int count = 1;  // we already matched one

    // Consume more repetitions greedily
    while ((max_rep < 0 || count < max_rep) && *text != '\0') {
        if (++tre_backtrack_steps > tre_peak_backtrack)   tre_peak_backtrack = tre_backtrack_steps;
        if (tre_backtrack_steps > tre_max_backtrack_steps) {
            if (tre_last_error == TRE_OK) tre_last_error = TRE_ERROR_BACKTRACK_LIMIT;
            return NULL;
        }

        char *probe_re = regexp;           // reset to beginning of repeated atom
        int probe_rep;
        int probe = matchoneatom(&probe_re, text, &probe_rep);
        if (probe == 0) break;
        text += probe;
        count++;
    }

    // Backtrack from max down to min_rep
    while (count >= min_rep) {
        int rest_len = 0;
        char *res = matchhere(after_quant, text, &rest_len, depth + 1);
        if (res) {
            if (outlen) *outlen = (int)((text - start) + rest_len);
            return start;
        }
        if (++tre_backtrack_steps > tre_peak_backtrack)   tre_peak_backtrack = tre_backtrack_steps;
        if (tre_backtrack_steps > tre_max_backtrack_steps) {
            if (tre_last_error == TRE_OK) tre_last_error = TRE_ERROR_BACKTRACK_LIMIT;
            return NULL;
        }
        // Undo one repetition (simple — assumes 1 char per repetition)
        if (count == min_rep) break;
        count--;
        text--;   // back up one character
    }
    return NULL;
}

/* match: search for regexp anywhere in text (unless ^) */
char* match(char *regexp, char *text, int *length, int igncase, int direction)
{
    tre_last_error = TRE_OK;
    if (length) *length = 0;

    if (!regexp || !text) {
        tre_last_error = TRE_ERROR_MALFORMED_PATTERN;
        return NULL;
    }
    // Check pattern length limit
    if ((int)strlen(regexp) > tre_max_pattern_length) {
        tre_last_error = TRE_ERROR_PATTERN_TOO_LONG;
        return NULL;
    }
    // Set global configuration for internal use (performance optimization)
    tre_igncase = igncase;

    // Reset backtrack step counter for this match operation
    tre_backtrack_steps = 0;

    if (regexp[0] == '^') {
        char *res = matchhere(regexp + 1, text, length, 0);
        if (!res && tre_last_error == TRE_OK) tre_last_error = TRE_ERROR_NO_MATCH;
        return res;
    }
    if (direction == -1) {
        char *p = text + strlen(text);
        while (p >= text) {
            char *res = matchhere(regexp, p, length, 0);
            if (res) return res;
            if (p == text) break;
            p--;
        }
    } else {
        char *p = text;
        do {
            char *res = matchhere(regexp, p, length, 0);
            if (res) return res;
        } while (*p++);
    }
    if (tre_last_error == TRE_OK) tre_last_error = TRE_ERROR_NO_MATCH;
    return NULL;
}
