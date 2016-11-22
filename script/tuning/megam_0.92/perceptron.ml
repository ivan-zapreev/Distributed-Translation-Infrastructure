open Array
open Util
open Data
open Fastdot
open Printf
module I = Int32

let useBias = ref true


let initializeVectors x =
  let _N = dat_N x in
  let rec findMaxFeature n mx =
    if n >= _N then mx else
      let _P = dat_P x n 0 in
      let rec findMaxFeature2 p mx =
        if p >= _P then mx else
          findMaxFeature2 (p+1) (if dat_i x n 0 p > mx then dat_i x n 0 p else mx) in
        findMaxFeature (n+1) (findMaxFeature2 0 mx) in
  let _J = 1 + findMaxFeature 0 1 in
    (make_ba _J 0., make_ba _J 0.)



let permute order =
  let _N = A.length order in
    for n = 0 to _N - 1 do
      let t = Random.int (_N - n) in
      let m = A.get order (n) in
        A.set order (n) (A.get order (t));
        A.set order (t)  m;
    done;
    ()

let compute_error_by_wtx (mass : bigarr) (wtx : float array A.arry) (y : multilabel A.arry) =
  let numerr  = ref 0. in
  let totmass = ref 0. in
  let _N = A.length wtx in
    if _N == 0 then 0. else (
      let _C = length (A.get wtx 0) in
        if _C > 1 then (
          for n = 0 to _N - 1 do
            let wtxn = A.get wtx n in
            let ML a = A.get y n in
              for c = 0 to (min (length a) _C) - 1 do
                if a.(c) < infinity then (
                  if (wtxn.(c) > 0.) != (a.(c) > 0.5) then
                    numerr := !numerr +. if A1.dim mass == 0 then 1. else mass.{n};
                  totmass := !totmass +. if A1.dim mass == 0 then 1. else mass.{n};
                );
              done;
          done;
        );
        if !totmass == 0. then 0. else (!numerr /. !totmass);
    )

let optimize lastweight quiet bias maxi x y xDev yDev xTst yTst mu =
  useBias := bias;
  let (w, a) = initializeVectors x in
  let vc = ref 1. in

  let wtx    = make_ba (dat_N x   ) 0. in
  let wtxDev = make_ba (dat_N xDev) 0. in
  let wtxTst = make_ba (dat_N xTst) 0. in

  let _J = A1.dim w in
  let _N = dat_N x in

  let best_w = make_ba _J 0. in let best_de = ref 1. in let best_te = ref 1. in
  let order  = A.init _N (fun n -> n) in
  let or_empty x n = if n >= A.length x then efa else A.get x (n) in

  let this_dotExample a x n =
    let _P = dat_P x n 0 in
    let rec dot p acc =
      if p >= _P then acc
      else if dat_i x n 0 p >= A1.dim a then dot (p+1) acc
      else dot (p+1) (acc +. dat_v x n 0 p *. a.{dat_i x n 0 p}) in
      dot 0 (if !useBias then a.{0} else 0.) in

  let mu_dot = match mu with 
      None   -> None 
    | Some (BinModel a) -> Some (init _N (fun n -> this_dotExample a x n)) in

  let compute_wtx wtx x = 
    let _N = dat_N x in
      for n = 0 to _N - 1 do
        wtx.{n} <- Cg.dotExample w x n -. Cg.dotExample a x n /. !vc +.
                   match mu with None -> 0. | Some (BinModel a) -> this_dotExample a x n;
      done in

  let rec iter inum =
    if not quiet then printIter inum maxi;
    permute order;
    let dn = ref 0 in
    for nn = 0 to _N - 1 do
      let n  = A.get order (nn) in
      let yv = if (A.get y n) == 0 then -1. else 1. in
      let wt = dat_wt x n in
      let md = match mu_dot with None -> 0. | Some a -> a.(n) in
        if yv *. (md +. Cg.dotExample w x n) <= 0. then (
          (* do an update *)
            incr dn;
            let _P = dat_P  x n 0 in
              if !useBias then (
                w.{0} <- w.{0} +. yv *. wt;
                a.{0} <- a.{0} +. yv *. wt *. !vc;
              );
            let xi = A.get x.datav_xi n in
            let xv = or_empty x.datav_xv n in
              if x.datav_bernoulli
              then ( add_dense_sparse     w xi    (yv *. wt       ) 0
                   ; add_dense_sparse     a xi    (yv *. wt *. !vc) 0 )
              else ( add_dense_sparse_val w xi xv (yv *. wt       ) 0
                   ; add_dense_sparse_val a xi xv (yv *. wt *. !vc) 0 )
(*              for p = 0 to _P - 1 do
                let j = dat_i x n 0 p in
                let v = dat_v x n 0 p in
                  w.{j} <- w.{j} +. yv *. wt *. v;
                  a.{j} <- a.{j} +. yv *. wt *. v *. !vc;
              done; *)
        );
        vc := !vc +. 1.;
    done;
    compute_wtx wtx    x;
    compute_wtx wtxDev xDev;
    compute_wtx wtxTst xTst;
    if not quiet then printDW (float_of_int !dn);
    let pp,de,te = print_info quiet Cg.computePerplexity Cg.computeError x.datav_weights wtx y (A1.dim wtxDev > 0) xDev.datav_weights wtxDev yDev (A1.dim wtxTst > 0) xTst.datav_weights wtxTst yTst in
    if lastweight || de <= !best_de then (
      for j = 0 to _J - 1 do
        best_w.{j} <- w.{j} -. a.{j} /. !vc +. 
          match mu with None -> 0. | Some (BinModel a) -> if j < A1.dim a then a.{j} else 0.;
      done;
      best_de := de;
      best_te := te;
    );
    if not quiet then (
      let p,r,f = Cg.computePRF yTst wtxTst in
        Printf.fprintf stderr "\t(%g %g %g)\n" p r f; flush stderr;
    );
    if inum < maxi && !dn > 0 then iter (inum+1) else (
        if not quiet then Printf.fprintf stderr "final dev error=%g test error=%g\n" !best_de !best_te;
        best_w
    ) in

    iter 1


