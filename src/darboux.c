// fonction de calcul principale : algorithme de Darboux
// (remplissage des cuvettes d'un MNT)
#include <string.h>
#include <stdbool.h>
#include <mpi.h>

#include "check.h"
#include "type.h"
#include "darboux.h"

// si ce define n'est pas commenté, l'exécution affiche sur stderr la hauteur
// courante en train d'être calculée (doit augmenter) et l'itération du calcul
//#define DARBOUX_PPRINT

#define PRECISION_FLOTTANT 1.e-5

// pour accéder à un tableau de flotant linéarisé (ncols doit être défini) :
#define WTERRAIN(w, i, j) (w[(i)*ncols+(j)])

// calcule la valeur max de hauteur sur un terrain
float max_terrain(const mnt *restrict m)
{
    float max = m->terrain[0];
    #pragma omp parallel for reduction(max: max)
    for (int i = 0; i < m->ncols * m->nrows; i++)
        max = m->terrain[i] > max ? m->terrain[i] : max;
    return (max);
}

// initialise le tableau W de départ à partir d'un mnt m
float *init_W(const mnt *restrict m)
{
    int ncols = m->ncols, nrows = m->nrows;
    float *restrict W;
    CHECK((W = malloc(ncols * nrows * sizeof(float))) != NULL);

    // initialisation W
    int j;
    float max = max_terrain(m) + 10.f;
#pragma omp parallel for default(none) private(j) shared(nrows, ncols, m, W, max)
    for (int i = 0; i < nrows; i++)
    {
        for (j = 0; j < ncols; j++)
        {
            if (i == 0 || i == nrows - 1 || j == 0 || j == ncols - 1 ||
                TERRAIN(m, i, j) == m->no_data)
                WTERRAIN(W, i, j) = TERRAIN(m, i, j);
            else
                WTERRAIN(W, i, j) = max;
        }
    }

    return (W);
}

// variables globales pour l'affichage de la progression
#ifdef DARBOUX_PPRINT
float min_darboux = 9999.; // ça ira bien, c'est juste de l'affichage
int iter_darboux = 0;

// fonction d'affichage de la progression
void dpprint()
{
    if (min_darboux != 9999.)
    {
        fprintf(stderr, "%.3f %d\r", min_darboux, iter_darboux++);
        fflush(stderr);
        min_darboux = 9999.;
    } else
        fprintf(stderr, "\n");
}

#endif


// pour parcourir les 8 voisins :
const int VOISINS[8][2] = {{-1, -1},
                           {-1, 0},
                           {-1, 1},
                           {0,  -1},
                           {0,  1},
                           {1,  -1},
                           {1,  0},
                           {1,  1}};

// cette fonction calcule le nouveau W[i,j] en utilisant Wprec[i,j]
// et ses 8 cases voisines : Wprec[i +/- 1 , j +/- 1],
// ainsi que le MNT initial m en position [i,j]
// inutile de modifier cette fonction (elle est sensible...):
int calcul_Wij(float *restrict W, const float *restrict Wprec, const mnt *m,
               const int i, const int j)
{
    const int nrows = m->nrows, ncols = m->ncols;
    int modif = 0;

    // on prend la valeur précédente...
    WTERRAIN(W, i, j) = WTERRAIN(Wprec, i, j);
    // ... sauf si :
    if (WTERRAIN(Wprec, i, j) > TERRAIN(m, i, j))
    {
        // parcourir les 8 voisins haut/bas + gauche/droite
        for (int v = 0; v < 8; v++)
        {
            const int n1 = i + VOISINS[v][0];
            const int n2 = j + VOISINS[v][1];

            // printf("[%d] - (%d %d) (%d %d)\n", rank, n1, n2, nrows, ncols);

            // vérifie qu'on ne sort pas de la grille.
            // ceci est théoriquement impossible, si les bords de la matrice Wprec
            // sont bien initialisés avec les valeurs des bords du mnt
            CHECK(n1 >= 0 && n1 < nrows && n2 >= 0 && n2 < ncols);

            // si le voisin est inconnu, on l'ignore et passe au suivant
            if (WTERRAIN(Wprec, n1, n2) == m->no_data)
                continue;

            CHECK(TERRAIN(m, i, j) > m->no_data);
            CHECK(WTERRAIN(Wprec, i, j) > m->no_data);
            CHECK(WTERRAIN(Wprec, n1, n2) > m->no_data);

            // il est important de mettre cette valeur dans un temporaire, sinon le
            // compilo fait des arrondis flotants divergents dans les tests ci-dessous
            const float Wn = WTERRAIN(Wprec, n1, n2) + EPSILON;
            if (TERRAIN(m, i, j) >= Wn)
            {
                WTERRAIN(W, i, j) = TERRAIN(m, i, j);
                modif = 1;
#ifdef DARBOUX_PPRINT
                if (WTERRAIN(W, i, j) < min_darboux)
                    min_darboux = WTERRAIN(W, i, j);
#endif
            } else if (WTERRAIN(Wprec, i, j) > Wn)
            {
                WTERRAIN(W, i, j) = Wn;
                modif = 1;
#ifdef DARBOUX_PPRINT
                if (WTERRAIN(W, i, j) < min_darboux)
                    min_darboux = WTERRAIN(W, i, j);
#endif
            }
        }
    }
    return (modif);
}

