open Array

module IH = IntHashtbl
module H  = Hashtbl

module BA = Bigarray
module A1 = BA.Array1
module A2 = BA.Array2
module A3 = BA.Array3

module I = Int32

type ia   = (int32, BA.int32_elt  , BA.c_layout) A1.t
type fa   = (float, BA.float32_elt, BA.c_layout) A1.t

type iaa  = (int32, BA.int32_elt  , BA.c_layout) A2.t
type faa  = (float, BA.float32_elt, BA.c_layout) A2.t

type iaaa = (int32, BA.int32_elt  , BA.c_layout) A3.t
type faaa = (float, BA.float32_elt, BA.c_layout) A3.t


type bigarr = fa

let eia : ia = A1.create BA.int32 BA.c_layout 0
let efa : fa = A1.create BA.float32 BA.c_layout 0

let make_ia (_F : int) v : ia =
  let arr = A1.create BA.int32 BA.c_layout _F in
    A1.fill arr (I.of_int v); arr

let init_ia (_F : int) (v : int -> int) : ia =
  let arr = A1.create BA.int32 BA.c_layout _F in
    for f = 0 to _F - 1 do arr.{f} <- I.of_int (v f); done;
    arr

let copy_ba (a : bigarr) : bigarr =
  let _P = A1.dim a in
  let b  = A1.create BA.float32 BA.c_layout _P in
    for p = 0 to _P - 1 do
      b.{p} <- a.{p};
    done;
    b

let make_ba (_F : int) (v : float) : bigarr =
  let arr = A1.create BA.float32 BA.c_layout _F in
    A1.fill arr v; arr

let init_ba (_F : int) (v : int -> float) : bigarr =
  let arr = A1.create BA.float32 BA.c_layout _F in
    for f = 0 to _F - 1 do arr.{f} <- v f; done;
    arr

let concat_ba (al : bigarr list) : bigarr =
  let h  = List.hd al in
  let _M = List.fold_right (fun a s -> s + A1.dim a) al 0 in
  let b  = make_ba _M h.{0} in
  let i  = ref 0 in
    List.iter (fun a ->
      for j = 0 to A1.dim a - 1 do
        b.{!i+j} <- a.{j};
      done;
      i := !i + A1.dim a;
              ) al;
    b

let create_matrix_ba (a : int) (b: int) (v : float) : bigarr array =
  init a (fun _ -> make_ba b v)

let fold_lefti_ba f z0 (a : bigarr) =
  let _P = A1.dim a in
  let rec fl p z =
    if p >= _P then z else
      fl (p+1) (f p a.{p} z) in
    fl 0 z0

let fold_left_ia f z0 (a : ia) =
  let _P = A1.dim a in
  let rec fl p z =
    if p >= _P then z else
      fl (p+1) (f z a.{p}) in
    fl 0 z0

let fold_left_ba f z0 a =
  let _P = A1.dim a in
  let rec fl p z =
    if p >= _P then z else
      fl (p+1) (f z a.{p}) in
    fl 0 z0

let iteri_ba f a =
  let _P = A1.dim a in
    for p = 0 to _P - 1 do
      f p a.{p};
    done;
    ()

let iter_ia f a =
  let _P = A1.dim a in
    for p = 0 to _P - 1 do
      f a.{p};
    done;
    ()

let multiply_ba a v =
  let _P = A1.dim a in
    for p = 0 to _P - 1 do
      a.{p} <- a.{p} *. v;
    done;
    ()

let bad_float_to_zero f = match classify_float f with
    FP_normal | FP_subnormal | FP_zero -> f
  | _ -> 0.

let bad_float_to_neginf f = match classify_float f with
    FP_normal | FP_subnormal | FP_zero -> f
  | _ -> neg_infinity

let _ = Random.self_init ()

let zero  = log 0.
let one   = log 1.
let addLog (x:float) (y:float) = 
  if x == zero then y
  else if y == zero then x
  else if x -. y > 32. then x
  else if x > y then (x +. log (1. +. exp (y -. x)))
  else if y -. x > 32. then y
  else (y +. log (1. +. exp (x -. y)))
let subLog (x:float) (y:float) = 
  if y == zero then x
  else if x <= y then zero
  else if x -. y > 32. then x
  else (x +. log (1. -. exp (y -. x)))

