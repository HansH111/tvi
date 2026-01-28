# === C syntax highlighting ===
#
# --- 256-color codes for: normal, kw1, kw2, comment/mlcomment, string, number, match, notprintable
colors256  : 249 117 48 73 73 246 246 195 196
#
# --- control flow keywords (=kw1)
keywords  : if else do while for break continue return goto switch case default
#
# --- type keywords with | at the end (=kw2)
keywords   : int| long| double| float| char| unsigned| signed| void| short| 
keywords   : struct| union| typedef| static| enum| cons|t volatile| auto| register|
keywords   : sizeof| typeof| class| public| private| protected| virtual| override|
keywords   : true| false| bool|
#
# --- single-line comment
remark     : //
#
# --- multi-line comment start/end
#remarkstart: /*
#remarkend  : */
#
# --- highlighting number, strings and searchmatch
numbers
strings
searchmatch
#
#EOF
