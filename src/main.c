// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <string.h>

#include "type.h"
#include "io.h"
#include "darboux.h"
#include "darboux_seq.h"
#include "check.h"

#define HYPERTHREADING 1 // 1 if hyperthreading is on, 0 otherwise

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

// inutile de paralleliser ici car trop peu utile
void calculate_counts(mnt *m, int *rowsPerProc, int *displ)
{
    for (size_t i = 0; i < size; i++)
        rowsPerProc[i] = m->nrows / size;

    // Check if there is more processes than mat rows
    if (size > m->nrows)
    {
        for (size_t i = size - m->nrows; i < size; i++)
            rowsPerProc[i] = 0;
    }

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
    mnt *m, *d, *r, *e = NULL;
    double time_reference, time_kernel = 0, speedup, efficiency;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input filename> "
                        "[<output filename>]\n", argv[0]);
        exit(1);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // READ INPUT ONLY IN PROCESS 0
    if (rank == 0)
    {
        printf("Starting with %d processes with %d threads.\n", size, omp_get_max_threads());
        m = mnt_read(argv[1]);

        CHECK((e = malloc(sizeof(*e))) != NULL);
        memcpy(e, m, sizeof(*e));

        time_kernel = omp_get_wtime();
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

    const int mnt_var_count = 3;
    int blocklengths[] = {1, 1, 1};
    MPI_Datatype mnt_datatypes[] = {MPI_INT, MPI_INT, MPI_FLOAT};
    MPI_Datatype mpi_mnt_type;
    MPI_Aint offsets[mnt_var_count];
    offsets[0] = offsetof(mnt, ncols);
    offsets[1] = offsetof(mnt, nrows);
    offsets[2] = offsetof(mnt, no_data);

    // Broadcast m to each process
    MPI_Type_create_struct(mnt_var_count, blocklengths, offsets,
                           mnt_datatypes, &mpi_mnt_type);
    MPI_Type_commit(&mpi_mnt_type);
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
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < m->nrows; ++i)
            for (int j = 0; j < m->ncols; ++j)
                TERRAIN(m, i, j) = 0;
    }

    MPI_Scatterv(m->terrain, rowsPerProc, displ,
                 MPI_FLOAT, &(m->terrain[startIdx]),
                 rowsPerProc[rank],
                 MPI_FLOAT, 0, MPI_COMM_WORLD);

    // COMPUTE
    d = darboux(m);

    MPI_Gatherv(&(d->terrain[startIdx]),
                rowsPerProc[rank],
                MPI_FLOAT, r->terrain,
                rowsPerProc, displ,
                MPI_FLOAT, 0, MPI_COMM_WORLD);

    // WRITE OUTPUT ONLY IN PROCESS 0
    if (rank == 0)
    {
        time_kernel = omp_get_wtime() - time_kernel ;

        // Value after gather
        // print_debug(r, "R");

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
        time_reference = omp_get_wtime();
        mnt *expected = darboux_seq(e);
        time_reference  = omp_get_wtime() - time_reference ;

        speedup = time_reference / time_kernel;
        efficiency = speedup / (omp_get_num_procs() / (1 + HYPERTHREADING));
        printf("Reference time : %3.5lf s\n", time_reference);
        printf("Kernel time    : %3.5lf s\n", time_kernel);
        printf("Speedup ------ : %3.5lf\n", speedup);
        printf("Efficiency --- : %3.5lf\n", efficiency);

        // Value expected
        // print_debug(expected, "E");
        mnt_compare(expected, r);
    }

    // free
    free(rowsPerProc);
    free(displ);
    free(m->terrain);
    free(m);
    free(d->terrain);
    free(d);
    free(r->terrain);
    free(r);

    // Finalize
    MPI_Finalize();

    return (0);
}
