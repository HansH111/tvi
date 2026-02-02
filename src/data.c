#include "tvi.h"

/*** data ***/

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

char *PY_HL_extensions[] = { ".py", NULL };
char *PY_HL_keywords[] = {
  "if", "elif", "else", "for", "while", "break", "continue", "return",
  "def", "class", "import", "from", "as", "try", "except", "finally",
  "with", "pass", "raise", "lambda", "and", "or", "not", "in", "is",
  "True", "False", "None",

  "int|", "float|", "str|", "list|", "dict|", "tuple|", "bool|", NULL
};

char *JS_HL_extensions[] = { ".js", ".jsx", NULL };
char *JS_HL_keywords[] = {
  "if", "else", "for", "while", "do", "switch", "case", "break", "continue",
  "return", "function", "const", "let", "var", "class", "extends",
  "import", "export", "default", "try", "catch", "finally", "throw",
  "new", "this", "typeof", "instanceof",

  "function|", NULL
};

char *RS_HL_extensions[] = { ".rs", NULL };
char *RS_HL_keywords[] = {
  "fn", "let", "mut", "const", "if", "else", "match", "for", "while",
  "loop", "break", "continue", "return", "struct", "enum", "impl",
  "trait", "pub", "mod", "use", "as", "self", "super", "where",
  "true", "false", "Some", "None", "Ok", "Err",

  "i8|", "i16|", "i32|", "i64|", "i128|", "u8|", "u16|", "u32|", "u64|",
  "u128|", "f32|", "f64|", "bool|", "char|", "str|", "String|", "Vec|",
  "Option|", "Result|", NULL
};

char *PL_HL_extensions[] = { ".pl", ".pm", NULL };
char *PL_HL_keywords[] = {
  "and", "cmp", "do", "eq", "foreach", "ge", "if", "le", "lt", "ne", "package",
  "qq","qx","qw", "tr", "unless", "until", "while", "for", "break", "continue",
  "next", "return", "else", "elsif", "switch", "sub", "split", "open", "print",
  "printf", "open", "shift", "substr",

  "my|", "our|", NULL
};

struct editorSyntax HLDB[] = {
  {
    "c",
    C_HL_extensions,
    C_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "python",
    PY_HL_extensions,
    PY_HL_keywords,
    "#", NULL, NULL,
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "javascript",
    JS_HL_extensions,
    JS_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "rust",
    RS_HL_extensions,
    RS_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "perl",
    PL_HL_extensions,
    PL_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
};
