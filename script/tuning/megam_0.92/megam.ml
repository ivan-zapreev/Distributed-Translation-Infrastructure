open Array

type fa = float array
type ia = int   array

type psi =
    { psiI : fa;
      psiO : fa;
      psiG : fa }

type phi =
    { phiI : fa array;
      phiO : fa array;
      phiG : fa array }

type pi = float

type data = int array * ia array

type params =
    { mutable psi : psi;
      mutable phi : phi;
      mutable pi  : pi;
      mutable pib : pi;
      fcount : int }

let predict_binary  (* produces p(y=+1) *)
  (psiG : fa) (psiN : fa) (pi : pi)
  (phiG : fa) (phiN : fa) (x  : ia)
  : float =
  let probG = ref (log pi) in
  let probN = ref (log (1.-.pi)) in
  let _P    = length x in
  let _F    = length psiG in
  let p     = ref 0 in
    for f = 0 to _F - 1 do
      if !p < _P && x.(!p) == f
      then ( probG := !probG +. log psiG.(f)       ; probN := !probN +. log psiN.(f)       ; incr p )
      else ( probG := !probG +. log (1.-.psiG.(f)) ; probN := !probN +. log (1.-.psiN.(f)) );
    done;
  let pGyP  = fold_left (fun p f -> p +. phiG.(f)) 0. x in
  let pGyN  = fold_left (fun p f -> p -. phiG.(f)) 0. x in
  let pNyP  = fold_left (fun p f -> p +. phiN.(f)) 0. x in
  let pNyN  = fold_left (fun p f -> p -. phiN.(f)) 0. x in
  let pP    = addLog (!probG +. (pGyP -. addLog pGyP pGyN))
                     (!probN +. (pNyP -. addLog pNyP pNyN)) in
  let pN    = addLog (!probG +. (pGyN -. addLog pGyP pGyN))
                     (!probN +. (pNyN -. addLog pNyP pNyN)) in
    exp (pP -. (addLog pP pN))

let compute_error
  (psiG : fa) (psiN : fa) (pi : pi)
  (phiG : fa) (phiN : fa) ((dY,dX) : data)
  : float =
  let _N = length dY in
  let e  = ref 0 in
    for n = 0 to _N - 1 do
      let p = predict_binary psiG psiN pi phiG phiN dX.(n) in
      let _ = Printf.fprintf stderr "n=%d y=%d p=%g\n" n dY.(n) p in
        if (p > 0.5 && dY.(n) == 0) || (p < 0.5 && dY.(n) == 1)
        then incr e;
    done;
    float_of_int !e /. float_of_int _N


let dump_params f p =
  Printf.fprintf f "---------------------------------------------------------------------";
  Printf.fprintf f "\npi = %5g\tpib = %5g" p.pi p.pib;
  Printf.fprintf f "\n\npsiI =";
  for i = 0 to length p.psi.psiI - 1 do Printf.fprintf f " %5g" p.psi.psiI.(i); done;
  Printf.fprintf f "\npsiO =";
  for i = 0 to length p.psi.psiO - 1 do Printf.fprintf f " %5g" p.psi.psiO.(i); done;
  Printf.fprintf f "\npsiG =";
  for i = 0 to length p.psi.psiG - 1 do Printf.fprintf f " %5g" p.psi.psiG.(i); done;
  Printf.fprintf f "\n\nphiI =";
  for i = 0 to length p.phi.phiI.(0) - 1 do Printf.fprintf f " %5g" p.phi.phiI.(0).(i); done;
  Printf.fprintf f "\nphiO =";
  for i = 0 to length p.phi.phiO.(0) - 1 do Printf.fprintf f " %5g" p.phi.phiO.(0).(i); done;
  Printf.fprintf f "\nphiG =";
  for i = 0 to length p.phi.phiG.(0) - 1 do Printf.fprintf f " %5g" p.phi.phiG.(0).(i); done;
  Printf.fprintf f "\n";
  flush f;
  ()

