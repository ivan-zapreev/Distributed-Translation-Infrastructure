open Array
open Util
open Data

module B = Bitvec

(* to save typing *)
type ia = int   array
type fa = float array
type ba = bigarr

type jnt = fa * fa  (* zn = gen * zn = non-gen *)
type mrg = fa

(* parameters *)
type psi = { psiI : fa ; psiO : fa ; psiG : fa }
type phi = { phiI : ba ; phiO : ba ; phiG : ba }   (* these are called lambda in the paper *)
type params =
    { mutable psi    : psi ;
      mutable phi    : phi ;
      mutable pi     : float ;
      mutable pib    : float ;
              fcount : int ;
              ccount : int }

(* copying params, checking differences *)
let copy_params p =
    { psi = { psiI = copy p.psi.psiI ; psiO = copy p.psi.psiO ; psiG = copy p.psi.psiG };
      phi = { phiI = copy_ba p.phi.phiI ; phiO = copy_ba p.phi.phiO ; phiG = copy_ba p.phi.phiG };
      pi  = p.pi;    fcount = p.fcount;
      pib = p.pib;   ccount = p.ccount }

let param_sum_abs_diff p p2 =
  let diff  a1 a2 = if a1 == a2 then 0. else fabs (a1 -. a2) in
  let diffL a1 a2 = fabs (a1 /@ a2) in
  let diff_array f a1 a2 = fold_lefti    (fun i a1i sm -> sm +. f a1i a2.(i)) 0. a1 in
  let diff_ba    f a1 a2 = fold_lefti_ba (fun i a1i sm -> sm +. f a1i a2.{i}) 0. a1 in
    diff_array diffL p.psi.psiI p2.psi.psiI +. 
    diff_array diffL p.psi.psiO p2.psi.psiO +. 
    diff_array diffL p.psi.psiG p2.psi.psiG +. 
    diff_ba    diff  p.phi.phiI p2.phi.phiI +. 
    diff_ba    diff  p.phi.phiO p2.phi.phiO +. 
    diff_ba    diff  p.phi.phiG p2.phi.phiG +. 
    diff p.pi  p2.pi  +.
    diff p.pib p2.pib

(* debugging output *)

let dump_expectations f str a =
  Printf.fprintf f "%s =" str;
  for i = 0 to length a - 1 do Printf.fprintf f " %g" a.(i); done;
  Printf.fprintf f "\n";
  flush f;
  ()

let dump_expectations_ba f str a =
  Printf.fprintf f "%s =" str;
  for i = 0 to A1.dim a - 1 do Printf.fprintf f " %g" a.{i}; done;
  Printf.fprintf f "\n";
  flush f;
  ()

let dump_params f p =
  Printf.fprintf f "---------------------------------------------------------------------\n";
  Printf.fprintf f "pi = %8g\tpib = %8g\n" p.pi p.pib;

  dump_expectations f "\npsiI" p.psi.psiI;
  dump_expectations f "psiO" p.psi.psiO;
  dump_expectations f "psiG" p.psi.psiG;

  dump_expectations_ba f "\nphiI" p.phi.phiI;
  dump_expectations_ba f "phiO" p.phi.phiO;
  dump_expectations_ba f "phiG" p.phi.phiG;

  Printf.fprintf f "\n";
  flush f;
  ()

(* this is the log probability of a data point according to a data point *)
let gen_log_prob (x : ia) (ps : fa) = 
  let _F = length ps in
  let _P = length x  in
  let v  = ref one in
  let p  = ref 0 in
    for f = 0 to _F - 1 do
      if !p < _P && x.(!p) == f
      then ( v := !v +. bounded_log ps.(f) ; incr p )
      else ( v := !v +. bounded_log (subLog one ps.(f)) );
    done;
    !v

let lr_class_log_prob _C _F x y (phi : ba) =
  let _P = length x in
  let dotEx c =
    let d = ref 0. in
      for i = 0 to _P - 1 do
        let id = x.(i) + c * _F in
          if id < A1.dim phi then d := !d +. phi.{id};
      done;
      !d in
    one /@ log (1. +. exp (0. -. dotEx 0))

