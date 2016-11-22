module A = Array

type 'a arry = 
  { mutable size: int
  ; mutable data: 'a array array
  }

let _MAXSZ = 1048576 (* == 2 lsl 19 *)
let _MAXSZm1 = 1048575 (* == 2 lsl 19 - 1 *)
let _MAXDV = 19
let _MAXDVp1 = 20

(* basics *)

let empty (v : 'a) : 'a arry = { size = 0; data = [||] }

let length (a : 'a arry) = a.size

let get (a : 'a arry) (i:int) = A.get (a.data.(i lsr _MAXDVp1)) (i land _MAXSZm1)

let set (a : 'a arry) (i:int) (v:'a) = A.set (a.data.(i lsr _MAXDVp1)) (i land _MAXSZm1) v

let make (i:int) (v:'a) : 'a arry = 
  let ii = 1 + (i lsr _MAXDVp1) in
    { size = i
    ; data = A.init ii (fun j -> A.make (if j<ii-1 then _MAXSZ else ((i land _MAXSZm1))) v)
    }

let init (i:int) (f : int->'a) : 'a arry =
  let ii = 1 + (i lsr _MAXDVp1) in
    { size = i
    ; data = A.init ii (fun j -> A.init (if j<ii-1 then _MAXSZ else ((i land _MAXSZm1)))
			                (fun k -> f ((j lsl _MAXDVp1) + k)))
    }
let iter (f:'a -> unit) (a : 'a arry) = A.iter (A.iter f) a.data

let iteri (f:int -> 'a -> unit) (a : 'a arry) = 
  A.iteri (fun j arr -> A.iteri (fun k v -> f ((j lsl _MAXDVp1)+k) v) arr) a.data

(* derived *)

let create = make

let map  (f : 'a -> 'b) (a : 'a arry) : 'b arry = init (a.size) (fun i -> f (get a i))

let mapi (f : int -> 'a -> 'b) (a : 'a arry) : 'b arry = init (a.size) (fun i -> f i (get a i))

let fold_left (f : 'a -> 'b -> 'a) (z0 : 'a) (a : 'b arry) =
  A.fold_left (A.fold_left f) z0 a.data

let fold_right (f : 'a -> 'b -> 'b) (a : 'b arry) (z0 : 'a) =
  A.fold_right (A.fold_right f) a.data z0

let copy arry = map (fun a -> a) arry

let concat (al : 'a arry list) : 'a arry =
  if List.length al == 1 then List.hd al else
    let sz = List.fold_right (fun a z -> z + a.size) al 0 in
    let z0 = get (List.hd al) 0 in
    let ar = make sz z0 in
    let rec fillit ii = function
        [] -> ()
      | (a::ax) ->
          iteri (fun j v -> set ar (ii+j) v) a;
          fillit (ii + a.size) ax in
      fillit 0 al;
      ar

let transpose d =
  let _N = length d in
  let _D = 1 + fold_left (A.fold_left max) 0 d in
  let ht = A.init _D (fun _ -> IntHashtbl.create 5) in
    for n = 0 to _N - 1 do
      A.iter (fun d -> IntHashtbl.replace ht.(d) n ()) (get d (n));
    done;
    ht
