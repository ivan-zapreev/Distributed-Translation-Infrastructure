open Array

type bitvec = { bv_len : int ; bv_arr : int array }

let bit_j = Array.init 30 (fun j -> 1 lsl j)
let bit_not_j = Array.init 30 (fun j -> max_int - bit_j.(j))




let create len = { bv_len = len ; bv_arr = make ((len+29)/30) 0 }

let unsafe_is_on b pos =
  (unsafe_get b.bv_arr (pos/30)) land (unsafe_get bit_j (pos mod 30)) > 0

let is_on b pos =
  if pos < 0 || pos >= b.bv_len then invalid_arg "Bitvec.is_on" else unsafe_is_on b pos

let unsafe_turn_on b pos =
  unsafe_set b.bv_arr (pos / 30) ((unsafe_get b.bv_arr (pos / 30)) lor (unsafe_get bit_j (pos mod 30)))

let turn_on b pos =
  if pos < 0 || pos >= b.bv_len then invalid_arg "Bitvec.turn_on" else unsafe_turn_on b pos

let unsafe_turn_off b pos =
  unsafe_set b.bv_arr (pos / 30) ((unsafe_get b.bv_arr (pos / 30)) land (unsafe_get bit_not_j (pos mod 30)))

let turn_off b pos =
  if pos < 0 || pos >= b.bv_len then invalid_arg "Bitvec.turn_off" else unsafe_turn_off b pos

let clear b = fill b.bv_arr 0 (length b.bv_arr) 0
