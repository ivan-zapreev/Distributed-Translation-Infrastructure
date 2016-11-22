open Printf
open Array
open Data
open Util
open Str

module A = Arry
module H = Hashtbl

type kernel = Linear | Poly of float | RBF of float
type kernel_cache = { diag : bigarr ; diaghit : bigarr ; hash : (int*int,float) H.t }

let or_empty  x n   = if n >= A.length x then efa else A.get x (n)
let sqr x = x *. x

let new_cache _N = { diag = make_ba _N 0. ; diaghit = make_ba _N (-1.) ; hash = H.create _N } 

let most_distant_point _N k a =
  let b = ref (-1) in let d = ref neg_infinity in
    for n = 0 to _N - 1 do
      if n != a then (
        let dd = sqr (k n n) +. sqr (k a a) -. 2. *. k n a in
          if dd > !d then ( 
            d := dd ; 
            b := n ;
          );
      );
    done;
    !b
      
let most_distant_point_class ok _N k a =
  let b = ref (-1) in 
  let d = ref neg_infinity in
    H.iter (fun _ n ->
              if n != a then (
                let dd = k n n -. 2. *. k n a in
                  if dd > !d then ( d := dd ; b := n );
              )
           ) ok;
    !b

let select_uar _N _ = 
  let a = Random.int _N in
  let b = Random.int (_N - 1) in
    (a, if b >= a then b + 1 else b)

let select_dist _N k =
  let a  = ref (Random.int _N) in
  let b  = ref !a in
    for i=1 to 4 do
      b := most_distant_point _N k !a;
      a := most_distant_point _N k !b;
    done;
    (!a, !b)

let rec select_uar_class  y _N k = 
  let a  = Random.int _N in
  let ya = A.get y a in
  let ok = H.create 50 in
  let c  = ref 0 in
    for n = 0 to _N - 1 do
      if n != a && A.get y n != ya then (
        H.replace ok !c n;
        incr c
      );
    done;
    if !c == 0 then select_uar_class y _N k
    else (a, H.find ok (Random.int !c))

let rec select_dist_class y _N k =
  let a  = ref (Random.int _N) in
  let ya = A.get y !a in
  let okB = H.create 50 in
  let c  = ref 0 in
    for n = 0 to _N - 1 do
      if n != !a && A.get y n != ya then (
        H.replace okB !c n;
        incr c
      );
    done;
    if !c == 0 then select_dist_class y _N k
    else (
      (!a, most_distant_point_class okB !c k !a)
(*
      let b = ref !a in
        for i=1 to 4 do
          b := most_distant_point_class okB !c k !a;
          a := most_distant_point_class okB !c k !b;
        done;
        (!a, !b) *)
    )

let dot xi xv zi zv =
  let _I = A1.dim xi in let _J = A1.dim zi in
  let i  = ref 0     in let j  = ref 0     in
  let v  = ref 0. in
    while !i < _I && !j < _J do
      if xi.{!i} == zi.{!j} then (
        if A1.dim xv == 0 
        then v := !v +. 1.
        else v := !v +. xv.{!i} *. zv.{!j};
        incr i; incr j;
      ) else if xi.{!i} < zi.{!j} then incr i
      else incr j;
    done;
    !v

let dist xi xv zi zv =
  let _I = A1.dim xi in let _J = A1.dim zi in
  let i  = ref 0     in let j  = ref 0     in
  let v  = ref 0. in
    while !i < _I && !j < _J do
      if xi.{!i} == zi.{!j} then (
        if A1.dim xv != 0 
        then v := !v +. sqr (xv.{!i} -. zv.{!j});
        incr i; incr j;
      ) else if xi.{!i} < zi.{!j} then (
        if A1.dim xv == 0
        then v := !v +. 1.
        else v := !v +. sqr xv.{!i};
        incr i;
      ) else (
        if A1.dim xv == 0
        then v := !v +. 1.
        else v := !v +. sqr zv.{!j};
        incr j;
      );
    done;
    while !i < _I do 
      if A1.dim xv == 0 then v := !v +. 1. else v := !v +. sqr xv.{!i};
      incr i;
    done;
    while !j < _J do 
      if A1.dim xv == 0 then v := !v +. 1. else v := !v +. sqr zv.{!j};
      incr j;
    done;
    !v


let compute_kernel0 k xi xv zi zv =
  match k with
      Linear -> dot xi xv zi zv
    | Poly d -> (1. +. dot xi xv zi zv) ** d
    | RBF  g -> exp(0. -. g *. dist xi xv zi xv)

let compute_kernel kc k n m xni xnv xmi xmv =
  if n == m then (
    if kc.diaghit.{n} > 0. then kc.diag.{n}
    else ( 
      let kv = compute_kernel0 k xni xnv xmi xmv in
        kc.diaghit.{n} <- 1. ;
        kc.diag.{n}    <- kv ;
        kv
    )
  ) else 
    let a = min n m in let b = max n m in
      try  H.find kc.hash (a,b)
      with Not_found -> (
        let kv = compute_kernel0 k xni xnv xmi xmv in
(*          H.replace kc.hash (a,b) kv; *)
          kv
      )

