type extLog = { el_belowhalf : bool; el_value : float }
(* for x < 0.5, we get { true, log x }; for x >= 0.5, we get { false, log (1-x) } *)

let zeroLog  = log 0.
let oneLog   = log 1.
let addLog (x:float) (y:float) = 
  if x == zero then y
  else if y == zero then x
  else if x -. y > 32. then x
  else if x > y then (x +. log (1. +. exp (y -. x)))
  else if y -. x > 32. then y
  else (y +. log (1. +. exp (x -. y)))
let subLog (x:float) (y:float) = 
  if y == zero then x
  else if x <= y then zero
  else if x -. y > 32. then x
  else (x +. log (1. -. exp (y -. x)))

let timesEL x y =
  if x.el_belowhalf
  then
    if y.el_belowhalf
    then { el_belowhalf = true; el_value = x.el_value +. y.el_value }
    else
      let v = subLog x.el_value (x.el_value +. y.el_value) in
        if v < log 0.5
        then { el_belowhalf = true;  el_value = v }
        else { el_belowhalf = false; el_value = subLog (addLog oneLog (x.el_value +. y.el_value)) x.el_value }
  else
    if y.el_belowhalf
    then 
      let v = subLog y.el_value (x.el_value +. y.el_value) in
        if v < log 0.5
        then { el_belowhalf = true;  el_value = v }
        else { el_belowhalf = false; el_value = subLog (addLog oneLog (x.el_value +. y.el_value)) y.el_value }
    else
      let v = subLog (addLog one (x.el_value +. y.el_value)) (addLog x.el_value y.el_value) in
        if v < log 0.5
        then { el_belowhalf = true;  el_value = v }
        else { el_belowhalf = false; el_value = subLog (addLog x.el_value y.el_value) (x.el_value +. y.el_value) }
