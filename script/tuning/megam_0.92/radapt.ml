open Util
open Array
open Printf

let compute_kl _N is_out cprob alpha beta =
  let numer = ref 0. in
  let denom = ref 0. in
    for n = 0 to _N - 1 do
      if is_out n then (
        let l = 0. -. log (cprob n) in
        let dots = beta *. l in (*  +. if is_out n then 0. else alpha in *)
          numer := !numer +. dots *. exp dots;
          denom := !denom +. exp dots;
      );
    done;
    !numer *. (log !denom) /. !denom

let search_for_beta _N is_out cprob alpha eps =
(*  let ckl = compute_kl _N is_out cprob alpha in *)
  let ckl b = let kl = compute_kl _N is_out cprob alpha b in eprintf "[%g %g] " b kl; kl in

  let rec bin_search numi lo lokl hi hikl =
    if hikl < eps then bin_search numi hi hikl (hi*.2.) (ckl (hi*.2.))
    else if numi > 10 then lo
    else
      let mi   = (hi+.lo) /. 2. in
      let mikl = ckl mi in
        if mikl > eps then bin_search (numi+1) lo lokl mi mikl
        else               bin_search (numi+1) mi mikl hi hikl in

    bin_search 0 0. (ckl 0.) 2. (ckl 2.)

let alpha = ref 5.

let radapt 
  (_N : int)                    (* # of training points *)
  (is_out : int -> bool)        (* is the nth training point out of domain? *)
  (train : bigarr -> 'a)        (* i give you weights; you give me a model *)
  (cprob : 'a -> int -> float)  (* i give you a model and an example, you give me the probability of the
                                   *true* class under that model *)
  (maxI  : int)                 (* how many iterations *)
  (eps    : float)
  (lambda : float)              (* step size *)
  : 'a =
  let num_out = ref 0 in
  let q = make_ba _N 1. in
  let cur_model = ref None in
  let qnew = make_ba _N 1. in
    for inum = 1 to maxI do
      eprintf "\n\n=========== RADAPT ITERATION %d ===========\n\n" inum; flush stderr;

      let m    = train q in
      let beta = search_for_beta _N is_out (cprob m) !alpha eps in

      let sum  = ref 0. in
      let maxb = ref 0. in
        for n = 0 to _N - 1 do
          let p = cprob m n in
          let l = 0. -. log p in
          let v = exp (if is_out n then beta *. l else !alpha) in
            qnew.{n} <- v;
            maxb := max !maxb (beta *. l);
            sum := !sum +. v;
        done;
        eprintf "{%g}" !maxb;
        sum :=  float_of_int _N /. !sum;
        for n = 0 to _N - 1 do
          q.{n} <- lambda *. q.{n} +. (1. -. lambda) *. qnew.{n} *. !sum;
        done;
      
        cur_model := Some m;
    done;
    alpha := !alpha /. 2.;

    (match !cur_model with None -> failwith "radapt: None" | Some m -> m)



(* | trim | cut -d' ' -f12,16 | sort -nur | gawk 'length > 0' | tail -1 *)
