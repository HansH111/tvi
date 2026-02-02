# === C++ syntax highlighting ===
#
# --- 256-color codes for: normal, kw1, kw2, comment/mlcomment, string, number, match, notprintable
colors256  : 249 117 48 73 73 246 246 195 196
#
# --- control flow keywords
keywords   : switch if while for break continue return else case default try catch throw
#
# --- type keywords (ends with a |)
keywords   : int| long| double| float| char| unsigned| signed| void| short| struct| union|
keywords   ; typedef| static| enum| const| volatile| auto| register| sizeof| typeof| class|
keywords   : public| private| protected| virtual| override| final| new| delete| this|
keywords   : namespace| using| template| typename| operator| friend| inline| explicit| mutable|
keywords   : constexpr| noexcept| nullptr| true| false| bool|
#
# --- single-line comment
remark     : //
# --- multi-line comment start/end
remarkstart: /*
remarkend  : */
#
#EOF
