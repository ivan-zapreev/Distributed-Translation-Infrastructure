open Array
open Util
module A = Arry
module H = Hashtbl
module I = Int32

type modeltype = Binary | Binomial | Multiclass | Sampled | Perceptron | Multitron

type loadedmodel = BinModel of bigarr | MultModel of float array array

type fsel = NoFsel | MinFC of int | BNS of int | ABFFS of int


type datav =
    { mutable datav_bernoulli : bool;  (* true if no "xv"s *)
      datav_implicit          : bool;  (* true if x^c = x^{c'} for all c,c' *)
      datav_sampled           : bool;  (* if data is for a sampling problem, this is true; assumed non-bernoulli, implicit *)
      mutable datav_numclass  : int;
      mutable datav_numfeat   : int;
      datav_task              : int A.arry;
      mutable datav_weights   : bigarr;
      mutable datav_xi        : ia A.arry;
      mutable datav_xi_c      : ia array A.arry;
      mutable datav_xv        : fa A.arry;
      mutable datav_xv_c      : fa array A.arry;
    }

let concat_datav l =
  let h = List.hd l in
    if List.length l == 1 then h else
      { datav_bernoulli = h.datav_bernoulli
      ; datav_implicit  = h.datav_implicit
      ; datav_sampled   = h.datav_sampled
      ; datav_numclass  = maximum_int (of_list (List.map (fun x -> x.datav_numclass) l))
      ; datav_numfeat   = maximum_int (of_list (List.map (fun x -> x.datav_numfeat) l))
      ; datav_task      = A.concat (List.map (fun x -> x.datav_task) l)
      ; datav_weights   = concat_ba (List.map (fun x -> x.datav_weights) l)
      ; datav_xi        = A.concat (List.map (fun x -> x.datav_xi) l)
      ; datav_xi_c      = A.concat (List.map (fun x -> x.datav_xi_c) l)
      ; datav_xv        = A.concat (List.map (fun x -> x.datav_xv) l)
      ; datav_xv_c      = A.concat (List.map (fun x -> x.datav_xv_c) l)
      }
type range_elem = Single of int | Range of (int * int)  (* inclusive, (x,-1) --> (x to end) *)
type range = All | Only of range_elem array
type filespec =
    { fs_filename    : string
    ; fs_range       : range
    ; fs_destination : int           (* 0 = train, 1 = dev, 2 = test *)
    ; fs_weight      : float
    }

let compute_range_size _N els =
  let i = ref 0 in
    iter (fun el ->
      match el with
          Single n -> incr i
        | Range (n,m) -> i := !i + ((if m < 0 then _N else m) - n + 1)
         ) els;
    !i

let space_re = Str.regexp "[ \t]+"
let comma_re = Str.regexp ","
let colon_re = Str.regexp ":"
let dash_re = Str.regexp "-"
let single_line_re = Str.regexp "[ \t]*#*[ \t]*$"
let sections_re = Str.regexp "[ \t]+#[ \t]+"

type multilabel = L of int | ML of float array

let lab = function L c -> c | _ -> failwith "lab ML"

let empty_data_set =
  { datav_bernoulli = true
  ; datav_implicit  = true
  ; datav_numclass  = -1
  ; datav_numfeat   = -1
  ; datav_task      = A.empty 0
  ; datav_weights   = make_ba 0 0.
  ; datav_xi        = A.empty eia
  ; datav_xi_c      = A.empty [||]
  ; datav_xv        = A.empty efa
  ; datav_xv_c      = A.empty [||]
  ; datav_sampled   = false }

let dat_N x =
  if x.datav_implicit then A.length x.datav_xi else A.length x.datav_xi_c

let dat_F x =
  if x.datav_implicit then x.datav_numfeat * (max 2 x.datav_numclass) else x.datav_numfeat

let dat_P x n c =
  if x.datav_implicit then A1.dim (A.get x.datav_xi n) else A1.dim (A.get x.datav_xi_c n).(c)

let dat_i x n c p =
  if x.datav_implicit then I.to_int (A.get x.datav_xi n).{p} + c * x.datav_numfeat else I.to_int (A.get x.datav_xi_c n).(c).{p}

let dat_v x n c p =
  if x.datav_bernoulli then 1.
  else if x.datav_implicit then (A.get x.datav_xv n).{p} else (A.get x.datav_xv_c n).(c).{p}

let dat_wt x n =
  if A1.dim x.datav_weights == 0 then 1. else x.datav_weights.{n}

let dat_prep_i x n c =
  if x.datav_implicit 
  then let d = (A.get x.datav_xi n) in fun p -> I.to_int d.{p} + c * x.datav_numfeat
  else let d = (A.get x.datav_xi_c n).(c) in fun p -> I.to_int d.{p}

let dat_prep_v x n c =
  if x.datav_bernoulli then fun p -> 1. else
  if x.datav_implicit 
  then let d = (A.get x.datav_xv n) in fun p -> d.{p}
  else let d = (A.get x.datav_xv_c n).(c) in fun p -> d.{p}

type inputLine = EOF | EmptyLine | TestLine | DevLine | CommentLine | Success



let sort_by_xi1 xi xv =
  if A1.dim xv == 0 then (
    fast_sort_ia xi;
    (xi,xv)
  ) else (
    fast_sort_by_ia xi xv;
    (xi,xv)
  )

let sort_by_xi x = 
  for n = 0 to dat_N x - 1 do
    let xi = A.get x.datav_xi n in
      if n >= A.length x.datav_xv then
        fast_sort_ia xi
      else
        let xv = A.get x.datav_xv n in
          fast_sort_by_ia xi xv
  done

let norm_data1 p x v =
  let sqr x = x *. x in 
  let id x = x in
  let psqr  = if p == 1 then abs_float else sqr in
  let psqrt = if p == 1 then id else sqrt in

    if A1.dim x == 0 then v
    else if A1.dim v == 0 then (
      let z = float_of_int (A1.dim x) in
        make_ba (A1.dim x) (1. /. psqrt z)
    ) else (
      let s = ref 0. in
        for i = 0 to A1.dim v - 1 do
          s := !s +. psqr v.{i};
        done;
      let s = psqrt !s in
        if s == 0. then v
        else (
          for i = 0 to A1.dim v - 1 do
            v.{i} <- v.{i} /. s;
          done;
          v
        )
    )

let norm_data p x =
  let sqr x = x *. x in 
  let id x = x in
  let psqr  = if p == 1 then abs_float else sqr in
  let psqrt = if p == 1 then id else sqrt in

  let xv = x.datav_xv in
    x.datav_bernoulli <- false;

    x.datav_xv <- A.init (dat_N x) (fun n -> make_ba (A1.dim (A.get x.datav_xi n)) 0.);
    for n = 0 to dat_N x - 1 do
      let s  = ref 0. in
      let xi = A.get x.datav_xi n in
        if n < A.length xv then
          let xv = A.get xv n in
            for i = 0 to A1.dim xi - 1 do
              s := !s +. psqr xv.{i};
            done
        else
          s := float_of_int (A1.dim xi);
        if !s > 0. then (
          s := psqrt !s;
          let newv = A.get x.datav_xv n in
            if n < A.length xv then
              let xv = A.get xv n in
                for i = 0 to A1.dim xi - 1 do
                  newv.{i} <- xv.{i} /. !s;
                done
            else
              for i = 0 to A1.dim xi - 1 do
                newv.{i} <- 1. /. !s;
              done;
        );
    done

let max_read_class = ref 0
let (class_vocab : (string,int) H.t) = H.create 5
let (class_recab : (int,string) H.t) = H.create 5

let read_multiclass named ln str =
  if named then (
    try  L (H.find class_vocab str)
    with Not_found -> ( 
      let i = H.fold (fun _ _ c -> c + 1) class_vocab 0 in
        H.replace class_vocab str i;
        H.replace class_recab i str;
        max_read_class := max !max_read_class i;
        (L i)
    )
  ) else (
    try
      let i = int_of_string str in
      let j = if i < 0 then 0 else i in
        max_read_class := max !max_read_class j;
        (L j)
    with Failure err ->
      failwith ("error: \"" ^ err ^ "\" occured on line " ^ string_of_int ln ^ " when trying to parse \"" ^ str ^ "\" as an int for multiclass class")
  )

let multilabel_regexp = Str.regexp ":"
let read_multilabel ln str =
  try
    let i  = Str.split multilabel_regexp str in
    let a  = of_list (List.map (fun s -> if s = "x" then infinity else float_of_string s) i) in
    (* let mv = minimum a in *)
      (* for i = 0 to length a - 1 do a.(i) <- a.(i) -. mv; done; *)
      max_read_class := max !max_read_class (length a-1);
      (ML a)
  with Failure err ->
    failwith ("error: \"" ^ err ^ "\" occured on line " ^ string_of_int ln ^ " when trying to parse \"" ^ str ^ "\" as multilabel array")


let read_range' s =
  match Str.split dash_re s with
      [a] -> Single (int_of_string a)
    | [a;"end"] -> Range (int_of_string a, -1)
    | [a;b] -> Range (int_of_string a, int_of_string b)
    | _     -> failwith ("cannot parse range: '" ^ s ^ "'")

let read_range str =
  match Str.split comma_re str with
      ["all"] -> All
    | l -> Only (of_list (List.map read_range' l))

let round_toward_zero x = if x < 0. then ceil x else floor x

let apply_removal to_remove x =
  let filter xi xv =
    let c = ref 0 in
      iter_ia (fun f -> if not (H.mem to_remove f) then incr c) xi;
      if !c == A1.dim xi then (xi,xv)
      else (
        let j   = ref 0 in
        let xi' = make_ia !c 0  in
        let xv' = if x.datav_bernoulli then make_ba 0 0. else make_ba !c 0. in
          for i = 0 to A1.dim xi - 1 do
            if not (H.mem to_remove xi.{i}) then (
              xi'.{!j} <- xi.{i};
              if not x.datav_bernoulli then xv'.{!j} <- xv.{i};
              incr j;
            );
          done;
          (xi',xv')
      ) in

    for n = 0 to dat_N x - 1 do
      if x.datav_implicit then (
        let (xi ,xv ) = (A.get x.datav_xi n, or_empty x.datav_xv n) in
        let (xi',xv') = filter xi xv in
          A.set x.datav_xi n xi';
          if not x.datav_bernoulli then A.set x.datav_xv n xv';
      ) else (
        let xi0 = A.get x.datav_xi_c n in
        for c = 0 to length xi0 - 1 do
          let xv = or_empty' x.datav_xv_c n c in
          let (xi',xv') = filter xi0.(c) xv in
            xi0.(c) <- xi';
            if not x.datav_bernoulli then (A.get x.datav_xv_c n).(c) <- xv';
        done;
      );
    done;
    ()



let fselect_by_fcount minc x =
  let counts = H.create (10 + x.datav_numfeat / 10) in
  let incr_h f = H.replace counts f (1 + try H.find counts f with Not_found -> 0) in
  let hit = H.create 100 in

    if x.datav_implicit then (
      A.iter (fun xn ->
                H.clear hit;
                iter_ia (fun f ->
                           if not (H.mem hit f) then (
                             incr_h f;
                             H.replace hit f ()
                           )
                        ) xn
             ) x.datav_xi
    ) else (
      A.iter (fun xn  ->
        iter (fun xnc ->
                H.clear hit;
                iter_ia (fun f ->
                           if not (H.mem hit f) then (
                             incr_h f;
                             H.replace hit f ()
                           )
                        ) xnc
             ) xn
             ) x.datav_xi_c
    );

  let to_remove = H.create 100 in
    H.iter (fun f c -> if c < minc then H.replace to_remove f ()) counts;

    apply_removal to_remove x;
    ()

let erf (x : float) =
  let res =
    if abs_float x <= 0.46875 then
      let a1 = 3.16112374387056560e00 in
      let a2 = 1.13864154151050156e02 in
      let a3 = 3.77485237685302021e02 in
      let a4 = 3.20937758913846947e03 in
      let a5 = 1.85777706184603153e-1 in
      let b1 = 2.36012909523441209e01 in
      let b2 = 2.44024637934444173e02 in
      let b3 = 1.28261652607737228e03 in
      let b4 = 2.84423683343917062e03 in
      let y = abs_float x in
      let z = y *. y in
      let xnum = a5 *. z in
      let xden = z in
      let xnum = (xnum +. a1) *. z in let xden = (xnum +. b1) *. z in
      let xnum = (xnum +. a2) *. z in let xden = (xnum +. b2) *. z in
      let xnum = (xnum +. a3) *. z in let xden = (xnum +. b3) *. z in
        x *. (xnum +. a4) /. (xden +. b4)
    else if 0.46875 <= abs_float x && abs_float x <= 4. then
      let c1 = 5.64188496988670089e-1 in
      let c2 = 8.88314979438837594e00 in
      let c3 = 6.61191906371416295e01 in
      let c4 = 2.98635138197400131e02 in
      let c5 = 8.81952221241769090e02 in
      let c6 = 1.71204761263407058e03 in
      let c7 = 2.05107837782607147e03 in
      let c8 = 1.23033935479799725e03 in
      let c9 = 2.15311535474403846e-8 in
      let d1 = 1.57449261107098347e01 in
      let d2 = 1.17693950891312499e02 in
      let d3 = 5.37181101862009858e02 in
      let d4 = 1.62138957456669019e03 in
      let d5 = 3.29079923573345963e03 in
      let d6 = 4.36261909014324716e03 in
      let d7 = 3.43936767414372164e03 in
      let d8 = 1.23033935480374942e03 in
      let y = abs_float x in
      let xnum = c9 *. y in
      let xden = y in
      let xnum = (xnum +. c1) *. y in let xden = (xden +. d1) *. y in
      let xnum = (xnum +. c2) *. y in let xden = (xden +. d2) *. y in
      let xnum = (xnum +. c3) *. y in let xden = (xden +. d3) *. y in
      let xnum = (xnum +. c4) *. y in let xden = (xden +. d4) *. y in
      let xnum = (xnum +. c5) *. y in let xden = (xden +. d5) *. y in
      let xnum = (xnum +. c6) *. y in let xden = (xden +. d6) *. y in
      let xnum = (xnum +. c7) *. y in let xden = (xden +. d7) *. y in
      let result = (xnum +. c8) /. (xden +. d8) in
      let z = (round_toward_zero (y *. 16.)) /. 16. in
      let del = (y -. z) *. (y +. z) in
        exp (0. -. z *. z) *. exp (0. -. del) *. result
    else if abs_float x > 5. then
      let p1 = 3.05326634961232344e-1 in
      let p2 = 3.60344899949804439e-1 in
      let p3 = 1.25781726111229246e-1 in
      let p4 = 1.60837851487422766e-2 in
      let p5 = 6.58749161529837803e-4 in
      let p6 = 1.63153871373020978e-2 in
      let q1 = 2.56852019228982242e00 in
      let q2 = 1.87295284992346047e00 in
      let q3 = 5.27905102951428412e-1 in
      let q4 = 6.05183413124413191e-2 in
      let q5 = 2.33520497626869185e-3 in
      let y = abs_float x in
      let z = 1. /. (y *. y) in
      let xnum = p6 *. z in
      let xden = z in
      let xnum = (xnum +. p1) *. z in let xden = (xden +. q1) *. z in
      let xnum = (xnum +. p2) *. z in let xden = (xden +. q2) *. z in
      let xnum = (xnum +. p3) *. z in let xden = (xden +. q3) *. z in
      let xnum = (xnum +. p4) *. z in let xden = (xden +. q4) *. z in
      let result = z *. (xnum +. p5) /. (xden +. q5) in
      let result = (1./.sqrt 3.141592654 -. result) /. y in
      let z = (round_toward_zero (y *. 16.)) /. 16. in
      let del = (y -. z) *. (y +. z) in
        exp (0.-.z*.z) *. exp (0.-.del) *. result 
    else failwith ("erf given an invalid argument") in
    if x > 0.46875 then 1. -. res
    else if x < -0.46875 then res -. 1.
    else res

let erfinv (y : float) =
  let a1 =  0.886226899 in let a2 = -1.645349621 in let a3 =  0.914624893 in let a4 = -0.140543331 in
  let b1 = -2.118377725 in let b2 =  1.442710462 in let b3 = -0.329097515 in let b4 =  0.012229801 in
  let c1 = -1.970840454 in let c2 = -1.624906493 in let c3 =  3.429567803 in let c4 =  1.641345311 in
  let d1 =  3.543889200 in let d2 =  1.637067800 in
  let x  =
    if abs_float y <= 0.7 then
      let z = y *. y in
        y *. (((a4*.z+.a3)*.z+.a2)*.z+.a1) /. ((((b4*.z+.b3)*.z+.b2)*.z+.b1)*.z+.1.)
    else if 0.7 < y && y < 1. then
      let z = sqrt (0. -. log ((1.-.y) /. 2.)) in
        (((c4*.z+.c3)*.z+.c2)*.z+.c1) /. ((d2*.z+.d1)*.z+.1.)
    else if y < -0.7 && y > -1. then
      let z = sqrt (0. -. log ((1.+.y) /. 2.)) in
        0. -. (((c4*.z+.c3)*.z+.c2)*.z+.c1) /. ((d2*.z+.d1)*.z+.1.) 
    else if y == -1. then neg_infinity
    else if y ==  1. then infinity
    else failwith ("erfinv: invalid argument -- " ^ string_of_float y) in

  (* Two steps of Newton-Raphson correction to full accuracy.
     Without these steps, erfinv(y) would be about 3 times
     faster to compute, but accurate to only about 6 digits. *)

(*  let x = x -. ((erf x) -. y) /. (2. *. exp (0. -. x *. x) /. sqrt 3.141592654) in *)
(*  let x = x -. ((erf x) -. y) /. (2. *. exp (0. -. x *. x) /. sqrt 3.141592654) in *)
    x

let normal_cdf_inverse (x : float) =
  sqrt 2. *. erfinv (2. *. x -. 1.)

let multilabel2class = function
    L c -> c
  | ML a ->
      let b = ref 0 in
        for i = 1 to length a - 1 do
          if a.(i) < a.(!b) then b := i;
        done;
        !b

let fselect_by_bns num_keep x (y : int A.arry) =
  let ccount  = H.create 5 in
  let fcount  = H.create (10 + x.datav_numfeat / 10) in
  let fccount = H.create 5 in

  let _N = dat_N x in
  let hit = H.create 100 in

  let incr_h h f = H.replace h f (1 + try H.find h f with Not_found -> 0) in
  let incr_h' h f c = 
    try  incr_h (H.find h f) c
    with Not_found -> (
      let h' = H.create 5 in
        H.replace h' c 1;
        H.replace h  f h';
      ) in
      

    if x.datav_implicit then (
      A.iteri (fun n xn ->
                 let yn = A.get y n in
                   incr_h ccount yn;
                   H.clear hit;
                   iter_ia (fun f ->
                              if not (H.mem hit f) then (
                                incr_h fcount (Int32.to_int f);
                                incr_h' fccount (Int32.to_int f) yn;
                                H.replace hit f ()
                              )
                           ) xn
              ) x.datav_xi;
    ) else (
      A.iteri (fun n xn ->
                 let yn = A.get y n in
                   incr_h ccount yn;
                   iter (fun xnc ->
                           H.clear hit;
                           iter_ia (fun f ->
                                      if not (H.mem hit f) then (
                                        incr_h fcount (Int32.to_int f);
                                        incr_h' fccount (Int32.to_int f) yn;
                                        H.replace hit f ()
                                      )
                                   ) xnc
                        ) xn;
              ) x.datav_xi_c;
    );

  let max_f = 1 + H.fold (fun f _ m -> max f m) fcount 0 in
  let bns = make max_f (neg_infinity,Int32.zero) in

    H.iter (fun f fc ->
              let sum = ref 0. in
              let fccount' = H.find fccount f in
                H.iter (fun c cc ->
                          let cfc = try H.find fccount' c with Not_found -> 0 in
                          let p_f_c    = (0.5 +. float_of_int cfc) /. (float_of_int cc +. 1.) in
                          let p_f_notc = (0.5 +. float_of_int (fc - cfc)) /. (float_of_int _N -. float_of_int cc +. 1.) in
                            sum := !sum +. abs_float (normal_cdf_inverse p_f_c -. normal_cdf_inverse p_f_notc)
                       ) ccount;
                bns.(f) <- (!sum, Int32.of_int f);
           ) fcount;

  let to_remove = H.create 100 in

    fast_sort (fun a b -> compare b a) bns;

    for ii = num_keep to length bns - 1 do
      H.replace to_remove (snd bns.(ii)) ();
    done;

    apply_removal to_remove x;

    ()




let load_filespec open_h close_h =
  let h = open_h () in
  let cur_line = ref 0 in
  let parse_dst = function
      "train" -> 0
    | "dev"   -> 1
    | "test"  -> 2
    | s       -> failwith ("destinations in filespecs must be one of 'train', 'dev' or 'test'...'" ^ s ^ "' not allowed") in
  let read_weight = function
      [] -> 1.
    | ["weight" ; w] -> float_of_string w
    | (l :: _) -> failwith ("filespec error: expecting 'weight ...', got '" ^ l ^ " ...'") in
  let rec rd acc = 
    incr cur_line;
    match maybe_read_line h with
        None -> of_list (List.rev acc)
      | Some l ->
          (match Str.split space_re l with
              (fn :: dst :: range :: rest) -> rd ({ fs_filename = fn ; fs_destination = parse_dst dst ; fs_range = read_range range ; fs_weight = read_weight rest } :: acc)
            | _ -> failwith ("error reading filespec on line " ^ string_of_int !cur_line)
          ) in
  let ret = rd [] in
    close_h h;
    ret
 
let load_data_by_filespec quiet ydef filespec ld =
  if not quiet then ( Printf.eprintf "Loading data from filespec..." ; flush stderr );
  let xt,yt,xd,yd,xe,ye = ref None, ref (A.empty ydef), ref None, ref (A.empty ydef), ref None, ref (A.empty ydef) in
  let concat_datav2 a b =
    match a with None -> Some b | Some a' -> Some (concat_datav [ a' ; b ]) in
  let setx x' x'' i n =
    if x'.datav_implicit then (
      A.set x''.datav_xi i (A.get x'.datav_xi n);
      if not x'.datav_bernoulli then A.set x''.datav_xv i (A.get x'.datav_xv n);
    ) else (
      A.set x''.datav_xi_c i (A.get x'.datav_xi_c n);
      if not x'.datav_bernoulli then A.set x''.datav_xv_c i (A.get x'.datav_xv_c n);
    ) in
    iter (fun fs ->
      if not quiet then ( Printf.eprintf "." ; flush stderr );
      let (xt',yt'),(xd',yd'),(xe',ye') = ld true (fun () -> my_open_in fs.fs_filename) (fun h -> my_close_in fs.fs_filename h) in
      let x' = concat_datav [ xt' ; xd' ; xe' ] in
        if fs.fs_weight <> 1. then
          multiply_ba x'.datav_weights fs.fs_weight;
      let y' = A.concat [ yt' ; yd' ; ye' ] in
      let (x'',y'') =
        match fs.fs_range with
            All -> (x',y')
          | Only els ->
              let _N = dat_N x' in
              let sz = compute_range_size _N els in
              let x'' = 
                { datav_bernoulli = x'.datav_bernoulli
                ; datav_implicit  = x'.datav_implicit
                ; datav_sampled   = x'.datav_sampled
                ; datav_numclass  = x'.datav_numclass
                ; datav_numfeat   = x'.datav_numfeat
                ; datav_task      = A.make sz 0
                ; datav_weights   = make_ba sz 1.
                ; datav_xi        = if x'.datav_implicit then A.make sz eia else A.empty eia
                ; datav_xi_c      = if x'.datav_implicit then A.empty [||] else A.make sz [||] 
                ; datav_xv        = if not x'.datav_bernoulli then (if x'.datav_implicit then A.make sz efa else A.empty efa) else A.empty efa
                ; datav_xv_c      = if not x'.datav_bernoulli then (if x'.datav_implicit then A.empty [||] else A.make sz [||]) else A.empty [||]
                } in
              let y'' = A.make sz (A.get y' 0) in
              let i   = ref 0 in
                iter (fun el ->
                  match el with
                      Single n -> 
                        if n < 1 || n > _N then failwith ("filespec contains an invalid position: " ^ string_of_int n ^ " in file '" ^ fs.fs_filename ^ "' (file size = " ^ string_of_int _N ^ ")");
                        setx x' x'' !i (n-1); (* x''.(!i) <- x'.(n); *)
                        A.set y'' (!i) (A.get y' (n-1));
                        incr i
                    | Range (n,m) ->
                        if n < 0 || n > _N || m > _N then failwith ("filespec contains an invalid range: " ^ string_of_int n ^ "-" ^ string_of_int m ^ " in file '" ^ fs.fs_filename ^ "' (file size = " ^ string_of_int _N ^ ")");
                        for j = n-1 to (if m < 0 then _N else m)-1 do
                          setx x' x'' !i j; (* x''.(!i) <- x'.(j); *)
                          A.set y'' (!i) (A.get y' j);
                          incr i;
                        done
                     ) els;
                (x'',y'')
      in
        match fs.fs_destination with
            0 -> ( xt := concat_datav2 !xt x'' ; yt := A.concat [ !yt ; y'' ] )
          | 1 -> ( xd := concat_datav2 !xd x'' ; yd := A.concat [ !yd ; y'' ] )
          | 2 -> ( xe := concat_datav2 !xe x'' ; ye := A.concat [ !ye ; y'' ] )
         ) filespec;

  let numf = function None -> 0 | Some v -> v.datav_numfeat in
  let maxf = max (max (numf !xt) (numf !xd)) (numf !xe) in
  let from_some2 = function
      Some v -> { v 
                  with datav_numfeat = maxf 
                }
    | None -> { datav_bernoulli = true
              ; datav_implicit  = true
              ; datav_sampled   = false
              ; datav_numclass  = 1
              ; datav_numfeat   = maxf+1
              ; datav_task      = A.empty 0
              ; datav_weights   = make_ba 0 1.
              ; datav_xi        = A.empty eia
              ; datav_xv        = A.empty efa
              ; datav_xi_c      = A.empty [||]
              ; datav_xv_c      = A.empty [||]
              } in
    if not quiet then ( Printf.eprintf " %d train, %d dev, %d test\n" (A.length !yt) (A.length !yd) (A.length !ye); flush stderr );
    ((from_some2 !xt,!yt),(from_some2 !xd,!yd),(from_some2 !xe,!ye))