let dump_expectations f str a =
  Printf.fprintf f "%s =" str;
  for i = 0 to length a - 1 do Printf.fprintf f " %5g" a.(i); done;
  Printf.fprintf f "\n";
  flush f;
  ()

type jnt = fa * fa  (* zn=gen * zn=non-gen *)
type mrg = fa

let zero  = log 0.
let one   = log 1.
let addLog x y = 
  if x = zero then y
  else if y = zero then x
  else if x -. y > 16.0 then x
  else if x > y then (x +. log (1. +. exp (y -. x)))
  else if y -. x > 16.0 then y
  else (y +. log (1. +. exp (x -. y)))
let subLog x y = 
  if y = zero then x
  else if x < y then zero
  else if x = y then zero
  else if x -. y > 16.0 then x
  else (x +. log (1. -. exp (y -. x)))


let gen_log_prob (x : ia) (ps : fa) =
  let _F = length ps in
  let _P = length x  in
  let v  = ref one in
  let p  = ref 0 in
    for f = 0 to _F - 1 do
      if !p < _P && x.(!p) == f
      then ( v := !v +. log ps.(f) ; incr p )
      else ( v := !v +. log (1. -. ps.(f)) );
    done;
    !v

let lr_class_log_prob (x : ia) (y : int) (phi : fa array) : float =
  let _C = length phi in
  let _P = length x   in
  let _Z = ref zero in
  let this = ref 0. in
    for c = 0 to _C - 1 do
      let v = ref 0. in
      for p = 0 to _P - 1 do
        v := !v +. phi.(c).(x.(p));
      done;
      _Z := addLog !_Z !v;
      if c == y then this := !v;
(*      Printf.fprintf stderr "c=%d _Z=%g this=%g\n" c !_Z !this; *)
    done;
    !this -. !_Z

let compute_joints
  ((iDy,iDx) : data) ((oDy,oDx) : data) (p : params)
  : jnt * jnt =
  let _N = length iDy in let _Nb = length oDy in
  let _F = p.fcount  in
  let jointIA = make _N  zero in
  let jointIB = make _N  zero in
  let jointOA = make _Nb zero in
  let jointOB = make _Nb zero in
    for n = 0 to _N - 1 do
      jointIA.(n) <- log p.pi +. gen_log_prob iDx.(n) p.psi.psiG +.
                     lr_class_log_prob iDx.(n) iDy.(n) p.phi.phiG;
      jointIB.(n) <- log (1.-.p.pi) +. gen_log_prob iDx.(n) p.psi.psiI +.
                     lr_class_log_prob iDx.(n) iDy.(n) p.phi.phiI;
    done;

    for n = 0 to _Nb - 1 do
      jointOA.(n) <- log p.pib +. gen_log_prob oDx.(n) p.psi.psiG +.
                     lr_class_log_prob oDx.(n) oDy.(n) p.phi.phiG;
      jointOB.(n) <- log (1.-.p.pib) +. gen_log_prob oDx.(n) p.psi.psiO +.
                     lr_class_log_prob oDx.(n) oDy.(n) p.phi.phiO;
    done;
    (jointIA,jointIB), (jointOA, jointOB)

let compute_marginals  (* produces *log* values *)
  ((_,iD) : data) ((_,oD) : data) (p : params)
  : mrg * mrg =
  let _N = length iD in let _Nb = length oD in
  let _F = p.fcount  in
  let mrgI = make _N  zero in
  let mrgO = make _Nb zero in
    for n = 0 to _N - 1 do
