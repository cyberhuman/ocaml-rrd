open Rrd
open Printf

type cf =
| AVERAGE
| MIN
| MAX
| LAST

let create ?start name period attrs = 
	let name = sprintf "%s.rrd" name in
	let f attr acc = 
		let ds = sprintf "DS:%s:GAUGE:%i:U:U" attr (period * 2) in
		let rra1 = sprintf "RRA:AVERAGE:0.5:%i:%i" (60 / period) (60) in (* 1 hour worth of 1 min avgs *)
		let rra2 = sprintf "RRA:AVERAGE:0.5:%i:%i" (60*5 / period) (12*24) in (* 1 day worth of 5 min avgs *)
		let rra3 = sprintf "RRA:AVERAGE:0.5:%i:%i" (60*60 / period) (24*7) in (* 1 week worth of 1 hour avgs *)
		let rra4 = sprintf "RRA:AVERAGE:0.5:%i:%i" (60*60*24 / period) (365) in (* 1 year worth of 1 day avgs *)
			ds::rra1::rra2::rra3::rra4::acc
	in
	let series = List.fold_right f attrs [] in
	let series = Array.of_list series in
	let start = match start with Some i -> i | None -> int_of_float (Unix.time () -. 10.) in
		rrd_create name (Int64.of_int period) start series

let store name time values =
	let name = sprintf "%s.rrd" name in
	let time = int_of_float time in
	let f (attr, value) acc =
		let attrs, values = acc in
			(attr::attrs, (string_of_int value)::values)
	in
	let attrs, values = List.fold_right f values ([], []) in
	let attrs = String.concat ":" attrs in
	let values = sprintf "%i:%s" time (String.concat ":" values) in
		rrd_update name attrs [| values |]

let fetch name start fin step =
	let name = sprintf "%s.rrd" name in
	let start = int_of_float start in
	let fin = int_of_float fin in
		rrd_fetch name "AVERAGE" start fin step
	
let test () =
	let period = 1 in
	
	print_endline "RRD Test";
	
	create "test" period [ "foo" ];

	let start = Unix.time () in

	let rec f i =
		let now = Unix.time () in
			print_endline (sprintf "Iteration %i" i);
			store "test" now ["foo", 1];
			if i > 0
			then (Unix.sleep period; f (pred i))
	in

	f 10;
	
	let fin = Unix.time () in

	let results = fetch "test" start fin period in

	let f (t, vs) = 
		let vs = Array.map (fun v -> sprintf "%f" v) vs in
		let vs = String.concat ", " (Array.to_list vs) in
			sprintf "%s: [%s]" t vs
	in
	let results = Array.map f results in
	let results = Array.to_list results in
	
		print_endline (String.concat ", " results)

let main () =
	Printexc.record_backtrace true;
	try test ()
	with e ->
		let backtrace = Printexc.get_backtrace () in
			print_endline (Printexc.to_string e);
			print_endline backtrace
	
let a : unit = main ()