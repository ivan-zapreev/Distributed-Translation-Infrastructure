open Array

let computeBeta gold uold g =
  let _J = length g in
  let rec computeNumerator j acc =
    if j >= _J then acc else
      computeNumerator (j+1) (acc +. g.(j) *. (g.(j) -. gold.(j))) in
  let rec computeDenominator j acc =
    if j >= _J then acc else
      computeDenominator (j+1) (acc +. uold.(j) *. (g.(j) -. gold.(j))) in
  let n = computeNumerator 0 0. in
  let d = computeDenominator 0 0. in
  let fabs x = if x >= 0. then x else -.x in
  let sign x = if x >= 0. then 1. else -1. in
    if fabs d <= 1e-10 then 1. else n /. d

(* for efficiency, this actually just *overwrites* uold with the new u *)
let computeU gold uold g =
  let beta = computeBeta gold uold g in
  let _J   = length g in
    for j = 0 to _J -1 do
      uold.(j) <- g.(j) -. uold.(j) *. beta;
    done;
    ()

let dotExample v x =
  let _P = length x in
  let rec dot p acc =
    if p >= _P || x.(p) >= length v then acc
    else dot (p+1) (acc +. v.(x.(p))) in
    dot 0 v.(0)

(* wtx = wtx + (wnew - wold) x
       = wtx + (wold + coeff * u - wold) x
       = wtx + (coeff * u) x *)
let updateWTX wtx (x : int array array) coeff u =
  let _N = length x in
  let _J = length u in
    for n = 0 to _N - 1 do
      let xi = x.(n) in
      let _P = length xi in
        for p = 0 to _P - 1 do
          if xi.(p) < length u then
            wtx.(n) <- wtx.(n) +. coeff *. u.(xi.(p));
        done;
        wtx.(n) <- wtx.(n) +. coeff *. u.(0);
    done;
    ()

let updateWeights lambda w x exWT xTst gold uold g wtx wtxTst =
  let _ = computeU gold uold g in  (* this replaces uold with u *)
  let _J = length gold in
  let _N = length x in
  let square x = x *. x in
  let sigma x = 1. /. (1. +. exp(-.x)) in
  let rec computeNumerator j acc = 
    if j >= _J then acc else
      computeNumerator (j+1) (acc +. g.(j) *. uold.(j)) in
  let rec computeDenominator1 j acc = 
    if j >= _J then acc else
      computeDenominator1 (j+1) (acc +. uold.(j) *. uold.(j)) in
  let rec computeDenominator2 n acc =
    if n >= _N then acc else
      computeDenominator2 (n+1) 
        (acc +. 
           exWT.(n) *.
           sigma wtx.(n) *.
           (1. -. sigma wtx.(n)) *.
           square (dotExample uold x.(n))) in
  let coeff = 
    (computeNumerator 0 0.) /. (lambda *. computeDenominator1 0 0. +. computeDenominator2 0 0.) in
  let dw = ref 0. in
    for j = 0 to _J - 1 do
      w.(j) <- w.(j) +. coeff *. uold.(j);
      dw := !dw +. square (coeff *. uold.(j));
    done;
    (coeff, !dw)

let computeGradient lambda w wtx y x exWT =
  let _J = length w in
  let _N = length x in
  let sigma x = 1. /. (1. +. exp(-.x)) in
  let g  = make _J 0. in
    for n = 0 to _N - 1 do
      let xi = x.(n) in
      let _P = length xi in
        g.(0) <- g.(0) +. (1. -. sigma(y.(n) *. wtx.(n))) *. y.(n) *. exWT.(n);
        for p = 0 to _P - 1 do
          let j = xi.(p) in
            g.(j) <- g.(j) +. (1. -. sigma(y.(n) *. wtx.(n))) *. y.(n) *. exWT.(n);
        done;
    done;
    if lambda > 0. then
      for j = 0 to _J - 1 do
        g.(j) <- g.(j) -. lambda *. w.(j);
      done;
    g

let singleIteration lambda w wtx wtxTst y x exWT xTst gold uold =
  let g = computeGradient lambda w wtx y x exWT in
    (* after the call to updateWeights, uold will contain u and w *)
  let (coeff, dw) = updateWeights lambda w x exWT xTst gold uold g wtx wtxTst in
  let _J = length g in
    updateWTX wtx    x    coeff uold;
    updateWTX wtxTst xTst coeff uold;
    (* now, update gold to g *)
    for j = 0 to _J - 1 do
      gold.(j) <- g.(j);
    done;
    dw

let initializeVectors x =
  let _N = length x in
  let rec findMaxFeature n mx =
    if n >= _N then mx else
      let xi = x.(n) in
      let _P = length xi in
      let rec findMaxFeature2 p mx =
        if p >= _P then mx else
          findMaxFeature2 (p+1) (if xi.(p) > mx then xi.(p) else mx) in
        findMaxFeature (n+1) (findMaxFeature2 0 mx) in
  let _J = 1 + findMaxFeature 0 1 in
  let w0   = make _J 0. in
  let wtx0 = make _N 0. in
  let g0   = make _J 0. in
  let u0   = make _J 0. in
    (w0, wtx0, g0, u0)

let computeError exWT wtx y =
  let _N = length wtx in
  let rec ce n acc =
    if n >= _N then acc else
      ce (n+1) (if y.(n) *. wtx.(n) >= 0. then acc else acc +. 1. +. 0. *. exWT.(n)) in
    ce 0 0.

let compute maxi lambda exWT x y =
  let (w, wtx, g, u) = initializeVectors x in
  let wtxTst = [||] in
  let _J = length w in
  let rec iter inum =
    if inum > maxi then () else (
      Printf.fprintf stderr "Iteration %d" inum;
      let dw = singleIteration lambda w wtx [||] y x exWT [||] g u in
      let er = computeError exWT wtx y in
        Printf.fprintf stderr "\tdw=%g" dw;
        Printf.fprintf stderr "\ter=%g" er;
        Printf.fprintf stderr "\n";
        iter (if dw > 1e-10 then (inum+1) else (maxi+1))
    ) in
    iter 1;
    w
