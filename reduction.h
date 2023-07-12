#include <stdio.h>
#include <stdlib.h>
#include "util.h"

int reduce_min(int *arr, TENSORSIZE_T num_elems);
int reduce_max(int *arr, TENSORSIZE_T num_elems);
TENSORSIZE_T reduce_sum(int *arr, TENSORSIZE_T num_elems);

int reduce(int *arr, TENSORSIZE_T num_elems, int operation(const int, const int, const int), int initial_value);
int reduce_2d(int **arr, int num_elems, int *num_elems_2, int operation(const int, const int, const int), int initial_value);

int reduce_adjNnzPerSlice(int order, int *dim, TENSORSIZE_T num_slices, int *nnzPerSlice);
int reduce_adjNnzPerFiber(int order, int *dim, TENSORSIZE_T num_fibers, int *nnzPerFiber);

int SUM_OP(int i1, int i2, int first_call);
int MIN_OP(int i1, int i2, int first_call);
int NNZ_MIN_OP(int i1, int i2, int first_call);
int MAX_OP(int i1, int i2, int first_call);
int UNIT_SUM_OP(int i1, int i2, int first_call);