let predict_binary y x pi psiG psiN phiG phiN =
  let psiRat = ref (log ((1. -. pi) /. pi)) in
  let p = ref 0 in
  let _P = length x in
  let _F = length psiG in

    for f = 0 to _F - 1 do
      let xnf = if !p < _P && x.(!p) == f then ( incr p; true ) else false in
        if xnf
        then psiRat := !psiRat *@ (psiG.(f) /@ psiN.(f))
        else psiRat := !psiRat *@ ((one -@ psiG.(f)) /@ (one -@ psiN.(f)));
    done;
    psiRat := one /@ (one +@ !psiRat);

  let dotEx phi =
    let d = ref phi.{0} in
      for i = 0 to _P - 1 do
        if x.(i) < A1.dim phi then d := !d +. phi.{x.(i)};
      done;
      !d in
  let p1g = one /@ log (1. +. exp (0. -. dotEx phiG)) in
  let p1n = one /@ log (1. +. exp (0. -. dotEx phiN)) in

  let p1  = ((log pi) *@ !psiRat *@ p1n         ) +@ ((log (1.-.pi)) *@ (one -@ !psiRat) *@ p1g         ) in
  let p0  = ((log pi) *@ !psiRat *@ (one -@ p1n)) +@ ((log (1.-.pi)) *@ (one -@ !psiRat) *@ (one -@ p1g)) in
  let p   = exp (p1 /@ (p1 +@ p0)) in
(*    Printf.fprintf stderr "predict>> %s y=%d pi=%g psiRat=%g p1g=%g p1n=%g p=%g\n" (if (exp p1g>0.5) == (exp p1n>0.5) then "*" else " ") y pi (exp !psiRat) (exp p1g) (exp p1n) p; *)
(*    exp (p1g /@ (p1g +@ (one -@ p1g))) *)
    p


let compute_error_rate match_domain y x pi psiG psiN phiG phiN =
  let _N = dat_N x in
  let cr = ref 0 in
  let tt = ref 0 in
    for n = 0 to _N - 1 do
      if match_domain x.datav_indomain.(n) then (
        let py = predict_binary y.(n) x.datav_xi.(n) pi psiG psiN phiG phiN in
          if (py > 0.5 && y.(n) == 1) || (py < 0.5 && y.(n) == 0)
          then incr cr;
          incr tt;
      );
    done;
    float_of_int (!tt - !cr) /. float_of_int !tt

let compute_joints
  (x : datav) (y : ia) (p : params)
  : jnt * jnt =
  let _N = length y in
  let _F = p.fcount  in
  let jointIA = make _N zero in
  let jointIB = make _N zero in
  let jointOA = make _N zero in
  let jointOB = make _N zero in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) then (
        jointIA.(n) <- log p.pi +. gen_log_prob x.datav_xi.(n) p.psi.psiG +.
                         lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) p.phi.phiG;
        jointIB.(n) <- log (1.-.p.pi) +. gen_log_prob x.datav_xi.(n) p.psi.psiI +.
                         lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) p.phi.phiI;
      ) else (
        jointOA.(n) <- log p.pib +. gen_log_prob x.datav_xi.(n) p.psi.psiG +.
                         lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) p.phi.phiG;
        jointOB.(n) <- log (1.-.p.pib) +. gen_log_prob x.datav_xi.(n) p.psi.psiO +.
                         lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) p.phi.phiO;
      );
    done;

    (jointIA,jointIB), (jointOA, jointOB)

let compute_marginals  (* produces *log* values *)
  (x : datav) (p : params)
  : mrg * mrg =
  let _N = dat_N x  in
  let _F = p.fcount in
  let mrgI = make _N zero in
  let mrgO = make _N zero in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) then
        mrgI.(n) <-
          one /@
            (addLog (log p.pi       +. gen_log_prob x.datav_xi.(n) p.psi.psiG)
                    (log (1.-.p.pi) +. gen_log_prob x.datav_xi.(n) p.psi.psiI))
      else 
        mrgO.(n) <-
            one /@
            (addLog (log p.pib       +. gen_log_prob x.datav_xi.(n) p.psi.psiG)
                    (log (1.-.p.pib) +. gen_log_prob x.datav_xi.(n) p.psi.psiO));
    done;
    mrgI, mrgO



