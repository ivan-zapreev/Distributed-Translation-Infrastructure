open Bigarray

type a  = (float, float32_elt, c_layout) Array1.t
type ai = (int32, int32_elt  , c_layout) Array1.t


external mult_dense_sparse : a -> ai -> int -> float = "c_mult_dense_sparse"
external mult_dense_sparse_val : a -> ai -> a -> int -> float = "c_mult_dense_sparse_val"
external norm_dense : a -> float = "c_norm_dense"
external mult_dense_dense : a -> a -> float = "c_mult_dense_dense"
external mult_dense : a -> float -> unit = "c_mult_dense"
external add_dense_dense : a -> a -> float -> float -> unit = "c_add_dense_dense"
external add_dense_sparse : a -> ai -> float -> int -> unit = "c_add_dense_sparse"
external add_dense_sparse_val : a -> ai -> a -> float -> int -> unit = "c_add_dense_sparse_val"