let ( *@ ) a b = a +. b
let (+@) a b = addLog a b
let (-@) a b = subLog a b
let (^@) a b = (exp b) *. a

let (/@) a b = 
  if classify_float a == FP_infinite && classify_float b == FP_infinite
  then 
    if a < 0. && b < 0. then 0.
    else if a > 0. && b > 0. then 0.
    else a
  else a -. b

let fabs x = if x < 0. then 0. -. x else x

let fold_lefti f z0 a =
  let _P = length a in
  let rec fl p z =
    if p >= _P then z else
      fl (p+1) (f p a.(p) z) in
    fl 0 z0

let h_size h = Hashtbl.fold (fun _ _ s -> s+1) h 0
let ih_size h = IntHashtbl.fold (fun _ _ s -> s+1) h 0

let maybe_read_line h =
  try
    Some (input_line h)
  with
      End_of_file -> None

let (vocabFwd : (string,int) Hashtbl.t) = Hashtbl.create 4
let (vocabBwd : (int,string) Hashtbl.t) = Hashtbl.create 4
let vocabSize = ref 1

let get_vocab str =
  if str = "#" then failwith "";
  try
    Hashtbl.find vocabFwd str
  with
      Not_found -> (
        let i = !vocabSize in
          Hashtbl.replace vocabFwd str i;
          Hashtbl.replace vocabBwd i str;
          incr vocabSize;
          i
      )

let get_vocab_str i =
  if i == 0 then "**BIAS**" else
    try Hashtbl.find vocabBwd i
    with Not_found -> "**UNKNOWN**"

let reset_vocab _D =
  Hashtbl.clear vocabFwd;
  Hashtbl.clear vocabBwd;
  vocabSize := 1;
  for d = 0 to _D - 1 do
    Hashtbl.replace vocabFwd (string_of_int d) !vocabSize;
    Hashtbl.replace vocabBwd !vocabSize (string_of_int d);
    incr vocabSize;
  done;
  ()

let printVectorVocab a =
  if length a == 0 then () else (
    if classify_float a.(0) <> FP_zero then
      Printf.printf "**BIAS** %10.20f\n" a.(0);
    for i = 1 to length a - 1 do
      try
        let s = Hashtbl.find vocabBwd i in
          if classify_float a.(i) <> FP_zero then
            Printf.printf "%s %10.20f\n" s a.(i)
      with
          Not_found -> ();
    done;
    ())

let printVectorVocab_ba kmap xTr a =
  if A1.dim a == 0 then () else (
    if abs_float a.{0} > 1e-10 then
      Printf.printf "**BIAS** %10.20f\n" a.{0};
    for i = 1 to A1.dim a - 1 do
      try
        let s = Hashtbl.find vocabBwd i in
          if abs_float a.{i} > 1e-10 then
            Printf.printf "%s %10.20f\n" s a.{i}
      with
          Not_found -> ();
    done;
    ())


let float_of_string_err (line_num : int) (s : string) : float =
  try float_of_string s
  with Failure err -> 
    failwith ("error: \"" ^ err ^ "\" occured on line " ^ string_of_int line_num ^ " when trying to parse \"" ^ s ^ "\" as a float")

let int_of_string_err (line_num : int) (s : string) : int =
  try int_of_string s
  with Failure err -> 
    failwith ("error: \"" ^ err ^ "\" occured on line " ^ string_of_int line_num ^ " when trying to parse \"" ^ s ^ "\" as an int")

let printIter inum maxi =
  let s = String.length (string_of_int inum) in
  let l = String.length (string_of_int maxi) in
    
    Printf.fprintf stderr "it %d%s" inum (String.make (l-s) ' ');
    flush stderr

let printDW f = Printf.fprintf stderr " dw %3.3e" f; flush stderr

let print_info quiet ppx err (tM : bigarr) tw ty useD (dM : bigarr) dw dy useS (sM : bigarr) sw sy =
  let pp = ppx tM tw ty in let de = err dM dw dy in let te = err sM sw sy in
    if not quiet then (
      Printf.fprintf stderr " pp %5.5e er %5.5f" (0.-.pp) (err tM tw ty);
      if useD then Printf.fprintf stderr " dpp %5.5e der %5.5f" (0.-.ppx dM dw dy) de;
      if useS then Printf.fprintf stderr " tpp %5.5e ter %5.5f" (0.-.ppx sM sw sy) te;
    );
    pp,de,te

