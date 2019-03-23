//
//  main.cpp
//

#include "Matrix.hpp"
#include "mpi.h"
#include <cstdlib>
#include <ctime>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <mpi.h>
#include <unistd.h>

using namespace std;

// Inverser la matrice par la méthode de Gauss-Jordan; implantation séquentielle.
void invertSequential(Matrix &iA)
{

    // vérifier que la matrice est carrée
    assert(iA.rows() == iA.cols());
    // construire la matrice [A I]
    MatrixConcatCols lAI(iA, MatrixIdentity(iA.rows()));

    // traiter chaque rangée
    for (size_t k = 0; k < iA.rows(); ++k)
    {
        // trouver l'index p du plus grand pivot de la colonne k en valeur absolue
        // (pour une meilleure stabilité numérique).
        size_t p = k;
        double lMax = fabs(lAI(k, k));
        for (size_t i = k; i < lAI.rows(); ++i)
        {
            if (fabs(lAI(i, k)) > lMax)
            {
                lMax = fabs(lAI(i, k));
                p = i;
            }
        }
        // vérifier que la matrice n'est pas singulière
        if (lAI(p, k) == 0)
            throw runtime_error("Matrix not invertible");

        // échanger la ligne courante avec celle du pivot
        if (p != k)
            lAI.swapRows(p, k);

        double lValue = lAI(k, k);
        for (size_t j = 0; j < lAI.cols(); ++j)
        {
            // On divise les éléments de la rangée k
            // par la valeur du pivot.
            // Ainsi, lAI(k,k) deviendra égal à 1.
            lAI(k, j) /= lValue;
        }

        // Pour chaque rangée...
        for (size_t i = 0; i < lAI.rows(); ++i)
        {
            if (i != k)
            { // ...différente de k
                // On soustrait la rangée k
                // multipliée par l'élément k de la rangée courante
                double lValue = lAI(i, k);
                lAI.getRowSlice(i) -= lAI.getRowCopy(k) * lValue;
            }
        }
    }

    // On copie la partie droite de la matrice AI ainsi transformée
    // dans la matrice courante (this).
    for (unsigned int i = 0; i < iA.rows(); ++i)
    {
        iA.getRowSlice(i) = lAI.getDataArray()[slice(i * lAI.cols() + iA.cols(), iA.cols(), 1)];
    }
}
typedef struct ind_and_val
{
    double value;
    int index;
} ind_and_val;
// void maxloc_idx_val(void *in, void *inout, int *len, MPI_Datatype *type)
// {
//     /* ignore type, just trust that it's our dbl_twoindex type */
//     ind_and_val *invals = in;
//     ind_and_val *inoutvals = inout;

