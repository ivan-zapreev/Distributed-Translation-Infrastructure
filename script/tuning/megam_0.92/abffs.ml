open Array
open Util
open Data
module A = Arry
(* implements my approx bayes factor feature selection *)

module H = Hashtbl

let max_prop_count = 10000

let bin_search (arr : int array) (target : int) =
  let lin_search (lo : int) (hi : int) =
    let rec lin_search' i = 
      if i >= hi then false
      else if arr.(i) == target then true
      else lin_search' (i+1) in
      lin_search' lo in

  let lo0 = 0 in let hi0 = length arr in let mi0 = (lo0 + hi0) / 2 in
  let rec bin_search' lo mi hi =
    if hi - lo < 5 then lin_search lo hi else
      if arr.(mi) == target then true
      else if arr.(mi) < target then bin_search' mi ((mi+hi)/2) hi
      else bin_search' lo ((lo+mi)/2) mi in
    bin_search' lo0 mi0 hi0

let optimize_single_param baseLik lambda x xtrans y wtx wtx_logSum g g_y mu0 = (* returns mu, a_g, data probability *)
  let _N = dat_N x in
  let _C = x.datav_numclass in
  let activeH = H.create 5 in
  let baseLikVal = ref one in

(*
    for n = 0 to _N - 1 do
      if bin_search (A.get x.datav_xi n) g 
      then H.replace activeH n ()
      else baseLikVal := !baseLikVal *@ baseLik.(n);
    done;
*)
(*
    H.iter (fun n _ -> Printf.fprintf stderr "%d " n) activeH   ; Printf.fprintf stderr "\n"; flush stderr;
    iter   (fun n   -> Printf.fprintf stderr "%d " n) xtrans.(g); Printf.fprintf stderr "\n"; flush stderr;
*)

    for n = 0 to _N - 1 do
      baseLikVal := !baseLikVal *@ baseLik.(n);
    done;
    iter (fun n -> baseLikVal := !baseLikVal /@ baseLik.(n)) xtrans.(g);

  let derivV  = ref 0. in
  let deriv2V = ref 0. in
  let probV   = ref 0. in

  let deriv_both mu =
    derivV  := 0. -. mu *. lambda;
    probV   := 0. -. mu *. mu *. lambda /. 2.;
    let a = ref 0. in let b = ref 0. in
    let s = ref zero in
      iter 
        (fun n ->
           let wtxn = A.get wtx n in
           let v = wtxn.(g_y) *@ mu in

             s     := ((A.get wtx_logSum n) +@ (wtxn.(g_y) *@ mu)) -@ wtxn.(g_y);
             probV := !probV +. wtxn.(lab (A.get y n)) *@ (if lab (A.get y n) == g_y then mu else one);
             
(*             s := zero;
             for y' = 0 to _C - 1 do
               let v = wtxn.(y') *@ if y' == g_y then mu else one in
                 s := !s +@ v;
                 if lab (A.get y n) == y' then probV := !probV +. v;
             done;
*)
             derivV := !derivV +. (if g_y == lab (A.get y n) then 1. else 0.) -. exp (v -. !s);
             probV  := !probV  -. !s;
             a := !a +. exp (v /@ !s) *. exp (v /@ !s);
             b := !b +. exp (v /@ !s);
        ) xtrans.(g);
     deriv2V := !a -. !b -. lambda in

  let rec iter mu inum =
    let lastP = !probV in
    let mu'   = mu -. !derivV /. !deriv2V in
      deriv_both mu';
      if inum > 10 || lastP +. 1e-4 > !probV || abs_float (!probV -. lastP) < 1e-6
      then mu', 0. -. !deriv2V, !probV *@ !baseLikVal
      else iter mu' (inum+1) in

    deriv_both mu0;
    iter mu0 1

let compute_likelihood lambda xTr xTrTrans _C priorV wtx wtx_logSum y errn current removeit =
  let _N = A.length wtx in
  let post = ref priorV in
  let sum  = ref zero in
  let toremove = H.create 5 in

    (match current with None -> () | Some (_,_,mu) ->
       post := !post -. (lambda /. 2.) *. mu *. mu);

    (match current with
        None ->
          H.iter 
            (fun n () ->
              post := !post +. (A.get wtx n).(lab (A.get y n)) -. (A.get wtx_logSum n); (* addLogArray wtxn; *)
            ) errn
      | Some (y_v,g,mu) ->
          H.iter 
            (fun n () ->
              let wtxn = A.get wtx n in
              let dwtx = if H.mem xTrTrans.(g) n then mu else 0. in
              let wtx_logSum2 = (A.get wtx_logSum n +@ (wtxn.(y_v) +. dwtx)) -@ wtxn.(y_v) in

                wtxn.(y_v) <- wtxn.(y_v) +. dwtx;
                post := !post +. wtxn.(lab (A.get y n)) -. wtx_logSum2; (* addLogArray wtxn; *)

                if removeit && lab (A.get y n) == y_v then (
                  let maxV = maximum wtxn in
                    A.set wtx_logSum (n) wtx_logSum2;
                    if abs_float (maxV -. wtxn.(lab (A.get y n))) < 1e-10 then
                      H.replace toremove n ();
                ) else (
                  wtxn.(y_v) <- wtxn.(y_v) -. dwtx;
                );

                ()
            ) errn
    );

    if removeit then (
      H.iter (fun n _ -> H.remove errn n) toremove;
      Printf.fprintf stderr "removing %d errors\n" (h_size toremove);
    );

    !post

let applyActive (active : (int32,int32) H.t) x =
  let _N = dat_N x in
  let _C = x.datav_numclass in
  let _F = dat_F x in

  let xi' = A.init _N (fun _ -> eia) in
  let xv' = if x.datav_bernoulli then A.empty efa else A.init _N (fun _ -> efa) in

    for n = 0 to _N - 1 do
      let (h : (int32,float) H.t) = H.create 5 in
        for f = 0 to A1.dim (A.get x.datav_xi n) - 1 do
          if H.mem active (A.get x.datav_xi n).{f} then
            H.replace h (H.find active (A.get x.datav_xi n).{f}) 
              (if x.datav_bernoulli then 1. else (A.get x.datav_xv n).{f});
        done;
      let arr = make_ia (h_size h) 0 in
      let arrv= if x.datav_bernoulli then efa else make_ba (h_size h) 0. in
      let p   = ref 0 in
        H.iter (fun f v ->
                  arr.{!p} <- f;
                  if not x.datav_bernoulli then arrv.{!p} <- v;
                  incr p) h;
        A.set xi' (n) arr;
        if not x.datav_bernoulli then A.set xv' (n) arrv;
    done;
    { datav_bernoulli = x.datav_bernoulli ;
      datav_implicit  = x.datav_implicit  ;
      datav_sampled   = x.datav_sampled   ;
      datav_numclass  = x.datav_numclass  ;
      datav_numfeat   = x.datav_numfeat   ; (* h_size active     ; *)
      datav_task      = x.datav_task  ;
      datav_weights   = x.datav_weights   ;
      datav_xi        = xi' ;
      datav_xi_c      = A.empty [||] ;
      datav_xv        = xv' ;
      datav_xv_c      = A.empty [||] ;
    }
  
let mem2  h (a,b) = try H.mem (H.find h a) b with Not_found -> false
let find2 h  a b  = H.find (H.find h a) b
let iter2 f h = H.iter (fun a h' -> H.iter (fun b g -> f (a,b) g) h') h

let abffs_selection quiet bias maxi optimize_l lambda dpp _M (xTr:datav) yTr xDe yDe xTe yTe sampOption mean_model init_model0 _S =
  let _N = dat_N xTr in
  let _C = xTr.datav_numclass in
  let _F = dat_F xTr in
  let nf = xTr.datav_numfeat in

  for n = 0 to dat_N xTr-1 do
   fast_sort_ia (A.get xTr.datav_xi n); 
  done;

  let xTrTrans0 = transpose xTr.datav_xi in
  let xTrTrans  =
    map (fun h ->
      let a = make (h_size h) 0 in
      let i = ref 0 in
        H.iter (fun n _ -> a.(!i) <- n; incr i) h;
        a) xTrTrans0 in

  let mu = make_ba _F 0. in

  let feature_counts = H.create 5 in
    for n = 0 to _N - 1 do
      for i = 0 to A1.dim (A.get xTr.datav_xi n) - 1 do
        H.replace feature_counts (A.get xTr.datav_xi n).{i}
          (try H.find feature_counts (A.get xTr.datav_xi n).{i} + 1 with Not_found -> 1);
      done;
    done;

  let init_model = ref None in

  let run xTr' xDe' xTe' = 
    Bfgs.optimize false 1 quiet bias maxi optimize_l lambda dpp _M 
      xTr' yTr xDe' yDe xTe' yTe 
      sampOption mean_model !init_model true false in

  let active = H.create 5 in
  let activeCount = ref 1 in
    H.replace active (Int32.zero) Int32.zero;

    (match init_model0 with None -> () | Some (MultModel im) -> 
       for f = 0 to length im - 1 do
         let nonzero = ref false in
           for c = 0 to _C - 1 do
             if abs_float im.(f).(c) > 1e-10 then nonzero := true;
           done;
           if !nonzero then (
             H.replace active (Int32.of_int f) (Int32.of_int !activeCount);
             incr activeCount;
           );
       done;
       let arr = create_matrix (h_size active) _C 0. in
       for c = 0 to _C - 1 do
         for f = 0 to length im - 1 do
           try 
             arr.(Int32.to_int (H.find active (Int32.of_int f))).(c) <- im.(f).(c);
           with Not_found -> ();
         done;
       done;
       init_model := Some (MultModel arr);
    );

    let addToCurrent c y g mu =
      try
        let ct = H.find c y in
          H.replace ct g mu
      with Not_found -> (
        let ct = H.create 2 in
          H.replace ct g mu;
          H.replace c  y ct) in

    let remFromCurrent c y g =
      try
        let ct = H.find c y in
          H.remove ct g
      with Not_found -> () in
  
  let w_best = ref (make_ba 0 0.) in
  let v_best = ref 1. in

  let rec run_iter inum =
    let xTr' = applyActive active xTr in
    let xDe' = applyActive active xDe in
    let xTe' = applyActive active xTe in
    let old_active_size = h_size active in

    let (w0,de,_,Some w0tx) = run xTr' xDe' xTe'  in
    let w0tx_logSum = A.map addLogArray w0tx in
    let priorV = ref (0. -. (lambda /. 2.) *. (Bfgs.dot w0 w0)) in
  
    let proposed  = H.create 5 in 
    let proposedN = ref 0 in
    let erroneous = H.create 5 in
  
    let baseLik   = make _N one in

      for n = 0 to _N - 1 do
        let y' = ref 0 in
        let s  = ref zero in
        let w0txn = A.get w0tx n in
          for y = 0 to _C - 1 do
            if w0txn.(y) > w0txn.(!y') then y' := y;
            s := !s +@ w0txn.(y);
          done;
          if !y' != lab (A.get yTr n) then (
            H.replace erroneous n ();
            baseLik.(n) <- w0txn.(lab (A.get yTr n)) -. !s;
            for f = 0 to A1.dim (A.get xTr.datav_xi n) - 1 do
              if not (H.mem active (A.get xTr.datav_xi n).{f}) then (
                if not (mem2 proposed (lab (A.get yTr n), (A.get xTr.datav_xi n).{f})) then (
                  addToCurrent proposed (lab (A.get yTr n)) (A.get xTr.datav_xi n).{f} !proposedN;
                  incr proposedN;
                );
                if not (mem2 proposed (!y', (A.get xTr.datav_xi n).{f})) then (
                  addToCurrent proposed (!y') (A.get xTr.datav_xi n).{f} !proposedN;
                  incr proposedN;
                );
              );
            done;
          );
      done;

      if !proposedN > max_prop_count then (    (* we'll not do well with > 10k features *)
        let maxCount = ref 0 in
          iter2 (fun (_,g) _ -> maxCount := max !maxCount (H.find feature_counts g)) proposed;
        let cntCnt = make (!maxCount+1) 0 in
          iter2 (fun (_,g) _ ->
                   for i = 0 to H.find feature_counts g do
                     cntCnt.(i) <- cntCnt.(i) + 1;
                   done) proposed;
        let rec find_max_prop_count i = 
          if i > !maxCount then 0
          else if cntCnt.(i) < max_prop_count then i
          else find_max_prop_count (i+1) in
        let min_count = find_max_prop_count 0 in
        let tokeep = H.create 5 in
          iter2 (fun (y,g) _ -> 
                   if H.find feature_counts g > min_count then
                     addToCurrent tokeep y g ()) proposed;
          H.clear proposed;
          proposedN := 0;
          iter2 (fun (y,g) _ -> 
                   addToCurrent proposed y g !proposedN;
                   incr proposedN) tokeep;
          Printf.fprintf stderr "Whittled down to %d features...\n" !proposedN;
      );

    let lik0 = compute_likelihood lambda xTr xTrTrans0 _C !priorV w0tx w0tx_logSum yTr erroneous None false in
  
    let szProposed = !proposedN in
    let a    = make szProposed 0. in
    let beta = make szProposed 0. in
    let pr   = make szProposed 0. in
  
      Printf.fprintf stderr "szProposed=%d" szProposed;
  
      iter2 (fun (y,g) gn ->
               let g = Int32.to_int g in
               let muv, av, pv = optimize_single_param baseLik lambda xTr xTrTrans yTr w0tx w0tx_logSum g y mu.{g+y*nf} in
                 mu.{g+y*nf} <- muv;
                 a.(gn)      <- av;
                 pr.(gn)     <- pv *@ !priorV;
                 if gn mod 100 == 0 then ( Printf.fprintf stderr "."; flush stderr );
            ) proposed;
  
      Printf.fprintf stderr "\n";
  
      Printf.fprintf stderr "beta0 = %g\n" lik0;
      flush stderr;
  
    let fin = ref false in
    let numAdded = ref 0 in
    let current = H.create 2 in
    let beta0 = ref lik0 in

    let firstIter = ref true in

      while not !fin do
        let bestBeta = ref None in
          iter2
            (fun (y,g) gn ->
               if not (H.mem active g) then (
                   let g = Int32.to_int g in
(*                 let ll = compute_likelihood lambda xTr _C !priorV w0tx yTr erroneous (Some (y,g,mu.{g+y*nf})) false in *)
                 let ll = 
                   if !firstIter
                   then pr.(gn)
                   else compute_likelihood lambda xTr xTrTrans0 _C !priorV w0tx w0tx_logSum yTr erroneous (Some (y,g,mu.{g+y*nf})) false in
                 beta.(gn) <- 0.5 *. log (2.*.3.14159265) +. 0.5 *. log a.(gn) +. ll;
                 if is_none !bestBeta || fst (from_some !bestBeta) < beta.(gn) then
                   bestBeta := Some (beta.(gn), (y,g));
               );
            ) proposed;
          match !bestBeta with
              None -> ( Printf.fprintf stderr "--> bestBeta = None\n"; fin := true )
            | Some (betaG, (y,g)) ->
                if betaG > !beta0 then (
                  let muv = mu.{g+y*nf} in
                  Printf.fprintf stderr "Adding (y=%d,g=%s), betaG=%g, " y (get_vocab_str g) betaG; flush stderr;
                  H.replace active (Int32.of_int g) (Int32.of_int !activeCount); incr activeCount;
                  addToCurrent   current  y (Int32.of_int g) muv;
                  remFromCurrent proposed y (Int32.of_int g);
                  beta0 := betaG;
                  priorV := !priorV -. (lambda/.2.) *. muv *. muv;
                  ignore (compute_likelihood lambda xTr xTrTrans0 _C !priorV w0tx w0tx_logSum yTr erroneous (Some (y,g,muv)) true);
                  incr numAdded;
                  if h_size erroneous == 0 then ( Printf.fprintf stderr "--> |erroneous| = 0\n"; fin := true );
                  if !numAdded >= _S then ( Printf.fprintf stderr "--> numAdded >= _S\n"; fin := true );
                ) else (
                  Printf.fprintf stderr "--> betaG=%g < beta0=%g\n" betaG !beta0;
                  fin := true;
                );

        firstIter := false;
      done;
  
      Printf.fprintf stderr "%d added (total=%d)\n" !numAdded !activeCount;
      
      if de <= !v_best then (
        v_best := de;
        w_best := w0;
      );

      if !numAdded == 0 || inum > 1000 then !w_best else
        let im = create_matrix (h_size active) _C 0. in
          for c = 0 to _C - 1 do
            for f = 0 to old_active_size - 1 do
              im.(f).(c) <- w0.{f + c * old_active_size};
            done;
          done;
          iter2 (fun (y,g) mu ->
                   let gn = H.find active g in
                     im.(Int32.to_int gn).(y) <- mu) current;
          init_model := Some (MultModel im);
          run_iter (inum + 1) in

  let w_final = run_iter 1 in
  let w = make_ba _F 0. in
    H.iter (fun i j -> 
              for c = 0 to _C - 1 do
                w.{c * xTr.datav_numfeat + Int32.to_int i} <- w_final.{c * xTr.datav_numfeat + Int32.to_int j};
              done
           ) active;
    w

  