let compute_logPSI
  (x : datav) (psG : fa) (psN : fa) (domain : bool) : fa * fa =
  let _N = dat_N x in
  let _F = length psG in
  let lPa = make _N one in let lPb = make _N one in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) == domain then (
        for f = 0 to _F - 1 do
          lPa.(n) <- lPa.(n) +. subLog one psG.(f);
          lPb.(n) <- lPb.(n) +. subLog one psN.(f);
        done;
        let _P = dat_P x n 0 in
          for p = 0 to _P - 1 do
            let xp = dat_i x n 0 p in
              lPa.(n) <- lPa.(n) /@ (subLog one psG.(xp)) +. psG.(xp);
              lPb.(n) <- lPb.(n) /@ (subLog one psN.(xp)) +. psN.(xp);
          done;
      ) else (
        lPa.(n) <- zero;
        lPb.(n) <- zero
      );
    done;
    lPa, lPb

let logPSIwithoutF
  (x : ia) (ps : fa) (lPSI : float) (f : int)
  : float =
  let xnf = ref false in
  let _P = length x in
    if lPSI >= 0. then 0. else (
      for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
      if !xnf
      then lPSI /@ ps.(f)
      else lPSI /@ subLog one ps.(f))

let update_logPSI
  (x : datav) (check_domain : bool -> bool) (lPSI : fa) (f : int) (oldv : float) (newv : float)
  (xnf_bv : B.bitvec)
  : unit =
  let _N = dat_N x in
    for n = 0 to _N - 1 do
      if check_domain x.datav_indomain.(n) then (
(*        let xnf = ref false in
        let _P = dat_P x n 0 in
          for p = 0 to _P - 1 do if dat_i x n 0 p == f then xnf := true; done;
*)
        let xnf = B.unsafe_is_on xnf_bv n in

        let v =
          if  xnf
          then lPSI.(n) /@ oldv +. newv
          else (lPSI.(n) /@ (subLog one oldv)) +. (subLog one newv) in

(*          if n == 1920 then (
            Printf.fprintf stderr "update_logPSI: n=%d xnf=%s lPSI.(n)=%g oldv=%g newv=%g ==> %g\n" n (if !xnf then "true " else "false") lPSI.(n) oldv newv v;
          ); *)

          lPSI.(n) <- v;
      );
    done;
    ()

let compute_expectations (* produces logs *)
  (x : datav) (y : ia) (indomain : bool) (pi : float) 
  (logPSIG : fa) (logPSIN : fa) 
  (phiG : ba) (phiN : ba) (p : params)
  : fa =
  let _N = length y in
  let ep = make _N zero in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) == indomain then (
        let lPzG = log pi +. 
                     logPSIG.(n) +. 
                     lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) phiG in
        let lPzN = log (1.-.pi) +. 
                     logPSIN.(n) +. 
                     lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) phiN in
          ep.(n) <- lPzG /@ (addLog lPzG lPzN);
(*          Printf.fprintf stderr "ce: %s %d -> (%g %g)=%g / +(%g %g)=%g = %g\n" 
            (if indomain then "id " else "ood") 
            n 

            logPSIG.(n) 
            (lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) phiG) 
            lPzG 

            logPSIN.(n) 
            (lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) phiN) 
            lPzN 

            ep.(n);
*)
      );
    done;
    ep

(* we need to optimize the pi parameter in the Mega model; i will
   use a fixed beta(2,2) prior *)

let best old v1 v2 =
  let bad1 = not (v1 >= 0. && v1 <= 1.) in
  let bad2 = not (v2 >= 0. && v2 <= 1.) in
    if bad1 && bad2 then old
    else if bad1 then v2
    else if bad2 then v1
    else if fabs (log v1 -. old) < fabs (log v2 -. old) then v1
    else v2

let best_log old v1 v2 =
  let bad1 = not (v1 >= zero && v1 <= one) in
  let bad2 = not (v2 >= zero && v2 <= one) in
    if bad1 && bad2 then old
    else if bad1 then v2
    else if bad2 then v1
    else if fabs (log v1 -. old) < fabs (log v2 -. old) then v1
    else v2