(*      Printf.fprintf stderr "%d: pi=%g gp=%g pi1=%g gp1=%g\n" n (log p.pi) (gen_log_prob iD.(n) p.psi.psiG) (log (1.-.p.pi)) (gen_log_prob iD.(n) p.psi.psiI); *)
(*      Printf.fprintf stderr "%d: 1/(%g + %g) 1/%g = %g\n" n 
        (log p.pi       +. gen_log_prob iD.(n) p.psi.psiG)
        (log (1.-.p.pi) +. gen_log_prob iD.(n) p.psi.psiI)
        (addLog (log p.pi       +. gen_log_prob iD.(n) p.psi.psiG)
           (log (1.-.p.pi) +. gen_log_prob iD.(n) p.psi.psiI))
        (one -. (addLog (log p.pi       +. gen_log_prob iD.(n) p.psi.psiG)
           (log (1.-.p.pi) +. gen_log_prob iD.(n) p.psi.psiI)));*)
      flush stderr;
      mrgI.(n) <-
          one -.
          (addLog (log p.pi       +. gen_log_prob iD.(n) p.psi.psiG)
                  (log (1.-.p.pi) +. gen_log_prob iD.(n) p.psi.psiI));
    done;
    for n = 0 to _Nb - 1 do
      mrgO.(n) <-
          one -.
          (addLog (log p.pib       +. gen_log_prob oD.(n) p.psi.psiG)
                  (log (1.-.p.pib) +. gen_log_prob oD.(n) p.psi.psiO));
    done;
    mrgI, mrgO

let compute_logPSI
  ((_,d) : data) (psG : fa) (psN : fa)
  : fa * fa =
  let _N = length d in
  let _F = length psG in
  let lPa = make _N one in let lPb = make _N one in
    for n = 0 to _N - 1 do
      for f = 0 to _F - 1 do
        lPa.(n) <- lPa.(n) +. log (1. -. psG.(f));
        lPb.(n) <- lPb.(n) +. log (1. -. psN.(f));
      done;
      let x = d.(n) in
      let _P = length x in
        for p = 0 to _P - 1 do
          lPa.(n) <- lPa.(n) -. log (1. -. psG.(x.(p))) +. log psG.(x.(p));
          lPb.(n) <- lPb.(n) -. log (1. -. psN.(x.(p))) +. log psN.(x.(p));
        done;
    done;
    lPa, lPb

let logPSIwithoutF
  (x : ia) (ps : fa) (lPSI : float) (f : int)
  : float =
  let xnf = ref false in
  let _P = length x in
    for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
    if !xnf
    then lPSI -. log ps.(f)
    else lPSI -. log (1. -. ps.(f))

let update_logPSI
  ((_,d) : data) (lPSI : fa) (f : int) (oldv : float) (newv : float)
  : unit =
  let _N = length d in
    for n = 0 to _N - 1 do
      let x = d.(n) in
      let xnf = ref false in
      let _P = length x in
        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
        if !xnf
        then lPSI.(n) <- lPSI.(n) -. log oldv +. log newv
        else lPSI.(n) <- lPSI.(n) -. log (1. -. oldv) +. log (1. -. newv);
    done;
    ()

let update_logPSIgen
  ((_,xI) : data) ((_,xO) : data) (lPSI : fa) (f : int) (oldv : float) (newv : float)
  : unit =
  let _N  = length xI in
  let _Nb = length xO in
    for n = 0 to _N - 1 do
      let x = xI.(n) in
      let xnf = ref false in
      let _P = length x in
        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
        if !xnf
        then lPSI.(n) <- lPSI.(n) -. log oldv +. log newv
        else lPSI.(n) <- lPSI.(n) -. log (1. -. oldv) +. log (1. -. newv);
    done;
    for n = 0 to _Nb - 1 do
      let x = xO.(n) in
      let xnf = ref false in
      let _P = length x in
        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
        if !xnf
        then lPSI.(_N+n) <- lPSI.(_N+n) -. log oldv +. log newv
        else lPSI.(_N+n) <- lPSI.(_N+n) -. log (1. -. oldv) +. log (1. -. newv);
    done;
    ()