let load_data fsel mkfsel normv (kernelmap,applymap,printmap) kernel quiet ydef is_fspec fname ld =
  let open_h  () = my_open_in fname in
  let close_h h  = my_close_in fname h in
  let (xTr,yTr),(xDe,yDe),(xTe,yTe) =
    if not is_fspec then ld quiet open_h close_h
    else
      let fspec = load_filespec open_h close_h in
        load_data_by_filespec quiet ydef fspec ld in

    if normv != 0 then (
      norm_data normv xTr; 
      norm_data normv xDe; 
      norm_data normv xTe;
    );

    (match fsel with
        NoFsel  -> ()
      | ABFFS _ -> ()  (* taken care of elsewhere *)
      | MinFC m -> fselect_by_fcount m xTr
      | BNS k   -> fselect_by_bns    k xTr (A.map mkfsel yTr));

            
    match kernel with
        None -> (None,(xTr,yTr),(xDe,yDe),(xTe,yTe))
      | Some (k,_D,(byDist,byClass)) ->
          if not quiet then Printf.eprintf "computing kernel map...";
          sort_by_xi xTr; sort_by_xi xDe; sort_by_xi xTe;
          let (kmap, xTr') = kernelmap byDist byClass k _D xTr yTr in
            printmap kmap xTr xTr';
          let xDe' = applymap kmap xTr xTr' xDe in
          let xTe' = applymap kmap xTr xTr' xTe in
            if not quiet then Printf.eprintf "\n";
            reset_vocab _D;
            (Some kmap, (xTr',yTr),(xDe',yDe),(xTe',yTe))

let out_of_domain_re = Str.regexp (".*" ^ (Str.quote "$$$OUTOFDOMAIN") ^ ".*")

let load_implicit noscan ignore_id ignore_ood quiet ber open_h close_h y_read y_def sampled predict =
  let h = open_h () in
    (* count number of examples *)
  let rec count_lines intr numtr numde numte  =
    match maybe_read_line h with
        None -> (numtr, numde, numte)
      | Some s ->
          let is_in_domain =
            (*if not ignore_id && not ignore_ood then true
            else*) not (Str.string_match out_of_domain_re s 0) in
          if s = "" then count_lines intr numtr numde numte 
          else if s = "DEV"  then count_lines 1 numtr numde numte
          else if s = "TEST" then count_lines 2 numtr numde numte
          else if String.get s 0 == '#' then count_lines intr numtr numde numte
          else if intr == 0 &&
                  ((ignore_id  && is_in_domain) ||
                   (ignore_ood && not is_in_domain)) then count_lines 0 numtr numde numte
          else if intr == 0 then count_lines 0 (numtr+1) numde numte
          else if intr == 1 then count_lines 1 numtr (numde+1) numte
          else count_lines 2 numtr numde (numte+1) in
  let h,numtr,numde,numte =
    if noscan then (h,1,1,1) else (
        let _ = if not quiet then (Printf.fprintf stderr "Scanning file..." ; flush stderr) in
        let (numtr, numde, numte) = count_lines 0 0 0 0 in
          close_h h;
        let h = open_h () in
        let _ = if not quiet then (Printf.fprintf stderr "%d train, %d dev, %d test, reading..." numtr numde numte ; flush stderr) in
          (h,numtr,numde,numte)
      ) in
  
    (* make arrays *)
  let tr = A.make numtr y_def, A.make numtr eia, (if ber then A.empty efa else A.make numtr efa), make_ba numtr 1., A.make numtr 0 in
  let de = A.make numde y_def, A.make numde eia, (if ber then A.empty efa else A.make numde efa), make_ba numde 1., A.make numde 0 in
  let te = A.make numte y_def, A.make numte eia, (if ber then A.empty efa else A.make numte efa), make_ba numte 1., A.make numte 0 in

  let maxFeat = ref 0 in
  let cur_line = ref 0 in

    (* our reading function *)
  let read_single_line in_train (y,x,v,w,d) ntrue =
    match maybe_read_line h with
        None -> EOF
      | Some s ->
          incr cur_line;

          let n = if noscan then 0 else ntrue in

          let is_in_domain =
            (*if not ignore_id && not ignore_ood then true
            else*) not (Str.string_match out_of_domain_re s 0) in

          if s = ""     then EmptyLine else
          if s = "DEV"  then DevLine   else
          if s = "TEST" then TestLine  else
          if in_train && ((ignore_id  && is_in_domain) ||
                          (ignore_ood && not is_in_domain))
          then EmptyLine else
          if String.get s 0 == '#' then CommentLine else (
            let a = of_list (Str.split (space_re) s) in
              (* check for weight and/or ID/OOD tag and task *)
            let start_idx = ref 1 in
            let is_done = ref false in
              while not !is_done && !start_idx <= min (length a-1) 6 do
                if a.(!start_idx) = "$$$WEIGHT" then (
                  if !start_idx+1 >= length a then failwith ("$$$WEIGHT expects an argument on line " ^ string_of_int !cur_line);
                  w.{n} <- float_of_string_err !cur_line a.(!start_idx+1);
                  start_idx := !start_idx + 2;
                ) else if a.(!start_idx) = "$$$TASK" then (
                  if !start_idx+1 >= length a then failwith ("$$$TASK expects an argument on line " ^ string_of_int !cur_line);
                  A.set d (n) (int_of_string_err !cur_line a.(!start_idx+1));
                  start_idx := !start_idx + 2;
                ) else if a.(!start_idx) = "$$$OUTOFDOMAIN" then (
                  A.set d (n) 1;
                  start_idx := !start_idx + 1;
                ) else (
                  is_done := true;
                );
              done;

(*              if length a > 1 then (
                if a.(1) = "$$$WEIGHT" then (
                  if length a <= 2 then failwith ("$$$WEIGHT expects an argument on line " ^ string_of_int !cur_line);
                  w.{n} <- float_of_string_err !cur_line a.(2);
                  if length a > 3 && a.(3) = "$$$OUTOFDOMAIN" then (
                    d.(n) <- false;
                    start_idx := 4
                  ) else ( start_idx := 3 )
                ) else if a.(1) = "$$$OUTOFDOMAIN" then (
                  d.(n) <- false;
                  if length a > 2 && a.(2) = "$$$WEIGHT" then (
                    if length a <= 3 then failwith ("$$$WEIGHT expects an argument on line " ^ string_of_int !cur_line);
                    w.{n} <- float_of_string_err !cur_line a.(3);
                    start_idx := 4
                  ) else ( start_idx := 2 )
                )
              );
*)
              if ber then (
                let xi = make_ia (length a - !start_idx) 0  in
                  for p = !start_idx to length a - 1 do
                    let fi = get_vocab a.(p) in
                      xi.{p - !start_idx} <- I.of_int fi;
                      maxFeat := max !maxFeat fi;
                  done;
                  A.set y n ( y_read !cur_line a.(0));
                  fast_sort_ia xi;
                  A.set x (n) xi;
                  predict (A.get y n) xi efa;
              ) else (
                let xi = make_ia ((length a - !start_idx)/2) 0 in
                let xv = make_ba ((length a - !start_idx)/2) 0. in
                  for p = 0 to (length a - !start_idx) / 2 - 1 do
                    let fi = get_vocab       a.(2*p + !start_idx) in
                    xi.{p} <- I.of_int fi;
                    xv.{p} <- float_of_string_err !cur_line a.(2*p + !start_idx+1);
                    maxFeat := max !maxFeat fi;
                  done;
                  A.set y n ( y_read !cur_line a.(0));
                  A.set x (n) xi;
                  A.set v (n) xv;
                  predict (A.get y n) xi xv;
              );
              Success
          ) in

    (* read data *)

  let rec read_lines intr n =
    match read_single_line (intr == 0) (if intr == 0 then tr else if intr == 1 then de else te) n with
        EOF -> ()
      | EmptyLine   -> read_lines intr  n
      | CommentLine -> read_lines intr  n
      | DevLine     -> if intr<1 then read_lines 1 0 else failwith "two 'DEV' lines"
      | TestLine    -> if intr<2 then read_lines 2 0 else failwith "two 'TEST' lines"
      | Success     -> read_lines intr (n+1) in

  let _ = read_lines 0 0 in
  let _ = if not quiet then (Printf.fprintf stderr "done\n" ; flush stderr) in
  let mk_data (y,x,v,w,d) =
    { datav_bernoulli = ber;
      datav_implicit  = true;
      datav_sampled   = sampled;
      datav_numclass  = 1;
      datav_numfeat   = !maxFeat+1;
      datav_task      = d;
      datav_weights   = w;
      datav_xi        = x;
      datav_xi_c      = A.empty [||];
      datav_xv        = if ber then A.empty efa else v;
      datav_xv_c      = A.empty [||] }, y in

    close_h h;

    mk_data tr, mk_data de, mk_data te




let load_explicit noscan ignore_id ignore_ood quiet bias ber open_h close_h predict multilabel named =
  let h = open_h () in
    (* count number of examples *)
  let rec count_lines intr numtr numde numte  =
    match maybe_read_line h with
        None -> (numtr, numde, numte)
      | Some s ->
          let is_in_domain =
            if not ignore_id && not ignore_ood then true
            else not (Str.string_match out_of_domain_re s 0) in
          if s = "" then count_lines intr numtr numde numte 
          else if s = "DEV"  then count_lines 1 numtr numde numte
          else if s = "TEST" then count_lines 2 numtr numde numte
          else if String.get s 0 == '#' then count_lines intr numtr numde numte
          else if intr == 0 &&
                  ((ignore_id  && is_in_domain) ||
                   (ignore_ood && not is_in_domain)) then count_lines 0 numtr numde numte
          else if intr == 0 then count_lines 0 (numtr+1) numde numte
          else if intr == 1 then count_lines 1 numtr (numde+1) numte
          else count_lines 2 numtr numde (numte+1) in
  let h,numtr,numde,numte =
    if noscan then (h,1,1,1) else (
        let _ = if not quiet then (Printf.fprintf stderr "Scanning file..." ; flush stderr) in
        let (numtr, numde, numte) = count_lines 0 0 0 0 in
          close_h h;
        let h = open_h () in
        let _ = if not quiet then (Printf.fprintf stderr "%d train, %d dev, %d test, reading..." numtr numde numte ; flush stderr) in
          (h,numtr,numde,numte)
      ) in
  
    (* make arrays *)
  let lab = if multilabel then L 0 else ML [||] in
  let tr = A.make numtr lab, A.make numtr [||], (if ber then A.empty [||] else A.make numtr [||]), make_ba numtr 1., A.make numtr 0 in
  let de = A.make numde lab, A.make numde [||], (if ber then A.empty [||] else A.make numde [||]), make_ba numde 1., A.make numde 0 in
  let te = A.make numte lab, A.make numte [||], (if ber then A.empty [||] else A.make numte [||]), make_ba numte 1., A.make numte 0 in

  let maxFeat  = ref 0 in
  let cur_line = ref 0 in
  let maxClass = ref 0 in

  let os = if bias then 1 else 0 in

    (* our reading function *)
  let read_single_line in_train (y,x,v,w,d) ntrue =
    match maybe_read_line h with
        None -> EOF
      | Some s ->
          incr cur_line;

          let n = if noscan then 0 else ntrue in

          let s = Str.replace_first single_line_re "" s in

          let is_in_domain =
            if not ignore_id && not ignore_ood then true
            else not (Str.string_match out_of_domain_re s 0) in

          if in_train && ((ignore_id  && is_in_domain) ||
                          (ignore_ood && not is_in_domain))
          then EmptyLine else
          if s = ""     then EmptyLine else
          if s = "DEV"  then DevLine   else
          if s = "TEST" then TestLine  else
          if String.get s 0 == '#' then CommentLine else
            let sections = of_list (Str.split sections_re s) in

              (* read section 0 *)
            let sec0 = of_list (Str.split (space_re) sections.(0)) in
              A.set y n ( (if multilabel then read_multilabel else read_multiclass named) !cur_line sec0.(0));

              if length sec0 > 1 then (
                let start_idx = ref 1 in
                let is_done = ref false in
                let a = sec0 in
                  while not !is_done && !start_idx <= min (length a-1) 6 do
                    if a.(!start_idx) = "$$$WEIGHT" then (
                      if !start_idx+1 >= length a then failwith ("$$$WEIGHT expects an argument on line " ^ string_of_int !cur_line);
                      w.{n} <- float_of_string_err !cur_line a.(!start_idx+1);
                      start_idx := !start_idx + 2;
                    ) else if a.(!start_idx) = "$$$TASK" then (
                      if !start_idx+1 >= length a then failwith ("$$$TASK expects an argument on line " ^ string_of_int !cur_line);
                      A.set d (n) (int_of_string_err !cur_line a.(!start_idx+1));
                      start_idx := !start_idx + 2;
                    ) else if a.(!start_idx) = "$$$OUTOFDOMAIN" then (
                      A.set d (n) 1;
                      start_idx := !start_idx + 1;
                    ) else (
                      is_done := true;
                    );
                  done;

(*
                if sec0.(1) = "$$$WEIGHT" then (
                  if length sec0 <= 2 then failwith ("$$$WEIGHT expects an argument on line " ^ string_of_int !cur_line);
                  w.{n} <- float_of_string_err !cur_line sec0.(2);
                  if length sec0 > 3 && sec0.(3) = "$$$OUTOFDOMAIN" then d.(n) <- false;
                ) else if sec0.(1) = "$$$OUTOFDOMAIN" then (
  d.(n) <- false;
                  if length sec0 > 2 && sec0.(2) = "$$$WEIGHT" then (
                    if length sec0 <= 3 then failwith ("$$$WEIGHT expects an argument on line " ^ string_of_int !cur_line);
                    w.{n} <- float_of_string_err !cur_line sec0.(3);
                  )
                )
*)
              );

              maxClass := max !maxClass (max (length sections-1)
                                            (match (A.get y n) with
                                                L c -> c
                                              | ML a -> length a
                                            ));

            let xiT = make !maxClass eia in
            let xvT = if ber then [||] else make !maxClass efa in

              for c = 0 to length sections - 2 do
                let a = of_list (Str.split (space_re) sections.(c+1)) in
                  if ber then (
                    let xi = make_ia (os+length a) 0  in
                      if bias then (
                        let fi = get_vocab ("**BIAS**" ^ string_of_int c ^ "**") in
                        xi.{0} <- I.of_int fi;
                        maxFeat := max !maxFeat fi;
                      );
                      for p = 0 to length a - 1 do
                        let fi = get_vocab a.(p) in
                        xi.{p+os} <- I.of_int fi;
                        maxFeat := max !maxFeat fi;
                      done;
                      xiT.(c) <- xi;
                  ) else (
                    let xi = make_ia (os+(length a)/2) 0 in
                    let xv = make_ba (os+(length a)/2) 0. in
                      if bias then (
                        let fi = get_vocab ("**BIAS**" ^ string_of_int c ^ "**") in
                        xi.{0} <- I.of_int fi;
                        xv.{0} <- 1.;
                        maxFeat := max !maxFeat fi;
                      );
                      for p = 0 to (length a) / 2 - 1 do
                        let fi = get_vocab                     a.(2*p) in
                        xi.{p+os} <- I.of_int fi;
                        xv.{p+os} <- float_of_string_err !cur_line a.(2*p+1);
                        maxFeat := max !maxFeat fi;
                      done;
                      xiT.(c) <- xi;
                      xvT.(c) <- xv;
                  );
              done;
              A.set x (n) xiT;

              if ber
              then predict (A.get y n) xiT [||]
              else ( A.set v (n) xvT
                   ; predict (A.get y n) xiT xvT );

              Success in

    (* read data *)

  let rec read_lines intr n =
    match read_single_line (intr == 0) (if intr == 0 then tr else if intr == 1 then de else te) n with
        EOF -> ()
      | EmptyLine   -> read_lines intr  n
      | CommentLine -> read_lines intr  n
      | DevLine     -> if intr<1 then read_lines 1 0 else failwith "two 'DEV' lines"
      | TestLine    -> if intr<2 then read_lines 2 0 else failwith "two 'TEST' lines"
      | Success     -> read_lines intr (n+1) in

  let _ = read_lines 0 0 in
  let _ = if not quiet then (Printf.fprintf stderr "done\n" ; flush stderr) in
  let mk_data (y,x,v,w,d) =
    { datav_bernoulli = ber;
      datav_implicit  = false;
      datav_sampled   = false;
      datav_numclass  = !maxClass;
      datav_numfeat   = !maxFeat+1;
      datav_task      = d;
      datav_weights   = w;
      datav_xi        = A.empty eia;
      datav_xi_c      = x;
      datav_xv        = A.empty efa;
      datav_xv_c      = if ber then A.empty [||] else v }, y in

    close_h h;

    mk_data tr, mk_data de, mk_data te

let read_twoclass_model readkernelmap h = 
  let ln = ref 0 in
(*  let w  = ref (init 32 (fun _ -> [|0.|])) in *)
  let w  = ref (make_ba 1024 0.) in
  let kres = ref None in
  let kerneled = ref false in
  let rec rd () =
    match maybe_read_line h with
        None -> ()
      | Some s -> (
          incr ln;
          match Str.split (space_re) s with
              ("***KERNELMAP***"::_) ->
                kres := Some (readkernelmap h s);
                kerneled := true;
                rd ()
            | [feature ; value] -> (
                let f = 
                  if feature = "**BIAS**" then 0
                  else if !kerneled then 1+int_of_string feature
                  else get_vocab feature in
                let v = float_of_string_err !ln value in
                  if f >= A1.dim !w then (  (* resize w *)
                    let dd = A1.dim !w in
                    let w' = init_ba (2*f) (fun i -> if i < dd then !w.{i} else 0.) in
                      w := w'
                  );
(*                    let w' = init (2 * f) (fun _ -> [|0.|]) in
                      for i = 0 to length !w - 1 do w'.(i).(0) <- !w.(i).(0); done; 
                  w := w'); *)
                  !w.{f} <- v;
                  rd ())
            | _ -> failwith ("malformed feature/value pair on line " ^ string_of_int !ln)) in
    rd ();
    (!kres, BinModel !w)
                
let read_multiclass_model readkernelmap h = 
  let ln = ref 0 in
  let w  = ref [||] in
  let kres = ref None in
  let rec rd () =
    match maybe_read_line h with
        None -> ()
      | Some s -> (
          incr ln;
          match Str.split (space_re) s with
              ("***NAMEDLABELSIDS***" :: names) -> (
                max_read_class := 0;
                List.iter (fun nam -> 
                             H.replace class_vocab nam !max_read_class ;
                             H.replace class_recab !max_read_class nam ;
                             incr max_read_class) names;
                decr max_read_class;
                rd ())
            | ("***KERNELMAP***"::_) ->
                kres := Some (readkernelmap h s);
                rd ()
            | (feature :: values) -> (
                let f  = if feature = "**BIAS**" then 0 else get_vocab feature in
                let vs = of_list (List.map (float_of_string_err !ln) values) in
                let _C = length vs in
                  if f >= length !w then (  (* resize w *)
                    let w' = init (2 * (f + 4)) (fun i -> if i < length !w then !w.(i) else make _C 0.) in
                      w := w');
                  iteri (fun c v -> !w.(f).(c) <- v) vs;
                  rd ())
            | _ -> failwith ("malformed feature/value pair on line " ^ string_of_int !ln)) in
    rd ();
    (!kres, MultModel !w)


let read_model readkernelmap h = function
    Binary | Perceptron | Binomial -> read_twoclass_model    readkernelmap h
  | Multiclass | Multitron         -> read_multiclass_model  readkernelmap h
  | Sampled                        -> read_twoclass_model    readkernelmap h

(* silly functions to read classes *)
let read_binary_class ln str =
  try
    let i = float_of_string str in
      if i < 0.5 then 0 else 1
  with Failure err ->
    failwith ("error: \"" ^ err ^ "\" occured on line " ^ string_of_int ln ^ " when trying to parse \"" ^ str ^ "\" as a float for binary class number")
  
let read_binomial_class ln str =
  try
    let i = float_of_string str in
      if i < 0. then 0. else if i > 1. then 1. else i
  with Failure err ->
    failwith ("error: \"" ^ err ^ "\" occured on line " ^ string_of_int ln ^ " when trying to parse \"" ^ str ^ "\" as a float for binomial class value")

let printNames () =
  let a = make (1 + !max_read_class) "***UNKNOWNCLASS***" in
    H.iter (fun nam i -> a.(i) <- nam) class_vocab;
    Printf.printf "***NAMEDLABELSIDS***";
    for i = 0 to !max_read_class do
      Printf.printf "\t%s" a.(i);
    done;
    Printf.printf "\n";
    ()


let printMatrixVocab kmap xTr w no_zero = 
  for f = 0 to xTr.datav_numfeat-1 do
    let offset = w.{f} in
    let keep = ref false in
      for c = 1 to xTr.datav_numclass - 1 do 
        if abs_float (w.{f + c * xTr.datav_numfeat} -. offset) > 1e-10 then keep := true;
      done;
      if !keep then (
        let offset2 = if no_zero then 0. else offset in
	  if f == 0 then Printf.printf "**BIAS**\t" else Printf.printf "%s\t" (Hashtbl.find vocabBwd f);
          Printf.printf "%10.20f" (w.{f} -. offset2);
          for c = 1 to xTr.datav_numclass - 1 do
	    Printf.printf " %10.20f" ((w.{f + c * xTr.datav_numfeat} -. offset2));
	  done;
	  Printf.printf "\n";
      );
      flush stdout;
  done