//     for (int i = 0; i < *len; i++)
//     {
//         if (invals[i].value > inoutvals[i].value)
//         {
//             inoutvals[i].value = invals[i].value;
//             inoutvals[i].index = invals[i].index;
//         }
//     }
//     return;
// }
// Inverser la matrice par la méthode de Gauss-Jordan; implantation MPI parallèle.
void invertParallel(Matrix &matrice)
{
    //cout << matrice.str() << endl;
    // Type de donnée d'une ligne de matrice
    typedef valarray<double> matData;
    // Structure pour trouver le maximum

    MPI_Datatype ind_et_val_types[2] = {MPI_DOUBLE, MPI_INT};

    MPI::Init();
    MPI::Status status;
    // Nombre de processus
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    // Rang du processus
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Vérifier que la matrice est carrée
    assert(matrice.rows() == matrice.cols());
    // Concatener la matrice à la matrice Identitée [ A I ]
    MatrixConcatCols matrice_et_id(matrice, MatrixIdentity(matrice.rows()));

    // Debut de l'algorithme
    //for (size_t idx_ligne = 0; idx_ligne < matrice.rows(); ++idx_ligne)
    for (size_t idx_ligne = 0; idx_ligne < 2; ++idx_ligne)
    {
        // Répartit les indices des colonnes dans les processeurs
        map<double, int> subMatrice_Col_Value;
        for (size_t idx_ligne_rest = idx_ligne; idx_ligne_rest < matrice.rows(); ++idx_ligne_rest)
            if (world_rank == (idx_ligne_rest % world_size))
                subMatrice_Col_Value.insert(pair<double, int>(matrice(idx_ligne_rest, idx_ligne), idx_ligne_rest));

        ind_and_val max_pivot_et_rang;
        cout << idx_ligne << "  " << world_rank << " : " << max_pivot_et_rang.value << " : " << max_pivot_et_rang.index << endl;

        MPI::COMM_WORLD.Allreduce(&subMatrice_Col_Value.rbegin()->first, &max_pivot_et_rang.value, 1, MPI_DOUBLE_INT, MPI_MAXLOC);
        if (world_rank == max_pivot_et_rang.index)
            MPI::COMM_WORLD.Bcast(&max_pivot_et_rang.index, 1, MPI_INT, world_rank);

        cout << idx_ligne << "  " << world_rank << " / " << max_pivot_et_rang.value << " / " << max_pivot_et_rang.index << endl;

        // // MPI_Datatype mpi_ind_and_val;
        // // MPI_Datatype types[2] = {MPI_DOUBLE, MPI_INT};
        // // MPI_Aint disps[2] = {offsetof(ind_and_val, val),
        // //                      offsetof(ind_and_val, index)};
        // // MPI_Type_create_struct(2, lens, disps, types, &mpi_dbl_twoindex);
        // // MPI_Type_commit(&mpi_dbl_twoindex);
        // // MPI_Op mpi_minloc_dbl_twoindex;
        // // MPI_Op_create(minloc_dbl_twoindex, 1, &mpi_minloc_dbl_twoindex);

        // // Vérifier que la matrice n'est pas singulière
        // // if (matrice_et_id(max_pivot_et_rang.index, idx_ligne) == 0)
        // //     throw runtime_error("Matrix not invertible");

        // // Échanger la ligne courante avec celle du pivot
        // // if (world_rank == 0)
        // //     cout << matrice.str() << endl;
        // // cout << "pivot idx : " << max_pivot_et_rang.index << "  pivot val : " << max_pivot_et_rang.value << "  ligne idx  " << idx_ligne << endl;
        // if (max_pivot_et_rang.index != idx_ligne)
        // {
        //     // Si l'indice de la ligne et celui du pivot appartiennent au meme processeur, operation immediate...
        //     cout << "Ligne  " << idx_ligne << "  pivot rk : " << max_pivot_et_rang.index % world_size << "  ligne rk : " << idx_ligne % world_size << "  rk  : " << world_rank << endl;
        //     if ((idx_ligne % world_size == max_pivot_et_rang.index % world_size) &&
        //         (max_pivot_et_rang.index % world_size == world_rank)) //effectuée par le processus
        //     {
        //         matrice_et_id.swapRows(max_pivot_et_rang.index, idx_ligne);
        //         cout << "Ligne  " << idx_ligne << "  pivot rk : " << max_pivot_et_rang.index % world_size << "  ligne rk : " << idx_ligne % world_size << "  rk  : " << world_rank << "Changement de ligne fait sur le même process" << endl;
        //         // cout << matrice_et_id.str() << endl;
        //     }
        //     // ... sinon, communication point à point entre les 2 processus concernés
        //     else
        //     {
        //         //cout << " Ligne    :   " << idx_ligne << endl;
        //         // int rank_pivot = max_pivot_et_rang.index % world_size;
        //         // int rank_ligne = idx_ligne % world_size;

        //         // valarray<double> line_to_swap = matrice_et_id.getRowSlice(idx_ligne);
        //         // if (max_pivot_et_rang.index % world_size == world_rank)
        //         //     cout << "Je suis le pivot et je sendrecv" << endl;
        //         // if (max_pivot_et_rang.index % world_size == world_rank)
        //         //     MPI::COMM_WORLD.Sendrecv_replace((void *)&line_to_swap, line_to_swap.size(), MPI::DOUBLE, idx_ligne % world_size, 666, max_pivot_et_rang.index % world_size, 666, status);
        //         if (world_rank == (idx_ligne % world_size))
        //         {
        //             //cout << "Je suis pas le pivot je récupère celle du pivot" << endl;
        //             valarray<double> ligne_pas_pivot = matrice_et_id.getRowSlice(idx_ligne);
        //             valarray<double> ligne_pivot(ligne_pas_pivot);
        //             // for (auto i : ligne_pivot)
        //             //     cout << "before send : " << i << "  ";
        //             // for (auto i : ligne_pas_pivot)
        //             //     cout << "Pas pivot " << i << ' ';
        //             // for (auto i : ligne_pivot)
        //             //     cout << "Pivot : " << i << ' ';
        //             // cout << endl;
        //             // cout << "Je vais envoyer ma ligne au pivot" << endl;
        //             //MPI::COMM_WORLD.Send((void *)&ligne_pas_pivot, ligne_pas_pivot.size(), MPI::DOUBLE, max_pivot_et_rang.index % world_size, 42);
        //             // cout << "J'ai envoyé ma ligne au pivot" << endl;
        //             // cout << "Je vais recevoir la ligne du pivot" << endl;
        //             //MPI::COMM_WORLD.Recv((void *)&ligne_pivot, ligne_pivot.size(), MPI::DOUBLE, max_pivot_et_rang.index % world_size, 666);
        //             // cout << "J'ai reçu la ligne du pivot" << endl;
        //             // for (auto i : ligne_pivot)
        //             //     cout << "after send : " << i << "  ";
        //             //matrice_et_id.getRowSlice(idx_ligne) = ligne_pivot;
        //             //cout << "J'ai changé ma ligne'" << endl;
        //         }
        //         if (world_rank == (max_pivot_et_rang.index % world_size))
        //         {
        //             cout << "Je suis le pivot je récupère l'autre" << endl;
        //             valarray<double> ligne_pivot = matrice_et_id.getRowSlice(max_pivot_et_rang.index);
        //             valarray<double> ligne_pas_pivot(ligne_pivot);

        //             // cout << "Je vais envoyer ma ligne a l'autre" << endl;
        //             //MPI::COMM_WORLD.Recv((void *)&ligne_pas_pivot, ligne_pas_pivot.size(), MPI::DOUBLE, idx_ligne % world_size, 42);
        //             // cout << "J'ai envoyé ma ligne a l'autre" << endl;
        //             // cout << "Je vais recevoir la ligne de l'autre" << endl;
        //             //MPI::COMM_WORLD.Send((void *)&ligne_pivot, ligne_pivot.size(), MPI_DOUBLE, idx_ligne % world_size, 666);
        //             // cout << "J'ai reçu la ligne de l'autre" << endl;

        //             //matrice_et_id.getRowSlice(max_pivot_et_rang.index) = ligne_pas_pivot;
        //             //cout << "J'ai changé ma ligne pivtor'" << endl;
        //             //usleep(10000000);
        //         }
        //     }
        // }
        // cout << "\n\n\n\n\n";
        // cout << "process  " << world_rank << endl;
        // cout << "LA MATRICE A ETE CHANGÉE" << endl;
        // cout << "Le pivot etait : " << max_pivot_et_rang.value << "  Indice  :  " << max_pivot_et_rang.index << endl;
        // cout << "MAtrice : \n"
        //      << matrice_et_id.str() << endl;
        //     //broadcast de la ligne idx_ligne à tous les processeurs...
        //     if (idx_ligne % world_size == world_rank)
        //     {
        //         valarray<double> ligne_bcast = matrice.getRowCopy(idx_ligne);
        //         MPI::COMM_WORLD.Bcast(&ligne_bcast, sizeof(valarray<double>), MPI_DOUBLE, world_rank);
        //     }
        //     int i = 0;
        //     for (i = 0; i < matrice.rows(); i++)
        //     {
        //         if (i % world_size == world_rank || i != idx_ligne)
        //         {
        //             //...qui calculent le facteur d'élimination l(ik) = A(ik)/A(kk)...  (equation 7.2)
        //             double fact_elim = matrice(idx_ligne, idx_ligne) / matrice(idx_ligne, idx_ligne);
        //             //...et effectuent l'élimination A(ij) = A(ij)-l(ik)A(kj)  (equation 7.3)
        //             matrice.getRowSlice(i) = matrice.getRowCopy(i) - fact_elim * matrice.getRowCopy(idx_ligne);
        //         }
        //         else if (i % world_size == world_rank || i == idx_ligne)
        //         {
        //             valarray<double> fact_elim(matrice.cols(), matrice(idx_ligne, idx_ligne));
        //             matrice.getRowSlice(i) /= fact_elim;
        //         }
        //     }
        // }
        // if (world_rank == 0)
        // {
        //     cout << matrice.str() << endl;
        //     cout << matrice_et_id.str() << endl;
        // }
    }
    MPI::Finalize();
}