let opt_pi old_pi x domain m logPSIG logPSIN ep =
  let alpha = 2. in let beta = 2. in
  let _N = length m in
  let sum_h = ref 0. in
  let sum_m = ref 0. in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) == domain then (
        sum_h := !sum_h +. ep.(n);
        sum_m := !sum_m +. exp m.(n) *. (exp logPSIN.(n) -. exp logPSIG.(n));
      );
    done;
  let a = 0.5 *. ( ( alpha +. beta -. 2. +. float_of_int _N ) /. !sum_m -. 1. ) in
  let b = !sum_m /. ( alpha -. 1. +. !sum_h ) in
    (* now, pi = -a +- sqrt(a^2 - b) *)
  let det = max 0. (a *. a -. b) in
  let v1  = 0. -. a +. det in
  let v2  = 0. -. a -. det in
    bounded (best old_pi v1 v2)

let opt_psi_nongen_atf old_v
  (xd : datav) (m : fa) (ps : fa) (lPSI : fa) 
  (pi : float) (ex : fa) (f : int) (domain : bool) (alpha,beta)
  (xnf_bv : B.bitvec)
  : float =
  let _N = dat_N xd in

  let sum_h  = ref zero in
  let sum_hx = ref zero in
  let sum_mP = ref zero in
  let sum_mN = ref zero in

    for n = 0 to _N - 1 do
      if xd.datav_indomain.(n) == domain then (
        let x   = xd.datav_xi.(n) in
        let _P  = length x in
        let xnf = B.unsafe_is_on xnf_bv n in
(*        let xnf = ref false in *)
        let psi' = logPSIwithoutF x ps lPSI.(n) f in
        let v = m.(n) *@ log (1. -. pi) *@ psi' in
(*          for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done; *)
          sum_h := !sum_h +@ (subLog one ex.(n));
          if  xnf then sum_hx := !sum_hx +@ (subLog one ex.(n));
          if v > zero then (
            if  xnf then sum_mP := !sum_mP +@ v
                    else sum_mN := !sum_mN +@ v;
          );
      );
    done;

  let sum_m = !sum_mP -@ !sum_mN in
  let neg_a = log 0.5 *@ ((((!sum_h +@ log alpha +@ log beta) -@ log 2.) /@ sum_m) +@ log 1.) in
  let b     = ((log alpha +@ !sum_hx) -@ log 1.) /@ sum_m in
  let det   = max zero ((2. *. neg_a) -@ b) in
  let v1    = neg_a +@ det in
  let v2    = neg_a -@ det in

(*    Printf.eprintf "neg-a=%g b=%g det=%g v1=%g v2=%g\n" neg_a b det v1 v2; flush stderr; *)

(*
  let a = 0.5 *. ( ( 2. -. alpha -. beta -. exp !sum_h ) /. exp !sum_m -. 1. ) in
  let b = ( alpha -. 1. +. exp !sum_hx ) /. exp !sum_m in
  let det = max 0. (a *. a -. b) in
  let v1  = 0. -. a +. det in
  let v2  = 0. -. a -. det in
*)

    bounded_log (best_log old_v v1 v2)

let opt_psi_gen_atf old_v
  (xd : datav) (mI : fa) (mO : fa) (ps : fa) (lPSIG : fa) 
  (pi : float) (pib : float) (exI : fa) (exO : fa) (f : int) (alpha,beta)
  (xnf_bv : B.bitvec)
  : float =
  let _N = dat_N xd in

  let sum_h  = ref zero in
  let sum_hx = ref zero in
  let sum_mP = ref zero in
  let sum_mN = ref zero in

    for n = 0 to _N - 1 do
      let d   = xd.datav_indomain.(n) in
      let m   = if d then mI else mO in
      let pi  = if d then pi else pib in
      let h   = if d then exI.(n) else exO.(n) in
      let x   = xd.datav_xi.(n) in
      let _P  = length x in
(*      let xnf = ref false in *)
      let xnf = B.unsafe_is_on xnf_bv n in
      let psi' = logPSIwithoutF x ps lPSIG.(n) f in
      let v    = m.(n) *@ log pi *@ psi' in
