open Array
open Util
open Data
open Fastdot
module A = Arry

let useBias = ref true

let b2f = function 0 -> -1. | 1 -> 1. | x -> failwith ("b2f: " ^ string_of_int x)

let computeBeta gold uold g =
(*
  let _J = A1.dim g in
  let rec computeNumerator j acc =
    if j >= _J then acc else
      computeNumerator (j+1) (acc +. g.{j} *. (g.{j} -. gold.{j})) in
  let rec computeDenominator j acc =
    if j >= _J then acc else
      computeDenominator (j+1) (acc +. uold.{j} *. (g.{j} -. gold.{j})) in
  let n = computeNumerator 0 0. in
  let d = computeDenominator 0 0. in
*)

  let n = norm_dense g -. mult_dense_dense g gold in
  let d = mult_dense_dense uold g -. mult_dense_dense uold gold in

  let fabs x = if x >= 0. then x else -.x in
  let sign x = if x >= 0. then 1. else -1. in
    if fabs d <= 1e-10 then 1. else n /. d

(* for efficiency, this actually just *overwrites* uold with the new u *)
let computeU gold uold g =
  let beta = computeBeta gold uold g in
(*  let _J   = A1.dim g in
    for j = 0 to _J -1 do
      uold.{j} <- g.{j} -. uold.{j} *. beta;
    done; *)
    add_dense_dense uold g (-.beta) 1.;
    ()

let updateWTX wtx x coeff u =
  let _N = dat_N x in
  let _J = A1.dim u in
    for n = 0 to _N - 1 do
(*      let _P = dat_P x n 0 in
        for p = 0 to _P - 1 do
          if dat_i x n 0 p < A1.dim u then
            wtx.{n} <- wtx.{n} +. coeff *. u.{dat_i x n 0 p} *. dat_v x n 0 p;
        done; *)
      if !useBias then
        wtx.{n} <- wtx.{n} +. coeff *. u.{0};

      if x.datav_bernoulli
      then wtx.{n} <- wtx.{n} +. coeff *. mult_dense_sparse     u (A.get x.datav_xi n)                      0
      else wtx.{n} <- wtx.{n} +. coeff *. mult_dense_sparse_val u (A.get x.datav_xi n) (A.get x.datav_xv n) 0;
    done;
    ()

(*
let dotExample v x n =
  let _P = dat_P x n 0 in
  let rec dot p acc =
    if p >= _P then acc 
    else if dat_i x n 0 p >= A1.dim v then dot (p+1) acc
    else dot (p+1) (acc +. dat_v x n 0 p *. v.{dat_i x n 0 p}) in
    dot 0 (if !useBias then v.{0} else 0.)
    *)

let dotExample v x n =
  (if x.datav_bernoulli
    then mult_dense_sparse     v (A.get x.datav_xi n) 0
    else mult_dense_sparse_val v (A.get x.datav_xi n) (A.get x.datav_xv n) 0)
  +. (if !useBias then v.{0} else 0.)

let updateWeights lambda w x gold uold g wtx mean_model =
  let _ = computeU gold uold g in  (* this replaces uold with u *)
  let _J = A1.dim gold in
  let _N = dat_N x in
  let square x = x *. x in
  let sigma x = 1. /. (1. +. exp(-.x)) in

(*
  let rec computeNumerator j acc = 
    if j >= _J then acc else
      computeNumerator (j+1) (acc +. g.{j} *. uold.{j}) in

  let rec computeDenominator1zeromean j acc = 
    if j >= _J then acc else
      computeDenominator1zeromean (j+1) (acc +. uold.{j} *. uold.{j}) in
*)
  let rec computeDenominator1modelmean j acc m = 
    if j >= _J then acc
    else if j >= A1.dim m
    then computeDenominator1modelmean (j+1) (acc +. (uold.{j} -. m.{j}) *. (uold.{j} -. m.{j})) m
    else computeDenominator1modelmean (j+1) (acc +. (uold.{j}         ) *. (uold.{j}         )) m in
  (*  
  let computeDenominator1 j acc =
    match mean_model with
        None -> computeDenominator1zeromean j acc
      | Some (BinModel m) -> computeDenominator1modelmean j acc m in
  *)    
  let rec computeDenominator2 n acc =
    if n >= _N then acc else
      computeDenominator2 (n+1) 
        (acc +. 
           (dat_wt x n) *.
           sigma wtx.{n} *.
           (1. -. sigma wtx.{n}) *.
           square (dotExample uold x n)) in
