
external rrd_create : string -> int64 -> int -> string array -> unit = "caml_rrd_create"
external rrd_update : string -> string -> string array -> unit = "caml_rrd_update_r"
external rrd_fetch : string -> string -> int -> int -> int -> (string * float array) array = "caml_rrd_fetch_r"
external rrd_fetch_ex : string -> string -> int -> int -> int -> (string * float array) array * int * int * int = "caml_rrd_fetch_ex_r"
