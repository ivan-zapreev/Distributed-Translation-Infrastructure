open Array

let round_toward_zero x = if x < 0. then ceil x else floor x

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


(* with two newton steps, at 0.01, we get: -2.32634787404084076
   with one newton step,           we get: -2.32634787404083632
   with no  newton steps,          we get: -2.32634793130187223

   clearly, two is unnecessary.  none is probably fine.
*)
      
let bns p1 p2 =
  let f1 = if p1 <= 1e-10 then -6.3614663 else if p1 >= 1.-.1e-10 then 6.3614663 else normal_cdf_inverse p1 in
  let f2 = if p2 <= 1e-10 then -6.3614663 else if p2 >= 1.-.1e-10 then 6.3614663 else normal_cdf_inverse p2 in
    abs_float (f1 -. f2)


(* main function *)
let _ =
  let rec rd () =
    match try Some (input_line stdin) with End_of_file -> None with
        None -> ()
      | Some l ->
          ((match Str.split (Str.regexp "[ \t]+") l with
                [p1;p2] -> Printf.printf "%g\n" (bns (float_of_string p1) (float_of_string p2))
              | _ -> failwith ("invalid line: " ^ l));
           rd ()) in
    rd ();
    ()

    