(*        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done; *)
        sum_h := !sum_h +@ h;
        if  xnf then sum_hx := !sum_hx +@ h;
        if v > zero then (
          if  xnf then sum_mP := !sum_mP +@ v
                  else sum_mN := !sum_mN +@ v;
        );
    done;

  let sum_m = !sum_mP -@ !sum_mN in
  let neg_a = log 0.5 *@ ((((!sum_h +@ log alpha +@ log beta) -@ log 2.) /@ sum_m) +@ log 1.) in
  let b     = ((log alpha +@ !sum_hx) -@ log 1.) /@ sum_m in
  let det   = max zero ((2. *. neg_a) -@ b) in
  let v1    = neg_a +@ det in
  let v2    = neg_a -@ det in
(*
  let a = 0.5 *. ( ( 2. -. alpha -. beta -. exp !sum_h ) /. exp !sum_m -. 1. ) in
  let b = ( alpha -. 1. +. exp !sum_hx ) /. exp !sum_m in
  let det = max 0. (a *. a -. b) in
  let v1  = 0. -. a +. det in
  let v2  = 0. -. a -. det in
*)
    bounded_log (best old_v v1 v2)

let opt_psi_nongen
  (xd : datav) (m : fa) (ps : fa) (lPSI : fa) 
  (pi : float) (ex : fa) (domain : bool) alphabeta
  : unit =
  let _F = length ps in
  (* make bitvectors *)
  let _N = dat_N xd in
  let xnf_bv = init _F (fun _ -> B.create _N) in
    for n = 0 to _N - 1 do
      iter (fun f -> B.unsafe_turn_on xnf_bv.(f) n) xd.datav_xi.(n);
    done;

  let rec iter inum =
    let maxDP = ref 0. in
      for f = 0 to _F - 1 do
        let new_pf = opt_psi_nongen_atf ps.(f) xd m ps lPSI pi ex f domain alphabeta xnf_bv.(f) in
        let new_pf = 0.9 *. ps.(f) +. 0.1 *. new_pf in
          maxDP := max !maxDP (fabs (new_pf /@ ps.(f)));
          update_logPSI xd (fun d -> d == domain) lPSI f ps.(f) new_pf xnf_bv.(f);
          ps.(f) <- new_pf;
      done;
      Printf.fprintf stderr "MSTEP psi %s: iter = %d mdp = %g\n" (if domain then "true" else "fals") inum !maxDP;
      flush stderr;
      if inum < 50 && !maxDP > 0.001 then iter (inum+1) else () in
    iter 1;
    Printf.fprintf stderr "MSTEP psi %s done\n" (if domain then "true" else "fals")

let opt_psi_gen
  (x : datav)
  (mI : fa)  (mO : fa)
  (ps : fa) (lPSIG : fa)
  (pi : float) (pib : float) (exI : fa) (exO : fa) alphabeta
  : unit =
  let _F = length ps in
  (* make bitvectors *)
  let _N = dat_N x in
  let xnf_bv = init _F (fun _ -> B.create _N) in
    for n = 0 to _N - 1 do
      iter (fun f -> B.unsafe_turn_on xnf_bv.(f) n) x.datav_xi.(n);
    done;
  let rec iter inum =
    let maxDP = ref 0. in
      for f = 0 to _F - 1 do
        let new_pf = opt_psi_gen_atf ps.(f) x mI mO ps lPSIG pi pib exI exO f alphabeta xnf_bv.(f) in
        let new_pf = 0.9 *. ps.(f) +. 0.1 *. new_pf in
          maxDP := max !maxDP (fabs (new_pf /@ ps.(f)));
          update_logPSI x (fun _ -> true) lPSIG f ps.(f) new_pf xnf_bv.(f);
          ps.(f) <- new_pf;
      done;
      Printf.fprintf stderr "MSTEP psi gen : iter = %d mdp = %g\n" inum !maxDP;
      flush stderr;
      if inum < 50 && !maxDP > 0.01 then iter (inum+1) else () in
    iter 1;
    Printf.fprintf stderr "MSTEP psi gen done\n"

(* wrapper for CG/BFGS stuff is here *)

let maxi = 12