let kernelmap byDist byClass kernel _D x y =
  if not x.datav_implicit then failwith "cannot run kernelmap on explicit data";
  if     x.datav_sampled  then failwith "cannot run kernelmap on sampled data";

  let select = 
    match (byDist,byClass) with
      (false,false) -> select_uar
    | (true ,false) -> select_dist
    | (false,true ) -> select_uar_class y
    | (true ,true ) -> select_dist_class y in
  
  let _N = dat_N x in

  (* create the new data -- must be non-bernoulli *)
  let xp = { datav_bernoulli = false ; datav_implicit = true ; datav_sampled = false ; datav_numclass = x.datav_numclass ; datav_numfeat = _D+1 ; datav_task = x.datav_task ; datav_weights = x.datav_weights ;
             datav_xi = A.init _N (fun _ -> init_ia _D (fun d -> d+1)) ; datav_xi_c = A.empty [||] ;
             datav_xv = A.init _N (fun _ -> make_ba _D 0.)             ; datav_xv_c = A.empty [||] } in
  (* and store the source examples *)
  let kmap = make _D (-1,-1,0.) in

  let kc = new_cache _N in
  let k0 n m xn xm = compute_kernel kc kernel n m xn xm in
  let k  d0 n m = 
    let xni = A.get x.datav_xi n in let xnv = or_empty x.datav_xv n in
    let xmi = A.get x.datav_xi m in let xmv = or_empty x.datav_xv m in

    let xnp = A.get xp.datav_xv n in
    let xmp = A.get xp.datav_xv m in

    let v  = ref (k0 n m xni xnv xmi xmv) in

      for d = 0 to d0-1 do
        v := !v -. xnp.{d} *. xmp.{d};
      done;
      !v in

    for d = 0 to _D - 1 do
      if (d+1) mod (_D / 20) == 0 then ( eprintf "."; flush stderr );
      (* select two points *)
      let (a,b) = select _N (k d) in

      (* compute the projection and store *)
      let c = sqrt(abs_float(k d a a +. k d b b -. 2. *. k d a b)) in

        kmap.(d) <- (a,b,c);
        for n = 0 to _N - 1 do
          (A.get xp.datav_xv n).{d} <- (1./.c) *. (k d n a -. k d n b);
        done;
    done;

    ((kernel,kmap), xp)

let applymap1 (kernel, kmap) xTr xtrp x v =
  let _N0 = dat_N xTr in
  let _D  = length kmap in
  let xpi = init_ia _D (fun d -> d+1) in
  let xpv = make_ba _D 0. in
  let kc  = new_cache (_N0+1) in
  let k0 n m xn xm = compute_kernel kc kernel n m xn xm in
  let k  d0 m =  (* n from x, m from xTr *)
    let xmi = A.get xTr.datav_xi m in let xmv = or_empty xTr.datav_xv m in
    let xmp = A.get xtrp.datav_xv m in

    let v  = ref (k0 _N0 m x v xmi xmv) in
      for d = 0 to d0-1 do
        v := !v -. xpv.{d} *. xmp.{d};
      done;
      !v in

    for d = 0 to _D - 1 do
      let (a,b,c) = kmap.(d) in
        xpv.{d} <- (1./.c) *. (k d a -. k d b);
    done;

    (xpi, xpv)


let applymap (kernel, kmap) xTr xtrp x =
  if not x.datav_implicit then failwith "cannot run kernelmap on explicit data";
  if     x.datav_sampled  then failwith "cannot run kernelmap on sampled data";

  let _N0= dat_N xTr in
  let _N = dat_N x in
  let _D = length kmap in
  let xp = { datav_bernoulli = false ; datav_implicit = true ; datav_sampled = false ; datav_numclass = x.datav_numclass ; datav_numfeat = _D+1 ; datav_task = x.datav_task ; datav_weights = x.datav_weights ;
             datav_xi = A.init _N (fun _ -> init_ia _D (fun d -> d+1)) ; datav_xi_c = A.empty [||] ;
             datav_xv = A.init _N (fun _ -> make_ba _D 0.)             ; datav_xv_c = A.empty [||] } in

  let kc = new_cache (_N+_N0) in
  let k0 n m xn xm = compute_kernel kc kernel n m xn xm in
  let k  d0 n m =  (* n from x, m from xTr *)
    let xni = A.get x.datav_xi n in let xnv = or_empty x.datav_xv n in
    let xmi = A.get xTr.datav_xi m in let xmv = or_empty xTr.datav_xv m in

    let xnp = A.get xp.datav_xv n in
    let xmp = A.get xtrp.datav_xv m in

    let v  = ref (k0 (_N0+n) m xni xnv xmi xmv) in
      for d = 0 to d0-1 do
        v := !v -. xnp.{d} *. xmp.{d};
      done;
      !v in

    eprintf ":";
    for d = 0 to _D - 1 do
      if (d+1) mod (_D / 20) == 0 then ( eprintf "."; flush stderr );
      let (a,b,c) = kmap.(d) in
        for n = 0 to _N - 1 do
          (A.get xp.datav_xv n).{d} <- (1./.c) *. (k d n a -. k d n b);
        done;
    done;

    xp
    
    