(*    
  let coeff = 
    (computeNumerator 0 0.) /. (lambda *. computeDenominator1 0 0. +. computeDenominator2 0 0.) in
*)

  let denom1 = 
    match mean_model with
        None              -> norm_dense uold
      | Some (BinModel m) -> computeDenominator1modelmean 0 0. m in
  let denom2 = computeDenominator2 0 0. in
  let coeff = (mult_dense_dense g uold) /. (lambda *. denom1 +. denom2) in
    
  let dw = ref 0. in
    (match mean_model with
        None ->
          add_dense_dense w uold 1. coeff;
          dw := !dw +. square coeff *. norm_dense uold
(*
          for j = 0 to _J - 1 do
            w.{j} <- w.{j} +. coeff *. uold.{j};
            dw := !dw +. square (coeff *. uold.{j});
          done
*)
      | Some (BinModel m) ->
          let dd = A1.dim m in
            for j = 0 to _J - 1 do
              let v = if j >= dd then uold.{j} else uold.{j} -. m.{j} in
                w.{j} <- w.{j} +. coeff *. v;
                dw := !dw +. square (coeff *. v);
            done
    );
    (coeff, !dw)

let computeGradient lambda w wtx (y : int A.arry) x mean_model g =
  let _J = A1.dim w in
  let _N = dat_N x in
  let sigma x = 1. /. (1. +. exp(-.x)) in
    A1.fill g 0.;

    for n = 0 to _N - 1 do
(*      let _P = dat_P  x n 0 in *)
      let wt = dat_wt x n in
      let yn = b2f (A.get y n) in
        if !useBias then
          g.{0} <- g.{0} +. (1. -. sigma(yn *. wtx.{n})) *. b2f (A.get y n) *. wt;

        if x.datav_bernoulli
        then add_dense_sparse     g (A.get x.datav_xi n)                      (wt *. yn *. (1. -. sigma(yn *. wtx.{n}))) 0
        else add_dense_sparse_val g (A.get x.datav_xi n) (A.get x.datav_xv n) (wt *. yn *. (1. -. sigma(yn *. wtx.{n}))) 0;

(*
        for p = 0 to _P - 1 do
          let j = dat_i x n 0 p in
            g.{j} <- g.{j} +. (1. -. sigma(yn *. wtx.{n})) *. yn *. dat_v x n 0 p *. wt;
        done;
*)

    done;

    if lambda > 0. then (
      match mean_model with
          None -> add_dense_dense g w 1. (-.lambda)
(*            for j = 0 to _J - 1 do
              g.{j} <- g.{j} -. lambda *. w.{j};
            done) *)
        | Some (BinModel m) -> (
            for j = 0 to _J - 1 do
              if j < A1.dim m then
                g.{j} <- g.{j} -. lambda *. (w.{j} -. m.{j});
            done)
    );
    ()

let computeGradientBinomial lambda svector w wtx y x mean_model g =
  let _J = A1.dim w in
  let _N = dat_N x in
  let sigma x = 1. /. (1. +. exp(-.x)) in
(*  let g  = make _J 0. in *)
    A1.fill g 0.;
    for n = 0 to _N - 1 do
      let _P = dat_P x n 0 in
      let wt = dat_wt x n in
        if !useBias then
          g.{0} <- g.{0} +. (1. -. sigma(wtx.{n})) *. wt;
          if x.datav_bernoulli
          then add_dense_sparse     g (A.get x.datav_xi n)                      (wt *. (1. -. sigma(wtx.{n}))) 0
          else add_dense_sparse_val g (A.get x.datav_xi n) (A.get x.datav_xv n) (wt *. (1. -. sigma(wtx.{n}))) 0;
(*        for p = 0 to _P - 1 do
          let j = dat_i x n 0 p in
            g.{j} <- g.{j} +. (1. -. sigma(wtx.{n})) *. dat_v x n 0 p *. wt;
        done; *)
    done;
    for j = 0 to _J - 1 do
      g.{j} <- g.{j} -. svector.{j} -. lambda *. w.{j};
    done;
    ()

let computeBinomialSufficientStats _J y x =
  let _N = dat_N x in
  let s  = make_ba _J 0. in
    for n = 0 to _N - 1 do
      let _P = dat_P x n 0 in
      let wt = dat_wt x n in
      let yn = A.get y n in
        if !useBias then
          s.{0} <- s.{0} +. yn *. wt;
        for p = 0 to _P - 1 do
          s.{dat_i x n 0 p} <- s.{dat_i x n 0 p} +. dat_v x n 0 p *. yn *. wt;
        done;
    done;
    s

let singleIteration lambda w wtx wtxDev wtxTst y x xDev xTst g gold uold mean_model =
  let () = computeGradient lambda w wtx y x mean_model g in
    (* after the call to updateWeights, uold will contain u and w *)
  let (coeff, dw) = updateWeights lambda w x gold uold g wtx mean_model in
  let _J = A1.dim g in
    updateWTX wtx    x    coeff uold;
    updateWTX wtxDev xDev coeff uold;
    updateWTX wtxTst xTst coeff uold;
    (* now, update gold to g *)
    for j = 0 to _J - 1 do
      gold.{j} <- g.{j};
    done;
    dw