let mstep_phi_nongen
  (x : datav) (y : ia) (si2 : float) (ex : fa) (domain : bool) (multiclass : bool) : ba  = 
  let _N   = length y in
  let _    = iteri (fun i exi -> 
                      if x.datav_indomain.(i) == domain
                      then x.datav_weights.{i} <- exp (subLog one exi)
                      else x.datav_weights.{i} <- 0.) ex in
  let maxW = let v = fold_left_ba max 0. x.datav_weights in if v <= 0. then 0.1 else v in
  let _    = iteri_ba (fun i v -> x.datav_weights.{i} <- v /. maxW) x.datav_weights in
  let _    = dump_expectations_ba stderr "phi weights" x.datav_weights in

    if multiclass
    then let (w,_,_) = Bfgs.optimize false 1 false true maxi false (1./.si2) 1e-10 5 x y empty_data_set [||] empty_data_set [||] None None None false in w
    else               Cg.compute    false   false true maxi false (1./.si2) 1e-10   x y empty_data_set [||] empty_data_set [||] None None

let mstep_phi_gen
  (x : datav) (y : ia) (si2 : float) (exI : fa) (exO : fa) (multiclass : bool) : ba =
  let _N   = dat_N x in 
  let _    = iteri (fun i exo -> 
                      if x.datav_indomain.(i)
                      then x.datav_weights.{i} <- exp exI.(i)
                      else x.datav_weights.{i} <- exp exo    ) exO in
  let maxW = let v = fold_left_ba max 0. x.datav_weights in if v <= 0. then 0.1 else v in
  let _    = iteri_ba (fun i v -> x.datav_weights.{i} <- v /. maxW) x.datav_weights in
  let _    = dump_expectations_ba stderr "phi weights" x.datav_weights in

    if multiclass
    then let (w,_,_) = Bfgs.optimize false 1 false true maxi false (1./.si2) 1e-10 5 x y empty_data_set [||] empty_data_set [||] None None None false in w
    else               Cg.compute    false   false true maxi false (1./.si2) 1e-10   x y empty_data_set [||] empty_data_set [||] None None



(* CEM stuff follows *)

(* we want to monitor complete and incomplete log likelihood and posterior *)

let compute_complete_log_likelihood x y epI epO lPSII lPSIO lPSIG p =
  let ll = ref zero in
  let _N = dat_N x  in
    for n = 0 to _N - 1 do
      let zn    = if x.datav_indomain.(n) then epI.(n) else epO.(n) in
      let pi    = if x.datav_indomain.(n) then p.pi    else p.pib   in
      let lPSIN = if x.datav_indomain.(n) then lPSII   else lPSIO   in
        ll := !ll +@ (
          ((log pi) ^@ zn) *@ ((log (1.-.pi)) ^@ (one -@ zn)) *@
          ((lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) p.phi.phiG) ^@ zn) *@
          ((lr_class_log_prob p.ccount p.fcount x.datav_xi.(n) y.(n) 
                      (if x.datav_indomain.(n) then p.phi.phiI else p.phi.phiO)) ^@ (one -@ zn)));

(*        ll := !ll +@ (zn *@ log pi)    +@ ((one -@ zn) *@ log (one -@ pi))
                  +@ (zn *@ lPSIG.(n)) +@ ((one -@ zn) *@ lPSIN.(n))
                  +@ (zn       *@ lr_class_log_prob p.ccount p.fcount 
                                      x.datav_xi.(n) y.(n) p.phi.phiG)
                  +@ ((one -@ zn) *@ lr_class_log_prob p.ccount p.fcount 
                                      x.datav_xi.(n) y.(n) 
                                      (if x.datav_indomain.(n) then p.phi.phiI else p.phi.phiO)); *)
    done;
    !ll

let compute_incomplete_log_likelihood x (jntIA,jntIB) (jntOA,jntOB) =
  let llN = ref one in let llG = ref one in
  let _N = dat_N x in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) 
      then ( llG := !llG +. jntIA.(n) ; llN := !llN +. jntIB.(n) )
      else ( llG := !llG +. jntOA.(n) ; llN := !llN +. jntOB.(n) );
    done;
    addLog !llN !llG

let compute_log_prior phiSigma2 (alpha,beta) p =
  let norm_phi phi si =
    fold_lefti_ba (fun i phii sm ->
                  sm -. 0.5 /. si *. phii *. phii) one phi in
  let norm_psi_beta psi =
    fold_lefti (fun i psii sm ->
                  sm +. (alpha-.1.) *. psii +. (beta-.1.) *. (subLog one psii)) one psi in
    norm_phi p.phi.phiI phiSigma2 +. 
    norm_phi p.phi.phiO phiSigma2 +. 
    norm_phi p.phi.phiG phiSigma2 +.
      norm_psi_beta p.psi.psiI +.
      norm_psi_beta p.psi.psiO +.
      norm_psi_beta p.psi.psiG