let compute_expectations (* produces logs *)
  ((dy,dx) : data) (pi : float) 
  (logPSIG : fa) (logPSIN : fa) (domain : bool)
  (phiG : fa array) (phiN : fa array)
  : fa =
  let _N = length dx in
  let ex = make _N zero in
    for n = 0 to _N - 1 do
      if x.datav_indomain.(n) == domain then (
        let yn = dy.(n) in let xn = dx.(n) in
        let lPzG =
          log pi +. logPSIG.(n) +. 
            lr_class_log_prob xn yn phiG in
        let lPzN =
          log (1.-.pi) +. logPSIN.(n) +. 
            lr_class_log_prob xn yn phiN in
        let _ = Printf.fprintf stderr "expectation: %d: %g %g\n" n lPzG lPzN in
          ex.(n) <- lPzG -. (addLog lPzG lPzN));
    done;
    ex

let mstep_pi
  (m : mrg) (logPSIG : fa) (logPSIN : fa) (psiGos : int) (ex : fa) =
  let _N = length m in
  let a  = ref zero in
  let nb = ref one  in
  let nc = ref zero in
    for n = 0 to _N - 1 do
      let mp = 
        if logPSIN.(n) > logPSIG.(n+psiGos)
        then m.(n) +. subLog logPSIN.(n) logPSIG.(n+psiGos)
        else m.(n) in
(*        Printf.fprintf stderr "%d: %g * (%g-%g) = %g * %g = %g\n" n m.(n) logPSIN.(n) logPSIG.(n+psiGos) m.(n) (subLog logPSIN.(n) logPSIG.(n+psiGos)) mp;  *)
        a  := addLog !a  mp;
        nb := addLog !nb mp;
    done;
    for n = 0 to _N - 1 do
      nb := subLog !nb (log 2. +. ex.(n));
      nc := addLog !nc ex.(n);
    done;
    (* now, x = (nb +- sqrt[nb^2 + 4 a nc]) / (2a) *)
  let det = 0.5 *. addLog (!nb +. !nb) (log 4. +. !a +. !nc) in
(*  let _ = Printf.fprintf stderr "a=%g nb=%g nc=%g det=%g\n" !a !nb !nc det ; flush stderr in
  let _ = Printf.fprintf stderr "v1=%g v2=%g\n" (exp ((subLog !nb det) -. (log 2. +. !a))) (exp ((addLog !nb det) -. (log 2. +. !a))) ; flush stderr in *)
    if !nb >= det
    then exp ((subLog !nb det) -. (log 2. +. !a))
    else exp ((addLog !nb det) -. (log 2. +. !a))

let mstep_psi_nongen_beta00_atf
  ((dy,dx) : data) ((_,j) : jnt) (ps : fa) (lPSI : fa) 
  (pi : pi) (ex : fa) (f : int)
  : float =
  let _N = length dy in
  let a  = ref zero in
  let nb = ref zero in
  let c  = ref one  in
    for n = 0 to _N - 1 do
      let (y,x) = dy.(n),dx.(n) in
      let jpp = j.(n) +. log (1. -. pi) +. 
                  logPSIwithoutF x ps lPSI.(n) f in
      let xnf = ref false in
      let _P = length x in
        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
        a  := addLog !a  jpp;
        nb := addLog !nb (addLog jpp (subLog one ex.(n)));
        if !xnf then c := addLog !c  (subLog one ex.(n));
    done;
  let det = 0.5 *. subLog (!nb +. !nb) (log 4. +. !a +. !c) in
(*  let _ = Printf.fprintf stderr "a=%g nb=%g c=%g det=%g\n" !a !nb !c det in*)
    if !nb >= det 
    then exp ((subLog !nb det) -. (log 2. +. !a))
    else exp ((addLog !nb det) -. (log 2. +. !a))

let bounded x = if x < 1e-6 then 1e-6 else if x > 1.-.1e-6 then 1.-.1e-6 else x

let mstep_psi_nongen_beta00
  (d : data) (j : jnt) (ps : fa) (lPSI : fa) 
  (pi : pi) (ex : fa)
  : unit =
  let _F = length ps in
  let fabs x = if x <= 0. then 0. -. x else x in
  let rec iter inum =
    let maxDP = ref 0. in
      for f = 0 to _F - 1 do
        let new_pf = bounded (mstep_psi_nongen_beta00_atf
                        d j ps lPSI pi ex f) in
