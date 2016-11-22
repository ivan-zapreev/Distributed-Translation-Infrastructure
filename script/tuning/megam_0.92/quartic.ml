module C = Complex

let solve_quartic_std c3 c2 c1 c0 =
  let cube x = x *. x *. x in
  let cbrt x =
    if x < 0. then 0. -. (0.-.x) ** (1./.3.) else x ** (1./.3.) in
  let u = (2.*.c2*.c2*.c2 -. 9.*.c2*.(c1*.c3-.4.*.c0) -. 27.*.(4.*.c2*.c0-.c1*.c1-.c3*.c3*.c0)) /. 54. in
  let v = u*.u +. cube((c1*.c3-.4.*.c0)/.3. -. c2*.c2/.9.) in
  let c x = { C.re = x ; C.im = 0. } in
  let y   = 
    if v >= 0.
    then
      let s = cbrt (u +. sqrt v) in
      let t = cbrt (u -. sqrt v) in
        (c2 /. 3.) +. s +. t
    else (* v <= 0, need to use complex numbers *)
      let s   = C.pow (C.add (c u) (C.sqrt (c v))) (c (1./.3.)) in
      let t   = C.pow (C.sub (c u) (C.sqrt (c v))) (c (1./.3.)) in
      let spt = C.add s t in
      let smt = C.sub s t in
      let isZero x = 
        match classify_float x with
            FP_zero -> true
          | _ -> false in
      let y0 = C.add (c (c2/.3.)) spt in
      let y1 = C.add (C.add (c (c2/.3.)) (C.mul (c 0.5) spt))
                     (C.mul { C.re = 0. ; C.im = (sqrt 3.) /. 2. } smt) in
      let y2 = C.sub (C.add (c (c2/.3.)) (C.mul (c 0.5) spt))
                     (C.mul { C.re = 0. ; C.im = (sqrt 3.) /. 2. } smt) in
        if isZero y0.C.im then y0.C.re
        else if isZero y1.C.im then y1.C.re
        else if isZero y2.C.im then y2.C.re
        else failwith "no real solutions"  in
  let r   = sqrt (c3 *. c3 /. 4. -. c2 +. y) in
  let d,e =
    if r == 0.
    then
      0.75*.c3*.c3 -. 2.*.c2 +. 2.*.sqrt(y*.y -. 4.*.c0),
      0.75*.c3*.c3 -. 2.*.c2 -. 2.*.sqrt(y*.y -. 4.*.c0)
    else
      0.75*.c3*.c3 -. r*.r -.2.*.c2 +. (4.*.c3*.c2 -. 8.*.c1 -. c3*.c3*.c3) /. (4.*.r),
      0.75*.c3*.c3 -. r*.r -.2.*.c2 -. (4.*.c3*.c2 -. 8.*.c1 -. c3*.c3*.c3) /. (4.*.r) in
  let _ = Printf.fprintf stderr "y=%g r=%g d=%g e=%g\n" y r d e ; flush stderr in
    if d >= 0. && e >= 0.
    then [ 0. -. c3/.4. +. r/.2. +. (sqrt d)/.2. ;
           0. -. c3/.4. +. r/.2. -. (sqrt d)/.2. ;
           0. -. c3/.4. +. r/.2. +. (sqrt e)/.2. ;
           0. -. c3/.4. +. r/.2. -. (sqrt e)/.2. ]
    else if d >= 0.
    then [ 0. -. c3/.4. +. r/.2. +. (sqrt d)/.2. ;
           0. -. c3/.4. +. r/.2. -. (sqrt d)/.2. ]
    else if e >= 0.
    then [ 0. -. c3/.4. +. r/.2. +. (sqrt e)/.2. ;
           0. -. c3/.4. +. r/.2. -. (sqrt e)/.2. ]
    else []
