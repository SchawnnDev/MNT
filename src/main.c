// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "type.h"
#include "io.h"
#include "darboux.h"
#include "check.h"

int main(int argc, char **argv)
{
    mnt *m, *d;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input filename> "
                        "[<output filename>]\n", argv[0]);
        exit(1);
    }

    MPI_Init(&argc, &argv);

    int rang, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /**
  typedef struct mnt_t
{
int ncols, nrows;                   // size
float xllcorner, yllcorner, cellsize; // not used
float no_data;                      // mnt value unknown

float *terrain;                     // linear array (size: ncols*nrows)
}
 */

    const int mnt_var_count = 4;
    int blocklengths[] = {1, 1, 1, 1};
    MPI_Datatype mnt_datatypes[] = {MPI_INT, MPI_INT,
                                                 MPI_FLOAT, MPI_FLOAT};
    MPI_Datatype mpi_mnt_type;
    MPI_Aint offsets[mnt_var_count];
    offsets[0] = offsetof(mnt, ncols);
    offsets[1] = offsetof(mnt, nrows);
    offsets[2] = offsetof(mnt, no_data);
    offsets[3] = offsetof(mnt, terrain);

    MPI_Type_create_struct(mnt_var_count, blocklengths,
                           offsets, mnt_datatypes,
                           &mpi_mnt_type);
    MPI_Type_commit(&mpi_mnt_type);

    // READ INPUT ONLY IN PROCESS 0
    if (rang == 0)
    {
        m = mnt_read(argv[1]);
    } else {
        CHECK((m = malloc(sizeof(*m))) != NULL);
    }

    // broadcast
    MPI_Bcast(m, 1, mpi_mnt_type, 0, MPI_COMM_WORLD);

    // COMPUTE
    d = darboux(m);

    // WRITE OUTPUT ONLY IN PROCESS 0
    if (rang == 0)
    {
        FILE *out;
        if (argc == 3)
            out = fopen(argv[2], "w");
        else
            out = stdout;
        mnt_write(d, out);
        if (argc == 3)
            fclose(out);
        else
            mnt_write_lakes(m, d, stdout);

        // SYNC COMPUTE
        mnt *expected;
        expected = darboux(m);
        mnt_compare(expected, d);
    }


    // free
    free(m->terrain);
    free(m);
    free(d->terrain);
    free(d);

    // Finalize
    MPI_Finalize();

    return (0);
}