(* initialization of CEM parameters *)

let initialize_parameters (x : datav) (y : ia) : params =
  let cf = dat_F x in
  let _F = x.datav_numfeat+1 in
  let lh = log 0.5 in
  let psi0 = { psiI = make _F lh ; psiO = make _F lh ; psiG = make _F lh } in
  let phi0 = { phiI = make_ba cf 0. ; phiO = make_ba cf 0. ; phiG = make_ba cf 0. } in
    { psi = psi0 ; phi = phi0 ; pi = 0.5 ; pib = 0.5 ; fcount = _F ; ccount = x.datav_numclass }

let initialize_logPSIs (x : datav) (p : params) : fa * fa * fa =
  let _N = dat_N x in
  let lPSII, lPSIIG = compute_logPSI x p.psi.psiG p.psi.psiI true  in
  let lPSIO, lPSIOG = compute_logPSI x p.psi.psiG p.psi.psiI false in
  let lPSIG = init _N (fun i -> if x.datav_indomain.(i) then lPSIIG.(i) else lPSIOG.(i)) in
    lPSII, lPSIO, lPSIG
  
(* perform one iteration of CEM *)

let cem_iteration (x : datav) (y : ia) (p : params) (lPSII : fa) (lPSIO : fa) (lPSIG : fa) (si2 : float) 
  (multiclass : bool) =
  let _N  = dat_N x in
  let exI   = compute_expectations x y true  p.pi  lPSIG lPSII p.phi.phiG p.phi.phiI p in
  let exO   = compute_expectations x y false p.pib lPSIG lPSIO p.phi.phiG p.phi.phiO p in
  let _     = dump_expectations stderr "exI" exI in
  let _     = dump_expectations stderr "exO" exO in
    (* compute marginals, joints *)
  let mrgI,mrgO = compute_marginals x   p in
  let jntI,jntO = compute_joints    x y p in