let optimize_bitvec online lastweight quiet bias maxi x y xDev yDev xTst yTst mu =
  useBias := bias;
  let _N = dat_N x in
  let _C = max 2 x.datav_numclass in
  let _F = dat_F x in

  let numF = x.datav_numfeat in

  let w = make_ba _F 0. in
  let a = make_ba _F 0. in
  let vc = make _C 1. in

  let wtx    = A.init _N (fun _ -> make _C 0.) in
  let wtxDev = A.init (dat_N xDev) (fun _ -> make _C 0.) in
  let wtxTst = A.init (dat_N xTst) (fun _ -> make _C 0.) in

  let best_w = make_ba _F 0. in
  let best_de = ref 1. in
  let best_te = ref 1. in

  let order  = A.init _N (fun n -> n) in

  let or_empty x n = if n >= A.length x then efa else A.get x (n) in

  let bernoulli = x.datav_bernoulli in

  let dot_implicit w c xi xv =
    let offset = c*numF in
      (if !useBias then w.{offset} else 0.) +.
      (if bernoulli
       then mult_dense_sparse w xi offset
       else mult_dense_sparse_val w xi xv offset) in

  let this_dot_implicit w c xi xv =
    let s = ref 0. in
      if !useBias then s := !s +. w.(0).(c);
      for i = 0 to A1.dim xi - 1 do
        let j  = I.to_int xi.{i} in
        let vl = if bernoulli then 1. else xv.{i} in
          if j < length w then
            if c < length w.(j) then
              s := !s +. w.(j).(c) *. vl;
      done;
      !s in

  let mu_dot = match mu with 
      None   -> None 
    | Some (MultModel a) -> Some (init _N (fun n -> init _C (fun c -> this_dot_implicit a c (A.get x.datav_xi n) (or_empty x.datav_xv n)))) in

  let compute_wtx_implicit wtx x =
    let _N = dat_N x in
      for n = 0 to _N - 1 do
        let wtxn = A.get wtx n in
        let xi = A.get x.datav_xi n in let xv = or_empty x.datav_xv n in
          for c = 0 to _C - 1 do
            wtxn.(c) <-
              dot_implicit w c xi xv -. 
              dot_implicit a c xi xv /. vc.(c) +.
              (match mu with None -> 0. | Some (MultModel a) ->
                 this_dot_implicit a c xi xv);
          done;
      done;
      () in

  let zeroc = make _C 0. in

  let rec iter_implicit inum =
    let dn = ref 0 in
      if not quiet  then printIter inum maxi;
      if not online then permute order;
      for nn = 0 to _N - 1 do
        let n  = A.get order (nn) in
        let wt = dat_wt x n in
          if online then printf "?";
          if wt > 0. then begin
            let muv = match mu_dot with None -> zeroc | Some a -> a.(n) in
            let xi = A.get x.datav_xi n in let xv = or_empty x.datav_xv n in
            let ML arr = A.get y n in
              for c = 0 to _C - 1 do
                let wc = if c < length arr then arr.(c) else infinity in
                  if online || (wc < infinity) then (
                    let v = muv.(c) +. dot_implicit w c xi xv in
                    let wt = if wc > 0.5 then wt else (0.-.wt) in
                      (* if online then printf "\t%g" v; *)
                      if online then printf "\t%g" (muv.(c) +. dot_implicit w c xi xv -. dot_implicit a c xi xv /. max 1. vc.(c));
                      if ((v > 0.) != (wc > 0.5)) && (wc < infinity) then (  (* update *)
                        incr dn;
                        if !useBias then (
                          w.{c*numF} <- w.{c*numF} +. wt;
                          a.{c*numF} <- a.{c*numF} +. wt *. vc.(c);
                        );
                        if bernoulli
                        then ( add_dense_sparse     w xi    (  wt       ) (c*numF) 
                             ; add_dense_sparse     a xi    (  wt *. vc.(c)) (c*numF) )
                        else ( add_dense_sparse_val w xi xv (  wt       ) (c*numF) 
                             ; add_dense_sparse_val a xi xv (  wt *. vc.(c)) (c*numF) );
                      );
                      vc.(c) <- vc.(c) +. 1.;
                  )
              done;
          end;
          if online then ( printf "\n"; flush stdout );
      done;
      compute_wtx_implicit wtx    x;
      compute_wtx_implicit wtxDev xDev;
      compute_wtx_implicit wtxTst xTst;
      let pp,de,te = print_info quiet Bfgs.compute_ppx_by_wtx compute_error_by_wtx x.datav_weights wtx y (A.length wtxDev > 0) xDev.datav_weights wtxDev yDev (A.length wtxTst > 0) xTst.datav_weights wtxTst yTst in 
      if not quiet then (Printf.fprintf stderr "\n"; flush stderr);
      if lastweight || de <= !best_de then (
        for c = 0 to _C - 1 do
          for f = 0 to numF - 1 do
            best_w.{c*numF+f} <- w.{c*numF+f} -. a.{c*numF+f} /. vc.(c);
          done;
        done;
        best_de := de;
        best_te := te;
      );
      if inum < maxi && !dn > 0 then iter_implicit (inum+1) else (
        if not quiet then Printf.fprintf stderr "final dev error=%g test error=%g\n" !best_de !best_te;
(*        let norm = norm_dense best_w in
          mult_dense best_w (1./.norm); *)
          best_w
      ) in

    iter_implicit 1