(*          Printf.fprintf stderr "%d: %g -> %g\n" f ps.(f) new_pf; flush stderr; *)
          maxDP := max !maxDP (fabs (new_pf -. ps.(f)));
          update_logPSI d lPSI f ps.(f) new_pf;
          ps.(f) <- new_pf;
      done;
      if inum < 50 && !maxDP > 1e-6 then iter (inum+1) else () in
    iter 1

let mstep_psi_gen_beta00_atf
  ((iDy,iDx) : data) ((oDy,oDx) : data) 
  ((jI,_) : jnt) ((jO,_) : jnt)
  (ps : fa) (lPSIG : fa)
  (pi : pi) (pib : pi) (exI : fa) (exO : fa) (f : int)
  : float =
  let _N  = length iDy in
  let _Nb = length oDy in
  let a   = ref zero in
  let nb  = ref zero in
  let c   = ref one  in
    for n = 0 to _N - 1 do
      let (y,x) = iDy.(n), iDx.(n) in
      let jpp = jI.(n) +. log pi +. 
                  logPSIwithoutF x ps lPSIG.(n) f in
      let xnf = ref false in
      let _P = length x in
        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
        a  := addLog !a  jpp;
        nb := addLog !nb (addLog jpp (subLog one exI.(n)));
        if !xnf then c := addLog !c  (subLog one exI.(n));
    done;
    for n = 0 to _Nb - 1 do
      let (y,x) = oDy.(n), oDx.(n) in
      let jpp = jO.(n) +. log pib +. 
                  logPSIwithoutF x ps lPSIG.(n+_N) f in
      let xnf = ref false in
      let _P = length x in
        for p = 0 to _P - 1 do if x.(p) == f then xnf := true; done;
        a  := addLog !a  jpp;
        nb := addLog !nb (addLog jpp (subLog one exO.(n)));
        if !xnf then c := addLog !c  (subLog one exO.(n));
    done;
  let det = 0.5 *. subLog (!nb +. !nb) (log 4. +. !a +. !c) in
(*  let _ = Printf.fprintf stderr "a=%g nb=%g c=%g det=%g\n" !a !nb !c det in*)
    if !nb >= det 
    then exp ((subLog !nb det) -. (log 2. +. !a))
    else exp ((addLog !nb det) -. (log 2. +. !a))

let mstep_psi_gen_beta00
  (iD : data) (oD : data)
  (jI : jnt)  (jO : jnt)
  (ps : fa) (lPSIG : fa)
  (pi : pi) (pib : pi) (exI : fa) (exO : fa)
  : unit =
  let _F = length ps in
  let fabs x = if x <= 0. then 0. -. x else x in
  let rec iter inum =
    let maxDP = ref 0. in
      for f = 0 to _F - 1 do
        let new_pf = bounded (mstep_psi_gen_beta00_atf
                        iD oD jI jO ps lPSIG pi pib exI exO f) in
(*          Printf.fprintf stderr "%d: %g -> %g\n" f ps.(f) new_pf; flush stderr; *)
          maxDP := max !maxDP (fabs (new_pf -. ps.(f)));
          update_logPSIgen iD oD lPSIG f ps.(f) new_pf;
          ps.(f) <- new_pf;
      done;
      if inum < 50 && !maxDP > 1e-6 then iter (inum+1) else () in
    iter 1

(* for gaussian priors *)

