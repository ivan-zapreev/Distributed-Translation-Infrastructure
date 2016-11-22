# Usage of this Makefile:
# To incrementally recompile the system, type
#     make
# To recompute dependancies between modules, type
#     make depend
# To remove the executable and all the compiled files, type
#     make clean
# To compile using the native code compiler
#     make opt
#
##################################################################

EXEC = megam

# If you need special libraries provided with the Caml system,
# (graphics, arbitrary precision numbers, regular expression on strings, ...),
# you must set the variable LIBS to the proper set of libraries. For
# instance, to use the graphics library set LIBS to $(WITHGRAPHICS):
LIBS=$(WITHSTR) $(WITHBIGARRAY) $(WITHUNIX) $(WITHCLIBS)

# You may use any of the following predefined variable
# WITHGRAPHICS : provides the graphics library
# WITHUNIX : provides the Unix interface library
# WITHSTR : provides the regular expression string manipulation library
# WITHNUMS : provides the arbitrary precision arithmetic package
# WITHTHREADS : provides the byte-code threads library
# WITHDBM : provides the Data Base Manager library
#
#
########################## End of Documentation ####################



########################## User's variables #####################
#
# The Caml sources (including camlyacc and camllex source files)

SOURCES = fastdot_c.c fastdot.ml intHashtbl.ml arry.ml util.ml data.ml bitvec.ml cg.ml wsemlm.ml bfgs.ml pa.ml perceptron.ml radapt.ml kernelmap.ml abffs.ml main.ml

########################## Advanced user's variables #####################
#
# The Caml compilers. #
# You may fix here the path to access the Caml compiler on your machine
CAMLC = ocamlc -g
CAMLOPT = ocamlopt -unsafe -ccopt -O4 -ccopt -ffast-math -inline 99999 
CAMLDEP = ocamldep
CAMLLEX = ocamllex
CAMLYACC = ocamlyacc

# Should be set to -custom if you use any of the libraries above
# or if any C code have to be linked with your program
# (irrelevant for ocamlopt)

CUSTOM=-custom 

# Default setting of the WITH* variables. Should be changed if your
# local libraries are not found by the compiler.
WITHGRAPHICS =graphics.cma -cclib -lgraphics -cclib -L/usr/X11R6/lib -cclib -lX11

WITHUNIX =unix.cma -cclib -lunix

WITHSTR =str.cma -cclib -lstr

WITHBIGARRAY =bigarray.cma -cclib -lbigarray

WITHNUMS =nums.cma -cclib -lnums

WITHTHREADS =threads.cma -cclib -lthreads

WITHDBM =dbm.cma -cclib -lmldbm -cclib -lndbm

#WITHCLIBS =-I /usr/lib/ocaml/3.09.2/caml
WITHCLIBS =-I /usr/lib/ocaml/caml

################ End of user's variables #####################


##############################################################
################ This part should be generic
################ Nothing to set up or fix here
##############################################################

all: depend $(EXEC)

opt : $(EXEC).opt

#ocamlc -custom other options graphics.cma other files -cclib -lgraphics -cclib -lX11
#ocamlc -thread -custom other options threads.cma other files -cclib -lthreads
#ocamlc -custom other options str.cma other files -cclib -lstr
#ocamlc -custom other options nums.cma other files -cclib -lnums
#ocamlc -custom other options unix.cma other files -cclib -lunix
#ocamlc -custom other options dbm.cma other files -cclib -lmldbm -cclib -lndbm

SOURCES1 = $(SOURCES:.mly=.ml)
SOURCES2 = $(SOURCES1:.mll=.ml)
OBJS = $(SOURCES2:.ml=.cmo)
OPTOBJS = $(SOURCES2:.ml=.cmx)

$(EXEC): $(OBJS)
	$(CAMLC) $(CUSTOM) -o $(EXEC) $(LIBS) $(OBJS)

$(EXEC).opt: $(OPTOBJS)
	$(CAMLOPT) -o $(EXEC).opt $(LIBS:.cma=.cmxa) $(OPTOBJS)

.SUFFIXES:
.SUFFIXES: .ml .mli .cmo .cmi .cmx .mll .mly

.ml.cmo:
	$(CAMLC) -c $<

.mli.cmi:
	$(CAMLC) -c $<

.ml.cmx:
	$(CAMLOPT) -c $<

.mll.cmo:
	$(CAMLLEX) $<
	$(CAMLC) -c $*.ml

.mll.cmx:
	$(CAMLLEX) $<
	$(CAMLOPT) -c $*.ml

.mly.cmo:
	$(CAMLYACC) $<
	$(CAMLC) -c $*.mli
	$(CAMLC) -c $*.ml

.mly.cmx:
	$(CAMLYACC) $<
	$(CAMLOPT) -c $*.mli
	$(CAMLOPT) -c $*.ml

.mly.cmi:
	$(CAMLYACC) $<
	$(CAMLC) -c $*.mli

.mll.ml:
	$(CAMLLEX) $<

.mly.ml:
	$(CAMLYACC) $<

clean:
	rm -f *.cm[iox] *~ .*~ #*#
	rm -f $(EXEC)
	rm -f $(EXEC).opt

.depend: $(SOURCES2)
	$(CAMLDEP) *.mli *.ml > .depend

depend: $(SOURCES2)
	$(CAMLDEP) *.mli *.ml > .depend

include .depend
