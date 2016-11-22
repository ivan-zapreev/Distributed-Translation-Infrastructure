#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "alloc.h"
#include "bigarray.h"
#include "custom.h"
#include "fail.h"
#include "intext.h"
#include "memory.h"
#include "mlvalues.h"
#include "math.h"

/* dense vector times sparse vector, no values, with offset */
value c_mult_dense_sparse(value aw, value axi, value offset) {
  long i;
  struct caml_bigarray* bw  = Bigarray_val(aw);
  struct caml_bigarray* bxi = Bigarray_val(axi);
  float* w   = (float*) bw->data;
  int*   xi  = (int*)   bxi->data;
  long wdim  = bw->dim[0];
  long xidim = bxi->dim[0];
  long off   = Long_val(offset);

  float d = 0;
  for (i=0; i<xidim; i++) {
    if ((xi[i]+off >= 0) && (xi[i]+off < wdim)) {
      d += w[xi[i]+off];
    }
  }

  return copy_double(d);
}

/* dense vector times sparse vector, with values, with offset */
value c_mult_dense_sparse_val(value aw, value axi, value axv, value offset) {
  long i;
  struct caml_bigarray* bw  = Bigarray_val(aw);
  struct caml_bigarray* bxi = Bigarray_val(axi);
  struct caml_bigarray* bxv = Bigarray_val(axv);
  float* w   = (float*) bw->data;
  int*   xi  = (int*)   bxi->data;
  float* xv  = (float*) bxv->data;
  long wdim  = bw->dim[0];
  long xidim = bxi->dim[0];
  long xvdim = bxv->dim[0];
  long xdim  = xidim < xvdim ? xidim : xvdim;
  long off   = Long_val(offset);

  float d = 0;

  for (i=0; i<xdim; i++) {
    if ((xi[i]+off >= 0) && (xi[i]+off < wdim)) {
      d += w[xi[i]+off] * xv[i];
    }
  }

  for (i=xdim; i<xidim; i++) {
    if ((xi[i]+off >= 0) && (xi[i]+off < wdim)) {
      d += w[xi[i]+off];
    }
  }

  return copy_double(d);
}

/* norm of a dense vector */
value c_norm_dense(value aw) {
  long i;
  struct caml_bigarray* bw  = Bigarray_val(aw);
  float* w   = (float*) bw->data;
  long wdim  = bw->dim[0];

  float d = 0;
  for (i=0; i<wdim; i++) {
    d += w[i] * w[i];
  }

  return copy_double(d);
}

/* dense vector times dense vector */
value c_mult_dense_dense(value aa, value ab) {
  long i;
  struct caml_bigarray* ba = Bigarray_val(aa);
  struct caml_bigarray* bb = Bigarray_val(ab);
  float* a   = (float*) ba->data;
  float* b   = (float*) bb->data;
  long dim   = (ba->dim[0] < bb->dim[0]) ? ba->dim[0] : bb->dim[0];

  float d = 0;
  for (i=0; i<dim; i++) {
    d += a[i] * b[i];
  }

  return copy_double(d);
}

/* dense vector times scalar */
value c_mult_dense(value aa, value ab) {
  long i;
  struct caml_bigarray* ba = Bigarray_val(aa);
  float  b   = Double_val(ab);
  float* a   = (float*) ba->data;
  long dim   = ba->dim[0];

  for (i=0; i<dim; i++) {
    a[i] *= b;
  }

  return Val_unit;
}

/* store  multa*a + multb*b  in a */
value c_add_dense_dense(value aa, value ab, value amulta, value amultb) {
  long i;
  struct caml_bigarray* ba = Bigarray_val(aa);
  struct caml_bigarray* bb = Bigarray_val(ab);
  float* a   = (float*) ba->data;
  float* b   = (float*) bb->data;
  float  ma  = Double_val(amulta);
  float  mb  = Double_val(amultb);
  long dim   = (ba->dim[0] < bb->dim[0]) ? ba->dim[0] : bb->dim[0];

  for (i=0; i<dim; i++) {
    a[i] = ma * a[i] + mb * b[i];
  }
  for (i=dim; i<ba->dim[0]; i++) {
    a[i] = ma * a[i];
  }

  return Val_unit;
}

/* store w + z * xi in w, where xi is sparse with no value and offset */
value c_add_dense_sparse(value aw, value axi, value az, value offset) {
  long i;
  struct caml_bigarray* bw  = Bigarray_val(aw);
  struct caml_bigarray* bxi = Bigarray_val(axi);
  float* w   = (float*) bw->data;
  int*   xi  = (int*)   bxi->data;
  float  z   = Double_val(az);
  long wdim  = bw->dim[0];
  long xidim = bxi->dim[0];
  long off   = Long_val(offset);

  for (i=0; i<xidim; i++) {
    if ((xi[i]+off >= 0) && (xi[i]+off < wdim)) {
      w[xi[i]+off] += z;
    }
  }

  return Val_unit;
}
/* store w + z * xv[xi] in w, where xi is sparse with value xv and offset */
value c_add_dense_sparse_val(value aw, value axi, value axv, value az, value offset) {
  long i;
  struct caml_bigarray* bw  = Bigarray_val(aw);
  struct caml_bigarray* bxi = Bigarray_val(axi);
  struct caml_bigarray* bxv = Bigarray_val(axv);
  float* w   = (float*) bw->data;
  int*   xi  = (int*)   bxi->data;
  float* xv  = (float*) bxv->data;
  float  z   = Double_val(az);
  long wdim  = bw->dim[0];
  long xidim = bxi->dim[0];
  long xvdim = bxv->dim[0];
  long xdim  = xidim < xvdim ? xidim : xvdim;
  long off   = Long_val(offset);

  for (i=0; i<xdim; i++) {
    if ((xi[i]+off >= 0) && (xi[i]+off < wdim)) {
      w[xi[i]+off] += z * xv[i];
    }
  }

  for (i=xdim; i<xidim; i++) {
    if ((xi[i]+off >= 0) && (xi[i]+off < wdim)) {
      w[xi[i]+off] += z;
    }
  }

  return Val_unit;
}