let singleIterationBinomial lambda svector w wtx wtxDev wtxTst y x xDev xTst g gold uold mean_model =
  let () = computeGradientBinomial lambda svector w wtx y x mean_model g in
    (* after the call to updateWeights, uold will contain u and w will be updated *)
  let (coeff, dw) = updateWeights lambda w x gold uold g wtx mean_model in
  let _J = A1.dim g in
    updateWTX wtx    x    coeff uold;
    updateWTX wtxDev xDev coeff uold;
    updateWTX wtxTst xTst coeff uold;
    (* now, update gold to g *)
    for j = 0 to _J - 1 do
      gold.{j} <- g.{j};
    done;
    dw

let initializeVectors x init_model =
  let _N = dat_N x in
  let rec findMaxFeature n mx =
    if n >= _N then mx else
      let _P = dat_P x n 0 in
      let rec findMaxFeature2 p mx =
        if p >= _P then mx else
          findMaxFeature2 (p+1) (if dat_i x n 0 p > mx then dat_i x n 0 p else mx) in
        findMaxFeature (n+1) (findMaxFeature2 0 mx) in
  let _J = 1 + findMaxFeature 0 1 in
  let w0   = make_ba _J 0. in
  let wtx0 = make_ba _N 0. in
  let g0   = make_ba _J 0. in
  let u0   = make_ba _J 0. in

    (match init_model with None -> () | Some (BinModel wi) ->
      for f = 0 to min (A1.dim wi - 1) (A1.dim w0 - 1) do
        w0.{f} <- wi.{f};
      done;
      for n = 0 to _N - 1 do
        wtx0.{n} <- dotExample w0 x n;
      done;
    );

    (w0, wtx0, g0, u0)

let computeError (mass : bigarr) wtx y =
  let _N = A1.dim wtx in
  let rec ce n accS accM =
    if n >= _N then accS /. accM
    else if mass.{n} <= 0. then ce (n+1) accS accM
    else
      ce (n+1) (if b2f (A.get y n) *. wtx.{n} >= 0. then accS else accS +. mass.{n}) (accM +. mass.{n}) in
    ce 0 0. 0.

let computePRF y wtx =
  let _N = A1.dim wtx in
  let rec ce n nc nt nh =
    if n >= _N then nc,nt,nh else
      let tr = b2f (A.get y n) >= 0.5 in
      let hr = wtx.{n} >= 0.5 in
      ce (n+1) (if tr && hr then nc +. 1. else nc)
               (if tr then nt +. 1. else nt)
               (if hr then nh +. 1. else nh) in
  let nc,nt,nh = ce 0 0. 0. 0. in
  let p = if nh == 0. then 0. else nc /. nh in
  let r = if nt == 0. then 0. else nc /. nt in
  let f = if p == 0. || r == 0. then 0. else 2.*.p*.r/.(p +. r) in
    p, r, f

(*
let computePerplexity mass wtx y =
  let _N = length wtx in
  let rec cp n acc =
    if n >= _N then ancc
    else if mass.(n) <= 0. then cp (n+1) acc
    else 
      cp (n+1) (let p = 1. /. (1. +. exp wtx.{n}) in 
                  if b2f (A.get y n) < 0. then acc +. mass.(n) *. log p else acc +. mass.(n) *. log (1.-.p)) in
    cp 0 0. /. fold_left (+.) 0. mass
*)

let computePerplexity (mass : bigarr) (wtx : bigarr) y =
  let _N = A1.dim wtx in
  let rec cp n acc =
    if n >= _N then acc
    else if mass.{n} <= 0. then cp (n+1) acc
    else 
      cp (n+1) 
        (let lp = one /@ addLog one wtx.{n} in 
           if b2f (A.get y n) < 0. 
           then addLog acc (mass.{n} *. lp)
           else addLog acc (mass.{n} *. subLog one lp)) in
    cp 0 zero /. fold_left_ba (+.) 0. mass

let computeErrorBinomial (mass : bigarr) wtx y =
  let sigma x = 1. /. (1. +. exp(-.x)) in
  let _N = A1.dim wtx in
  let rec ce n acc =
    if n >= _N then acc else
      let yn = A.get y n in
        ce (n+1) (acc +. (sigma (-.wtx.{n}) -. yn) *. (sigma (-.wtx.{n}) -. yn)) in
    (ce 0 0.) /. float_of_int _N


let computePerplexityBinomial (mass : bigarr) wtx y =
  let _N = A1.dim wtx in
  let rec cp n acc =
    if n >= _N then acc else
      let yn = A.get y n in
        cp (n+1) (let p = 1. /. (1. +. exp wtx.{n}) in 
                    yn *. log p +. (1.-.yn) *. log (1.-.p)) in
    cp 0 0. /. float_of_int _N


