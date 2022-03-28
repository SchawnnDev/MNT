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

    CHECK(MPI_Init(&argc, &argv));

    int rang, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // READ INPUT ONLY IN PROCESS 0
    if(rang == 0)
    {
        m = mnt_read(argv[1]);
    }

    // COMPUTE
    d = darboux(m);

    // WRITE OUTPUT ONLY IN PROCESS 0
    if(rang == 0)
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
    }


    // free
    free(m->terrain);
    free(m);
    free(d->terrain);
    free(d);

    // Finalize
    CHECK(MPI_Finalize());

    return (0);
}