let mstep_psi_gen_gaussian
  ((_,iD) as iDD : data) ((_,oD) as oDD : data)
  ((jI,_) : jnt)  ((jO,_) : jnt)
  (ps : fa) (lPSI : fa) 
  (pi : pi) (pib : pi) 
  (exI : fa) (exO : fa) 
  (lambda : float)
  : unit =
  let _F = length ps in
  let _N = length iD in
  let fabs x = if x <= 0. then 0. -. x else x in
  let sumEp0 =    fold_left (fun sm ep -> sm +. (exp ep)) 0. exI 
               +. fold_left (fun sm ep -> sm +. (exp ep)) 0. exO in
  let rec iter inum =
    let dw = ref 0. in
    for f = 0 to _F - 1 do
      let oldps = ps.(f) in
      for ii = 1 to 100 do
        let sm = fold_lefti (fun n j sm -> sm +. j *. pi  *. exp (logPSIwithoutF iD.(n) ps lPSI.(n   ) f)) 0. jI +.
                 fold_lefti (fun n j sm -> sm +. j *. pib *. exp (logPSIwithoutF oD.(n) ps lPSI.(n+_N) f)) 0. jO in
        let g = (lambda *. lambda *. (log (1. -. ps.(f)) -. log ps.(f))
                 -. ps.(f) *. sumEp0) /. ps.(f) /. (1. -. ps.(f)) -. sm in
        let h = (lambda *. lambda) /. ps.(f) /. (1. -. ps.(f)) /. ps.(f) /. (1. -. ps.(f)) *.
                  (log (1. -. ps.(f)) -. log ps.(f) -. 1.)
                -. sumEp0 /. ps.(f) /. (1. -. ps.(f)) in
        let new_psf = bounded (ps.(f) -. g/.h) in
          update_logPSIgen iDD oDD lPSI f ps.(f) new_psf;
          ps.(f) <- new_psf;
      done;
      dw := max !dw (fabs (oldps -. ps.(f)));
    done;
    Printf.fprintf stderr "iter %d, maxdw = %g \n" inum !dw;
    flush stderr;
    if inum < 500 && !dw > 1e-8 then iter (inum+1)
  in
    iter 1;
    ()

let mstep_psi_nongen_gaussian
  ((_,d) as dd : data) ((_,j) : jnt) (ps : fa) (lPSI : fa) (pi : pi) (ex : fa) (lambda : float)
  : unit =
  let _F = length ps in
  let fabs x = if x <= 0. then 0. -. x else x in
  let sumEp0 = fold_left (fun sm ep -> sm +. (1. -. exp ep)) 0. ex in
  let rec iter inum =
    let dw = ref 0. in
    for f = 0 to _F - 1 do
      let oldps = ps.(f) in
      for ii = 1 to 100 do
        let sm = fold_lefti (fun n j sm -> sm +. j *. (1.-.pi) *. exp (logPSIwithoutF d.(n) ps lPSI.(n) f)) 0. j in
        let g = (lambda *. lambda *. (log (1. -. ps.(f)) -. log ps.(f))
                 -. ps.(f) *. sumEp0) /. ps.(f) /. (1. -. ps.(f)) -. sm in
        let h = (lambda *. lambda) /. ps.(f) /. (1. -. ps.(f)) /. ps.(f) /. (1. -. ps.(f)) *.
                  (log (1. -. ps.(f)) -. log ps.(f) -. 1.)
                -. sumEp0 /. ps.(f) /. (1. -. ps.(f)) in
        let new_psf = bounded (ps.(f) -. g/.h) in
(*          Printf.fprintf stderr "f=%d ep0=%g ii=%d sm=%g g=%g h=%g g/h=%g\n" f sumEp0 ii sm g h (g /. h); *)
          flush stderr;
          update_logPSI dd lPSI f ps.(f) new_psf; 
          ps.(f) <- new_psf; 
      done;
      dw := max !dw (fabs (oldps -. ps.(f)));
      flush stderr;
    done;
    Printf.fprintf stderr "iter %d, maxdw = %g \n" inum !dw;
    flush stderr;
    if inum < 500 && !dw > 1e-8 then iter (inum+1)
  in
    iter 1;
    ()

(* phi... *)

