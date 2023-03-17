
#include "reduction.h"

int MAX_OP(int i1, int i2, int first_call)
{
    return (i1 < i2) ? i2 : i1;
}

int MIN_OP(int i1, int i2, int first_call)
{
    return (i1 < i2) ? i1 : i2;
}

int NNZ_MIN_OP(int i1, int i2, int first_call)
{
    return (i2 > 0) && (i2 < i1) ? i2 : i1;
}

int SUM_OP(int i1, int i2, int first_call)
{
    return i1 + i2;
}

int UNIT_SUM_OP(int i1, int i2, int first_call)
{
    return i2 > 0 ? i1 + 1 : i1;
}

int reduce_2d(int **arr, int num_elems, int *num_elems_2, int operation(const int, const int, const int), int initial_value)
{
    if (num_elems <= 1)
        return 0;

    int reduction_val = operation(initial_value, arr[0][0], 1);
    int init_j = 1;
    for (int i = 0; i < num_elems; i++)
    {
        for (int j = init_j; j < num_elems_2[i]; j++)
        {
            reduction_val = operation(reduction_val, arr[i][j], 0);
        }
        init_j = 0;
    }

    return reduction_val;
}

int reduce(int *arr, TENSORSIZE_T num_elems, int operation(const int, const int, const int), int initial_value)
{
    if (num_elems <= 1)
        return 0;

    int reduction_val = operation(initial_value, arr[0], 1);

    TENSORSIZE_T i; // TT: there was a warning to compare i & num_elems

    // for (int i = 1; i < num_elems; i++)
    for (i = 1; i < num_elems; i++)
    {
        reduction_val = operation(reduction_val, arr[i], 0);
    }

    return reduction_val;
}

int reduce_min(int *arr, TENSORSIZE_T num_elems)
{
    if (num_elems <= 1)
        return 0;

    int reduction_val = INT_MAX;

#pragma omp parallel for reduction(min \
                                   : reduction_val)
    // #pragma omp parallel for
    for (TENSORSIZE_T i = 0; i < num_elems; i++)
    {
        if (arr[i] < reduction_val)
        {
            // #pragma omp critical
            reduction_val = arr[i];
        }
    }

    return reduction_val;
}

int reduce_max(int *arr, TENSORSIZE_T num_elems)
{
    if (num_elems <= 1)
        return 0;

    int reduction_val = 0;

#pragma omp parallel for reduction(max \
                                   : reduction_val)
    for (TENSORSIZE_T i = 0; i < num_elems; i++)
    {
        if (arr[i] > reduction_val)
            reduction_val = arr[i];
    }

    return reduction_val;
}

int reduce_sum(int *arr, TENSORSIZE_T num_elems)
{
    if (num_elems <= 1)
        return 0;

    int reduction_val = 0;

#pragma omp parallel for reduction(+ \
                                   : reduction_val)
    for (TENSORSIZE_T i = 0; i < num_elems; i++)
    {
        reduction_val += arr[i];
    }

    return reduction_val;
}

int reduce_adjNnzPerSlice(int order, int *dim, TENSORSIZE_T num_slices, int *nnzPerSlice)
{
    int total = 0;
    int index_offset = 0;
    for (int i = 0; i < order; i++)
    {
        for (int j = 1; j < dim[i]; j++)
        {
            total += abs(nnzPerSlice[index_offset + j] - nnzPerSlice[index_offset + j - 1]);
        }
        index_offset += dim[i];
    }
    return ((double)total) / num_slices;
}

int reduce_adjNnzPerFiber(int order, int *dim, TENSORSIZE_T num_fibers, int *nnzPerFiber)
{
    return 0;
}
