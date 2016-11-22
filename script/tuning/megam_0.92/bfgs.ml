open Array
open Util
open Data
open Fastdot
module A = Arry
module I = Int32

let useBias = ref false

type memory =
    { mutable mem_filled : bool;
      mutable mem_posn   : int;
      mem_darray : bigarr array;
      mem_uarray : bigarr array;
      mem_aarray : bigarr }

let memory_create mem_size len =
  { mem_filled = false;
    mem_posn   = 0;
    mem_darray = create_matrix_ba mem_size len 0.;
    mem_uarray = create_matrix_ba mem_size len 0.;
    mem_aarray = make_ba mem_size 0. }

let memory_size mem =
  if mem.mem_filled then length mem.mem_darray else mem.mem_posn

let memory_d mem m =
  if mem.mem_filled
  then
    let len = length mem.mem_darray in
      mem.mem_darray.((len + mem.mem_posn + m - 1) mod len)
  else mem.mem_darray.(m)

let memory_u mem m =
  if mem.mem_filled
  then
    let len = length mem.mem_uarray in
      mem.mem_uarray.((len + mem.mem_posn + m - 1) mod len)
  else mem.mem_uarray.(m)

let memory_a mem m =
  if mem.mem_filled
  then
    let len = length mem.mem_uarray in
      mem.mem_aarray.{(len + mem.mem_posn + m - 1) mod len}
  else mem.mem_aarray.{m}

let memory_add mem d u a =
  let p  = mem.mem_posn in
(*  let _F = A1.dim d in
    for f = 0 to _F - 1 do
      mem.mem_darray.(p).{f} <- d.{f};
      mem.mem_uarray.(p).{f} <- u.{f};
    done; *)
    add_dense_dense mem.mem_darray.(p) d 0. 1.;
    add_dense_dense mem.mem_uarray.(p) u 0. 1.;

    mem.mem_aarray.{mem.mem_posn} <- a;
    if mem.mem_filled
    then mem.mem_posn <- (mem.mem_posn + 1) mod (length mem.mem_darray)
    else (
      if mem.mem_posn == length mem.mem_darray - 1
      then ( mem.mem_posn <- 0; mem.mem_filled <- true; )
      else ( mem.mem_posn <- mem.mem_posn + 1 ) );
    ()

(*
let dot (a1 : bigarr) (a2 : bigarr) =
  let _N = min (A1.dim a1) (A1.dim a2) in
  let rec dr n acc =
    if n >= _N then acc else
      dr (n+1) (acc +. a1.{n} *. a2.{n}) in
    dr 0 0.
*)

let dot = mult_dense_dense

let dot_minus _C _F m (a1 : bigarr) (a2 : bigarr) =
  let acc = ref 0. in
  let mind = min (A1.dim a1) (A1.dim a2) in
    for c = 0 to _C - 1 do
      for f = 0 to _F - 1 do
        let cnf = c * _F + f in
          if cnf < mind then (
            if f < length m && c < length m.(f) then
              acc := !acc +. a1.{cnf} *. (a2.{cnf} -. m.(f).(c))
            else 
              acc := !acc +. a1.{cnf} *. a2.{cnf}
          );
      done;
    done;
    !acc

let classify_by_wtx y (wtx : float array) =  (* we use y to check allowability of multilabel predictions *)
  let _C = length wtx in
    match y with
        L _ ->
          let mx = ref 0 in
            for c = 1 to _C -1 do
              if wtx.(c) > wtx.(!mx) then mx := c;
            done;
            !mx
      | ML arr -> 
          let mx = ref (-1) in
            for c = 0 to (min _C (length arr)) - 1 do
              if arr.(c) < infinity && c < length wtx then (
                if !mx < 0 then mx := c
                else if wtx.(c) > wtx.(!mx) then mx := c;
              );
            done;
            !mx

let compute_error_by_wtx (mass : bigarr) (wtx : float array A.arry) (y : multilabel A.arry) =
  let numerr  = ref 0. in
  let totmass = ref 0. in
  let _N = A.length wtx in
    if _N == 0 then 0. else (
      let _C = length (A.get wtx 0) in
        if _C > 1 then (
          for n = 0 to _N - 1 do
            let wtxn = A.get wtx n in
            let yn = A.get y n in
            match yn with
                L c ->
                  if c != classify_by_wtx yn wtxn then 
                    numerr := !numerr +. if A1.dim mass == 0 then 1. else mass.{n};
                  totmass := !totmass +. if A1.dim mass == 0 then 1. else mass.{n};
              | ML a ->
                  let c = classify_by_wtx yn wtxn in
                    numerr  := !numerr  +. (if c >= length a then maximum_not_inf a else a.(c)) *. if A1.dim mass == 0 then 1. else mass.{n};
                    totmass := !totmass +. (maximum_not_inf a) *. if A1.dim mass == 0 then 1. else mass.{n};
          done;
          !numerr /. !totmass
        ) else ( 0. ))

let compute_ppx_by_wtx (mass : bigarr) wtx (y : multilabel A.arry) =
  let ppx = ref 0. in
  let _N = A.length wtx in
  let _C = length (A.get wtx 0) in
    if _C > 1 then (
      for n = 0 to _N - 1 do
        let wtxn = A.get wtx n in
        let s = ref zero in
        let yn = A.get y n in
          for c = 0 to (max (length wtxn) _C) - 1 do
            s := addLog !s wtxn.(c);
          done;
          match yn with
              L  c -> if c < length wtxn then ppx := !ppx +. wtxn.(c) -. !s;
            | ML a -> (let mx = maximum_not_inf a in 
                         for c = 0 to length a - 1 do
                           if a.(c) < infinity && c < length wtxn then
                             ppx := !ppx +. (if c >= length a then 0. else (mx -. a.(c))) *. wtxn.(c); 
                         done;
                         ppx := !ppx -. !s;
                      )
      done;
      !ppx /. float_of_int _N
    ) else (
      ppx := zero;
      for n = 0 to _N - 1 do
        let wtxn = A.get wtx n in
          ppx := !ppx +@ wtxn.(0);
      done;
      !ppx -. float_of_int _N
    )