// Multiplier deux matrices.
Matrix multiplyMatrix(const Matrix &iMat1, const Matrix &iMat2)
{

    // vérifier la compatibilité des matrices
    assert(iMat1.cols() == iMat2.rows());
    // effectuer le produit matriciel
    Matrix lRes(iMat1.rows(), iMat2.cols());
    // traiter chaque rangée
    for (size_t i = 0; i < lRes.rows(); ++i)
    {
        // traiter chaque colonne
        for (size_t j = 0; j < lRes.cols(); ++j)
        {
            lRes(i, j) = (iMat1.getRowCopy(i) * iMat2.getColumnCopy(j)).sum();
        }
    }
    return lRes;
}

int main(int argc, char **argv)
{

    srand((unsigned)time(NULL));

    unsigned int lS = 5;
    if (argc == 2)
    {
        lS = atoi(argv[1]);
    }

    MatrixRandom lA(lS, lS);
    // cout << "Matrice random:\n"
    //  << lA.str() << endl;

    Matrix lB(lA);
    // invertSequential(lB);
    // cout << "Matrice inverse:\n"
    //  << lB.str() << endl;

    //Matrix lRes = multiplyMatrix(lA, lB);
    // cout << "Produit des deux matrices:\n"
    //      << lRes.str() << endl;

    //cout << "Erreur: " << lRes.getDataArray().sum() - lS << endl;
    invertParallel(lA);
    return 0;
}

