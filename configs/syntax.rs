# === Rust syntax highlighting ===
#
# --- 256-color codes for: normal, kw1, kw2, comment/mlcomment, string, number, match, notprintable
colors256  : 249 117 48 73 73 246 246 195 196
#
# --- control flow keywords (=kw1)
keywords   : fn let mut const if else match for while loop break continue return struct
keywords   : enum impl trait pub mod use as self super where true false Some None Ok Err
#
# --- type keywords (=kw2)
keywords   : i8| i16| i32| i64| i128| u8| u16| u32| u64| u128| f32| f64| bool| char| str|
keywords   : String| Vec| Option| Result|
#
# --- single-line comment
remark     : //
#
# --- multi-line comment start/end
remarkstart: /*
remarkend  : */
#
#EOF