(* (w+e dw)*(w+e dw) = w*w + e^2 dw*dw + 2e w*dw *)

let compute_log_posterior_nonsampled _C lambda x y wtx dwtx eta wtw dwtdw dwtw =
  let _N = A.length wtx in
    (* (w + f dw)*x = w*x + f (dw*x) *)
  let post = ref (0. -. (lambda /. 2.) *. (wtw *. wtw +. eta *. eta *. dwtdw *. dwtdw +. 2. *. eta *. dwtw)) in
    for n = 0 to _N - 1 do
      let sum = ref zero in
      let yn = A.get y n in
      let y_  = lab yn in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
      let dwtxn = A.get dwtx n in
        for c = 0 to _C - 1 do
          let wtx' = wtxn.(c) +. eta *. dwtxn.(c) in
            sum := addLog !sum wtx';
            if c == y_ then post := !post +. wt *. wtx';
        done;
        post := !post -. wt *. !sum;
    done;
    !post

let compute_log_posterior_nonsampled_ml _C lambda x y wtx dwtx eta wtw dwtdw dwtw =
  let _N = A.length wtx in
    (* (w + f dw)*x = w*x + f (dw*x) *)
  let post = ref (0. -. (lambda /. 2.) *. (wtw *. wtw +. eta *. eta *. dwtdw *. dwtdw +. 2. *. eta *. dwtw)) in
    for n = 0 to _N - 1 do
      let yn = A.get y n in
      let ML y_ = yn in
      let max_y = maximum_not_inf y_ in
      let min_y = minimum y_ in
      let sumN = ref zero in
      let sumD = ref zero in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
      let dwtxn = A.get dwtx n in
        for c = 0 to (min _C (length y_)) - 1 do
          if y_.(c) < infinity then (
            let wtx' = wtxn.(c) +. eta *. dwtxn.(c) in
              if y_.(c) < max_y then sumN := addLog !sumN (wt *. (wtx' +. log (max_y -. y_.(c))));
              sumD := addLog !sumD (wt *. (wtx' +. log (max_y -. min_y)));
(*              sum := addLog !sum (max_y *. wtx');
              post := !post +. (if c < length y_ then max_y -. y_.(c) else 0.) *. wtx'; *)
          );
        done;
(*        post := !post -. !sum; *)
        post := !post +. !sumN -. !sumD;
    done;
    !post

let compute_log_posterior_sampled lambda x y wtx dwtx eta wtw dwtdw dwtw samp_expZ =
  let _N = A.length wtx in
    (* (w + f dw)*x = w*x + f (dw*x) *)
  let post = ref (0. -. (lambda /. 2.) *. (wtw *. wtw +. eta *. eta *. dwtdw *. dwtdw +. 2. *. eta *. dwtw)) in
    for n = 0 to _N - 1 do
      let wt   = dat_wt x n in
      let wtxn = A.get wtx n in
      let dwtxn = A.get dwtx n in
      let wtx' = wtxn.(0) +. eta *. dwtxn.(0) in
        post := !post +. wt *. (wtx' -. samp_expZ);
    done;
    !post

let compute_log_posterior _C lambda x y wtx dwtx eta wtw dwtdw dwtw sampOpt multilabel =
  match sampOpt with
      None       -> if multilabel
                    then bad_float_to_neginf (compute_log_posterior_nonsampled_ml _C lambda x y wtx dwtx eta wtw dwtdw dwtw)
                    else bad_float_to_neginf (compute_log_posterior_nonsampled    _C lambda x y wtx dwtx eta wtw dwtdw dwtw)
    | Some (_,z) ->      bad_float_to_neginf (compute_log_posterior_sampled          lambda x y wtx dwtx eta wtw dwtdw dwtw z)


let perform_line_search _C _F lambda x y wtx (w : bigarr) (dw : bigarr) dwtx (g : bigarr) sampOpt mean_model multilabel =
  let dwtdw = dot dw dw in 
  let sum    = sqrt dwtdw in
  let scale = if sum <= 100. then 1. else 100. /. sum in

  let dwtdw = dwtdw *. scale *. scale in

  let wtw,dwtw = 
    match mean_model with
        None               -> (dot               w w, dot               dw w *. scale)
      | Some (MultModel m) -> (dot_minus _C _F m w w, dot_minus _C _F m dw w *. scale) in

  let f_old = compute_log_posterior _C lambda x y wtx dwtx 0. wtw dwtdw dwtw sampOpt multilabel in

  let slope = scale *. dot g dw in
    if slope < 0. then 0. else

  let test = fold_lefti_ba (fun i dwi test ->
                              let temp = (fabs (scale *. dwi)) /. max (fabs w.{i}) 1. in
                                if temp > test then temp else test) 0. dw in
  let alamin = 1e-10 /. test in
  let init_alam = 1. in

  let rec lsrch alam alam2 f2 =
    (* Printf.fprintf stderr "(lsrch %g %g %g) " alam alam2 f2; *)
    if scale *. alam < 1e-6 && is_some sampOpt then 1e-6 *. min 1. scale else
    if scale *. alam < alamin then 1e-8 *. min 1. scale
    else 
      let cur_f = compute_log_posterior _C lambda x y wtx dwtx (alam*.scale) wtw dwtdw dwtw sampOpt multilabel in
        if f_old <= neg_infinity then 0. (* failwith "" *)
        else if cur_f >= f_old +. 1e-4 *. alam *. scale *. slope then alam *. scale
        else
          if fabs (alam -. init_alam) < 1e-20
          then ( 
            let tmplam = slope /. (2. *. (f_old +. slope -. cur_f)) in
              lsrch (max tmplam (0.1*.alam)) alam cur_f
          ) else (
            let rhs1 = cur_f -. f_old -. alam  *. slope in
            let rhs2 = f2    -. f_old -. alam2 *. slope in
            let a    = (rhs1/.(alam*.alam) -. rhs2/.(alam2*.alam2)) /. (alam-.alam2) in
            let b    = (alam*.rhs2/.(alam2*.alam2) -. alam2*.rhs1/.(alam*.alam)) /. (alam-.alam2) in
            let tmplam = ref 0. in
              if fabs a < 1e-20 then tmplam := slope/.(-2.*.b)
              else (
                let disc = b*.b -. 3.*.a*.slope in
                  if disc < 0. then tmplam := 0.5 *. alam
                  else if b <= 0. then tmplam := ((sqrt disc) -. b) /. (3.*.a)
                  else tmplam := 0. -. slope /. (b +. sqrt disc);
              );
              if !tmplam > 0.5*.alam then tmplam := 0.5*.alam;
              lsrch (max !tmplam (0.1*.alam)) alam cur_f
          ) in
    lsrch init_alam 0. f_old

(* these are for the simple label case *)
let compute_gradient_bernoulli_implicit _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
  let numF = x.datav_numfeat in
    for n = 0 to _N - 1 do
      let yn = A.get y n in
      let y_ = lab yn in
      let wti = ref zero in
      let xi  = (A.get x.datav_xi n) in
      let _P  = A1.dim xi in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C - 1 do
          wti := addLog !wti wtxn.(c');
        done;
	for c = 0 to _C - 1 do
          let cnf = c * numF in
          let factor = (if c == y_ then 1. else 0.) -. exp (wtxn.(c) -. !wti) in
            if !useBias then g.{cnf} <- g.{cnf} +. factor *. wt;
            add_dense_sparse g xi (factor *. wt) cnf;
(*            for p = 0 to _P - 1 do
              if xi.{p} == 0 then failwith "ack";
              g.{xi.{p} + cnf} <- g.{xi.{p} + cnf} +. factor *. wt;
            done; *)
	done;
    done;
    ()
  
let compute_gradient_nonbernoulli_implicit _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
  let numF = x.datav_numfeat in
    for n = 0 to _N - 1 do
      let yn = A.get y n in
      let y_ = lab yn in
      let wti = ref zero in
      let xi = A.get x.datav_xi n in
      let _P  = A1.dim xi in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C - 1 do
          wti := addLog !wti wtxn.(c')
        done;
	for c = 0 to _C - 1 do
          let cnf = c * numF in
          let factor = (if c == y_ then 1. else 0.) -. exp (wtxn.(c) -. !wti) in
            if !useBias then g.{cnf} <- g.{cnf} +. factor *. wt;
            add_dense_sparse_val g xi (A.get x.datav_xv n) (factor *. wt) cnf;
(*            for p = 0 to _P - 1 do
              g.{xi.{p} + cnf} <- g.{xi.{p} + cnf} +. factor *. (A.get x.datav_xv n).{p} *. wt;
            done; *)
	done;
    done;
    ()

let compute_gradient_bernoulli_explicit _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
    for n = 0 to _N - 1 do
      let yn = A.get y n in
      let y_ = lab yn in
      let xi = (A.get x.datav_xi_c n) in
      let _C' = min _C (length xi) in
      let wti = ref zero in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C' - 1 do
          wti := addLog !wti wtxn.(c');
        done;
	for c = 0 to _C' - 1 do
          let _P  = A1.dim xi.(c) in
          let factor = (if c == y_ then 1. else 0.) -. exp (wtxn.(c) -. !wti) in
            add_dense_sparse g xi.(c) (factor *. wt) 0;
(*            for p = 0 to _P - 1 do
              g.{(A.get x.datav_xi_c n).(c).{p}} <- g.{(A.get x.datav_xi_c n).(c).{p}} +. factor *. wt;
            done; *)
	done;
    done;
    ()
  
let compute_gradient_nonbernoulli_explicit _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
    for n = 0 to _N - 1 do
        let yn = A.get y n in
      let y_ = lab yn in
      let xi = (A.get x.datav_xi_c n) in
      let xv = (A.get x.datav_xv_c n) in
      let _C' = min _C (length xi) in
      let wti = ref zero in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C' - 1 do
          wti := addLog !wti wtxn.(c')
        done;
	for c = 0 to _C' - 1 do
          let _P = A1.dim xi.(c) in
          let factor = (if c == y_ then 1. else 0.) -. exp (wtxn.(c) -. !wti) in
            add_dense_sparse_val g xi.(c) xv.(c) (factor *. wt) 0;
(*            for p = 0 to _P - 1 do
              g.{(A.get x.datav_xi_c n).(c).{p}} <- g.{(A.get x.datav_xi_c n).(c).{p}} +. factor *. (A.get x.datav_xv_c n).(c).{p} *. wt;
            done; *)
	done;
    done;

    ()

let compute_gradient_sampled lambda (w : bigarr) x y wtx (g : bigarr) (samp_exp_F : bigarr) samp_exp_Z =
  let _N = dat_N x in
  let _F = A1.dim w in
  let numF = x.datav_numfeat in
    for f = 0 to _F - 1 do
      g.{f} <- g.{f} -. (float_of_int _N) *. (if f == 0 then 1. else samp_exp_F.{f});
    done;
    for n = 0 to _N - 1 do
        let yn = A.get y n in
      let y_ = lab yn in
      let wti = ref zero in
      let xi = A.get x.datav_xi n in
      let _P  = A1.dim xi in
      let wt  = dat_wt x n in
        if x.datav_implicit then (
          for p = 0 to _P - 1 do
            g.{I.to_int xi.{p}} <- g.{I.to_int xi.{p}} +. wt;
          done;
        ) else (
          for p = 0 to _P - 1 do
            g.{I.to_int xi.{p}} <- g.{I.to_int xi.{p}} +. wt *. (A.get x.datav_xv n).{p};
          done;
        );
    done;
    ()



(* these are for the multi-label case *)
let compute_gradient_bernoulli_implicit_ml _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
  let numF = x.datav_numfeat in
    for n = 0 to _N - 1 do
        let yn = A.get y n in
      let ML y_ = yn in
      let max_y = maximum_not_inf y_ in
      let min_y = minimum y_ in
      let wti = ref zero in
      let wtiV = ref zero in
      let xi = A.get x.datav_xi n in
      let _P  = A1.dim xi in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C - 1 do
          if y_.(c') < infinity then (
            wti  := addLog !wti  wtxn.(c');
            if y_.(c') < max_y then wtiV := addLog !wtiV (wtxn.(c') +. log (max_y -. y_.(c')));
          );
        done;
	for c = 0 to _C - 1 do
          if y_.(c) < infinity then (
            let cnf = c * numF in
            let vc = if c < length y_ then max_y -. y_.(c) else 0. in
            let factor = (if vc <= 0. then 0. else exp (wtxn.(c) +. log vc -. !wtiV)) -. exp(wtxn.(c) -. !wti) in
              if !useBias then g.{cnf} <- g.{cnf} +. factor *. wt;
              add_dense_sparse g xi (factor *. wt) cnf;
(*              for p = 0 to _P - 1 do
                g.{xi.{p} + cnf} <- g.{xi.{p} + cnf} +. factor *. wt;
              done; *)
          );
	done;
    done;
    ()
  
let compute_gradient_nonbernoulli_implicit_ml _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
  let numF = x.datav_numfeat in
    for n = 0 to _N - 1 do
        let yn = A.get y n in
      let ML y_ = yn in
      let max_y = maximum_not_inf y_ in
      let min_y = minimum y_ in
      let wti = ref zero in
      let wtiV = ref zero in
      let xi = A.get x.datav_xi n in
      let _P  = A1.dim xi in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C - 1 do
          if y_.(c') < infinity then (
            wti  := addLog !wti  wtxn.(c');
            if y_.(c') < max_y then wtiV := addLog !wtiV (wtxn.(c') +. log (max_y -. y_.(c')));
          );
        done;
(*        Printf.eprintf "[%g %g]" !wti !wtiV; *)
	for c = 0 to _C - 1 do
          if y_.(c) < infinity then (
            let cnf = c * numF in
            let vc = if c < length y_ then max_y -. y_.(c) else 0. in
            let factor = (if vc <= 0. then 0. else exp (wtxn.(c) +. log vc -. !wtiV)) -. exp(wtxn.(c) -. !wti) in
              if !useBias then g.{cnf} <- g.{cnf} +. factor *. wt;
              add_dense_sparse_val g xi (A.get x.datav_xv n) (factor *. wt) cnf;
(*              for p = 0 to _P - 1 do
                g.{xi.{p} + cnf} <- g.{xi.{p} + cnf} +. factor *. (A.get x.datav_xv n).{p} *. wt;
              done; *)
          );
	done;
    done;
    ()

let compute_gradient_bernoulli_explicit_ml _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
    for n = 0 to _N - 1 do
      let yn = A.get y n in
      let ML y_ = yn in
      let max_y = maximum_not_inf y_ in
      let min_y = minimum y_ in
      let xi = (A.get x.datav_xi_c n) in
      let _C' = min (length y_) (min _C (length xi)) in
      let wti = ref zero in
      let wtiV = ref zero in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C' - 1 do
          if y_.(c') < infinity then
            wti := addLog !wti wtxn.(c');
(*            if y_.(c') < max_y then wtiV := addLog !wtiV (wtxn.(c') +. log (max_y -. y_.(c')));
          ); *)
        done;
	for c = 0 to _C' - 1 do
          if y_.(c) < infinity then (
            let vc = if c < length y_ then max_y -. y_.(c) else min_y in
              (* (if vc <= 0. then 1. else exp (wtxn.(c) +. log vc -. !wtiV)) *)
            let factor =  vc -. exp(wtxn.(c) -. !wti) in
              add_dense_sparse g xi.(c) (factor *. wt) 0;
(*              for p = 0 to _P - 1 do
                g.{(A.get x.datav_xi_c n).(c).{p}} <- g.{(A.get x.datav_xi_c n).(c).{p}} +. factor *. wt;
              done; *)
          );
	done;
    done;
    ()
  
let compute_gradient_nonbernoulli_explicit_ml _C lambda (w : bigarr) x y wtx (g : bigarr) =
  let _N = dat_N x in
  let _F = A1.dim w in
    for n = 0 to _N - 1 do
        let yn = A.get y n in
      let ML y_ = yn in
      let max_y = maximum_not_inf y_ in
      let min_y = minimum y_ in
      let xi = (A.get x.datav_xi_c n) in
      let xv = (A.get x.datav_xv_c n) in
      let _C' = min (length y_) (min _C (length xi)) in
      let wti = ref zero in
      let wtiV = ref zero in
      let wt  = dat_wt x n in
      let wtxn = A.get wtx n in
        for c' = 0 to _C' - 1 do
          if y_.(c') < infinity then (
            wti := addLog !wti wtxn.(c');
            if y_.(c') < max_y then wtiV := addLog !wtiV (wtxn.(c') +. log (max_y -. y_.(c')));
          );
        done;
	for c = 0 to _C' - 1 do
          if y_.(c) < infinity then (
            let vc = if c < length y_ then max_y -. y_.(c) else min_y in
            let factor = vc -. exp(wtxn.(c) -. !wti) in
            add_dense_sparse_val g xi.(c) xv.(c) (factor *. wt) 0;
(*              for p = 0 to _P - 1 do
                g.{(A.get x.datav_xi_c n).(c).{p}} <- g.{(A.get x.datav_xi_c n).(c).{p}} +. factor *. (A.get x.datav_xv_c n).(c).{p} *. wt;
              done; *)
          );
	done;
    done;

    ()


let compute_gradient _C lambda (w : bigarr) x y wtx (g : bigarr) (sampOption : (bigarr * float) option) mean_model multilabel =
  let _F = A1.dim w in
    (match mean_model with
         None -> (
           for f = 0 to _F - 1 do 
             g.{f} <- 0. -. lambda *. w.{f}; 
           done;
           )
       | Some (MultModel m) -> (
           for c = 0 to _C - 1 do
             for f = 0 to x.datav_numfeat - 1 do
               let cnf = c * x.datav_numfeat + f in
                 if cnf < A1.dim w && cnf < A1.dim g then (
                   if f < length m && c < length m.(f) then
                     g.{cnf} <- 0. -. lambda *. (w.{cnf} -. m.(f).(c))
                   else 
                     g.{cnf} <- 0. -. lambda *. w.{cnf};
                 );
             done;
           done)
    );
    match sampOption with
        None ->
          (match (multilabel, x.datav_implicit, x.datav_bernoulli) with
              (false, true , true ) -> compute_gradient_bernoulli_implicit    _C lambda w x y wtx g
            | (false, true , false) -> compute_gradient_nonbernoulli_implicit _C lambda w x y wtx g
            | (false, false, true ) -> compute_gradient_bernoulli_explicit    _C lambda w x y wtx g
            | (false, false, false) -> compute_gradient_nonbernoulli_explicit _C lambda w x y wtx g

            | (true , true , true ) -> compute_gradient_bernoulli_implicit_ml    _C lambda w x y wtx g
            | (true , true , false) -> compute_gradient_nonbernoulli_implicit_ml _C lambda w x y wtx g
            | (true , false, true ) -> compute_gradient_bernoulli_explicit_ml    _C lambda w x y wtx g
            | (true , false, false) -> compute_gradient_nonbernoulli_explicit_ml _C lambda w x y wtx g
          );
      | Some (f,z) ->
          compute_gradient_sampled lambda w x y wtx g f z

  

(* compute_gradient is where we spend all our time -- let's optimize it
   for the different data types separately...original version is commented
   below, and takes:
     44.37%=111.44sec in 41 calls, 2.72/2.76 sec/call    --  without prep
     16.33%=108.47sec in 41 calls, 0.83/1.70 sec/call    --  with prep

   when we optimize for bernoulli/implicit, this drops to
     18.92%=80.97sec  in 41 calls, 0.67/0.67 sec/call

let compute_gradient _C lambda w x y wtx g =
  let _N = dat_N x in
  let _F = length w in
  let _  = for f = 0 to _F - 1 do g.(f) <- 0. -. lambda *. w.(f); done in

  let add_to_grad w n c =
    let _P = dat_P x n c in
      for p = 0 to _P - 1 do
	g.(dat_i x n c p) <- g.(dat_i x n c p) +. w *. dat_v x n c p;
      done in

    for n = 0 to _N - 1 do
      let wti = ref zero in
        for c' = 0 to _C - 1 do
          wti := addLog !wti wtxn.(c')
        done;
	for c = 0 to _C - 1 do
          if c == yn
          then add_to_grad (1. -. exp (wtxn.(c) -. !wti)) n c
          else add_to_grad (0. -. exp (wtxn.(c) -. !wti)) n c;
	done;
    done;
    ()
*)

let compute_update_direction_bfgs _C lambda memory x y wtx (w : bigarr) (w_old : bigarr) (g_old : bigarr) (q : bigarr) (g : bigarr) sampOption mean_model multilabel =
  let _N = dat_N x in
  let _F = A1.dim w in
  (* first, compute the gradient *)
  let () = compute_gradient _C lambda w x y wtx g sampOption mean_model multilabel in

  (* now, compute u, d, alpha and sigma *)
  let utd = ref 0. in let utu = ref 0. in
(*
    for f = 0 to _F - 1 do
      let u = g.{f} -. g_old.{f} in
      let d = w.{f} -. w_old.{f} in
	utd := !utd +. u*.d;
	utu := !utu +. u*.u;
	g_old.{f} <- u;
	w_old.{f} <- d;
    done;
*)

    add_dense_dense g_old g (-1.) 1.;
    add_dense_dense w_old w (-1.) 1.;

    utd := mult_dense_dense g_old w_old;
    utu := mult_dense_dense g_old g_old;

  let alpha = !utd in 
  let sigma = !utd /. !utu in

  (* update our memory *)
    memory_add memory w_old g_old alpha;

  (* now, compute q = Hg *)
  let _    = for f = 0 to _F - 1 do q.{f} <- g.{f}; done in
  let _M   = memory_size memory in
  let beta = make _M 0. in
    for m = _M - 1 downto 0 do
      beta.(m) <- (dot (memory_d memory m) g) /. memory_a memory m;
      let um = memory_u memory m in
(*        for f = 0 to _F - 1 do
          q.{f} <- q.{f} -. beta.(m) *. um.{f};
        done; *)
        add_dense_dense q um 1. (-.beta.(m));
    done;
(*    for f = 0 to _F - 1 do
      q.{f} <- q.{f} *. sigma;
    done; *)
    mult_dense q sigma;
    for m = 0 to _M - 1 do
      let dm  = memory_d memory m in
      let um  = memory_u memory m in
      let am  = memory_a memory m in
      let umq = ref (dot um q) in
        for f = 0 to _F - 1 do
          let dq = dm.{f} *. (beta.(m) -. !umq /. am) in
            umq := !umq +. um.{f} *. dq;
            q.{f} <- q.{f} +. dq;
        done;
        
    done;

  (* replace g_old with g *)

(*    for f = 0 to _F - 1 do
      g_old.{f} <- g.{f} -. g_old.{f};
    done; *)
    add_dense_dense g_old g (-1.) 1.;

    ()

let update_wtx_bfgs _C x wtx eta dwtx =
  let _N = dat_N x  in
    for n = 0 to _N - 1 do
      let wtxn = A.get wtx n in
      let dwtxn = A.get dwtx n in
      let _C' = min _C (length wtxn) in
        for c = 0 to _C' - 1 do
	  wtxn.(c) <- wtxn.(c) +. bad_float_to_zero (eta *. dwtxn.(c));
        done;
    done;
    ()

let update_wtx_nodwtx _C x wtx eta (dw : bigarr) =
  let _N = dat_N x in
  let c0 = A1.dim dw in
  let or_empty x n = if n >= A.length x then efa else A.get x (n) in
  let or_empty' x n c = 
    if n >= A.length x then efa 
    else if c >= length (A.get x n) then efa
    else (A.get x n).(c) in

    for n = 0 to _N - 1 do
      let wtxn = A.get wtx n in
      let _C' = min (min _C (length wtxn)) (A1.dim dw) in
      if x.datav_implicit then (
        let xi  = (A.get x.datav_xi n) in 
        let xv = or_empty x.datav_xv n in
          for c = 0 to _C' - 1 do
            wtxn.(c) <- wtxn.(c) +. bad_float_to_zero (eta *. dw.{c * x.datav_numfeat});
            if x.datav_bernoulli
            then wtxn.(c) <- wtxn.(c) +. eta *. bad_float_to_zero (mult_dense_sparse     dw xi    (c * x.datav_numfeat))
            else wtxn.(c) <- wtxn.(c) +. eta *. bad_float_to_zero (mult_dense_sparse_val dw xi xv (c * x.datav_numfeat))
          done;
      ) else (
        if x.datav_bernoulli then (
          let xi = A.get x.datav_xi_c n in 
            for c = 0 to _C' - 1 do
              wtxn.(c) <- wtxn.(c) +. eta *. bad_float_to_zero (mult_dense_sparse     dw xi.(c)        0)
            done;
        ) else (
          let xi = A.get x.datav_xi_c n in 
          let xv = A.get x.datav_xv_c n in
            for c = 0 to _C' - 1 do
              wtxn.(c) <- wtxn.(c) +. eta *. bad_float_to_zero (mult_dense_sparse_val dw xi.(c) xv.(c) 0);
            done;
        );
      );
    done;
    ()

(*        for c = 0 to _C' - 1 do
        let _P = dat_P x n c in 
          if x.datav_implicit then
            wtxn.(c) <- wtxn.(c) +. bad_float_to_zero (eta *. dw.{c * x.datav_numfeat});

          for p = 0 to _P - 1 do
            let di = dat_i x n c p in
              wtxn.(c) <- wtxn.(c) +. bad_float_to_zero (eta *. dw.{di} *. dat_v x n c p); 
          done;
          done;
          done;
          ()
*)


(* a lot of time is spent in update_weights after compute_wtx has been
   inlined, so we also want to optimize this for different data types.  
   the original version is included below:
    for bernoulli, implicit, this drops update_weights from
      36%, 74.63sec, 41 calls at 1.82/2.43 s/call
    to
      20%, 50.13sec, 41 calls at 0.57/1.19 s/call
      
let compute_wtx _C x dw wtx =
  let _N = dat_N x in
    for n = 0 to _N - 1 do
      for c = 0 to _C - 1 do
	let _P = dat_P x n c in
	  wtxn.(c) <- 0.;
	  for p = 0 to _P - 1 do
	    wtxn.(c) <- wtxn.(c) +. dw.(dat_i x n c p) *. dat_v x n c p;
	  done;
      done;
    done;
    ()
*)

let compute_wtx_bernoulli_implicit _C x (dw : bigarr) wtx =
  let _N = dat_N x in
  let _F = x.datav_numfeat in
    for n = 0 to _N - 1 do
      let xi = A.get x.datav_xi n in
      let _P = A1.dim xi in
      let wtxn = A.get wtx n in
        for c = 0 to _C - 1 do
          let cnf = c * _F in
          wtxn.(c) <- dw.{cnf} +. mult_dense_sparse dw xi cnf;
(*          for p = 0 to _P - 1 do
            wtxn.(c) <- wtxn.(c) +. dw.{xi.{p} + cnf};
          done; *)
        done;
    done;
    ()

let compute_wtx_nonbernoulli_implicit _C x (dw : bigarr) wtx =
  let _N = dat_N x in
  let _F = x.datav_numfeat in
    for n = 0 to _N - 1 do
      let xi = A.get x.datav_xi n in
      let xv = A.get x.datav_xv n in
      let _P = A1.dim xi in
      let wtxn = A.get wtx n in
      let _C' = min _C (length wtxn) in
        for c = 0 to _C' - 1 do
          let cnf = _F * c in
          wtxn.(c) <- dw.{cnf} +. mult_dense_sparse_val dw xi xv cnf;
(*          for p = 0 to _P - 1 do
            wtxn.(c) <- wtxn.(c) +. dw.{xi.{p}+cnf} *. (A.get x.datav_xv n).{p};
          done; *)
        done;
    done;
    ()

let compute_wtx_bernoulli_explicit _C x (dw : bigarr) wtx =
  let _N = dat_N x in
    for n = 0 to _N - 1 do
      let _C' = min _C (length (A.get x.datav_xi_c n)) in
      let wtxn = A.get wtx n in
      let xi = A.get x.datav_xi_c n in
        for c = 0 to _C' - 1 do
          wtxn.(c) <- mult_dense_sparse dw xi.(c) 0;
(*          let _P = A1.dim (A.get x.datav_xi_c n).(c) in *)
(*          if !useBias then
            wtxn.(c) <- dw.{0}; *)
(*          for p = 0 to _P - 1 do
            wtxn.(c) <- wtxn.(c) +. dw.{(A.get x.datav_xi_c n).(c).{p}};
          done; *)
        done;
    done;
    ()

let compute_wtx_nonbernoulli_explicit _C x (dw : bigarr) wtx =
  let _N = dat_N x in
    for n = 0 to _N - 1 do
      let _C' = min _C (length (A.get x.datav_xi_c n)) in
      let wtxn = A.get wtx n in
      let xi = A.get x.datav_xi_c n in
      let xv = A.get x.datav_xv_c n in
        for c = 0 to _C' - 1 do
          wtxn.(c) <- mult_dense_sparse_val dw xi.(c) xv.(c) 0;
(*          let _P = A1.dim (A.get x.datav_xi_c n).(c) in *)
(*          if !useBias then
            wtxn.(c) <- dw.{0}; *)
(*          for p = 0 to _P - 1 do
            wtxn.(c) <- wtxn.(c) +. dw.{(A.get x.datav_xi_c n).(c).{p}} *. (A.get x.datav_xv_c n).(c).{p};
          done; *)
        done;
    done;
    ()

let update_weights _C lambda x y wtx (w : bigarr) (w_old : bigarr) (dw : bigarr) dwtx (g : bigarr) xDe wDe xTe wTe sampOpt mean_model multilabel =
  let _   = 
    if x.datav_implicit
    then
      if x.datav_bernoulli 
      then compute_wtx_bernoulli_implicit    _C x dw dwtx
      else compute_wtx_nonbernoulli_implicit _C x dw dwtx
    else
      if x.datav_bernoulli 
      then compute_wtx_bernoulli_explicit    _C x dw dwtx
      else compute_wtx_nonbernoulli_explicit _C x dw dwtx in
  let eta = perform_line_search _C x.datav_numfeat lambda x y wtx w dw dwtx g sampOpt mean_model multilabel in
  let _N = dat_N x in
  let _F = A1.dim w in
    (* update wtx *)
    update_wtx_bfgs   _C x   wtx eta dwtx;
    update_wtx_nodwtx _C xDe wDe eta dw;
    update_wtx_nodwtx _C xTe wTe eta dw;

    (* update w and w_old *)
    add_dense_dense w_old w 0. 1.;
    add_dense_dense w dw 1. eta;
(*    for f = 0 to _F - 1 do
      w_old.{f} <- w.{f};
      w.{f}     <- w.{f} +. bad_float_to_zero (eta *. dw.{f});
    done; *)

    (* return amount of change *)
    eta *. fold_left_ba (fun mx v -> max mx (fabs v)) 0. dw

let z_only = function None -> None | Some (_,z) -> Some z

let optimize_bfgs lastweight repeat_count quiet maxi lambda dpp _M x y xDe yDe xTe yTe (w : bigarr) wtx wtxDev wtxTst sampOption mean_model returnStuff multilabel =
  let _N = dat_N x in
  let _C = x.datav_numclass in
  let _F = dat_F x in

  let _ = if not quiet then printIter 1 maxi in

  let w_old  = make_ba _F 0. in
  let g      = make_ba _F 0. in
  let g_old  = make_ba _F 0. in

  let do_samp w =
    match sampOption with
        None -> None
      | Some ((prog,file),args,(burn,num,space)) -> 
          if prog = "" && file = "" then failwith "optimize_bfgs: sample estimation requires either program or file"
          else if prog <> "" && file <> "" then failwith "optimize_bfgs: sample estimation requires only one of program and file"
          else if prog <> ""
          then Some (Wsemlm.independence_sampling _F 
                       (Wsemlm.mk_external_gen_q       prog args (fun s -> get_vocab s) 
                                                       (1 + burn + num*space) x.datav_implicit)
                       w burn num space)
          else Some (Wsemlm.independence_sampling _F
                       (Wsemlm.mk_gen_q_from_file_list file      (fun s -> get_vocab s) 
                                                       (1 + burn + num*space) x.datav_implicit)
                       w burn num space) in


  let bestW  = ref None in
  let bestDE = ref 100. in
  let bestTE = ref 100. in


  let rec do_repeat repnum =
      let samp0 = do_samp w_old in

      let ()     = compute_gradient _C lambda w x y wtx g samp0 mean_model multilabel in

      let dwtx    = A.init _N (fun _ -> make _C 0.) in
      let gtg    = sqrt (dot g g) in
      let step   = make_ba _F 0. in
      let _      = iteri_ba (fun i v -> step.{i} <- v /. gtg) g in
      let myfstc = update_weights _C lambda x y wtx w w_old step dwtx g xDe wtxDev xTe wtxTst samp0 mean_model multilabel in

      let _  = if not quiet then printDW myfstc in
      let pp,_,_ = print_info quiet compute_ppx_by_wtx compute_error_by_wtx x.datav_weights wtx y (A.length wtxDev > 0) xDe.datav_weights wtxDev yDe (A.length wtxTst > 0) xTe.datav_weights wtxTst yTe in 
      let _  = if not quiet then (Printf.fprintf stderr "\n"; flush stderr) in
        
      let _ = for f = 0 to _F - 1 do g_old.{f} <- g.{f}; done in

      let memory = memory_create _M _F in

      let rec iterate lambda inum lastpp =
        if not quiet then printIter inum maxi;
        let samp = do_samp w in
        let () = compute_update_direction_bfgs _C lambda memory x y wtx w w_old g_old step g samp mean_model multilabel in
          for f = 0 to _F - 1 do 
            step.{f} <- bad_float_to_zero (0. -. step.{f}); 
          done;
          let dw = update_weights _C lambda x y wtx w w_old step dwtx g  xDe wtxDev xTe wtxTst samp mean_model multilabel in

            if not quiet then printDW dw;
            let pp,de,te = print_info quiet compute_ppx_by_wtx compute_error_by_wtx x.datav_weights wtx y (A.length wtxDev > 0) xDe.datav_weights wtxDev yDe (A.length wtxTst > 0) xTe.datav_weights wtxTst yTe in 
              if not quiet then (Printf.fprintf stderr "\n"; flush stderr);
      
(*              for f = 0 to _F - 1 do
                g_old.{f} <- g.{f};
              done; *)
              add_dense_dense g_old g 0. 1.;

              if not lastweight && de <= !bestDE then (
                  bestDE := de;
                  bestTE := te;
                  bestW  := Some (copy_ba w);
              );

              if inum < maxi && fabs(lastpp-.pp) > dpp && dw > 1e-20
              then iterate lambda (inum+1) pp 
              else 
                ((if lastweight then w else match !bestW with None -> w | Some wb -> ( if not quiet then Printf.fprintf stderr "final dev error=%g test error=%g\n" !bestDE !bestTE ; wb)),
                de,z_only samp,
                if returnStuff
                then Some wtx
                else None )in 

        if repnum >= repeat_count then iterate lambda 2 myfstc
        else (iterate lambda 2 myfstc ; if not quiet then (Printf.fprintf stderr "-------------------------\nit 1" ; flush stderr); do_repeat (repnum+1)) in

    do_repeat 1

    


let optimize lastweight repeat_count quiet bias maxi optimize_l lambda dpp _M x y xDe yDe xTe yTe sampOption mean_model init_model returnStuff multilabel =
  useBias := bias;
  let _N = dat_N x in
  let _C = x.datav_numclass in
  let _F = dat_F x in

  let w      = make_ba _F 0. in

  let wtx    = A.init _N (fun _ -> make _C 0.) in
  let wtxDev = A.init (dat_N xDe) (fun _ -> make _C 0.) in
  let wtxTst = A.init (dat_N xTe) (fun _ -> make _C 0.) in

(*  let wtx    = create_matrix _N _C 0. in
  let wtxDev = create_matrix (dat_N xDe) _C 0. in
  let wtxTst = create_matrix (dat_N xTe) _C 0. in *)

  let or_empty  x n   = if n >= A.length x then efa else A.get x (n) in
  let or_empty' x n c = 
    if n >= A.length x then efa 
    else if c >= length (A.get x n) then efa
    else (A.get x n).(c) in

  let _ = match init_model with None -> () | Some (MultModel wi) -> (
    for f = 0 to min (x.datav_numfeat-1) (length wi-1) do
      for c = 0 to min (length wi.(f)-1) (_C-1) do
        w.{f + c * x.datav_numfeat} <- wi.(f).(c);
      done;
    done;
    
    if x.datav_implicit then (
      for n = 0 to dat_N x   - 1 do 
        let xi = A.get x.datav_xi n in let xv = or_empty xDe.datav_xv n in
        let wn = A.get wtx n in
          for c = 0 to _C-1 do
            wn.(c) <- dotEx bias wi c xi xv;
          done;
      done;
      for n = 0 to dat_N xDe - 1 do 
        let xi = A.get xDe.datav_xi n in let xv = or_empty xDe.datav_xv n in
        let wn = A.get wtxDev n in
          for c = 0 to _C-1 do
            wn.(c) <- dotEx bias wi c xi xv;
          done;
      done;
      for n = 0 to dat_N xTe - 1 do 
        let xi = A.get xTe.datav_xi n in let xv = or_empty xTe.datav_xv n in
        let wn = A.get wtxTst n in
          for c = 0 to _C-1 do
            wn.(c) <- dotEx bias wi c xi xv;
        done;
      done;
    ) else (
      for n = 0 to dat_N x - 1 do
        let wtxn = A.get wtx n in
        let xi = A.get x.datav_xi_c n in 
        for c = 0 to (min _C (length xi)) - 1 do
          let xv = or_empty' x.datav_xv_c n c in
          wtxn.(c) <- dotEx bias wi c xi.(c) xv;
        done;
      done;
      for n = 0 to dat_N xDe - 1 do
        let wtxn = A.get wtxDev n in
        let xi = A.get xDe.datav_xi_c n in
        for c = 0 to (min _C (length xi)) - 1 do
          let xv = or_empty' xDe.datav_xv_c n c in
          wtxn.(c) <- dotEx bias wi c xi.(c) xv;
        done;
      done;
      for n = 0 to dat_N xTe - 1 do
        let wtxn = A.get wtxTst n in
        let xi = A.get xTe.datav_xi_c n in
        for c = 0 to (min _C (length xi)) - 1 do
          let xv = or_empty' xTe.datav_xv_c n c in
          wtxn.(c) <- dotEx bias wi c xi.(c) xv;
        done;
      done;
    )
  ) in

  let best_w = ref None in let best_w_de = ref 0. in
  let best_stuff = ref None in

  let lastweight' = lastweight || dat_N xDe == 0 in

  let rec opt_lambda lambda =
    let _ = Printf.fprintf stderr "optimizing with lambda = %g\n" lambda in
    let finalw,de,_,stuff = optimize_bfgs lastweight' repeat_count quiet maxi lambda dpp _M x y xDe yDe xTe yTe w wtx wtxDev wtxTst sampOption mean_model returnStuff multilabel in
      if is_some !best_w && lambda > 0. && de -. !best_w_de > -2e-6
      then from_some !best_w, de, None, !best_stuff
      else ( best_w := Some (copy_ba w) ; best_w_de := de ; best_stuff := stuff ;
             if de > 0. then opt_lambda (lambda *. 0.5) else w,de,None,stuff ) in
    
    if optimize_l 
    then opt_lambda lambda
    else 
      let finalw,de,samp,stuff = optimize_bfgs lastweight' repeat_count quiet maxi lambda dpp _M x y xDe yDe xTe yTe w wtx wtxDev wtxTst sampOption mean_model returnStuff multilabel in 
        finalw,de,samp,stuff