let compute lastweight quiet bias maxi optimize_l lambda dpp x (y : int A.arry) xDev yDev xTst yTst mean_model init_model =
  useBias := bias;
  if lastweight then failwith "lastweight not yet implemented for binary problems";
  let (w, wtx, g, u) = initializeVectors x init_model in
  let wtxDev = make_ba (dat_N xDev) 0. in
  let wtxTst = make_ba (dat_N xTst) 0. in

  let _ =
    match init_model with None -> () | Some _ ->
      (for n = 0 to dat_N xDev - 1 do
         wtxDev.{n} <- dotExample w xDev n;
       done;
       for n = 0 to dat_N xTst - 1 do
         wtxTst.{n} <- dotExample w xTst n;
       done) in

  let _J = A1.dim w in
  let best_w = ref None in let best_w_de = ref 0. in
  let g_new = make_ba _J 0. in
  let rec iter lambda inum lastpp =
    if not quiet then printIter inum maxi;
    let dw = singleIteration lambda w wtx wtxDev wtxTst y x xDev xTst g_new g u mean_model in
    if not quiet then printDW dw;
      let pp,de,te = print_info quiet computePerplexity computeError x.datav_weights wtx y (A1.dim wtxDev > 0) xDev.datav_weights wtxDev yDev (A1.dim wtxTst > 0) xTst.datav_weights wtxTst yTst in
      if not quiet then (
        if A1.dim wtxTst > 0 then (
          let p,r,f = computePRF yTst wtxTst in
            Printf.fprintf stderr "\t(%g %g %g)" p r f; 
        );
        Printf.fprintf stderr "\n";
        flush stderr;
      );
      if inum < maxi && (inum==1 || (fabs(lastpp-.pp) > dpp))
      then iter lambda (inum+1) pp else de in

  let rec opt_lambda lambda =
    let _ = Printf.fprintf stderr "optimizing with lambda = %g\n" lambda in
    let de = iter lambda 1 0. in
      if is_some !best_w && lambda > 0. && de -. !best_w_de > -2e-6
      then from_some !best_w
      else ( best_w := Some (copy_ba w) ; best_w_de := de ; 
             if de > 0. then opt_lambda (lambda *. 0.5) else w ) in

    if optimize_l then opt_lambda lambda
    else (let _ = iter lambda 1 0. in w)
    

let computeBinomial lastweight quiet bias maxi optimize_l lambda dpp x (y : float A.arry) xDev yDev xTst yTst mean_model init_model =
  useBias := bias;
  if lastweight then failwith "lastweight not yet implemented for binary problems";
  let (w, wtx, g, u) = initializeVectors x init_model in
  let wtxTst = make_ba (dat_N xTst) 0. in
  let wtxDev = make_ba (dat_N xDev) 0. in

  let _ =
    match init_model with None -> () | Some _ ->
      (for n = 0 to dat_N xDev - 1 do
         wtxDev.{n} <- dotExample w xDev n;
       done;
       for n = 0 to dat_N xTst - 1 do
         wtxTst.{n} <- dotExample w xTst n;
       done) in

  let _J = A1.dim w in
  let svector = computeBinomialSufficientStats _J y x in
  let best_w = ref None in let best_w_de = ref 0. in
    
  let g_new = make_ba _J 0. in
  let rec iter lambda inum lastpp =
    if not quiet then printIter inum maxi;
    let dw = singleIterationBinomial lambda svector w wtx wtxDev wtxTst y x xDev xTst g_new g u mean_model in
    if not quiet then printDW dw;
    let pp,de,te = print_info quiet computePerplexityBinomial computeErrorBinomial x.datav_weights wtx y (A1.dim wtxDev > 0) xDev.datav_weights wtxDev yDev (A1.dim wtxTst > 0) xTst.datav_weights wtxTst yTst in
      Printf.fprintf stderr "\n"; flush stderr;
      if inum < maxi && (inum == 1 || (fabs(lastpp-.pp) > dpp))
      then iter lambda (inum+1) pp else de in

  let rec opt_lambda lambda =
    let _ = Printf.fprintf stderr "optimizing with lambda = %g\n" lambda in
    let de = iter lambda 1 0. in
      if is_some !best_w && lambda > 0. && de -. !best_w_de > -2e-6
      then from_some !best_w
      else ( best_w := Some (copy_ba w) ; best_w_de := de ; 
             if de > 0. then opt_lambda (lambda *. 0.5) else w ) in

  let w =
    if optimize_l then opt_lambda lambda
    else (let _ = iter lambda 1 0. in w) in
    
    for j = 0 to _J - 1 do
      w.{j} <- 0. -. w.{j};
    done;
    w
      
