// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "type.h"
#include "io.h"
#include "darboux.h"
#include "check.h"

int rank, size; // External ints

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

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
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

    const int mnt_var_count = 3;
    int blocklengths[] = {1, 1, 1};
    MPI_Datatype mnt_datatypes[] = {MPI_INT, MPI_INT,
                                    MPI_FLOAT};
    MPI_Datatype mpi_mnt_type;
    MPI_Aint offsets[mnt_var_count];
    offsets[0] = offsetof(mnt, ncols);
    offsets[1] = offsetof(mnt, nrows);
    offsets[2] = offsetof(mnt, no_data);

    printf("[%d] Before MPI_Type_create_struct\n", rank);

    MPI_Type_create_struct(mnt_var_count, blocklengths,
                           offsets, mnt_datatypes,
                           &mpi_mnt_type);
    printf("[%d] Before MPI_Type_commit\n", rank);
    MPI_Type_commit(&mpi_mnt_type);

    // READ INPUT ONLY IN PROCESS 0
    if (rank == 0)
    {
        m = mnt_read(argv[1]);
    } else
    {
        // Malloc new structure
        CHECK((m = malloc(sizeof(*m))) != NULL);
        m->xllcorner = 0;
        m->yllcorner = 0;
        m->cellsize = 0;
        m->no_data = 0;
        CHECK((m->terrain = malloc(m->ncols * m->nrows * sizeof(float))) !=
              NULL);
    }

    printf("[%d] Before MPI_Bcast\n", rank);

    // broadcast m
    MPI_Bcast(m, 1, mpi_mnt_type, 0, MPI_COMM_WORLD);

    // Init array and set nrows for each process
    int *rowsPerProc = malloc(size*sizeof(int));
    for(size_t i=0; i<size; i++)
        rowsPerProc[i] = m->nrows / size;

    // Distribute the remaining rows to the first processes
    int remainingRows = (rowsPerProc[0] * size) - m->nrows;
    if(rank == 0 && remainingRows > 0)
        for(size_t i=0; remainingRows < 0; i++, remainingRows--)
            rowsPerProc[i]++;

    for(size_t i=0; i < size; i++)
        printf("%d - ", rowsPerProc[i]);

    printf("[%d] ncols=%d, nrows=%d, no_data=%f\n", rank, m->ncols,
           m->nrows, m->no_data);

    if (rank != 0)
    {
        fprintf(stdout, "[%d] %d\n", rank, m->ncols);
        fprintf(stdout, "[%d] %d\n", rank, m->nrows);
        fprintf(stdout, "[%d] %.2f\n", rank, m->xllcorner);
        fprintf(stdout, "[%d] %.2f\n", rank, m->yllcorner);
        fprintf(stdout, "[%d] %.2f\n", rank, m->cellsize);
        fprintf(stdout, "[%d] %.2f\n", rank, m->no_data);

        for (int i = 0; i < m->nrows; i++)
        {
            fprintf(stdout, "[%d] ", rank);
            for (int j = 0; j < m->ncols; j++)
            {
                fprintf(stdout, "%.2f ", TERRAIN(m, i, j));
            }
            fprintf(stdout, "\n");
        }
    }
    printf("[%d] Before darboux compute\n", rank);

    //const int t_size = m->ncols * m->nrows;
    /* MPI_Scatter(m->terrain, rowsPerProc, MPI_FLOAT,
                m->terrain, rowsPerProc, MPI_FLOAT,
                0, MPI_COMM_WORLD);

    printf("print %d\n", rowsPerProc);

    // On change pour le process 0
    m->nrows = rowsPerProc;*/

    // COMPUTE
    d = darboux(m);

    printf("[%d] Before write\n", rank);

    // WRITE OUTPUT ONLY IN PROCESS 0
    if (rank == 0)
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
    MPI_Finalize();

    return (0);
}
