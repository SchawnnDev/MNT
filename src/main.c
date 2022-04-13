// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "type.h"
#include "io.h"
#include "darboux.h"
#include "check.h"

int rank, size; // External ints

void print_debug(mnt *m, char* prout)
{
    fprintf(stdout, "%s[%d] %d\n",prout, rank, m->ncols);
    fprintf(stdout, "%s[%d] %d\n",prout, rank, m->nrows);
    fprintf(stdout, "%s[%d] %.2f\n", prout, rank, m->xllcorner);
    fprintf(stdout, "%s[%d] %.2f\n", prout, rank, m->yllcorner);
    fprintf(stdout, "%s[%d] %.2f\n", prout, rank, m->cellsize);
    fprintf(stdout, "%s[%d] %.2f\n", prout, rank, m->no_data);

    for (int i = 0; i < m->nrows; i++)
    {
        fprintf(stdout, "%s[%d] ", prout,rank);
        for (int j = 0; j < m->ncols; j++)
        {
            fprintf(stdout, "%.2f ", TERRAIN(m, i, j));
        }
        fprintf(stdout, "\n");
    }
}

void calculate_counts(mnt *m, int *rowsPerProc, int *displ)
{
    for (size_t i = 0; i < size; i++)
        rowsPerProc[i] = m->nrows / size;

    // Check if there is more processes than mat rows
    // TODO : check if a process had 0 as nrows value
    if (size > m->nrows)
        for (size_t i = size - m->nrows; i < size; i++)
            rowsPerProc[i] = 0;

    // Distribute the remaining rows to the first processes in line
    int remainingRows = m->nrows % size;
    for (size_t i = 0; remainingRows > 0; i++, remainingRows--)
        rowsPerProc[i]++;

    // Displacement array
    int sum = 0;
    for (size_t i = 0; i < size; i++)
    {
        // Elements * cols
        rowsPerProc[i] *= m->ncols;
        displ[i] = sum;
        sum += rowsPerProc[i];
    }
}

int main(int argc, char **argv)
{
    mnt *m, *d, *r;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input filename> "
                        "[<output filename>]\n", argv[0]);
        exit(1);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int mnt_var_count = 3;
    int blocklengths[] = {1, 1, 1};
    MPI_Datatype mnt_datatypes[] = {MPI_INT, MPI_INT, MPI_FLOAT};
    MPI_Datatype mpi_mnt_type;
    MPI_Aint offsets[mnt_var_count];
    offsets[0] = offsetof(mnt, ncols);
    offsets[1] = offsetof(mnt, nrows);
    offsets[2] = offsetof(mnt, no_data);

    MPI_Type_create_struct(mnt_var_count, blocklengths, offsets,
                           mnt_datatypes, &mpi_mnt_type);
    MPI_Type_commit(&mpi_mnt_type);

    // READ INPUT ONLY IN PROCESS 0
    if (rank == 0)
    {
        printf("Size = %d\n", size);
        m = mnt_read(argv[1]);
    }
    else
    {
        // Malloc new structure
        CHECK((m = malloc(sizeof(*m))) != NULL);
        m->xllcorner = 0;
        m->yllcorner = 0;
        m->cellsize = 0;
        m->no_data = 0;
    }

    // Broadcast m to each process
    MPI_Bcast(m, 1, mpi_mnt_type, 0, MPI_COMM_WORLD);

    // Set result mnt
    CHECK((r = malloc(sizeof(*r))) != NULL);
    CHECK((r->terrain = malloc(m->ncols * m->nrows * sizeof(float))) !=NULL);
    r->nrows = m->nrows;
    r->ncols = m->ncols;
    r->xllcorner = m->xllcorner;
    r->yllcorner = m->yllcorner;
    r->cellsize = m->cellsize;
    r->no_data = m->no_data;

    // Init arrays
    int *rowsPerProc = malloc(size * sizeof(int));
    int *displ = malloc(size * sizeof(int));

    // Set nrows for each process
    // effective rows + the one before and after (except on borders)
    calculate_counts(m, rowsPerProc, displ);
    m->nrows = (rowsPerProc[rank] / m->ncols);
    if(size != 1) m->nrows++;

    int startIdx = (rank == 0) ? 0 : m->ncols;

    if (rank != 0)
    {
        // Processes in the middle should have 2 rows
        if (rank != size - 1)
            m->nrows += 1;

        CHECK((m->terrain = malloc(m->ncols * m->nrows * sizeof(float))) !=
              NULL);

        // Init table terrain with 0
        for (int i = 0; i < m->nrows; ++i)
            for (int j = 0; j < m->ncols; ++j)
                TERRAIN(m, i, j) = 0;
    }

    MPI_Scatterv(m->terrain, rowsPerProc, displ,
                 MPI_FLOAT, &(m->terrain[startIdx]),
                 rowsPerProc[rank] * m->ncols,
                 MPI_FLOAT, 0, MPI_COMM_WORLD);

    printf("[%d] After scatter\n", rank);
    print_debug(m, "1");

    // COMPUTE
    d = darboux(m);

    printf("[%d] After darboux\n", rank);
    print_debug(m, "2");

/*    if(rank == 0)
        for(int i=0; i<size; i++)
            printf("[0]         => %d\n", rowsPerProc[i]);
    printf("[%d] = %d * %d = %d\n", rank, rowsPerProc[rank], m->ncols, rowsPerProc[rank] * m->ncols);*/

    MPI_Gatherv(&(m->terrain[startIdx]),
                rowsPerProc[rank],
                MPI_FLOAT, r->terrain,
                rowsPerProc, displ,
                MPI_FLOAT, 0, MPI_COMM_WORLD);

    if(rank == 0)
    {
        printf("[%d] Result\n", rank);
        print_debug(m, "R");
    }

    // WRITE OUTPUT ONLY IN PROCESS 0
    if (rank == 0)
    {
        FILE *out;
        if (argc == 3)
            out = fopen(argv[2], "w");
        else
            out = stdout;
        mnt_write(r, out);
        if (argc == 3)
            fclose(out);
        else
            mnt_write_lakes(m, r, stdout);

        // SYNC COMPUTE
        mnt *expected;
        expected = darboux_seq(m);
        mnt_compare(expected, r);
    }

    // free
    if(rank == 0)
    {
        free(r->terrain);
        free(r);
    }
    free(rowsPerProc);
    free(displ);
    free(m->terrain);
    free(m);
    free(d->terrain);
    free(d);

    // Finalize
    MPI_Finalize();

    return (0);
}
