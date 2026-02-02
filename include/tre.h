#ifndef TRE_H
#define TRE_H

// Basic safety restrictions
#define TRE_DEFAULT_MAX_PATTERN_LENGTH     64    // Default max regex pattern length
#define TRE_DEFAULT_MAX_RECURSION_DEPTH   128    // Default max recursive calls
#define TRE_DEFAULT_MAX_BACKTRACK_STEPS 20480    // Default max backtracking steps


// Error codes (returned in tre_last_error when match() returns NULL)
#define TRE_OK                        0
#define TRE_ERROR_NO_MATCH            1   // normal "no match" (not really an error)
#define TRE_ERROR_PATTERN_TOO_LONG    2
#define TRE_ERROR_RECURSION_DEPTH     3
#define TRE_ERROR_BACKTRACK_LIMIT     4
#define TRE_ERROR_MALFORMED_PATTERN   5   // e.g. unbalanced { }, invalid escape, etc.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * match - search for regexp anywhere in text (unless ^)
 *
 * @param regexp   regular expression pattern
 * @param text     input string to search
 * @param length   [out] length of matched substring (optional, can be NULL)
 * @param igncase  1 = case-insensitive, 0 = case-sensitive
 * @param direction 1 = forward search, -1 = backward search
 *
 * @return pointer to the start of the match in text, or NULL if no match
 *
 * NOTE: For optimal performance in single-threaded embedded environments,
 *       the igncase parameter is automatically set in the global tre_igncase variable
 *       for use by internal functions. Safety limits are configurable via globals.
 *
 * Supported features:
 * - Literals: abc, hello123
 * - Any character: .
 * - Character classes: [abc], [^0-9], [a-zA-Z0-9]
 * - Quantifiers: * (zero or more), + (one or more), ? (zero or one), {n} (exact repetition)
 * - Anchors: ^ (start), $ (end)
 * - Escaping: \., \*, \+, \?, \[, \\, etc.
 * - Case-insensitive mode via igncase parameter
 *
 * Limitations:
 * - No non-greedy quantifiers (*?, +?, ??, {n}?)
 * - No alternation (|)
 * - No grouping ((...))
 * - No backreferences
 * - No lookahead/lookbehind
 * - Greedy quantifiers only
 */
char* match(char *regexp, char *text, int *length, int igncase, int direction);

// Reset the peak trackers to zero
void tre_reset_peaks(void);

#ifdef __cplusplus
}
#endif

// Global configuration variables (for single-threaded use)
// Set these before calling match() to configure behavior
extern int tre_max_pattern_length;   // Max pattern length     (default: TRE_DEFAULT_MAX_PATTERN_LENGTH)
extern int tre_max_depth;            // Max recursion depth    (default: TRE_DEFAULT_MAX_RECURSION_DEPTH)
extern int tre_max_backtrack_steps;  // Max backtracking steps (default: TRE_DEFAULT_MAX_BACKTRACK_STEPS)
extern int tre_last_error;

// Global high-water mark trackers (persistent until tre_reset_peaks() is called)
extern int tre_peak_backtrack;     // highest backtrack steps seen in any match() so far
extern int tre_peak_recursion;     // deepest recursion level reached in any match() so far

#endif /* TRE_H */
