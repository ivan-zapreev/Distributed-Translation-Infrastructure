open Sys
open Array
open Util
open Data

let usage_text = "usage: megam [options] <model-type> <input-file>
[options] are any of:
   -filespec        treat <input-file> as a filespec, not a normal
                    input file

   -fvals           data is in not in bernoulli format (i.e. feature
                    values appear next to their features int the file)

   -explicit        this specifies that each class gets its own feature
                    vector explicitely, rather than using the same one
                    independently of class
                    (only valid for multiclass problems)

   -quiet           don't generate per-iteration output

   -maxi <int>      specify the maximum number of iterations
                    (default: 100 for maxent, 20 for perceptron)

   -dpp <float>     specify the minimum change in perplexity
                    (default: -99999)

   -memory <int>    specify the memory size for LM-BFGS (multiclass only)
                    (default: 5)

   -lambda <float>  specify the precision of the Gaussian prior for maxent;
                    or the value for C for passive-aggressive algorithms
                    (default: 1)

   -tune            tune lambda using repeated optimizations (starts with
                    specified -lambda value and drops by half each time
                    until optimal dev error rate is achieved)

   -sprog <prog>    for density estimation problems, specify the program 
                    that will generate samples for us (see also -sfile)

   -sfile <files>   for de problems, instead of -sprog, just read from a
                    (set of) file(s); specify as file1:file2:...:fileN

   -sargs <string>  set the arguments for -sprog; default \"\"

   -sspec <i,i,i>   set the <burn-in time, number of samples, sample space>
                    parameters; default: 1000,500,50

   -sforcef <file>  include features listed in <file> in feature vectors
                    (even if they don't exist in the training data)

   -predict <file>  load parameters from <file> and do prediction
                    (will not optimize a model)

   -mean <file>     the Gaussian prior typically assumes mu=0 for all features;
                    you can instead list means in <file> in the same format
                    as is output by this program (baseline adaptation)

   -init <file>     initialized weights as in <file>

   -minfc <int>     remove all features with frequency <= <int>

   -bnsfs <int>     keep only <int> features by the BNS selection criteria

   -abffs <int>     use approximate Bayes factor feature selection; add features
                    in batches of (at most) <int> size

   -curve <spec>    produce a learning curve, where spec = \"min,step\"
                    and we start with min examples and increase (multiply!)
                    by step each time; eg: -curve 2,2

   -nobias          do not use the bias features

   -repeat <int>    repeat optimization <int> times (sometimes useful because
                    bfgs thinks it converges before it actually does)

   -lastweight      if there is DEV data, we will by default output the best
                    weight vector; use -lastweight to get the last one

   -multilabel      for multiclass problems, optimize a weighted multiclass
                    problem; labels should be of the form \"c1:c2:c3:...:cN\"
                    where there are N classes and ci is the cost for
                    predicting class i... if ci is 'x' then it forbids this
                    class from being correct (even during test)

   -bitvec          optimize bit vectors; implies multilabel input format,
                    but costs must be 0, 1 or 'x'

   -nc              use named classes (rather than numbered) for multi{class,tron};
                    incompatible with -multilabel

   -pa              perform passive-aggressive updates (should be used only
                    with perceptron or multitron inference; also see -lambda)

   -kernel <spec>   perform kernel mapping; <spec> should be one of:
                    '#:linear', '#:poly:#', or '#:rbf:#'; the first # should
                    be the desired dimensionality; the # for poly
                    is the degree and the # for rbf is the width of the
                    Gaussian.  Any of these options can be followed by
                    ':dist' (to select by distance) or ':class' (to select
                    by class)

   -norm[1|2]       l1 (or l2) normalization on instances

<model-type> is one of:
   binary           this is a binary classification problem; classes
                    are determined at a threshold of 0.5 (anything
                    less is negative class, anything greater is positive)

   perceptron       binary classification with averaged perceptron

   multitron        multiclass classification with averaged perceptron

   binomial         this is a binomial problem; all values should be
                    in the range [0,1]

   multiclass       this is a multiclass problem; classes should be
                    numbered [0, 1, 2, ...]; anything < 0 is mapped
                    to class 0

   density          this is a density estimation problem and thus the
                    partition function must be calculated through samples
                    (must use -sprog or -sfile arguments, above)
"

let arg_error str = 
  Printf.fprintf stderr "%s\n" usage_text;
  failwith ("Error: " ^ str)

let l = length argv
let _ = if l < 3 then arg_error "not enough arguments"
let fspec = ref false
let named = ref false
let ber = ref true
let imp = ref true
let maxi = ref (-1)
let dpp  = ref (-99999.)
let memory = ref 5
let lambda = ref 1.
let pred_file = ref None
let mean_file = ref None
let quiet = ref false
let bias  = ref true
let init_file = ref None
let sprog = ref ""
let fsel : fsel ref = ref NoFsel
let sfile = ref ""
let sargs = ref ""
let sspec = ref (100,500,50)
let sforcef = ref ""
let curve = ref None
let ignore_id = ref false
let ignore_ood = ref false
(* let radapt = ref None
let radapt_scale = ref 1. *)
let repeat_count = ref 1
let lastweight = ref false
let multilabel = ref false
let bitvec = ref false
let online = ref false
let pa = ref false
let normv = ref 0
let kernel = ref None
let tune = ref false
let opt = ref
  (match argv.(l-2) with
       "binary"     -> Binary
     | "perceptron" -> Perceptron
     | "multitron"  -> Multitron
     | "binomial"   -> Binomial
     | "multiclass" -> Multiclass
     | "density"    -> Sampled
     | s -> arg_error ("'" ^ s ^ "' is not a valid optimization type"))
let fname = argv.(l-1)

let read_sspec s =
  match Str.split comma_re s with
      [a;b;c] -> int_of_string a, int_of_string b, int_of_string c
    | _ -> failwith ""

let read_cspec s =
  match Str.split comma_re s with
      [a;b] -> int_of_string a, int_of_string b
    | _ -> failwith ""

let read_kernelspec s =
  let def = (false,false) in
  let rec parse_kernel_rest (byDist,byClass) = function
      [] -> (byDist,byClass)
    | ("dist" ::xs) -> parse_kernel_rest (true  ,byClass) xs
    | ("class"::xs) -> parse_kernel_rest (byDist,true   ) xs in
    match Str.split colon_re s with
        (_D :: "linear"      :: rest) -> (Kernelmap.Linear                    , int_of_string _D, parse_kernel_rest def rest)
      | (_D :: "poly" :: dim :: rest) -> (Kernelmap.Poly (float_of_string dim), int_of_string _D, parse_kernel_rest def rest)
      | (_D :: "rbf"  :: gam :: rest) -> (Kernelmap.RBF  (float_of_string gam), int_of_string _D, parse_kernel_rest def rest)
      | _ -> failwith "incorrectly specified argument to -kernel"

let _ = 
  let rec read_opt p =
    if p >= l-2 then () else
      match argv.(p) with
          "-filespec" -> ( fspec := true  ; read_opt (p+1) )
        | "-nc"       -> ( named := true  ; read_opt (p+1) )
        | "-fvals"    -> ( ber   := false ; read_opt (p+1) )
        | "-explicit" -> ( imp   := false ; read_opt (p+1) )
        | "-tune"     -> ( tune  := true  ; read_opt (p+1) )
        | "-quiet"    -> ( quiet := true  ; read_opt (p+1) )
        | "-nobias"   -> ( bias  := false ; read_opt (p+1) )
        | "-no-ood"   -> ( ignore_ood := true ; read_opt (p+1) )
        | "-no-id"    -> ( ignore_id  := true ; read_opt (p+1) )
(*        | "-radapt"   -> ( (try radapt := Some (int_of_string argv.(p+1)) with _ -> arg_error "-radapt expects an integer argument")
                          ; read_opt (p+2) )
        | "-rscale"   -> ( (try radapt_scale := float_of_string argv.(p+1) with _ -> arg_error "-rscale expects a float argument")
                          ; read_opt (p+2) ) *)
        | "-maxi"     -> ( (try maxi := int_of_string argv.(p+1) with _ -> arg_error "-maxi expects an integer argument")
                          ; read_opt (p+2) )
        | "-dpp"      -> ( (try dpp  := float_of_string argv.(p+1) with _ -> arg_error "-dpp expects a float argument")
                          ; read_opt (p+2) )
        | "-memory"   -> ( (try memory := int_of_string argv.(p+1) with _ -> arg_error "-memory expects an integer argument")
                          ; read_opt (p+2) )
        | "-lambda"   -> ( (try lambda := float_of_string argv.(p+1) with _ -> arg_error "-lambda expects a float argument")
                          ; read_opt (p+2) )
        | "-sprog"    -> ( (try sprog := argv.(p+1) with _ -> arg_error "-sprog expects an argument")
                          ; read_opt (p+2) )
        | "-sfile"    -> ( (try sfile := argv.(p+1) ; sforcef := argv.(p+1) with _ -> arg_error "-sfile expects an argument")
                          ; read_opt (p+2) )
        | "-sargs"    -> ( (try sargs := argv.(p+1) with _ -> arg_error "-sargs expects an argument")
                          ; read_opt (p+2) )
        | "-sspec"    -> ( (try sspec := read_sspec argv.(p+1) with _ -> arg_error "-sspec expects an argument of the form 'burnin,numsamples,spacing'")
                          ; read_opt (p+2) )
        | "-sforcef"  -> ( (try sforcef := argv.(p+1) with _ -> arg_error "-sforcef expects an argument")
                          ; read_opt (p+2) )
        | "-predict"  -> ( (try pred_file := Some argv.(p+1) with _ -> arg_error "-predict expects a file argument")
                          ; read_opt (p+2) )
        | "-mean"     -> ( (try mean_file := Some argv.(p+1) with _ -> arg_error "-mean expects a file argument")
                          ; read_opt (p+2) )
        | "-init"     -> ( (try init_file := Some argv.(p+1) with _ -> arg_error "-init expects a file argument")
                          ; read_opt (p+2) )
        | "-minfc"    -> ( (try fsel := MinFC (int_of_string argv.(p+1)) with _ -> arg_error "-minfc expects an integer argument")
                          ; read_opt (p+2) )
        | "-bnsfs"    -> ( (try fsel := BNS (int_of_string argv.(p+1)) with _ -> arg_error "-bnsfs expects an integer argument")
                          ; read_opt (p+2) )
        | "-abffs"    -> ( (try fsel := ABFFS (int_of_string argv.(p+1)) with _ -> arg_error "-abffs expects an integer argument")
                          ; read_opt (p+2) )
        | "-repeat"   -> ( (try repeat_count := (int_of_string argv.(p+1)) with _ -> arg_error "-repeat expects an integer argument")
                          ; read_opt (p+2) )
        | "-lastweight" -> ( lastweight := true ; read_opt (p+1) )
        | "-multilabel" -> ( multilabel := true ; read_opt (p+1) )
        | "-bitvec"     -> ( bitvec     := true ; read_opt (p+1) )
        | "-online"     -> ( online     := true ; read_opt (p+1) )
        | "-pa"       -> ( if !opt == Perceptron || !opt == Multitron then pa := true else arg_error "cannot use -pa except with perceptron or multitron" ; read_opt (p+1) )
        | "-kernel"   -> ( (try kernel := Some (read_kernelspec argv.(p+1)) with _ -> arg_error "-kernel expects an argument") 
                         ; read_opt (p+2) )
        | "-norm1"    -> ( normv := 1 ; read_opt (p+1) )
        | "-norm2"    -> ( normv := 2 ; read_opt (p+1) )
        | "-curve"    -> ( (try curve := Some (read_cspec argv.(p+1)) with _ -> arg_error "-curve expects and argument of the form 'min,step'")
                          ; read_opt (p+2) )
        | _ -> arg_error ("unknown option: '" ^ argv.(p) ^ "'") in
    read_opt 1

(* now, check for stuff that doesn't make sense *)

let _ = if !maxi == -1 then (
          if (!opt == Perceptron) || (!opt == Multitron)
          then maxi := 20
          else maxi := 100
        )
let _ = if !maxi   < 1  then arg_error "-maxi should be >= 1"
let _ = if !memory < 2  then arg_error "-memory should be >= 2"
let _ = if !lambda < 0. then arg_error "-lambda should be >= 0"

let _ = if (!opt != Multitron) && (!opt != Multiclass) && !multilabel then
          arg_error "-multilabel should only be used for multiclass problems...for binary problems, use binomial optimization"

let _ = if (!opt != Multitron) && !bitvec then
          arg_error "-bitvec should only be used for multitron problems...for binary problems, use binomial optimization"


(*
let _ = if !multilabel && is_some !abffs then
          arg_error "-multilabel cannot be used with -abffs"
*)

let _ = if !opt == Sampled && !sprog = "" && !sfile = "" && !pred_file = None then 
          arg_error "need to specify -sprog for sampled problems"
let _ = if !opt != Sampled && !sprog <> "" then arg_error "spurious use of -sprog for non-sampled problem"
let _ = if !opt != Sampled && !sfile <> "" then arg_error "spurious use of -sfile for non-sampled problem"
let _ = if !sfile <> "" && !sprog <> "" then
          arg_error "only one of -sprog and -sfile allowed"

let _ = if not !imp && !opt = Binomial then arg_error "cannot use -explicit with binomial optimization"
let _ = if not !imp && !opt = Binary then (
  Printf.fprintf stderr "Warning: it makes no sense to use -explicit with binary optimization\n";
  Printf.fprintf stderr "         we will proceed as if you had said multiclass, but the output\n";
  Printf.fprintf stderr "         will differ slightly from standard binary weights\n";
  opt := Multiclass )

let _ = if !named  && !multilabel then failwith "cannot use both -nc and -multilabel"
let _ = if !bitvec && !multilabel then failwith "cannot use both -bitvec and -multilabel"
let _ = if !named  && !bitvec     then failwith "cannot use both -nc and -bitvec"
let _ = if !named  && !opt != Multiclass && !opt != Multitron then failwith "-nc must be used with multiclass or multitron optimization"

let abffs = ref (match !fsel with ABFFS i -> Some i | _ -> None)

let _ = if is_some !abffs && (not !imp (* || not !ber *) ) then arg_error "can only use abffs feature selection with implicit data"
let _ = if is_some !abffs && !opt != Multiclass then arg_error "can only use abffs feature selection with multiclass problems"

let model = 
  match !pred_file with
      None -> None
    | Some f -> (
        if !fspec then failwith ("cannot predict on filespecs");
        let model_h = my_open_in f in
        let w = read_model Kernelmap.readkernelmap model_h !opt in
        let _ = close_in model_h in
          Some w)

let init_model = 
  match !init_file with
      None -> None
    | Some f -> (
        try
          let model_h = my_open_in f in
          let (_,w) = read_model Kernelmap.readkernelmap model_h !opt in
          let _ = close_in model_h in
            Some w
        with _ -> (
            if not !quiet then Printf.fprintf stderr "Warning: cannot load init file\n";
            None
          )
        )

let mean_model = 
  match !mean_file with
      None -> None
    | Some f -> (
        let model_h = my_open_in f in
        let (_,w) = read_model Kernelmap.readkernelmap model_h !opt in
        let _ = close_in model_h in
          Some w)

let pred_err = ref 0.
let pred_cnt = ref 0

let warnedMissingZ = ref false

let predict_imp_binary y x v =
  match model with
      None -> ()
    | Some (kres, BinModel w) ->
        let v = if !normv == 0 then v else norm_data1 !normv x v in
        let (x,v) = match kres with None -> (x,v) | Some (k,xTr,xtrp) -> let (x,v) = sort_by_xi1 x v in Kernelmap.applymap1 k xTr xtrp x v in
        (match !opt with
             Binary | Perceptron -> 
               let p = (1. /. (1. +. exp (0. -. dotExBA !bias w x v))) in
               let _ = incr pred_cnt in
               let _ = if not (y==0 && p < 0.5 || y==1 && p > 0.5) then pred_err := !pred_err +. 1. in
                 Printf.printf "%d\t%10.20f\n" (if p < 0.5 then 0 else 1) p;
                 flush stdout;
        )

let predict_imp_binomial y x v =
  match model with
      None -> ()
    | Some (kres, BinModel w) ->
        let v = if !normv == 0 then v else norm_data1 !normv x v in
        let (x,v) = match kres with None -> (x,v) | Some (k,xTr,xtrp) -> let (x,v) = sort_by_xi1 x v in Kernelmap.applymap1 k xTr xtrp x v in
        (match !opt with
           | Binomial ->
               let p = (1. /. (1. +. exp (0. -. dotExBA !bias w x v))) in
               let _ = incr pred_cnt in
               let _ = pred_err := !pred_err +. (p -. y) *. (p -. y) in
                 Printf.printf "%d\t%10.20f\n" (if p < 0.5 then 0 else 1) p
	   | Sampled -> 
               let _Z = 
                 try  
                   let _Zid = Hashtbl.find vocabFwd "***NORMALIZING-CONSTANT-Z***" in
                     w.{_Zid}
                 with Not_found -> (
                   if not !warnedMissingZ
                   then ( Printf.fprintf stderr "WARNING: Normalizing constant not found -- not normalizing\n";
                          warnedMissingZ := true );
                   one ) in
               let p    = (dotExBA !bias w x v) *@ _Z in
               let _    = incr pred_cnt in
                 Printf.printf "%10.20f\n" p;
                 flush stdout;
        )

let predict_imp_mc y x v =
  match model with
      None -> ()
    | Some (kres, MultModel w) ->
        let v = if !normv == 0 then v else norm_data1 !normv x v in
        let (x,v) = match kres with None -> (x,v) | Some (k,xTr,xtrp) -> let (x,v) = sort_by_xi1 x v in Kernelmap.applymap1 k xTr xtrp x v in
        (match !opt with
             Binary | Binomial | Perceptron -> failwith "shouldn't get here"
           | Multiclass | Multitron -> (
               let _C = length w.(0) in
               let p  = init _C (fun c -> dotEx !bias w c x v) in
                 if not !bitvec then (
                   let s  = fold_left addLog zero p in
                     iteri (fun i v -> p.(i) <- exp (v -. s)) p;
                 );
               let mxp = ref 0 in
                 (match y with
                     L _ ->
                       for c = 0 to _C - 1 do
                         if p.(c) > p.(!mxp) then mxp := c;
                       done
                   | ML arr ->
                       for c = 0 to _C - 1 do
                         if arr.(c) < infinity then (
                           if !mxp < 0 then mxp := c
                           else if p.(c) > p.(!mxp) then mxp := c;
                         );
                       done
                 );
                 if !named
                 then Printf.printf "%s\t" (try H.find class_recab !mxp with Not_found -> ("UNK:" ^ string_of_int !mxp))
                 else Printf.printf "%d\t" !mxp;
                 for c = 0 to _C - 1 do
                   if c > 0 then Printf.printf " ";
                   Printf.printf "%10.20f" p.(c);
                 done;
                 incr pred_cnt;
                 (match y with
                     L c -> if !mxp != c then pred_err := !pred_err +. 1.
                   | ML a -> pred_err := !pred_err +. a.(!mxp));
                 Printf.printf "\n";
                 flush stdout;
             )
	   | Sampled -> failwith "predict_imp_mc (Sampled): not yet implemented"
	)

let predict_exp y x v =
  match model with
      None -> ()
    | Some (kres, MultModel w) ->
(*        let (x,v) = match kres with None -> (x,v) | Some (k,xTr,xtrp) -> Kernelmap.applymap1 k xTr xtrp x v in *)
        (match kres with None -> () | _ -> failwith "cannot predict explicit with kernels");
        (match !opt with
             Binary | Binomial | Sampled -> failwith "predict_exp: shouldn't get here"
           | Multiclass -> (
               let _C = length x in
               let vc c = if length v == 0 then efa else v.(c) in
               let p  = init _C (fun c -> dotEx !bias w 0 x.(c) (vc c)) in
               let s  = fold_left addLog zero p in
               let _  = iteri (fun i v -> p.(i) <- exp (v -. s)) p in
               let mxp = ref 0 in
                 (match y with
                     L _ ->
                       for c = 0 to _C - 1 do
                         if p.(c) > p.(!mxp) then mxp := c;
                       done
                   | ML arr ->
                       for c = 0 to _C - 1 do
                         if arr.(c) < infinity then (
                           if !mxp < 0 then mxp := c
                           else if p.(c) > p.(!mxp) then mxp := c;
                         );
                       done
                 );
                 Printf.printf "%d\t" !mxp;
                 for c = 0 to _C - 1 do
                   if c > 0 then Printf.printf " ";
                   Printf.printf "%10.20f" p.(c);
                 done;
                 incr pred_cnt;
                 (match y with
                     L c -> if !mxp != c then pred_err := !pred_err +. 1.
                   | ML a -> pred_err := !pred_err +. a.(!mxp));
                 Printf.printf "\n";
                 flush stdout;
             ))



let map_flipped list f = List.map f list

(* read forced features *)
let read_forced_features () =
  if !sforcef = "" then () else (
    ignore (map_flipped (Str.split colon_re !sforcef)
      (fun fname ->
         let h = my_open_in fname in
         let rec rd () =
           match maybe_read_line h with
	       None -> ()
	     | Some l -> (
	         List.iter (fun s -> ignore (get_vocab s)) 
	                    (Str.split space_re l);
	         rd ()) in
           rd ();
           close_in h;
           ()));
    ();
  )

(* now, do the optimization specified *)

let curve_loop xTr yTr w0 mk_m f =
  match !curve with
      None -> f xTr yTr w0
    | Some (minC,step) ->
        (let real_N  = dat_N xTr in
         let real_Y  = A.copy yTr in
         let real_XI  = A.copy xTr.datav_xi in
         let real_XIC = A.copy xTr.datav_xi_c in
         let order = A.init real_N (fun n -> n) in
           Perceptron.permute order;
         let n = ref minC in
         let lastW = ref None in
         let model = ref w0 in
           while !n < real_N do
             xTr.datav_xi   <- A.init (min !n (A.length real_XI )) (fun i -> A.get real_XI (A.get order (i)));
             xTr.datav_xi_c <- A.init (min !n (A.length real_XIC)) (fun i -> A.get real_XIC (A.get order (i)));
            
             Printf.eprintf "\nRUNNING LEARNING CURVE WITH N=%d\n" !n; flush stderr;

             let res = f xTr (A.init (min !n (A.length real_Y)) (fun i -> A.get real_Y (A.get order (i)))) !model in
              
               lastW := Some res;
               model := mk_m res;
               n := !n * step;
           done;
           (match !lastW with
                None -> failwith "learning curve failed!"
              | Some w -> w
           )
        )

(*
let myDotEx useBias w x v =
  let _I = length x in
  let _F = A1.dim w in
  let d  = ref (if useBias then w.{0} else 0.) in
    if length v > 0 then (
      for i = 0 to _I - 1 do
        let f = x.(i) in
          if f >= 0 && f < _F then d := !d +. w.{f} *. v.(i);
      done;
    ) else (
      for i = 0 to _I - 1 do
        let f = x.(i) in
          if f >= 0 && f < _F then d := !d +. w.{f};
      done;
    );
    !d
*)

let curve_or_robust xTr yTr w0 mk_m f =
  match !curve with
      None -> f xTr yTr w0
    | Some _ -> curve_loop xTr yTr w0 mk_m f

(*
let curve_or_robust xTr yTr w0 mk_m f =
  match (!radapt, !curve) with
      (None  ,None  ) -> f xTr yTr w0
    | (Some _,Some _) -> failwith "cannot do both -curve and -radapt"
    | (None  ,Some _) -> curve_loop xTr yTr w0 mk_m f
    | (Some maxri,None) ->
        let _N = dat_N xTr in
        let is_out n = if n < 0 || n >= _N then false else not xTr.datav_indomain.(n) in
        let train w = xTr.datav_weights <- w; let z = f xTr yTr w0 in (z, mk_m z) in
        let cprob (w,Some w_z) n = 
          let sqr x = x *. x in
          let cls = float_of_int (lab yTr.(n)) in
          let xi = xTr.datav_xi.(n) in
          let xv = if length xTr.datav_xv == 0 then [||] else xTr.datav_xv.(n) in
            match !opt with
                Binary
              | Perceptron -> abs_float (1. -. cls -. (1. /. (1. +. (0. -. dotEx !bias w_z 0 xi xv))))
              | Binomial   -> failwith "cannot radapt binomial data"
              | Sampled    -> failwith "cannot radapt sampled data"
              | Multitron
              | Multiclass ->
                  if !imp then (
                    let _C = length w_z.(0) in
                    let p  = init _C (fun c -> dotEx !bias w_z c xi xv) in
                    let s  = fold_left addLog zero p in
                      exp (p.(lab yTr.(n)) -. s)
                  ) else (
                    failwith ""
                  ) in

          fst (Radapt.radapt _N is_out train cprob maxri 1. 0.5)
*)

let w2init w = BinModel w (* init (A1.dim w) (fun n -> [| w.{n} |]) *)

let w2init_mc x w =
  let _C = x.datav_numclass in
  let _F = x.datav_numfeat  in
  let m  = create_matrix _F _C 0. in
    for f = 0 to _F - 1 do
      for c = 0 to _C - 1 do
        m.(f).(c) <- w.{f + c * _F};
      done;
    done;
    MultModel m

(*let oh () = my_open_in fname
let ch h  = my_close_in fname h *)
let noscan = match !pred_file with None -> false | Some _ -> true
let kernel0 = if noscan then None else !kernel
let kmapF = (Kernelmap.kernelmap, Kernelmap.applymap, Kernelmap.printmap)
let () =
  match !opt with
      Binary     -> ( (* must be implicit *)
        let kmap,(xTr,yTr),(xDe,yDe),(xTe,yTe) = load_data !fsel (fun x -> x) !normv kmapF kernel0 !quiet 0 !fspec fname (fun quiet oh ch -> load_implicit noscan !ignore_id !ignore_ood quiet !ber oh ch read_binary_class 1 false predict_imp_binary) in
          if !pred_file == None then (
            let w = curve_or_robust xTr yTr init_model (fun w -> Some (w2init w)) (fun x y w0 -> Cg.compute !lastweight !quiet !bias !maxi  !tune !lambda !dpp x y xDe yDe xTe yTe mean_model w0) in
              printVectorVocab_ba kmap xTr w ))

    | Perceptron -> (
        let kmap,(xTr,yTr),(xDe,yDe),(xTe,yTe) = load_data !fsel (fun x -> x) !normv kmapF kernel0 !quiet 0 !fspec fname (fun quiet oh ch -> load_implicit noscan !ignore_id !ignore_ood quiet !ber oh ch read_binary_class 1 false predict_imp_binary) in
          if !pred_file == None then (
            let myopt = if not !pa then Perceptron.optimize else Pa.optimize !lambda in
            let w = curve_or_robust xTr yTr None (fun w -> None) (fun x y _ -> myopt !lastweight !quiet !bias !maxi x y xDe yDe xTe yTe mean_model) in
              printVectorVocab_ba kmap xTr w ))

    | Binomial   -> ( (* must be implicit *)
        let kmap,(xTr,yTr),(xDe,yDe),(xTe,yTe) = load_data !fsel (fun x -> if x > 0.5 then 1 else 0) !normv kmapF kernel0 !quiet 0. !fspec fname (fun quiet oh ch -> load_implicit noscan !ignore_id !ignore_ood quiet !ber oh ch read_binomial_class 1. false predict_imp_binomial) in
          if !pred_file == None then (
            let w = curve_loop xTr yTr init_model (fun w -> Some (w2init w)) (fun x y w0 -> Cg.computeBinomial !lastweight !quiet !bias !maxi !tune !lambda !dpp x y xDe yDe xTe yTe mean_model w0) in
              printVectorVocab_ba kmap xTr w ))

    | Sampled    -> (* must be implicit *)
        let kmap,(xTr,yTr),(xDe,yDe),(xTe,yTe) = load_data !fsel (fun x -> 0) !normv kmapF kernel0 !quiet 0. !fspec fname (fun quiet oh ch -> load_implicit noscan !ignore_id !ignore_ood quiet !ber oh ch read_binomial_class 1. false predict_imp_binomial) in
          if !pred_file = None then (
	    read_forced_features ();
	    xTr.datav_numfeat <- 1 + !vocabSize;
            let w,_,s_z,_ = curve_loop xTr yTr init_model (fun (w,_,_,_) -> Some (w2init_mc xTr w)) (fun x y w0 -> Bfgs.optimize !lastweight (max 1 !repeat_count) !quiet !bias !maxi !tune !lambda !dpp !memory x (A.empty (L 0)) xDe (A.empty (L 0)) xTe (A.empty (L 0)) (Some ((!sprog, !sfile), !sargs, !sspec)) mean_model w0 false false) in
              (match s_z with None -> () | Some z -> Printf.printf "***NORMALIZING-CONSTANT-Z***\t%10.20f\n" z);
              printVectorVocab_ba kmap xTr w
          );

    | Multitron ->
        let def_label = if !multilabel || !bitvec then (ML [||]) else (L 0) in
        let kmap,(xTr,yTr),(xDe,yDe),(xTe,yTe) = 
          load_data !fsel multilabel2class !normv kmapF kernel0 !quiet def_label !fspec fname (fun quiet oh ch -> 
            if !imp
            then load_implicit noscan !ignore_id !ignore_ood quiet !ber oh ch (if !multilabel || !bitvec then read_multilabel else read_multiclass !named) def_label false predict_imp_mc
            else load_explicit noscan !ignore_id !ignore_ood quiet !bias !ber oh ch predict_exp (!multilabel || !bitvec) !named) in
          if !pred_file == None then (
            let _ = if !imp then (
              xTr.datav_numclass <- 1 + !max_read_class;
              xDe.datav_numclass <- 1 + !max_read_class;
              xTe.datav_numclass <- 1 + !max_read_class ) in
            let _ =
              if !max_read_class == 1 && is_none !abffs then (
                Printf.fprintf stderr "Warning: there only appear to be two classes, but we're\n";
                Printf.fprintf stderr "         optimizing with multitron...using binary optimization\n";
                Printf.fprintf stderr "         with perceptron would be much faster\n" ) in

            let myopt = 
              if !bitvec
              then (if not !pa then Perceptron.optimize_bitvec !online else failwith ("cannot -pa and -bitvec yet")) (* Pa.optimize_bitvec !lambda) *)
              else (if not !pa then Perceptron.optimize_mc             else Pa.optimize_mc     !lambda) in

            let w = curve_loop xTr yTr init_model (fun w -> Some (w2init_mc xTr w)) (fun x y _ -> myopt !lastweight !quiet !bias !maxi x y xDe yDe xTe yTe mean_model) in
              if not !online then (
                if !named then printNames ();
                if not !imp then printVectorVocab_ba kmap xTr w else printMatrixVocab kmap xTr w !bitvec;
              );
          )

    | Multiclass -> ( (* might or might not be implicit *)
        let def_label = if !multilabel || !bitvec then (ML [||]) else (L 0) in
        let kmap,(xTr,yTr),(xDe,yDe),(xTe,yTe) = 
          load_data !fsel multilabel2class !normv kmapF kernel0 !quiet def_label !fspec fname (fun quiet oh ch -> 
            if !imp
            then load_implicit noscan !ignore_id !ignore_ood quiet !ber oh ch (if !multilabel || !bitvec then read_multilabel else read_multiclass !named) def_label false predict_imp_mc
            else load_explicit noscan !ignore_id !ignore_ood quiet !bias !ber oh ch predict_exp (!multilabel || !bitvec) !named) in
          if !pred_file == None then (
            let _ = if !imp then (
              xTr.datav_numclass <- 1 + !max_read_class;
              xDe.datav_numclass <- 1 + !max_read_class;
              xTe.datav_numclass <- 1 + !max_read_class ) in
            let _ =
              if !max_read_class == 1 && is_none !abffs then (
                Printf.fprintf stderr "Warning: there only appear to be two classes, but we're\n";
                Printf.fprintf stderr "         optimizing with BFGS...using binary optimization\n";
                Printf.fprintf stderr "         with CG would be much faster\n" ) in

            let w =
              match !abffs with
                  None ->
                    let w,_,_,_ = curve_or_robust xTr yTr init_model (fun (w,_,_,_) -> Some (w2init_mc xTr w)) (fun x y w0 -> Bfgs.optimize !lastweight (max 1 !repeat_count) !quiet !bias !maxi !tune !lambda !dpp !memory x y xDe yDe xTe yTe None mean_model w0 false !multilabel) in
                      w
                | Some _S -> 
                    curve_loop xTr yTr init_model (fun w -> Some (w2init_mc xTr w)) (fun x y w0 -> Abffs.abffs_selection !quiet !bias !maxi !tune !lambda !dpp !memory x y xDe yDe xTe yTe None mean_model w0 _S) in

              if !named then printNames ();
              if not !imp then printVectorVocab_ba kmap xTr w else printMatrixVocab kmap xTr w !bitvec
          )
      )

let _ =
  if not !quiet then (
      match !pred_file with None -> () | Some _ ->
        Printf.fprintf stderr "Error rate = %g / %g = %g\n" 
          !pred_err (float_of_int !pred_cnt) (!pred_err /. float_of_int !pred_cnt)
    )

