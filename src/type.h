// structure de données principale
#ifndef __TYPE_H__
#define __TYPE_H__

typedef struct mnt_t
{
  int ncols, nrows;                   // size
  float xllcorner, yllcorner, cellsize; // not used
  float no_data;                      // mnt value unknown

  float *terrain;                     // linear array (size: ncols*nrows)
}
mnt;

// access to terrain in an mnt m as a 2D array:
#define TERRAIN(m,i,j) (m->terrain[(i)*m->ncols+(j)])

#endif