let printmap (kernel, kmap) xTr xtrp =
  let _D = length kmap in
  let usedX = H.create 5 in
  let usedS = ref 0 in

    for d = 0 to _D - 1 do
      let (a,b,c) = kmap.(d) in
        if not (H.mem usedX a) then ( H.replace usedX a !usedS; incr usedS );
        if not (H.mem usedX b) then ( H.replace usedX b !usedS; incr usedS );
    done;

    printf "***KERNELMAP*** %d %d %d " _D xTr.datav_numfeat !usedS;
    (match kernel with
         Linear -> printf "Linear"
       | Poly d -> printf "Poly %g" d
       | RBF  g -> printf "RBF %g" g);

    for d = 0 to _D - 1 do
      let (a,b,c) = kmap.(d) in
        printf " %d %d %g" (H.find usedX a) (H.find usedX b) c;
    done;
    printf "\n";

  let ids = make !usedS (-1) in
    H.iter (fun n i -> ids.(i) <- n) usedX;

    for i = 0 to !usedS - 1 do
      let pv = A.get xtrp.datav_xv ids.(i) in
        for d = 0 to _D - 1 do
          if d > 0 then printf " ";
          printf "%g" pv.{d};
        done;
        printf "\n";

      let xi = A.get xTr.datav_xi ids.(i) in
      let xv = or_empty xTr.datav_xv ids.(i) in
        for j = 0 to A1.dim xi - 1 do
          if j > 0 then printf " ";
          printf "%s %g" (get_vocab_str (I.to_int xi.{j})) (if j < A1.dim xv then xv.{j} else 1.);
        done;
        printf "\n";
    done;
    ()

let readkernelmap h str = 
  let mrl () = try Some (input_line h) with End_of_file -> None in
    
  let kernel, _D, _D0, _M, kmap_list = 
    match split space_re str with
        (_::_Ds::_D0s::_Ms::"Linear"  ::kmap_list) -> (Linear, int_of_string _Ds, int_of_string _D0s, int_of_string _Ms, kmap_list)
      | (_::_Ds::_D0s::_Ms::"Poly"::kp::kmap_list) -> (Poly (float_of_string kp), int_of_string _Ds, int_of_string _D0s, int_of_string _Ms, kmap_list)
      | (_::_Ds::_D0s::_Ms::"RBF" ::kp::kmap_list) -> (RBF  (float_of_string kp), int_of_string _Ds, int_of_string _D0s, int_of_string _Ms, kmap_list)
      | _ -> failwith ("invalid kernelmap line: " ^ str) in

  let kmap = make _D (0,0,-1.) in
  let rec rd_kmap i = function
      (a::b::c::xs) -> 
        kmap.(i) <- (int_of_string a, int_of_string b, float_of_string c);
        rd_kmap (i+1) xs
    | _ -> () in

    rd_kmap 0 kmap_list;

  let x  = { datav_bernoulli = false ; datav_implicit = true ; datav_sampled = false ; datav_numclass = 1 ; datav_numfeat = _D0+1 ; datav_task = A.empty 0 ; datav_weights = make_ba 0 0. ;
             datav_xi = A.init _M (fun _ -> eia) ; datav_xi_c = A.empty [||] ;
             datav_xv = A.init _M (fun _ -> efa) ; datav_xv_c = A.empty [||] } in

  let xp = { datav_bernoulli = false ; datav_implicit = true ; datav_sampled = false ; datav_numclass = 1 ; datav_numfeat = _D +1 ; datav_task = A.empty 0 ; datav_weights = make_ba 0 0. ;
             datav_xi = A.init _M (fun _ -> init_ia _D (fun d -> d+1)) ; datav_xi_c = A.empty [||] ;
             datav_xv = A.init _M (fun _ -> make_ba _D 0.)             ; datav_xv_c = A.empty [||] } in
          
  let int_line () =
    match mrl () with None -> failwith "not enough lines in kernelmap" | Some l ->
      A1.of_array Bigarray.float32 Bigarray.c_layout (of_list (List.map float_of_string (split space_re l))) in

    for m = 0 to _M - 1 do
      A.set xp.datav_xv m (int_line ());
      match mrl () with
          None -> failwith "not enough lines in kernelmap"
        | Some s ->
            let a  = of_list (split space_re s) in
            let dd = length a / 2 in
            let xi = make_ia dd 0  in
            let xv = make_ba dd 0. in
              for i = 0 to (length a/2) - 1 do
                xi.{i} <- I.of_int (get_vocab a.(2*i));
                xv.{i} <- float_of_string a.(2*i+1);
              done;
              A.set x.datav_xi m xi;
              A.set x.datav_xv m xv;
    done;
    sort_by_xi x;
    ((kernel, kmap), x, xp)
