#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "reduction.h"

#define DEBUG 0

#define CHOICE_nzPerSlice 0
#define CHOICE_fibersPerSlice 1
#define CHOICE_nzPerFiber 2

struct SliceMap
{
    int **indices;
    int ignored_dim1;
    int ignored_dim2;
    int order;

    struct Hasher
    {
        int **indices;
        int ignored_dim1;
        int ignored_dim2;
        int order;

        bool pad_first = false;

        Hasher(int **indices,
               int ignored_dim1,
               int ignored_dim2,
               int order) : indices(indices),
                            ignored_dim1(ignored_dim1),
                            ignored_dim2(ignored_dim2),
                            order(order)
        {
        }

        size_t operator()(const int &index) const
        {
            int hash = order;
            bool flag = pad_first;

            for (int j = 0; j < order; j++)
            {
                if (j == ignored_dim1 || j == ignored_dim2)
                    continue;

                hash ^= indices[j][flag ? index + 1 : index] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                if (flag)
                    flag = false;
            }
            return hash;
        };
    };

    struct EqualTo
    {
        int **indices;
        int ignored_dim1;
        int ignored_dim2;
        int order;

        bool pad_first = false;

        EqualTo(int **indices,
                int ignored_dim1,
                int ignored_dim2,
                int order) : indices(indices),
                             ignored_dim1(ignored_dim1),
                             ignored_dim2(ignored_dim2),
                             order(order)
        {
        }

        bool operator()(const int &i1, const int &i2) const
        {
            bool flag = pad_first;
            for (int j = 0; j < order; j++)
            {
                if (j == ignored_dim1 || j == ignored_dim2)
                    continue;

                if (indices[j][flag ? i1 + 1 : i1] != indices[j][flag ? i2 + 1 : i2])
                    return false;

                if (flag)
                    flag = false;
            }
            return true;
        };
    };

    Hasher hasher;
    EqualTo equal_to;

    std::unordered_map<int, int, Hasher, EqualTo> map;

    SliceMap(int **indices, int ignored_dim1, int ignored_dim2, int order) : indices(indices),
                                                                             ignored_dim1(ignored_dim1),
                                                                             ignored_dim2(ignored_dim2),
                                                                             order(order),
                                                                             hasher(Hasher(indices, ignored_dim1, ignored_dim2, order)),
                                                                             equal_to(EqualTo(indices, ignored_dim1, ignored_dim2, order)),
                                                                             map(100, hasher, equal_to){};

    bool increment(int index)
    {
        if (map.find(index) != map.end())
        {
            map[index]++;
            return false;
        }
        else
        {
            map[index] = 1;
        }

        return true;
    }
};

struct FiberMap
{
    int **indices;
    int ignored_dim;
    int order;

    struct Hasher
    {
        int **indices;
        int ignored_dim;
        int order;

        Hasher(int **indices,
               int ignored_dim,
               int order) : indices(indices),
                            ignored_dim(ignored_dim),
                            order(order)
        {
        }

        size_t operator()(const int &index) const
        {
            int hash = order;
            for (int j = 0; j < order; j++)
            {
                if (j == ignored_dim)
                    continue;

                hash ^= indices[j][index] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        };
    };

    struct EqualTo
    {
        int **indices;
        int ignored_dim;
        int order;

        EqualTo(int **indices,
                int ignored_dim,
                int order) : indices(indices),
                             ignored_dim(ignored_dim),
                             order(order)
        {
        }

        bool operator()(const int &i1, const int &i2) const
        {
            for (int j = 0; j < order; j++)
            {
                if (j == ignored_dim)
                    continue;

                if (indices[j][i1] != indices[j][i2])
                    return false;
            }
            return true;
        };
    };

    std::unordered_map<int, int, Hasher, EqualTo> map;

    FiberMap(int **indices, int ignored_dim, int order) : indices(indices),
                                                          ignored_dim(ignored_dim),
                                                          order(order),
                                                          map(100, Hasher(indices, ignored_dim, order), EqualTo(indices, ignored_dim, order)){};