let is_some = function
    Some s -> true
  | None -> false

let is_none = function
    Some s -> false
  | None -> true

let from_some = function
    Some s -> s
  | None -> failwith "from_some: None"

let bounded x = if x < 1e-6 then 1e-6 else if x > 1.-.1e-6 then 1.-.1e-6 else x

let bounded_log x = 
  if x >= -1e-50 then -1e-50
  else if x < -250. then -250. else x

let dotEx useBias w c x v =
  let d = ref (if useBias then w.(0).(c) else 0.) in
    for i = 0 to A1.dim x - 1 do
      let xi = I.to_int x.{i} in
        if xi < length w then 
          if c < length w.(xi) then
            d := !d +. w.(xi).(c) *. if A1.dim v == 0 then 1. else v.{i};
    done;
    !d

let dotExBA useBias w x v =
  let d  = ref (if useBias then w.{0} else 0.) in
  let dd = A1.dim w in
  let lv = A1.dim v == 0 in
    for i = 0 to A1.dim x - 1 do
      let xi = I.to_int x.{i} in
        if xi < dd then 
          d := !d +. w.{xi} *. if lv then 1. else v.{i};
    done;
    !d

let suffix_of str suf =
  let n = String.length str in
  let m = String.length suf in
    if m > n then false
    else String.sub str (n-m) m = suf

(* let my_open_in = open_in *)

let my_open_in f =
  (* this only works under unix; for windows, comment this version and
     uncomment the one above it *)
  if      f = "-" then stdin
  else if suffix_of f ".Z"   then Unix.open_process_in ("zcat " ^ f)
  else if suffix_of f ".gz"  then Unix.open_process_in ("gunzip -c " ^ f)
  else if suffix_of f ".bz2" then Unix.open_process_in ("bzcat " ^ f)
  else open_in f

let my_close_in f h =
  if f = "-" then () else 
    if suffix_of f ".Z" || suffix_of f ".gz" || suffix_of f ".bz2" 
    then ignore (Unix.close_process_in h)
    else close_in h

let maximum_int a = 
  if length a == 0 then 0
  else if length a == 1 then a.(0) else
    let mx = ref a.(0) in
      for i = 1 to length a - 1 do
        mx := max !mx a.(i);
      done;
      !mx

let maximum a = 
  if length a == 0 then zero
  else if length a == 1 then a.(0) else
    let mx = ref a.(0) in
      for i = 1 to length a - 1 do
        mx := max !mx a.(i);
      done;
      !mx

let maximum_not_inf a = 
  if length a == 0 then zero
  else 
    let mx = ref zero in
      for i = 0 to length a - 1 do
        if a.(i) < infinity then mx := max !mx a.(i);
      done;
      !mx

let minimum a = 
  if length a == 0 then -.zero
  else if length a == 1 then a.(0) else
    let mn = ref a.(0) in
      for i = 1 to length a - 1 do
        mn := min !mn a.(i);
      done;
      !mn

let minimum_pos a = 
  if length a == 0 then 0
  else if length a == 1 then 0 else
    let mn = ref 0 in
      for i = 1 to length a - 1 do
        if a.(!mn) > a.(i) then mn := i;
      done;
      !mn

let transpose (d : ia Arry.arry) =
  let _N = Arry.length d in
  let _D = Int32.add Int32.one (Arry.fold_left (fold_left_ia max) (Int32.zero) d) in
  let ht = init (Int32.to_int _D) (fun _ -> H.create 5) in
    for n = 0 to _N - 1 do
      iter_ia (fun d -> H.replace ht.(Int32.to_int d) n ()) (Arry.get d n);
    done;
    ht

let addLogArray (a : float array) =
  if length a == 0 then zero
  else if length a == 1 then a.(0)
  else if length a == 2 then addLog a.(0) a.(1)
  else
    let b = maximum a in
      b +. log( fold_left (fun s v -> s +. exp(v -. b)) 0. a )


let fast_sort_ia (a : ia) = ()
let fast_sort_by_ia (a : ia) (b : fa) = ()

let or_empty x n = if n >= Arry.length x then efa else Arry.get x (n)

let or_empty' x n c = 
  if n >= Arry.length x then efa 
  else if c >= length (Arry.get x n) then efa
  else (Arry.get x n).(c)