(*  let _         = dump_expectations stderr "mrgI" mrgI in
  let _         = dump_expectations stderr "mrgO" mrgO in
  let _         = dump_expectations stderr "jntI1" (fst jntI) in
  let _         = dump_expectations stderr "jntI2" (snd jntI) in
  let _         = dump_expectations stderr "jntO1" (fst jntO) in
  let _         = dump_expectations stderr "jntO2" (snd jntO) in *)
    (* update phis *)
  let phiI' = mstep_phi_nongen x y si2 exI     true  multiclass in
  let phiO' = mstep_phi_nongen x y si2     exO false multiclass in
  let phiG' = mstep_phi_gen    x y si2 exI exO       multiclass in
  let _     = p.phi <- { phiI = phiI' ; phiO = phiO' ; phiG = phiG' } in

  let pi', pib' = p.pi, p.pib in
    (* update psis *)
    opt_psi_nongen x mrgI      p.psi.psiI lPSII pi'      exI     true  (2.,2.);
    opt_psi_nongen x mrgO      p.psi.psiO lPSIO pib'     exO     false (2.,2.); 
    opt_psi_gen    x mrgI mrgO p.psi.psiG lPSIG pi' pib' exI exO       (2.,2.);  

    (* update pis *)
  let pi'   = opt_pi pi'  x true  mrgI lPSIG lPSII exI in
  let pib'  = opt_pi pib' x false mrgO lPSIG lPSIO exO in  
  let pi', pib' = 0.5,0.5 in

    (* and update p *)
  let ()    =  p.pi <- pi' ; p.pib <- pib'  in
    (* compute error on in-domain data *)
(*  let errI  = compute_error p.psi.psiG p.psi.psiI pi'  p.phi.phiG.(0) p.phi.phiI.(0) iD in
  let errO  = compute_error p.psi.psiG p.psi.psiO pib' p.phi.phiG.(0) p.phi.phiO.(0) oD in
    Printf.fprintf stderr "ID  error: %g\n" errI;
    Printf.fprintf stderr "OOD error: %g\n" errO; *)

  let pll = one in (* compute_log_prior si2 Beta22 p in *)
  let cll = pll +. compute_complete_log_likelihood x y exI exO lPSII lPSIO lPSIG p in
  let ill = pll +. compute_incomplete_log_likelihood x jntI jntO in
  let _   = Printf.fprintf stderr "CLL = %g\tILL = %g\n" cll ill in

    flush stderr; 
    ()

(*
let testX =
  { datav_bernoulli = true;
    datav_implicit  = true;
    datav_sampled   = false;
    datav_numclass  = 1;
    datav_numfeat   = 5;
    datav_indomain  = [| true; true; true; true; true; true; false; false; false; false; false; false |];
    datav_weights   = A1.of_array BA.float32 BA.c_layout [| 1.; 1.; 1.; 1.; 1.; 1.; 1.; 1.; 1.; 1.; 1.; 1. |];
    datav_xi        = [| [|0;2;3;4|] ; [|0;3;4|] ; [|0;2|] ; [|0;4;5|] ; [|0;5|] ; [|0;5|] ;
                         [|0;1;3;4|] ; [|0;3;4|] ; [|0;1|] ; [|0;4;5|] ; [|0;5|] ; [|0;5|] |];
    datav_xi_c      = [||];
    datav_xv        = [||];
    datav_xv_c      = [||];
  }

let testY = [| 1;1;1;0;0;0 ; 1;1;1;0;0;0 |]
*)

let ((testX, testY),(testdX,testdY),(testsX, testsY)) = load_implicit false false true true (fun _ -> open_in "tagging.adapt.small") close_in read_binary_class 1 false (fun _ _ _ -> ())

let _ =  (* sort the datav_xi-s *)
  let _N = dat_N testX in
    for n = 0 to _N - 1 do
      fast_sort compare testX.datav_xi.(n);
    done;
  let _N = dat_N testdX in
    for n = 0 to _N - 1 do
      fast_sort compare testdX.datav_xi.(n);
    done;
  let _N = dat_N testsX in
    for n = 0 to _N - 1 do
      fast_sort compare testsX.datav_xi.(n);
    done;
    ()

let p = initialize_parameters testX testY
let lPSII, lPSIO, lPSIG = initialize_logPSIs testX p

let _ =
  for iter = 1 to 20 do
    let p_old = copy_params p in
      Printf.fprintf stderr "\n\n================= ITERATION %d =================\n\n" iter;
      cem_iteration testX testY p lPSII lPSIO lPSIG 1. false;
      dump_params stderr p;
      Printf.fprintf stderr "\n=========== difference = %g ===========\n" (param_sum_abs_diff p p_old);
      let ide  = compute_error_rate (fun x -> x    ) testY  testX  p.pi  p.psi.psiG p.psi.psiI p.phi.phiG p.phi.phiI in
      let ode  = compute_error_rate (fun x -> not x) testY  testX  p.pib p.psi.psiG p.psi.psiO p.phi.phiG p.phi.phiO in
      Printf.fprintf stderr "train: id error = %g\tood error = %g\ttot error = %g\n" ide  ode  (ide  +. ode );
      let ide2 = compute_error_rate (fun x -> x    ) testdY testdX p.pi  p.psi.psiG p.psi.psiI p.phi.phiG p.phi.phiI in
      let ode2 = compute_error_rate (fun x -> not x) testdY testdX p.pib p.psi.psiG p.psi.psiO p.phi.phiG p.phi.phiO in 
      Printf.fprintf stderr "test : id error = %g\tood error = %g\ttot error = %g\n" ide2 ode2 (ide2 +. ode2);
      let ide3 = compute_error_rate (fun x -> x    ) testsY testsX p.pi  p.psi.psiG p.psi.psiI p.phi.phiG p.phi.phiI in
      let ode3 = compute_error_rate (fun x -> not x) testsY testsX p.pib p.psi.psiG p.psi.psiO p.phi.phiG p.phi.phiO in 
      Printf.fprintf stderr "test : id error = %g\tood error = %g\ttot error = %g\n" ide3 ode3 (ide3 +. ode3);
      flush stderr;
  done;
  ()