    bool increment(int index)
    {
        if (map.find(index) != map.end())
        {
            map[index]++;
            return false;
        }
        else
        {
            map[index] = 1;
        }

        return true;
    }
};

template <typename KeyType>
struct VectorHasher
{
    int operator()(const std::vector<KeyType> &V) const
    {
        int hash = V.size();
        for (auto &i : V)
        {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

namespace
{
    inline bool increment(std::unordered_map<std::vector<int>, int, VectorHasher<int>> &map, std::vector<int> &key)
    {
        if (map.find(key) != map.end())
        {
            map[key]++;
            return false;
        }
        else
        {
            map[key] = 1;
        }

        return true;
    }
}

enum EXTRACTION_METHOD
{
    MAP,
    SORT,
	FRAGMENT,
	HYBRID,
    COO //unused yet.
};

typedef struct
{
    int size0;
    int *cnt0;
    int *ind0;
    int *size1;

    // int** ind1;
    // int** cnt1;
    int size1_tot;
    int *ind1;
    int *cnt1;
} dim2_tensor_fragment;

typedef struct
{
	char *tensor_name;
    double *values;
    int **indices;
    int *dim;
	int *org_dim;
    int order;
	int org_order;
    TENSORSIZE_T nnz;
} tensor;

typedef struct
{
    int *csr_rowptr;
    int *csr_cols;
    double *csr_vals;

    int *dim;
    int order;
    TENSORSIZE_T nnz;

} csr_tensor;

typedef struct
{
    int *csf_ptr1;
    int *csf_inds1;

    int *csf_ptr2;
    int *csf_inds2;

    int *csf_ptr3;
    int *csf_inds3;
    double *csf_vals;

    int *inds_size;
    int *ptr_size;

    int *dim;
    int order;
    TENSORSIZE_T nnz;
} csf_tensor;

typedef struct
{
    TENSORSIZE_T sliceCnt, fiberCnt, nnz;
    double sparsity, avgNnzPerFiber, avgNnzPerSlice, avgFibersPerSlice;
    double cvNnzPerFiber, cvNnzPerSlice, cvFibersPerSlice;
    double stDevNnzPerSlice, stDevNnzPerFiber, stDevFibersPerSlice;

    int nnzSliceCnt, nnzFiberCnt;
    int maxNnzPerSlice, minNnzPerSlice, devNnzPerSlice;
    int maxNnzPerFiber, minNnzPerFiber, devNnzPerFiber;
    int maxFibersPerSlice, minFibersPerSlice, devFibersPerSlice;
    long double adjNnzPerFiber, adjNnzPerSlice, adjFibersPerSlice;

} tensor_features;

struct base_features
{
	char *tensor_name;
	int *dim;
    int order;
	int org_order;
    TENSORSIZE_T sliceCnt, fiberCnt, nnz;
    double sparsity;
    int nnzSliceCnt, nnzFiberCnt;
};

struct mode_features
{
	int ignored_dim1;
    int ignored_dim2;
	TENSORSIZE_T all_cnt, sum;
    int nz_cnt, max, min, dev;
    double avg;
    double cv;
    double stDev;
	double avg_onlynz;
    double cv_onlynz;
    double stDev_onlynz;
};

struct mode_based_features
{
    base_features *base;
    mode_features **slice;
    mode_features **fiber;
    mode_features **fps;
};

void sort_tensor(tensor *T, int mode = 0);

csr_tensor *coo2csr(tensor *T);
dim2_tensor_fragment *coo2fragment(tensor *T, int mode_order1, int mode_order2);

tensor *create_tensor(int order, TENSORSIZE_T nnz);
tensor *read_tensor(FILE *file_ptr, int order, TENSORSIZE_T nnz);
tensor *read_tensor_binary(FILE *file_ptr, int order, TENSORSIZE_T nnz);

void tensor_to_3d (tensor *T, tensor * T3d);
void find_max3 ( int * arr, int arr_size, int *max_arr );

TENSORSIZE_T calculate_num_slices(int order, int *dim);
TENSORSIZE_T calculate_num_fibers(int order, int *dim);

void calculate_nnzPerSlice_fragment(dim2_tensor_fragment **fragments, int num_fragments, int *nnzPerSlice);

double calculate_sparsity(TENSORSIZE_T nnz, int order, int *dim);
double calculate_std(int *arr, int arr_size, TENSORSIZE_T num_elems, double mean);

mode_based_features *extract_features(tensor *T, enum EXTRACTION_METHOD method);
mode_based_features *extract_features_fragment(tensor *T);
mode_based_features *extract_features_sort(tensor *T);
mode_based_features *extract_features_hybrid(tensor *T);

mode_based_features *extract_features_modes(tensor *T);

std::string tensor_features_to_json(tensor_features *features);
std::string all_mode_features_to_json(mode_based_features *features, tensor *T);
std::string mode_features_to_json(mode_features *features);
std::string mode_features_to_json_fibers(mode_features *features);
std::string base_features_to_json(base_features *features);

std::string all_mode_features_to_csv(mode_based_features *features, tensor *T);
std::string mode_features_to_csv(mode_features *features);
std::string mode_features_to_csv_fibers(mode_features *features);
std::string base_features_to_csv(base_features *features);

std::string org_dim_to_json(tensor *T);
std::string org_dim_to_csv(tensor *T);

void print_tensor_features(tensor_features *features);
void print_csr_tensor(csr_tensor *T);
void print_csf_tensor(csf_tensor *T);

// tensor_features *create_tensor_features();
// csr_tensor *create_csr_tensor(int nnz, int *dim);
// csf_tensor *create_csf_tensor(int nnz, int *dim);

// void calculate_nnzPerSlice(tensor *T, int order, int *dim, int *nnzPerSlice);
// void calculate_nnzPerFiber(tensor *T, int order, int *dim, int *nnzPerFiber);
// void calculate_fibersPerSlice(tensor *T, int order, int *dim, int *fibersPerSlice);

// int calculate_adjPerSlice(int order, int *dim, int *nnzPerSlice);
// int calculate_adjPerFiber(int order, int *dim, int *nnzPerFiber);
// int calculate_adjacent_fragment(dim2_tensor_fragment **fragments, int num_fragments, int *dim, int arr_choice);

// tensor_features *extract_features_coo(tensor *T);
// tensor_features *extract_features_map(tensor *T, int mode);