let mstep_phi_nongen_binary
  ((dy,dx) : data) (si2 : float) (ex : fa)
  : fa array = (* of length 2 *)
  let _N   = length dy in
  let my_y = init _N (fun i -> if dy.(i) == 1 then 1. else -1.) in
  let exWT = init _N (fun i -> exp (subLog one ex.(i))) in
  let sumW = fold_left (+.) 0. exWT in
  let _    = iteri (fun i v -> exWT.(i) <- v /. sumW *. float_of_int _N) in
  let phi  = (* CGopt. *) compute 50 (1./.si2) exWT dx my_y in
  let phiN = init (length phi) (fun i -> 0. -. phi.(i)) in
    [| phi ; phiN |]

let mstep_phi_gen_binary
  ((iDy,iDx) : data) ((oDy,oDx) : data) (si2 : float) (exI : fa) (exO : fa)
  : fa array = (* of length 2 *)
  let _N   = length iDy in 
  let _Nb  = length oDy in
  let my_x = init (_N+_Nb) 
                (fun i -> if i < _N then iDx.(i) else oDx.(i-_N)) in
  let my_y = init (_N+_Nb)
                (fun i -> 
                   if i < _N 
                   then if iDy.(i   ) == 1 then 1. else -1.
                   else if oDy.(i-_N) == 1 then 1. else -1.) in
  let exWT = init (_N+_Nb)
                (fun i -> if i < _N then exp exI.(i) else exp exO.(i-_N)) in
  let sumW = fold_left (+.) 0. exWT in
  let _    = iteri (fun i v -> exWT.(i) <- v /. sumW *. float_of_int (_N+_Nb)) in
  let phi  = (* CGopt. *) compute 50 (1./.si2) exWT my_x my_y in
  let phiN = init (length phi) (fun i -> 0. -. phi.(i)) in
    [| phi ; phiN |]

let initialize_parameters ((iDy,iDx) : data) ((oDy,oDx) : data) : params =
  let _N  = length iDy in let _Nb = length oDy in
  let _C  = fold_left max (fold_left max 1 oDy) iDy + 1 in
  let _Fi = fold_left (fun x a -> fold_left max x a) 1 iDx + 1 in
  let _Fo = fold_left (fun x a -> fold_left max x a) 1 oDx + 1 in
  let _F  = max _Fi _Fo in
  let psi0 = { psiI = make _F 0.5 ; 
               psiO = make _F 0.5 ; 
               psiG = make _F 0.5 } in
  let phi0 = { phiI = init _C (fun _ -> make _F 0.) ;
               phiO = init _C (fun _ -> make _F 0.) ;
               phiG = init _C (fun _ -> make _F 0.) } in
    { psi=psi0 ; phi=phi0 ; pi=0.5 ; pib=0.5 ; fcount=_F }

let initialize_logPSIs 
  (iD : data) (oD : data) (p : params)
  : fa * fa * fa =
  let _N  = length (fst iD) in
  let _Nb = length (fst oD) in
  let lPSII, lPSIIG = compute_logPSI iD p.psi.psiG p.psi.psiI in
  let lPSIO, lPSIOG = compute_logPSI oD p.psi.psiG p.psi.psiO in
  let lPSIG = init (_N+_Nb) (fun i -> if i < _N then lPSIIG.(i) else lPSIOG.(i-_N)) in
    lPSII, lPSIO, lPSIG