// if (idx_ligne % world_size == world_rank)
// {
//     matData ligne_pas_pivot = matrice.getRowCopy();
//     matData ligne_pivot;
//     MPI::COMM_WORLD.Send((void *)ligne_pas_pivot, sizeof(matData), matData, idx_pivot % world_size);
//     MPI::COMM_WORLD.Recv((void *)ligne_pivot, sizeof(matData), matData, idx_pivot % world_size);
// }
// matrice_et_id.swapRows(idx_pivot, idx_ligne);
// if (idx_pivot % world_size == world_rank)
// {
//     matData ligne_pivot;
//     ;
//     matData ligne_pas_pivot = matrice.getRowCopy();
//     MPI::COMM_WORLD.Recv((void *)ligne_rcv, sizeof(matData), matData, idx_pivot % world_size);
//     MPI::COMM_WORLD.Send((void *)ligne_send, sizeof(matData), matData, idx_ligne % world_size);
// }

// Renvoie l'indice du maximum et sa valeur pour les lignes sous la ligne courante
// vector<double>::iterator iter_max = max_element(subMatrice_Col_Value.begin(), subMatrice_Col_Value.end());
// pivot.index = distance(subMatrice_Pivot_Value.begin(), iter_max);

//pivot.value = *iter_max;
// ligne_ind_val.index = idx_ligne;
// ligne.ind_val.value = matrice(idx_ligne, idx_ligne);
