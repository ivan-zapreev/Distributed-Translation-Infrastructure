open Array
open Util

let independence_sampling 
  (_F     : int)
  (gen_q  : unit -> float * int array * float array)
  (w      : bigarr)
  (burnin : int)
  (numsamples : int)
  (spacing : int)
  : bigarr * float =    (* produces E(f_i) and Z *)
  let eF = make_ba _F 0. in
  let eZ = ref zero in

  let dot (ix : int array) (v : float array) : float =
    let _M = length ix in
    let rec dot' m acc = 
      if m >= _M then acc
      else if ix.(m) < _F
      then 
        if length v > 0
        then dot' (m+1) (acc +. w.{ix.(m)} *. v.(m))
        else dot' (m+1) (acc +. w.{ix.(m)})
      else dot' (m+1) acc in
      dot' 0 0. in

  let q0,s0i,s0v = gen_q () in
  let p0         = q0 *@ dot s0i s0v in
  let curS       = ref (q0,p0,s0i,s0v) in

  let numacc = ref 0 in let numtot = ref 0 in

  let is_step () =
    let q ,p ,si ,sv  = !curS in
    let q',   si',sv' = gen_q () in
    let    p'         = q' *@ dot si' sv' in
    let alpha         = exp (min one ((p' *@ q) /@ (p *@ q'))) in
    let r             = Random.float 1. in
      incr numtot;
      if r <= alpha then ( incr numacc; curS := q',p',si',sv' ) in

    Printf.fprintf stderr "burn"; flush stderr;
    for i = 1 to burnin do
      (* Printf.fprintf stderr "."; flush stderr; *)
      is_step ();
    done;
    Printf.fprintf stderr " samp"; flush stderr;
    for i = 1 to numsamples do
      for i = 1 to spacing do
        is_step ();
      done;

      let q,p,si,sv = !curS in
      let _M = length si in
        for m = 0 to _M - 1 do
          if si.(m) < _F then
            (if length sv > 0
             then eF.{si.(m)} <- eF.{si.(m)} +. sv.(m) /. float_of_int numsamples
             else eF.{si.(m)} <- eF.{si.(m)} +. 1. /. float_of_int numsamples);
        done;
        eZ := !eZ +@ p;

        (* Printf.fprintf stderr "."; flush stderr; *)
    done;
    eZ := !eZ /@ float_of_int numsamples;
    Printf.fprintf stderr " (%g %d/%d) " !eZ !numacc !numtot; (* (float_of_int !numacc /. float_of_int !numtot); *)
    
    eF, !eZ

let mk_gen_q_from_file_list
  (filelist     : string)
  (get_vocab    : string -> int)
  (totalsamples : int)
  (implicit     : bool)
  : unit -> float * int array * float array =
  let fl = of_list (Str.split (Str.regexp ":") filelist) in
  let fl_pos = ref 0 in
  let _F = length fl in
  let warningShown = ref false in

  let cur_handle = ref (open_in fl.(0)) in

  let rec get_next_line attempts =
    if attempts == _F then failwith "mk_gen_q_from_file_list: all files are empty!" else
      try input_line !cur_handle
      with End_of_file -> (
        close_in !cur_handle;
        if !fl_pos == _F - 1 then (
          if not !warningShown then ( 
            Printf.fprintf stderr "Warning: mk_gen_q_from_file_list repeating files\n"; 
            flush stderr;
            warningShown := true;
          );
          fl_pos := 0;
        ) else (
          incr fl_pos;
        );
        cur_handle := open_in fl.(!fl_pos);
        get_next_line (attempts + 1);
      ) in

  let f () =
    let l = get_next_line 0 in
    let p,vl = 
      match Str.split (Str.regexp "[ \t]+") l with
          p::vl -> float_of_string p, vl
        | _     -> failwith ("malformed input line: " ^ l) in
    let ai = 
      if implicit 
      then make (1 + List.length vl) 0
      else make ((1+List.length vl) / 2) 0  in
    let vi = 
      if not implicit then (
        let vi = make ((1+List.length vl) / 2) 0. in
        let rec setv j = function
            [] -> ()
          | (fi::fv::rest) -> ( ai.(j) <- get_vocab fi ; vi.(j) <- float_of_string fv ;
                                setv (j+1) rest )
          | _ -> failwith ("missing feature value in " ^ l) in
          setv 0 vl;
          vi
      ) else (
        let rec setv j = function
            [] -> ()
          | (fi::rest) -> ( ai.(j) <- get_vocab fi ;
                            setv (j+1) rest ) in
          setv 0 vl;
          [||]
      ) in
      p,ai,vi in
  
    f

let mk_external_gen_q
  (cmd : string) 
  (args : string)
  (get_vocab : string -> int)
  (totalsamples : int)
  (implicit : bool)
  : unit -> float * int array * float array =
  let safe_getenv x = try Sys.getenv x with Not_found -> "unk" in
  let fp = ".wsemlm." ^ safe_getenv "HOST" ^ "." ^ safe_getenv "USER" ^ "." ^ string_of_int (Random.int max_int) in
  let _ = Sys.command (cmd ^ " " ^ fp ^ " " ^ string_of_int totalsamples ^ " " ^ args) in
  let h = open_in fp in

  let curPos = ref 0 in
  let warningShown = ref false in

  let f () =
    if !curPos == totalsamples then (
      if not !warningShown then ( Printf.fprintf stderr "Warning: mk_external_gen_q: too many samples requested\n"; flush stderr; warningShown := true );
      seek_in h 0;
    );
    let l = 
      try input_line h 
      with End_of_file -> (
        if not !warningShown then ( Printf.fprintf stderr "Warning: mk_external_gen_q: too many samples requested\n"; flush stderr; warningShown := true );
        seek_in h 0;
        input_line h ) in
    let p,vl = 
      match Str.split (Str.regexp "[ \t]+") l with
          p::vl -> float_of_string p, vl
        | _     -> failwith ("malformed input line: " ^ l) in
    let ai = 
      if implicit 
      then make (1 + List.length vl) 0
      else make ((1+List.length vl) / 2) 0  in
    let vi = 
      if not implicit then (
        let vi = make ((1+List.length vl) / 2) 0. in
        let rec setv j = function
            [] -> ()
          | (fi::fv::rest) -> ( ai.(j) <- get_vocab fi ; vi.(j) <- float_of_string fv ;
                                setv (j+1) rest )
          | _ -> failwith ("missing feature value in " ^ l) in
          setv 0 vl;
          vi
      ) else (
        let rec setv j = function
            [] -> ()
          | (fi::rest) -> ( ai.(j) <- get_vocab fi ;
                            setv (j+1) rest ) in
          setv 0 vl;
          [||]
      ) in
      incr curPos;
      p,ai,vi in
  
    f