let single_iteration
  (iD : data) (oD : data) (p : params) 
  (lPSII : fa) (lPSIO : fa) (lPSIG : fa)
  (si2 : float)
  : unit =
  let _N    = length (fst iD) in
  let exI   = compute_expectations iD p.pi  lPSIG lPSII 0  p.phi.phiG p.phi.phiI in
  let exO   = compute_expectations oD p.pib lPSIG lPSIO _N p.phi.phiG p.phi.phiO in
  let _     = dump_expectations stderr "exI" exI in
  let _     = dump_expectations stderr "exO" exO in
    (* compute marginals, joints *)
  let mrgI,mrgO = compute_marginals iD oD p in
  let jntI,jntO = compute_joints    iD oD p in
  let _         = dump_expectations stderr "mrgI" mrgI in
  let _         = dump_expectations stderr "mrgO" mrgO in
  let _         = dump_expectations stderr "jntI1" (fst jntI) in
  let _         = dump_expectations stderr "jntI2" (snd jntI) in
  let _         = dump_expectations stderr "jntO1" (fst jntO) in
  let _         = dump_expectations stderr "jntO2" (snd jntO) in
    (* update pis *)
  let pi'   = bounded (mstep_pi mrgI lPSIG lPSII 0  exI) in
  let pib'  = bounded (mstep_pi mrgO lPSIG lPSIO _N exO) in
  let pi'   = 0.5 in let pib' = 0.5 in 
    (* update phis *)
  let phiI' = mstep_phi_nongen_binary iD si2 exI in
  let phiO' = mstep_phi_nongen_binary oD si2 exO in
  let phiG' = mstep_phi_gen_binary    iD oD si2 exI exO in
  let _     = p.phi <- { phiI = phiI' ; phiO = phiO' ; phiG = phiG' } in
    (* update psis *)
(*  let ()    = mstep_psi_nongen_beta00 iD jntI p.psi.psiI lPSII pi'  exI in
  let ()    = mstep_psi_nongen_beta00 oD jntO p.psi.psiO lPSIO pib' exO in 
  let ()    = mstep_psi_gen_beta00    iD oD jntI jntO p.psi.psiG lPSIG pi' pib' exI exO in *)
  let lambda = 1. in
  let ()    = mstep_psi_nongen_gaussian iD jntI p.psi.psiI lPSII pi'  exI lambda in
  let ()    = mstep_psi_nongen_gaussian oD jntO p.psi.psiO lPSIO pib' exO lambda in
  let ()    = mstep_psi_gen_gaussian    iD oD jntI jntO p.psi.psiG lPSIG pi' pib' exI exO lambda in
    (* and update p *)
  let ()    =  p.pi <- pi' ; p.pib <- pib'  in
    (* compute error on in-domain data *)
  let errI  = compute_error p.psi.psiG p.psi.psiI pi'  p.phi.phiG.(0) p.phi.phiI.(0) iD in
  let errO  = compute_error p.psi.psiG p.psi.psiO pib' p.phi.phiG.(0) p.phi.phiO.(0) oD in
    Printf.fprintf stderr "ID  error: %g\n" errI;
    Printf.fprintf stderr "OOD error: %g\n" errO;
    flush stderr;
    ()

(*
  example data is a binary classification task.
    feature 1 is a perfect predictor for p^o data
    feature 2 is a perfect predictor for p^i data
    feature 3 is a perfect predictor for p^g data
    feature 4 is an irrelevant feature
    feature 5 is a general negative indicator
  each data point can be perfectly classified, but
  not necessarily with a particular feature
*)
let (iDy,iDx) as iD =
  [| 1         ; 1         ; 1       ; 0       ; 0       ; 0       |],
  [| [| 0;2;3;4 |] ; [| 0;3;4 |] ; [| 0;2 |] ; [| 0;4;5 |] ; [| 0;5 |] ; [| 0;5 |] |]
let (oDy,oDx) as oD =
  [| 1         ; 1         ; 1       ; 0       ; 0       ; 0       |],
  [| [| 0;1;3;4 |] ; [| 0;3;4 |] ; [| 0;1 |] ; [| 0;4;5 |] ; [| 0;5 |] ; [| 0;5 |] |]

let p = initialize_parameters iD oD
let _ = dump_params stderr p
let lPSII, lPSIO, lPSIG = initialize_logPSIs iD oD p
let si2 = 1.

let _ =
  for iter=1 to 10 do
    Printf.fprintf stderr "\n\n================= ITERATION %d =================\n\n" iter;
    single_iteration iD oD p lPSII lPSIO lPSIG si2;
    dump_params stderr p
  done;
  ()