/*****************************************************************************/
/*           Fonction de calcul principale - À PARALLÉLISER                  */
/*****************************************************************************/
// applique l'algorithme de Darboux sur le MNT m, pour calculer un nouveau MNT
mnt *darboux(const mnt *restrict m)
{
    int ncols = m->ncols, nrows = m->nrows;

    // initialisation
    float *restrict W, *restrict Wprec;
    CHECK((W = malloc(ncols * nrows * sizeof(float))) != NULL);
    Wprec = init_W(m);

    // calcul : boucle principale
    bool modif = true, running = true;
    int j;
    // set start and end indexes for nrows loop
    int j_start = size != 1 && rank != 0;
    int j_end = nrows - (size != 1 && rank != size - 1);

    while (running)
    {
        modif = 0; // sera mis à 1 s'il y a une modification

        // 1 process = main process
        if (size != 1)
        {

            if (rank != size - 1)
            {
                // On envoie Wprec au processus suivant
                MPI_Send(&Wprec[(nrows - 2) * ncols], ncols,
                         MPI_FLOAT, rank + 1,
                         0, MPI_COMM_WORLD);
            }

            if (rank != 0)
            {
                // Attend de recevoir la ligne précédente du processus précédent
                MPI_Recv(&Wprec[0], ncols,
                         MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);

                // Envoie la première ligne du processus actuel au processus précédent
                MPI_Send(&Wprec[ncols], ncols,
                         MPI_FLOAT, rank - 1,
                         0, MPI_COMM_WORLD);
            }

            if (rank != size - 1)
            {
                // Attend de recevoir la première ligne du processus suivant
                MPI_Recv(&Wprec[(nrows - 1) * ncols], ncols,
                         MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            }

        }

        // Clang-tidy: openmp-use-default-none
        // Using default(none) clause forces developers to explicitly specify
        // data sharing attributes for the variables referenced in the construct,
        // thus making it obvious which variables are referenced, and what is
        // their data sharing attribute, thus increasing readability and
        // possibly making errors easier to spot.
#pragma omp parallel for reduction(|:modif) default(none) private(j) shared(nrows,rank, j_start, j_end, ncols, W, Wprec, m)
        // calcule le nouveau W fonction de l'ancien (Wprec) en chaque point [i,j]
        for (int i = j_start; i < j_end; i++)
        {
            for (j = 0; j < ncols; j++)
            {
                // calcule la nouvelle valeur de W[i,j]
                // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
               modif |= calcul_Wij(W, Wprec, m, i, j);
            }
        }

#ifdef DARBOUX_PPRINT
        dpprint();
#endif

        // échange W et Wprec
        // sans faire de copie mémoire : échange les pointeurs sur les deux tableaux
        float *tmp = W;
        W = Wprec;
        Wprec = tmp;

        // Va faire un || sur toutes les valeurs modif,
        // si toutes les valeurs sont 0 alors le programme est terminé
        MPI_Allreduce(&modif, &running, 1, MPI_C_BOOL,
                      MPI_LOR, MPI_COMM_WORLD);
        // Donc si running == 0, alors le programme sera terminé

    }
    // fin du while principal


    // fin du calcul, le résultat se trouve dans W
    free(Wprec);
    // crée la structure résultat et la renvoie
    mnt *res;
    CHECK((res = malloc(sizeof(*res))) != NULL);
    memcpy(res, m, sizeof(*res));
    res->terrain = W;
    return (res);
}