let optimize_mc lastweight quiet bias maxi x y xDev yDev xTst yTst mu =
  useBias := bias;
  let _N = dat_N x in
  let _C = max 2 x.datav_numclass in
  let _F = dat_F x in

  let numF = x.datav_numfeat in

  let w = make_ba _F 0. in
  let a = make_ba _F 0. in
  let vc = ref 1. in

  let wtx    = A.init _N (fun _ -> make _C 0.) in
  let wtxDev = A.init (dat_N xDev) (fun _ -> make _C 0.) in
  let wtxTst = A.init (dat_N xTst) (fun _ -> make _C 0.) in

  let best_w = make_ba _F 0. in
  let best_de = ref 1. in
  let best_te = ref 1. in

  let order  = A.init _N (fun n -> n) in

  let or_empty x n = if n >= A.length x then efa else A.get x (n) in

  let bernoulli = x.datav_bernoulli in

  let dot_implicit w c xi xv =
    let offset = c*numF in
      (if !useBias then w.{offset} else 0.) +.
      (if bernoulli
       then mult_dense_sparse w xi offset
       else mult_dense_sparse_val w xi xv offset) in

  let this_dot_implicit w c xi xv =
    let s = ref 0. in
      if !useBias then s := !s +. w.(0).(c);
      for i = 0 to A1.dim xi - 1 do
        let j  = I.to_int xi.{i} in
        let vl = if bernoulli then 1. else xv.{i} in
          if j < length w then
            if c < length w.(j) then
              s := !s +. w.(j).(c) *. vl;
      done;
      !s in

  let mu_dot = match mu with 
      None   -> None 
    | Some (MultModel a) -> Some (init _N (fun n -> init _C (fun c -> this_dot_implicit a c (A.get x.datav_xi n) (or_empty x.datav_xv n)))) in

  let compute_wtx_implicit wtx x =
    let _N = dat_N x in
      for n = 0 to _N - 1 do
        let wtxn = A.get wtx n in
        let xi = A.get x.datav_xi n in let xv = or_empty x.datav_xv n in
          for c = 0 to _C - 1 do
            wtxn.(c) <-
              dot_implicit w c xi xv -. 
              dot_implicit a c xi xv /. !vc +.
              (match mu with None -> 0. | Some (MultModel a) ->
                 this_dot_implicit a c xi xv);
          done;
      done;
      () in

  let zeroc = make _C 0. in

  let rec iter_implicit inum =
    let dn = ref 0 in
      if not quiet then printIter inum maxi;
      permute order;
      for nn = 0 to _N - 1 do
        let n  = A.get order (nn) in
        let max_a = match (A.get y n) with L _ -> 1. | ML a -> maximum_not_inf a in
        let wt = dat_wt x n in
          if wt > 0. then begin
            let p  = ref (-1) in
            let pv = ref 0. in
            let muv = match mu_dot with None -> zeroc | Some a -> a.(n) in
            let xi = A.get x.datav_xi n in let xv = or_empty x.datav_xv n in
              for c = 0 to _C - 1 do
                let wc = wt *. (match (A.get y n) with 
                                  L  d -> if c == d then 1. else 0.
                                | ML a -> if c < length a && a.(c) < infinity then max_a -. a.(c) else infinity
                               ) in
                let v = muv.(c) +. dot_implicit w c xi xv -. wc in
                  if (v < infinity) && ((!p < 0) || (v >= !pv)) then ( pv := v; p := c );
              done;
            let p  = !p in
              (match (A.get y n) with
                  L c -> 
                    let wt = 1. in
                      if p != c then (
                        incr dn;
                        if !useBias then (
                          w.{c*numF} <- w.{c*numF} +. wt;
                          a.{c*numF} <- a.{c*numF} +. wt *. !vc;
                          w.{p*numF} <- w.{p*numF} -. wt;
                          a.{p*numF} <- a.{p*numF} -. wt *. !vc;
                        );
                        if bernoulli
                        then ( add_dense_sparse     w xi    (  wt       ) (c*numF) 
                             ; add_dense_sparse     w xi    (-.wt       ) (p*numF)
                             ; add_dense_sparse     a xi    (  wt *. !vc) (c*numF)
                             ; add_dense_sparse     a xi    (-.wt *. !vc) (p*numF) )
                        else ( add_dense_sparse_val w xi xv (  wt       ) (c*numF) 
                             ; add_dense_sparse_val w xi xv (-.wt       ) (p*numF)
                             ; add_dense_sparse_val a xi xv (  wt *. !vc) (c*numF)
                             ; add_dense_sparse_val a xi xv (-.wt *. !vc) (p*numF) );
                      )
                | ML arr ->
                    let updated = ref false in
                    let arr_p = if p < length arr then arr.(p) else maximum_not_inf arr in
                      for c = 0 to length arr - 1 do
                        if p != c && arr.(c) < arr_p then (
                          let wt = 1. in  (* maybe play with this *)
                            updated := true;
                            if !useBias then (
                              w.{c*numF} <- w.{c*numF} +. wt;
                              a.{c*numF} <- a.{c*numF} +. wt *. !vc;
                              w.{p*numF} <- w.{p*numF} -. wt;
                              a.{p*numF} <- a.{p*numF} -. wt *. !vc;
                            );
                            if bernoulli
                            then ( add_dense_sparse     w xi    (  wt       ) (c*numF) 
                                 ; add_dense_sparse     w xi    (-.wt       ) (p*numF)
                                 ; add_dense_sparse     a xi    (  wt *. !vc) (c*numF)
                                 ; add_dense_sparse     a xi    (-.wt *. !vc) (p*numF) )
                            else ( add_dense_sparse_val w xi xv (  wt       ) (c*numF) 
                                 ; add_dense_sparse_val w xi xv (-.wt       ) (p*numF)
                                 ; add_dense_sparse_val a xi xv (  wt *. !vc) (c*numF)
                                 ; add_dense_sparse_val a xi xv (-.wt *. !vc) (p*numF) );
                        );
                      done;
                      if !updated then incr dn;
              );
                    
              vc := !vc +. 1.;
            end;
      done;
      compute_wtx_implicit wtx    x;
      compute_wtx_implicit wtxDev xDev;
      compute_wtx_implicit wtxTst xTst;
      let pp,de,te = print_info quiet Bfgs.compute_ppx_by_wtx Bfgs.compute_error_by_wtx x.datav_weights wtx y (A.length wtxDev > 0) xDev.datav_weights wtxDev yDev (A.length wtxTst > 0) xTst.datav_weights wtxTst yTst in 
      if not quiet then (Printf.fprintf stderr "\n"; flush stderr);
      if lastweight || de <= !best_de then (
        for j = 0 to _F - 1 do
          best_w.{j} <- w.{j} -. a.{j} /. !vc;
        done;
        best_de := de;
        best_te := te;
      );
      if inum < maxi && !dn > 0 then iter_implicit (inum+1) else (
        if not quiet then Printf.fprintf stderr "final dev error=%g test error=%g\n" !best_de !best_te;
        best_w
      ) in

    iter_implicit 1
