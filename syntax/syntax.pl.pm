# === Perl syntax highlighting ===
#
# --- 256-color codes for: normal, kw1, kw2, comment/mlcomment, string, number, match, notprintable
colors256  : 249 117 48 73 73 246 246 195 196
#
# --- control flow keywords (=kw1)
keywords   : and cmp do eq ge if le lt ne or package qq qx qw tr unless until while
keywords   : for foreach break continue next return else elsif switch sub split open print
keywords   : printf shift substr
#
# --- type keywords (=kw2)
keywords2  : my| our|
#
# --- single-line comment
remark     : #
#
# --- highlight numbers, strings and searchmatch
numbers
strings
searchmatch
#
#EOF
