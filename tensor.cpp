#include "tensor.h"
#include <iostream>
#include <unordered_map>
#include <map>
#include <numeric>
#include <memory>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>
#include <algorithm>
#include <parallel/algorithm>
#include <parallel/numeric>

#define PRINT_HEADER 0
#define PRINT_DEBUG 0

#define LAMBDA 100000000000ULL

/*

    Tensor Feature Extraction
    -------------------------
*/

using namespace std;

int comb(int n, int r)
{
    if (r == 0)
        return 1;

    if (r > n / 2)
        return comb(n, n - r);

    long res = 1;

    for (int k = 1; k <= r; ++k)
    {
        res *= n - k + 1;
        res /= k;
    }

    return res;
}

void sort_tensor(tensor *T, int mode)
{

    std::vector<std::tuple<std::vector<int>, double>> sort_vec;

    for (TENSORSIZE_T i = 0; i < T->nnz; i++)
    {
        int value = (T->values != nullptr) ? T->values[i] : 0;

        std::vector<int> c_indices;
        for (int j = 0; j < T->order; j++)
            c_indices.push_back(T->indices[(j + mode) % T->order][i]);

        sort_vec.emplace_back(c_indices, value);
    }

    std::sort(sort_vec.begin(), sort_vec.end(),
              [T](std::tuple<std::vector<int>, double> t1,
                  std::tuple<std::vector<int>, double> t2)
              {
                  for (int j = 0; j < T->order; j++)
                  {
                      if (std::get<0>(t1)[j] != std::get<0>(t2)[j])
                          return std::get<0>(t1)[j] < std::get<0>(t2)[j];
                  }
                  return false;
              });

    for (TENSORSIZE_T i = 0; i < T->nnz; i++)
    {
        auto &t = sort_vec[i];
        for (int j = 0; j < T->order; j++)
        {
            T->indices[j][i] = std::get<0>(t)[j];
        }

        if (T->values != nullptr)
        {
            T->values[i] = std::get<1>(t);
        }
    }

    int dim_temp[T->order];
    for (int i = 0; i < T->order; i++)
        dim_temp[i] = T->dim[(i + mode) % T->order];

    for (int i = 0; i < T->order; i++)
        T->dim[i] = dim_temp[i];
}

dim2_tensor_fragment *coo2fragment(tensor *T, int mode_order1, int mode_order2)
{
    // int i, j, k;

    int I0 = T->dim[mode_order1];
    int I1 = T->dim[mode_order2];

    TENSORSIZE_T nnz = T->nnz;
    int *i0 = T->indices[mode_order1];
    int *i1 = T->indices[mode_order2];

    int *temp_cnt0 = (int *)safe_calloc(I0, sizeof(int));

    // #pragma omp parallel for
    for (TENSORSIZE_T i = 0; i < nnz; i++)
    {
        temp_cnt0[i0[i]]++;
    }

    int size0 = 0;

	#pragma omp parallel for reduction(+ : size0)
    for (int i = 0; i < I0; i++)
    {
        if (temp_cnt0[i] > 0)
        {
            size0 += 1;
        }
    }

    // printf("%d \t\t :  size0 \n", size0);

    int *cnt0 = (int *)safe_malloc(size0 * sizeof(int));
    int *ind0 = (int *)safe_malloc(size0 * sizeof(int));

    int *strt0 = (int *)safe_malloc((size0 + 1) * sizeof(int));
    strt0[0] = 0;

    int j = 0;
    // #pragma omp parallel for
    for (int i = 0; i < I0; i++)
    {
        int temp_cnt0_i = temp_cnt0[i];
        if (temp_cnt0_i != 0)
        {
            // #pragma omp critical
            // {
            cnt0[j] = temp_cnt0_i;
            ind0[j] = i;
            strt0[j + 1] = strt0[j] + temp_cnt0_i;
            j++;
            // }
        }
    }

    free(temp_cnt0);

    int size_temp_loc0 = ind0[size0 - 1] + 1;

    int *temp_loc0 = (int *)safe_calloc(size_temp_loc0, sizeof(int));

	#pragma omp parallel for
    for (int i = 0; i < size0; i++)
    {
        temp_loc0[ind0[i]] = strt0[i];
    }

    // timer *order0_tm = timer_start("time_order0");
    int *order0 = (int *)safe_calloc(nnz, sizeof(int));

    // #pragma omp parallel for
    for (TENSORSIZE_T k = 0; k < nnz; k++)
    {
        int coo_ind0 = i0[k];
        order0[temp_loc0[coo_ind0]++] = k;
    }
    // timer_end(order0_tm);
    // timer_end(level0_tm);

    free(temp_loc0);

    int *size1 = (int *)safe_calloc(size0, sizeof(int));
    int **ind1 = (int **)safe_malloc(size0 * sizeof(int *));
    int **cnt1 = (int **)safe_malloc(size0 * sizeof(int *));

    // int *tmp_cnt1 = (int *)safe_calloc(I1, sizeof(int));
    // int *tmp_cnt1 = (int *)safe_malloc(I1 * sizeof(int));

    int *size1_start = (int *)safe_malloc((size0 + 1) * sizeof(int));
    size1_start[0] = 0;

    // timer *level1_tm = timer_start("time_level1");

	// double time_tmp_cnt1=0;

	#pragma omp parallel
    {
        int *tmp_cnt1 = (int *)safe_malloc(I1 * sizeof(int)); // TT REUSE ALLOC
		#pragma omp for
        for (int i = 0; i < size0; i++)
        {
            // timer *sub_level1_tm = timer_start("sub_level1");

            // #pragma omp parallel for
            for (int j = 0; j < I1; j++)
            {
                tmp_cnt1[j] = 0;
            }
            int size1_i = 0;

            // double time_start = omp_get_wtime();

            // #pragma omp parallel for
            for (int j = strt0[i]; j < strt0[i + 1]; j++)
            {
                int curr_i1 = i1[order0[j]];
                tmp_cnt1[curr_i1]++;
                if (tmp_cnt1[curr_i1] == 1) // TT: MERGE HERE
                    size1_i++;
            }

            // double time_tmp_cnt1 += omp_get_wtime() - time_start;	// TT :: check times seperately

            // #pragma omp parallel for reduction(+: size1_i)
            // for ( j = 0; j < I1; j++){
            // if(tmp_cnt1[j] > 0)
            // size1_i += 1;
            // }

            // TT: CHECK MALLOC TIMES
            // size1[i] = reduce(tmp_cnt1, I1, UNIT_SUM_OP, 0);
            cnt1[i] = (int *)calloc(size1_i, sizeof(int));
            ind1[i] = (int *)calloc(size1_i, sizeof(int));

            // int* cnt1_i = cnt1[i];
            // int* ind1_i = ind1[i];

            size1[i] = size1_i;

            int k = 0;
            // #pragma omp parallel for
            for (int z = 0; z < I1; z++)
            {
                int tmp_cnt1_z = tmp_cnt1[z];
                if (tmp_cnt1_z != 0)
                {
                    // cnt1_i[k]=tmp_cnt1[z];
                    // ind1_i[k]=z;
                    cnt1[i][k] = tmp_cnt1_z;
                    ind1[i][k] = z;
                    // #pragma omp atomic
                    k++;
                }
            }
        }
        free(tmp_cnt1);
        // memset(tmp_cnt1, 0, sizeof(int)*I1);
        // timer_end(sub_level1_tm);
    }

    // timer_end(level1_tm);

    // free(tmp_cnt1);
    free(order0);
    free(strt0);

    for (int i = 0; i < size0; i++)
    {
        size1_start[i + 1] = size1_start[i] + size1[i];
    }

    int size1_tot = size1_start[size0];

    int *ind1_all = (int *)safe_malloc(size1_tot * sizeof(int));
    int *cnt1_all = (int *)safe_malloc(size1_tot * sizeof(int));

	#pragma omp parallel for
    for (int i = 0; i < size0; i++)
    {
        int strt = size1_start[i];
        int end = size1_start[i + 1];
        for (j = strt; j < end; j++)
        {
            int j_strt = j - strt;
            ind1_all[j] = ind1[i][j_strt];
            cnt1_all[j] = cnt1[i][j_strt];
        }
    }

    for (int i = 0; i < size0; i++)
    {
        free(ind1[i]);
        free(cnt1[i]);
    }
    free(ind1);
    free(cnt1);

    dim2_tensor_fragment *fragment = (dim2_tensor_fragment *)safe_malloc(sizeof(dim2_tensor_fragment));
    fragment->cnt0 = cnt0;
    fragment->ind0 = ind0;

    // fragment->ind1 = ind1;
    // fragment->cnt1 = cnt1;
    fragment->size1_tot = size1_tot;
    fragment->ind1 = ind1_all;
    fragment->cnt1 = cnt1_all;

    fragment->size0 = size0;
    fragment->size1 = size1;
	
	// timer_end(mode_tm);

    return fragment;
}


tensor *create_tensor(int order, TENSORSIZE_T nnz)
{
    tensor *T = (tensor *)safe_malloc(sizeof(tensor));

    T->indices = (int **)safe_malloc(order * sizeof(int *));
    T->dim = (int *)safe_malloc(order * sizeof(int));

    for (int i = 0; i < order; i++)
    {
        T->indices[i] = (int *)safe_malloc(nnz * sizeof(int));
    }

    T->values = (double *)safe_malloc(nnz * sizeof(double));
    T->order = order;
    T->nnz = nnz;
    return T;
}

void tensor_to_3d (tensor *T, tensor * T3d){
	
	int max_arr [6];
	
	T3d->org_order = T->order;
	for (int i = 0; i <T->order; i++){
		T3d->org_dim[i] = T->dim[i];
	}
	
	find_max3 ( T->dim, T->order, max_arr );
	
	// update dim of T3d
	for (int i=0; i<3; i++){
		T3d->dim [i] = max_arr[i+3] ;
		// T3d->dim [i] = T->dim [ max_arr[i] ];
		for (TENSORSIZE_T j=0; j<T3d->nnz; j++){
			T3d->indices[i][j] = T->indices[max_arr[i]][j];
		}
	}
	
	for (int i = 0; i < T->order; i++){
        free(T->indices[i]);
    }
	free(T->indices);
	free(T->values);

}


void find_max3 ( int * arr, int arr_size, int * max_arr ) {
	
	int first, second, third, first_i, second_i, third_i;
 
    third = first = second = first_i = second_i = third_i = 0;
	int curr;
	
	
	for(int i = 0; i < arr_size; i++)
    {
         curr = arr[i];
		 
        // If current element is
        // greater than first
        if (curr > first)
        {
            third = second;
            second = first;
            first = curr;
			
			third_i = second_i;
            second_i = first_i;
            first_i = i;
        }
 
        // If arr[i] is in between first
        // and second then update second
        else if (curr > second )
        {
            third = second;
            second = curr;
			
			third_i = second_i;
            second_i = i;
        }
 
        else if (curr > third ){
            third = curr;
			third_i = i;
		}
    }
	
	max_arr[0] = first_i;
	max_arr[1] = second_i;
	max_arr[2] = third_i;
	max_arr[3] = first;
	max_arr[4] = second;
	max_arr[5] = third;
	
}


tensor *read_tensor(FILE *file_ptr, int order, TENSORSIZE_T nnz)
{

    size_t tensor_MAXLINE = 500; // May be inline
    char line[tensor_MAXLINE];
	int curr_ind, max_dim;

    tensor *T = create_tensor(order, nnz);

    char *fgets_out = NULL;
	
	fgets_out = fgets(line, tensor_MAXLINE, file_ptr);
	fgets_out = fgets(line, tensor_MAXLINE, file_ptr);

    // read from file to tensor in COO format
    for (TENSORSIZE_T i = 0; i < nnz; i++)
    {
        fgets_out = fgets(line, tensor_MAXLINE, file_ptr);

        char *token = strtok(line, " \t");
        // loop through the string to extract all other tokens
        for (int j = 0; j < order; j++)
        {
            // T->indices[j][i] = atoi(token);
            T->indices[j][i] = atoi(token) - 1;
            token = strtok(NULL, " \t");
        }
        char *dummy;
        T->values[i] = strtod(token, &dummy);
    }
	
	if (fgets_out == NULL)
        printf("\n fgets output is NULL! \n\n");
	
	// find dim
	for (int j = 0; j < order; j++){
		max_dim = 0;
		for (TENSORSIZE_T i = 0; i < nnz; i++){
			curr_ind = T->indices[j][i];
			if(max_dim < curr_ind ){ 
				max_dim = curr_ind ;
			}
		}
		T->dim[j] = max_dim + 1 ;
	}

    return T;
}


tensor *read_tensor_binary(FILE *file_ptr, int order, TENSORSIZE_T nnz)
{

    size_t tensor_MAXLINE = 500; // May be inline
    char line[tensor_MAXLINE];
	int curr_ind, max_dim;

    tensor *T = create_tensor(order, nnz);

    char *fgets_out = NULL;

    // read from file to tensor in COO format
    for (TENSORSIZE_T i = 0; i < nnz; i++)
    {
        fgets_out = fgets(line, tensor_MAXLINE, file_ptr);

        char *token = strtok(line, " \t");
        // loop through the string to extract all other tokens
        for (int j = 0; j < order; j++)
        {
            T->indices[j][i] = atoi(token) - 1;
            token = strtok(NULL, " \t");
        }
    }
	
	if (fgets_out == NULL)
        printf("\n fgets output is NULL! \n\n");
	
	// find dim
	for (int j = 0; j < order; j++){
		max_dim = 0;
		for (TENSORSIZE_T i = 0; i < nnz; i++){
			curr_ind = T->indices[j][i];
			if(max_dim < curr_ind ){ 
				max_dim = curr_ind ;
			}
		}
		T->dim[j] = max_dim + 1 ;
	}

    return T;
}


TENSORSIZE_T calculate_num_slices(int order, int *dim)
{
    TENSORSIZE_T num_slices = 0;
    for (int i = 0; i < order - 1; i++)
    {
        for (int j = i + 1; j < order; j++)
        {
            TENSORSIZE_T mult = 1;
            for (int k = 0; k < order; k++)
            {
                if (k != i && k != j)
                    mult *= dim[k];
            }
            num_slices += mult;
        }
    }

    return num_slices;
}

TENSORSIZE_T calculate_num_fibers(int order, int *dim)
{
    TENSORSIZE_T num_fibers = 0;
    for (int i = 0; i < order; i++)
    {
        TENSORSIZE_T mult = 1;
        for (int j = 0; j < order; j++)
            if (j != i)
                mult *= dim[j];
        num_fibers += mult;
    }
    return num_fibers;
}


/* sparsity */
double calculate_sparsity(TENSORSIZE_T nnz, int order, int *dim)
{
	
	double density = (nnz + 0.0) / dim[0]; 

    for (int i = 1; i < order; i++)
        density =  density / dim[i];

    return density;
}

/*Calculates std for nonzeros. Discards 0 values */
double calculate_std(int *arr, int arr_size, TENSORSIZE_T num_elems, double mean)
{
    double c_mean;
    if (mean == -1)
        c_mean = ((double)reduce_sum(arr, arr_size)) / num_elems;
    else
        c_mean = mean;

    double sqr_sum = 0;

#pragma omp parallel for reduction(+ : sqr_sum)
    for (int i = 0; i < arr_size; i++)
    {
        // if(arr[i]==0) continue;	// TT: already all entries nonzero !
        double mean_diff = arr[i] - c_mean;
        sqr_sum += mean_diff * mean_diff;
    }
	
	// If we want std of all elements including zero !
	if (num_elems - arr_size > 0){
		sqr_sum += (num_elems-arr_size) * c_mean * c_mean;
	}

    return sqrt(sqr_sum / num_elems);
}


void calculate_nnzSliceCnt_fragment(dim2_tensor_fragment **fragments, int num_fragments, int *offsets)
{
    offsets[0] = 0;

    for (int i = 0; i < num_fragments; i++)
    {
        offsets[i+1] = offsets[i] + fragments[i]->size0;
    }
}

void calculate_nnzFiberCnt_fragment(dim2_tensor_fragment **fragments, int num_fragments, int* offsets)
{
    offsets[0] = 0;

    for (int i = 0; i < num_fragments; i++){
        fragments[i]->size1_tot = reduce_sum(fragments[i]->size1, fragments[i]->size0);
        offsets[i+1] = offsets[i] + fragments[i]->size1_tot;
    }
}

void calculate_nnzPerSlice_fragment(dim2_tensor_fragment **fragments, int num_fragments, int *nnzPerSlice)
{
    int start_index = 0;
    for (int i = 0; i < num_fragments; i++)
    {
        dim2_tensor_fragment *curr_frag = fragments[i];

        int curr_frag_size0 = curr_frag->size0;

#pragma omp parallel for
        for (int j = 0; j < curr_frag_size0; j++)
            nnzPerSlice[start_index + j] = curr_frag->cnt0[j];

        start_index += curr_frag_size0;
    }
}

void calculate_nnzPerFiber_fragment(dim2_tensor_fragment **fragments, int num_fragments, int *nnzPerFiber)
{
    int start_index = 0;
    for (int i = 0; i < num_fragments; i++)
    {
        dim2_tensor_fragment *curr_frag = fragments[i];

        int curr_frag_size1_tot = curr_frag->size1_tot;

#pragma omp parallel for
        for (int j = 0; j < curr_frag_size1_tot; j++)
            nnzPerFiber[start_index + j] = curr_frag->cnt1[j];

        start_index += curr_frag_size1_tot;

        // for(int j=0; j<cur_frag->size0; j++){
        // memcpy(nnzPerFiber + start_index, cur_frag->cnt1[j], (size_t)sizeof(int) * cur_frag->size1[j]);
        // start_index += cur_frag->size1[j];
        // }
    }
}

void calculate_fibersPerSlice_fragment(dim2_tensor_fragment **fragments, int num_fragments, int *fibersPerSlice)
{
    int start_index = 0;

    for (int i = 0; i < num_fragments; i++)
    {

        dim2_tensor_fragment *curr_frag = fragments[i];

        int curr_frag_size0 = curr_frag->size0;

#pragma omp parallel for
        for (int j = 0; j < curr_frag_size0; j++)
            fibersPerSlice[start_index + j] = curr_frag->size1[j];

        start_index += curr_frag_size0;
    }
}


mode_based_features *extract_features(tensor *T, enum EXTRACTION_METHOD method)
{
    switch (method)
    {
    case MAP:
        return extract_features_modes(T);
        break;
    case SORT:
        return extract_features_sort(T);
        break;
	case FRAGMENT:
        return extract_features_fragment(T);
        break;
	case HYBRID:
		return extract_features_hybrid(T);
    default:
        return extract_features_sort(T);
        break;
    }
}


void print_vec ( int *array, int array_size)
{
	for (int i = 0; i<array_size; i++){
		printf ("%d ", array[i]);
	}
	
}


void extract_final_mode(mode_features *features, TENSORSIZE_T allCnt, int *nnzPerX, int nnzPerX_size)
{
    features->nz_cnt = nnzPerX_size;
	features->max = reduce_max(nnzPerX, nnzPerX_size);
    features->min = reduce_min(nnzPerX, nnzPerX_size);
	features->dev = features->max - features->min;
	
	if (PRINT_DEBUG){
		printf(" ig_dim1 : %d, ig_dim2 : %d, ", features->ignored_dim1, features->ignored_dim2);
		printf(" all_cnt : %Ld, nz_cnt: %d, array : [ ", allCnt, nnzPerX_size);
		print_vec (nnzPerX, nnzPerX_size);
		printf(" ] ");
	}
	
	TENSORSIZE_T sum = reduce_sum(nnzPerX, nnzPerX_size);
	
	double avg = (sum + 0.0) / allCnt;
	features->sum = sum;
    features->avg = avg;
	
	features->stDev = calculate_std(nnzPerX, nnzPerX_size, allCnt, avg);
	features->cv = features->stDev / avg;
	
	// This mean is the mean of only nonzero values !
	features->avg_onlynz = (sum + 0.0) / nnzPerX_size;
	// printf(" sum : %ld, avg: %f, avg_onlynz : %f \n", sum, avg, features->avg_onlynz);
	
	// This std is the std of only nonzero values !
    features->stDev_onlynz = calculate_std(nnzPerX, nnzPerX_size, nnzPerX_size, features->avg_onlynz);
	features->cv_onlynz = features->stDev_onlynz / features->avg_onlynz;
	
}


int calculate_nnz_per_slice_sort(tensor *T, int *nnz_per_slice, int *adj_nnz_per_slice)
{
    nnz_per_slice[0] = 1;

   // bool adj = false;
    int offset = 0;

    for (TENSORSIZE_T i = 1; i < T->nnz; i++)
    {
        if (T->indices[0][i - 1] != T->indices[0][i])
        {
            /*
            if (offset > 0)
            {
                if (adj)
                    *adj_nnz_per_slice += abs(nnz_per_slice[offset] - nnz_per_slice[offset - 1]);
                else
                    *adj_nnz_per_slice += nnz_per_slice[offset] + nnz_per_slice[offset - 1];
            }
            */

            //adj = T->indices[0][i] - T->indices[0][i - 1] == 1;

            offset++;
        }

        nnz_per_slice[offset]++;
    }
    /*
    if (T->indices[0][0] > 0)
        *adj_nnz_per_slice += nnz_per_slice[0];
    if (T->indices[0][T->nnz - 1] < T->dim[0] - 1)
        *adj_nnz_per_slice += nnz_per_slice[offset];
    */
    return offset + 1;
}

int calculate_nnz_per_fiber_sort(tensor *T, int *nnz_per_fiber, int *adj_nnz_per_fiber)
{
    int offset = 0;
    nnz_per_fiber[0] = 1;

//    bool adj = false;

    for (TENSORSIZE_T i = 1; i < T->nnz; i++)
    {
        if (T->indices[0][i - 1] != T->indices[0][i] ||
            T->indices[1][i - 1] != T->indices[1][i])
        {
            /*
            if (offset > 0)
            {
                if (adj)
                    *adj_nnz_per_fiber += abs(nnz_per_fiber[offset] - nnz_per_fiber[offset - 1]);
                else
                    *adj_nnz_per_fiber += nnz_per_fiber[offset] + nnz_per_fiber[offset - 1];
            }

            adj = (T->indices[0][i] - T->indices[0][i - 1]) == 1;*/

            offset++;
        }

        nnz_per_fiber[offset]++;
    }
/*
    if (T->indices[0][0] > 0)
        *adj_nnz_per_fiber += nnz_per_fiber[0];
    if (T->indices[0][T->nnz - 1] < T->dim[0] - 1)
        *adj_nnz_per_fiber += nnz_per_fiber[offset];
*/
    return offset + 1;
}

int calculate_fibers_per_slice_sort(tensor *T, int *fibers_per_slice, int *adj_fibers_per_slice)
{
    int offset = 0;
    //bool adj = false;
    fibers_per_slice[0] = 1;
    for (TENSORSIZE_T i = 1; i < T->nnz; i++)
    {
        if (T->indices[0][i - 1] != T->indices[0][i])
        {
            /*
            if (offset > 0)
            {
                if (adj)
                    *adj_fibers_per_slice += abs(fibers_per_slice[offset] - fibers_per_slice[offset - 1]);
                else
                    *adj_fibers_per_slice += fibers_per_slice[offset] + fibers_per_slice[offset - 1];
            }

            adj = T->indices[0][i] - T->indices[0][i - 1] == 1;*/
            offset++;
            fibers_per_slice[offset] = 1;
        }
        else
        {
            fibers_per_slice[offset] += (T->indices[1][i - 1] != T->indices[1][i]);
        }
    }
    /*
    if (T->indices[0][0] > 0)
        *adj_fibers_per_slice += fibers_per_slice[0];
    if (T->indices[0][T->nnz - 1] < T->dim[0] - 1)
        *adj_fibers_per_slice += fibers_per_slice[offset];
*/
    return offset + 1;
}

void calculate_nnz_per_slice_map(tensor *T, SliceMap *nnz_per_slice_m)
{

    for (TENSORSIZE_T j = 0; j < T->nnz; j++)
    {
        nnz_per_slice_m->increment(j);
    }
}

void calculate_nnz_per_fiber_map(tensor *T, int fiber_id, FiberMap *nnz_per_fiber_m)
{
	
    for (TENSORSIZE_T j = 0; j < T->nnz; j++)
    {
        nnz_per_fiber_m->increment(j);
		
    }
}

void calculate_fibers_per_slice_map(tensor *T, int fiber_id, FiberMap *nnz_per_fiber_m, SliceMap **fibers_per_slice_m)
{
    int s = T->order - 2 - fiber_id;
	
	// printf("\nfiber_id: %d\n", fiber_id); 
	// printf("s: %d\n", s);

    for (TENSORSIZE_T j = 0; j < T->nnz; j++)
    {
        bool first = nnz_per_fiber_m->increment(j);
		
		// printf("---------------------\n");
		// printf("j: %d\n", j);

        if (first)
        {
            for (int z = fiber_id + 1; z < T->order; z++)
            {
                int fps_index = T->order - 1 - z + (s * (s + 1)) / 2;
                fibers_per_slice_m[fps_index]->increment(j);
				
				// printf("z: %d\n", z);
				// printf("fps_index: %d\n", fps_index);
            }
        }
    }
}

void calculate_nnz_per_fiber_and_fibers_per_slice_map(tensor *T, int fiber_id, FiberMap *nnz_per_fiber_m, SliceMap **fibers_per_slice_m)
{
	int order = T->order;
	
	fiber_id = order - 1 - fiber_id; //added for reverse fps order
	
    int s = order - 2 - fiber_id;
	
	// printf("\nfiber_id: %d, ", fiber_id); 
	// printf("s: %d\n", s);

    for (TENSORSIZE_T j = 0; j < T->nnz; j++)
    {
        bool first = nnz_per_fiber_m->increment(j);
		
		// printf("---------------------\n");
		// printf("j: %d\n", j);

		if (first)
		{
			for (int z = fiber_id + 1; z < order; z++)
			{
				int fps_index = order - 1 - z + (s * (s + 1)) / 2;
			
				// int s = floor((sqrt(1 + 8 * i) - 1.0) / 2.0);
				// int fps_index = T->order - 2 - s;
				
				fibers_per_slice_m[fps_index]->increment(j);
				
				// int ignored_dim_2 = T->order - 2 - s;
				// int ignored_dim_1 = T->order - 1 - i + (s * (s + 1)) / 2;
		
				// printf("z: %d, ", z);
				// printf("fps_index: %d\n", fps_index);
			}
		}
    }
}



mode_based_features *extract_features_sort(tensor *T)
{

    TENSORSIZE_T nnz = T->nnz;
    int order = T->order;
    int *dim = T->dim;

    TENSORSIZE_T num_slices = calculate_num_slices(order, dim);
    TENSORSIZE_T num_fibers = calculate_num_fibers(order, dim);

    int *nnzPerSlice = (int *)safe_calloc(order * nnz, sizeof(int));
    int *fibersPerSlice = (int *)safe_calloc(order * nnz, sizeof(int));
    int *nnzPerFiber = (int *)safe_calloc(order * nnz, sizeof(int));

    int adjNnzPerSlice = 0;
    int adjNnzPerFiber = 0;
    int adjFibersPerSlice = 0;

    int mode_num = 3;
	
	int num_modes_for_slices = comb(T->order, 2);
    int num_modes_for_fibers = T->order; // comb(T->order, 1) = T->order

    int slice_offsets[num_modes_for_slices+1];
    int fiber_offsets[num_modes_for_fibers+1];
    int fps_offsets[num_modes_for_slices+1];

    slice_offsets[0] = 0;
    fiber_offsets[0] = 0;
    fps_offsets[0] = 0;
	
	// printf ("before \n");

    timer *arrays_tm = timer_start("time_find_arrays_total");

    for (int mode = 0; mode < mode_num; mode++)
    {
		// printf ("before mode %d \n", mode);
		
		char name_string[30];
		sprintf(name_string, "time_sort_mode_%d", mode);
		
		timer *arrays_tm_ind = timer_start(name_string);
        sort_tensor(T, 1);

        int slice_offset = calculate_nnz_per_slice_sort(T, nnzPerSlice + slice_offsets[mode], &(adjNnzPerSlice));
        int fiber_offset = calculate_nnz_per_fiber_sort(T, nnzPerFiber + fiber_offsets[mode], &(adjNnzPerFiber));
        int fps_offset = calculate_fibers_per_slice_sort(T, fibersPerSlice + fps_offsets[mode], &(adjFibersPerSlice));

        slice_offsets[mode+1] = slice_offset + slice_offsets[mode];
        fiber_offsets[mode+1] = fiber_offset + fiber_offsets[mode];
        fps_offsets[mode+1] = fps_offset + fps_offsets[mode];
		
		timer_end(arrays_tm_ind);
		
    }
    timer_end(arrays_tm);
	
	// printf ("after \n");
    
    timer *extract_final_tm = timer_start("time_extract_final");

    base_features *features = new base_features();
	mode_features **slice_mode_features = new mode_features *[num_modes_for_slices];
    mode_features **fps_mode_features = new mode_features *[num_modes_for_slices];
    mode_features **fiber_mode_features = new mode_features *[num_modes_for_fibers];
	

    mode_based_features *all = new mode_based_features{features, slice_mode_features, fiber_mode_features, fps_mode_features};

	features->tensor_name = T->tensor_name ;
	features->order = order;
	features->org_order = T->org_order;
	features->org_order = T->org_order;
	features->dim = dim;
    features->sliceCnt = num_slices;
    features->fiberCnt = num_fibers;
    features->nnz = T->nnz;
    features->sparsity = calculate_sparsity(nnz, order, dim);
    features->nnzSliceCnt = reduce(nnzPerSlice, order * nnz, UNIT_SUM_OP, 0);
    features->nnzFiberCnt = reduce(nnzPerFiber, order * nnz, UNIT_SUM_OP, 0);
	
	if(PRINT_DEBUG){
		printf(	"SORT METHOD\n");
		printf("nnzPerSlice array: [ ");
		print_vec(nnzPerSlice, order * nnz);
		printf("] \n slice_offsets array: [ ");
		print_vec(slice_offsets, num_modes_for_slices+1);
		printf("]\n nnzSliceCnt : %d \n", features->nnzSliceCnt);
		
		printf("fibersPerSlice array: [ ");
		print_vec(fibersPerSlice, order * nnz);
		printf("] \n fps_offsets array: [ ");
		print_vec(fps_offsets, num_modes_for_slices+1);
		printf("]\n nnzSliceCnt : %d \n", features->nnzSliceCnt);
		
		printf("nnzPerFiber array: [ ");
		print_vec(nnzPerFiber, order * nnz);
		printf("] \n fiber_offsets array: [ ");
		print_vec(fiber_offsets, num_modes_for_fibers+1);
		printf("]\n nnzFiberCnt : %d \n", features->nnzFiberCnt);
	}
	
	
	TENSORSIZE_T curr_all_cnt;

#pragma omp parallel
    {
#pragma omp for nowait
        for (int i = 0; i < num_modes_for_slices; i++)
        {
			int curr_dim = (i+1)% num_modes_for_slices;
			int ignored_dim_1 = i;
			
			// int curr_dim = i;
			// int ignored_dim_1 = (i+1)% num_modes_for_slices;
			int ignored_dim_2 = (i+2)% num_modes_for_slices;
			
			slice_mode_features[curr_dim] = new mode_features();
			
			slice_mode_features[curr_dim]->ignored_dim1 = ignored_dim_1;
			slice_mode_features[curr_dim]->ignored_dim2 = ignored_dim_2;
			
			curr_all_cnt = 1;
			for (int j = 0; j < order; j++)
				if (j != ignored_dim_1 && j != ignored_dim_2)
					curr_all_cnt *= dim[j];
			
			slice_mode_features[curr_dim]->all_cnt = curr_all_cnt;
			
            // extract_final_mode(slice_mode_features[curr_dim], features->nnzSliceCnt, nnzPerSlice + slice_offsets[i], slice_offsets[i+1]- slice_offsets[i]);
			extract_final_mode(slice_mode_features[curr_dim], curr_all_cnt, nnzPerSlice + slice_offsets[i], slice_offsets[i+1]- slice_offsets[i]);
		}
		
		#pragma omp for nowait
        for (int i = 0; i < num_modes_for_slices; i++)
        {
			int curr_dim = (i+1)% num_modes_for_slices;
			int ignored_dim_1 = i;
			
			// int curr_dim = i;
			// int ignored_dim_1 = (i+1)% num_modes_for_slices;
			int ignored_dim_2 = (i+2)% num_modes_for_slices;
			
			fps_mode_features[curr_dim] = new mode_features();
			
			fps_mode_features[curr_dim]->ignored_dim1 = ignored_dim_1;
			fps_mode_features[curr_dim]->ignored_dim2 = ignored_dim_2;
			
			curr_all_cnt = 1;
			for (int j = 0; j < order; j++)
				if (j != ignored_dim_1 && j != ignored_dim_2)
					curr_all_cnt *= dim[j];
			
			fps_mode_features[curr_dim]->all_cnt = curr_all_cnt;
		
            // extract_final_mode(fps_mode_features[curr_dim], features->nnzSliceCnt, fibersPerSlice + fps_offsets[i], fps_offsets[i+1]- fps_offsets[i]);
			extract_final_mode(fps_mode_features[curr_dim], curr_all_cnt, fibersPerSlice + fps_offsets[i], fps_offsets[i+1]- fps_offsets[i]);
        }

#pragma omp for
        for (int i = 0; i < num_modes_for_fibers; i++)
        {

			fiber_mode_features[i] = new mode_features();
		
			fiber_mode_features[i]->ignored_dim1 = i;
			fiber_mode_features[i]->ignored_dim2 = -1;
			
			curr_all_cnt = 1;
			for (int j = 0; j < order; j++)
				if (j != i)
					curr_all_cnt *= dim[j];
			
			fiber_mode_features[i]->all_cnt = curr_all_cnt;
		
		
			// extract_final_mode(fiber_mode_features[i], features->nnzFiberCnt, nnzPerFiber+ fiber_offsets[i], fiber_offsets[i+1] - fiber_offsets[i]);
            extract_final_mode(fiber_mode_features[i], curr_all_cnt, nnzPerFiber+ fiber_offsets[i], fiber_offsets[i+1] - fiber_offsets[i]);
        }
    }
    
    timer_end(extract_final_tm);

    free(nnzPerSlice);
    free(nnzPerFiber);
    free(fibersPerSlice);

    return all;
}


// TT: TO-DO : Correct cnt input to extract_final_mode !
mode_based_features *extract_features_fragment(tensor *T)
{
    TENSORSIZE_T nnz = T->nnz;
    int order = T->order;
    int *dim = T->dim;

    dim2_tensor_fragment **fragments = (dim2_tensor_fragment **)safe_malloc(3 * sizeof(dim2_tensor_fragment *));
    
 
    timer *fragment_i_tm = timer_start("time_coo2fragment_all_modes");
    
    omp_set_nested(1);
    
    #pragma omp parallel for
    for (int i = 0; i < 3; i++)
    {
		// printf("\n%d \t\t :  Mode number \n", i);
		char name_string[40];
		sprintf(name_string, "time_fragment_mode_%d_%d", i, (i + 1) % 3);
		timer *mode_tm = timer_start(name_string);
        fragments[i] = coo2fragment(T, i, (i + 1) % 3);
		timer_end(mode_tm);
    }
    timer_end(fragment_i_tm);
  
    TENSORSIZE_T num_slices = calculate_num_slices(order, dim);
    TENSORSIZE_T num_fibers = calculate_num_fibers(order, dim);

    int slice_offsets[4];
    int fiber_offsets[4];

    calculate_nnzSliceCnt_fragment(fragments, 3, slice_offsets);
    calculate_nnzFiberCnt_fragment(fragments, 3, fiber_offsets);

    int nnzSliceCnt = slice_offsets[3];
    int nnzFiberCnt = fiber_offsets[3];


    int *nnzPerSlice = (int *)safe_calloc(nnzSliceCnt, sizeof(int));
    int *fibersPerSlice = (int *)safe_calloc(nnzSliceCnt, sizeof(int));
    int *nnzPerFiber = (int *)safe_calloc(nnzFiberCnt, sizeof(int));

    calculate_nnzPerSlice_fragment(fragments, 3, nnzPerSlice);
    calculate_nnzPerFiber_fragment(fragments, 3, nnzPerFiber);
    calculate_fibersPerSlice_fragment(fragments, 3, fibersPerSlice);

    timer *extract_final_tm = timer_start("time_extract_final");

    base_features *features = new base_features();
    mode_features **slice_mode_features = new mode_features *[3];
    mode_features **fps_mode_features = new mode_features *[3];
    mode_features **fiber_mode_features = new mode_features *[3];

    mode_based_features *all = new mode_based_features{features, slice_mode_features, fiber_mode_features, fps_mode_features};

	features->tensor_name = T->tensor_name ;
	features->order = order;
	features->org_order = T->org_order;
	features->dim = dim;
    features->sliceCnt = num_slices;
    features->fiberCnt = num_fibers;
    features->sparsity = calculate_sparsity(nnz, order, dim);
    features->nnzSliceCnt = nnzSliceCnt;
    features->nnzFiberCnt = nnzFiberCnt;
    features->nnz = T->nnz;
	
if(PRINT_DEBUG){
	printf(	"GROUPING METHOD \n");
		printf("nnzPerSlice array: [ ");
		print_vec(nnzPerSlice, nnzSliceCnt);
		printf("] \n slice_offsets array: [ ");
		print_vec(slice_offsets, 4);
		printf("]\n nnzSliceCnt : %d \n", nnzSliceCnt);
		
		printf("fibersPerSlice array: [ ");
		print_vec(fibersPerSlice, nnzSliceCnt);
		printf("] \n fps_offsets array: [ ");
		print_vec(slice_offsets, 4);
		printf("]\n nnzSliceCnt : %d \n", nnzSliceCnt);
		
		printf("nnzPerFiber array: [ ");
		print_vec(nnzPerFiber, nnzFiberCnt);
		printf("] \n fiber_offsets array: [ ");
		print_vec(fiber_offsets, 4);
		printf("]\n nnzFiberCnt : %d \n", nnzFiberCnt);
	}

#pragma omp parallel
    {
#pragma omp for nowait
        for (int i = 0; i < 3; i++)
        {
            slice_mode_features[i] = new mode_features();
			
			slice_mode_features[i]->ignored_dim1 = (i+1)%3;
			slice_mode_features[i]->ignored_dim2 = (i+2)%3;
			slice_mode_features[i]->all_cnt = dim[i];
			
			// extract_final_mode(slice_mode_features[i], features->nnzSliceCnt, nnzPerSlice + slice_offsets[i], fragments[i]->size0);
            extract_final_mode(slice_mode_features[i], slice_mode_features[i]->all_cnt, nnzPerSlice + slice_offsets[i], fragments[i]->size0);
        }

#pragma omp for nowait
        for (int i = 0; i < 3; i++)
        {
            fps_mode_features[i] = new mode_features();
			
			fps_mode_features[i]->ignored_dim1 = (i+1)%3;
			fps_mode_features[i]->ignored_dim2 = (i+2)%3;
			fps_mode_features[i]->all_cnt = dim[i];
   
            // extract_final_mode(fps_mode_features[i], features->nnzSliceCnt, fibersPerSlice + slice_offsets[i], fragments[i]->size0);
			extract_final_mode(fps_mode_features[i], fps_mode_features[i]->all_cnt, fibersPerSlice + slice_offsets[i], fragments[i]->size0);
        }

#pragma omp for
        for (int i = 0; i < 3; i++)
        {
			int curr_dim = (i+2)%3;
            fiber_mode_features[curr_dim] = new mode_features();
			
			fiber_mode_features[curr_dim]->ignored_dim1 = curr_dim;
			fiber_mode_features[curr_dim]->ignored_dim2 = -1;
			fiber_mode_features[curr_dim]->all_cnt = (unsigned long long) dim[i]*dim[(i+1)%3];

			// extract_final_mode(fiber_mode_features[curr_dim], features->nnzFiberCnt, nnzPerFiber+ fiber_offsets[i], fragments[i]->size1_tot);
            extract_final_mode(fiber_mode_features[curr_dim], fiber_mode_features[curr_dim]->all_cnt, nnzPerFiber+ fiber_offsets[i], fragments[i]->size1_tot);
        }
    }

    timer_end(extract_final_tm);

    free(nnzPerSlice);
    free(nnzPerFiber);
    free(fibersPerSlice);

    return all;
}

// TT: TO-DO : Correct cnt input to extract_final_mode !
mode_based_features *extract_features_hybrid(tensor *T)
{

    TENSORSIZE_T nnz = T->nnz;
    int order = T->order;
    int *dim = T->dim;

    TENSORSIZE_T num_slices = calculate_num_slices(order, dim);
    TENSORSIZE_T num_fibers = calculate_num_fibers(order, dim);

    int *nnzPerSlice = (int *)safe_calloc(order * nnz, sizeof(int));
    int *fibersPerSlice = (int *)safe_calloc(order * nnz, sizeof(int));
    int *nnzPerFiber = (int *)safe_calloc(order * nnz, sizeof(int));

    int adjNnzPerSlice = 0;
    int adjNnzPerFiber = 0;
    int adjFibersPerSlice = 0;

    int mode_num = 3;

    int slice_offsets[4];
    int fiber_offsets[4];
    int fps_offsets[4];

    slice_offsets[0] = 0;
    fiber_offsets[0] = 0;
    fps_offsets[0] = 0;

    timer *arrays_tm = timer_start("time_find_arrays");
	
	int real_mode = 1;
	unsigned long long curr_fiber_cnt;
	
	// unsigned long long LAMBDA = 100000000000;
	
	int slice_offset, fiber_offset, fps_offset;
    for (int mode = 0; mode < mode_num; mode++)
    {
		curr_fiber_cnt = ( unsigned long long ) dim[real_mode] * dim[(real_mode+1)%3];
		
		// printf("mode: %d (%d) real_mode : %d (%d), curr_fiber_cnt(over 1e9) : %llu \n ", mode, dim[real_mode], real_mode, T->dim[real_mode], curr_fiber_cnt/1000000000);
		// printf("\n Mode: %d , real_mode : %d , dim[real_mode] : %d , dim[real_mode+1] : %d , curr_fiber_cnt : %llu (%llu) ", mode, real_mode, dim[real_mode], dim[(real_mode+1)%3], curr_fiber_cnt, curr_fiber_cnt/1000000000);
		
		if (curr_fiber_cnt > LAMBDA)
		{		
			// printf(" -> Sort in mode %d \n", mode); //tt_last
			
			char name_string[100];
			sprintf(name_string, "time_sort_mode_%d currfib: %llu", mode, curr_fiber_cnt);
			timer *sort_mode_tm = timer_start(name_string);
			
			sort_tensor(T, 1);

			// printf("Sort is finished. \n");

			slice_offset = calculate_nnz_per_slice_sort(T, nnzPerSlice + slice_offsets[mode], &(adjNnzPerSlice));
			fiber_offset = calculate_nnz_per_fiber_sort(T, nnzPerFiber + fiber_offsets[mode], &(adjNnzPerFiber));
			fps_offset = calculate_fibers_per_slice_sort(T, fibersPerSlice + fps_offsets[mode], &(adjFibersPerSlice));
			
			timer_end(sort_mode_tm);

		}
		else{
			int next_mode = (real_mode+1) %3;
			
			// printf(" -> Group in modes %d - %d \n", real_mode, next_mode); //tt_last
			
			char name_string[100];
			sprintf(name_string, "time_fragment_mode_%d_%d currfib: %llu", real_mode, next_mode, curr_fiber_cnt);
			
			timer *frag_mode_tm = timer_start(name_string);
			
			dim2_tensor_fragment *fragment = coo2fragment(T, real_mode, next_mode);
			
			timer_end(frag_mode_tm);
			// printf("Fragment is finished. \n");

			slice_offset = fragment->size0;
			fiber_offset = reduce_sum(fragment->size1, fragment->size0);
			fps_offset = slice_offset;
			
			for (int j = 0; j < slice_offset; j++)
				nnzPerSlice[slice_offsets[mode] + j] = fragment->cnt0[j];
			
			for (int j = 0; j < fiber_offset; j++)
				nnzPerFiber[fiber_offsets[mode] + j] = fragment->cnt1[j];

			for (int j = 0; j < fps_offset; j++)
				fibersPerSlice[fps_offsets[mode] + j] = fragment->size1[j];

			real_mode = next_mode;
			
		}
		
		
		slice_offsets[mode+1] = slice_offset + slice_offsets[mode];
		fiber_offsets[mode+1] = fiber_offset + fiber_offsets[mode];
		fps_offsets[mode+1] = fps_offset + fps_offsets[mode];
		
    }
    timer_end(arrays_tm);
    
    timer *extract_final_tm = timer_start("time_extract_final");

    base_features *features = new base_features();
    mode_features **slice_mode_features = new mode_features *[3];
    mode_features **fps_mode_features = new mode_features *[3];
    mode_features **fiber_mode_features = new mode_features *[3];

    mode_based_features *all = new mode_based_features{features, slice_mode_features, fiber_mode_features, fps_mode_features};

	features->tensor_name = T->tensor_name ;
	features->order = order;
	features->org_order = T->org_order;
	features->dim = dim;
    features->sliceCnt = num_slices;
    features->fiberCnt = num_fibers;
    features->nnz = nnz;
    features->sparsity = calculate_sparsity(nnz, order, dim);
    features->nnzSliceCnt = reduce(nnzPerSlice, order * nnz, UNIT_SUM_OP, 0);
    features->nnzFiberCnt = reduce(nnzPerFiber, order * nnz, UNIT_SUM_OP, 0);
	
	if(PRINT_DEBUG){
		printf(	"HYBRID METHOD\n");
		printf("nnzPerSlice array: [ ");
		print_vec(nnzPerSlice, order * nnz);
		printf("] \n slice_offsets array: [ ");
		print_vec(slice_offsets, 4);
		printf("]\n nnzSliceCnt : %d \n", features->nnzSliceCnt);
		
		printf("fibersPerSlice array: [ ");
		print_vec(fibersPerSlice, order * nnz);
		printf("] \n fps_offsets array: [ ");
		print_vec(fps_offsets, 4);
		printf("]\n nnzSliceCnt : %d \n", features->nnzSliceCnt);
		
		printf("nnzPerFiber array: [ ");
		print_vec(nnzPerFiber, order * nnz);
		printf("] \n fiber_offsets array: [ ");
		print_vec(fiber_offsets, 4);
		printf("]\n nnzFiberCnt : %d \n", features->nnzFiberCnt);
	}

#pragma omp parallel
    {
#pragma omp for nowait
        for (int i = 0; i < 3; i++)
        {
			int curr_dim = (i+1)%3;
            slice_mode_features[curr_dim] = new mode_features();
			
			slice_mode_features[curr_dim]->ignored_dim1 = i;
			slice_mode_features[curr_dim]->ignored_dim2 = (i+2)%3;
			slice_mode_features[curr_dim]->all_cnt = dim[curr_dim];
			
            extract_final_mode(slice_mode_features[curr_dim], features->nnzSliceCnt, nnzPerSlice + slice_offsets[i], slice_offsets[i+1]- slice_offsets[i]);
        }

#pragma omp for nowait
        for (int i = 0; i < 3; i++)
        {
			int curr_dim = (i+1)%3;
            fps_mode_features[curr_dim] = new mode_features();
			
			fps_mode_features[curr_dim]->ignored_dim1 = i;
			fps_mode_features[curr_dim]->ignored_dim2 = (i+2)%3;
			fps_mode_features[curr_dim]->all_cnt = dim[curr_dim];
   
            extract_final_mode(fps_mode_features[curr_dim], features->nnzSliceCnt, fibersPerSlice + fps_offsets[i], fps_offsets[i+1]- fps_offsets[i]);
        }

#pragma omp for
        for (int i = 0; i < 3; i++)
        {
            fiber_mode_features[i] = new mode_features();
			
			fiber_mode_features[i]->ignored_dim1 = i;
			fiber_mode_features[i]->ignored_dim2 = -1;
			fiber_mode_features[i]->all_cnt = (unsigned long long) dim[(i+1)%3] * dim[(i+2)%3];

            extract_final_mode(fiber_mode_features[i], features->nnzFiberCnt, nnzPerFiber+ fiber_offsets[i], fiber_offsets[i+1] - fiber_offsets[i]);
        }
    }
    
    timer_end(extract_final_tm);


    free(nnzPerSlice);
    free(nnzPerFiber);
    free(fibersPerSlice);

    return all;
}


mode_based_features *extract_features_modes(tensor *T)
{
    TENSORSIZE_T nnz = T->nnz;
    int order = T->order;
    int *dim = T->dim;

    base_features *features = new base_features;

    TENSORSIZE_T num_slices = calculate_num_slices(order, dim);
    TENSORSIZE_T num_fibers = calculate_num_fibers(order, dim);
	
	features->tensor_name = T->tensor_name ;
	features->order = order;
	features->org_order = T->org_order;
	features->dim = dim;
    features->sliceCnt = num_slices;
    features->fiberCnt = num_fibers;

    int num_modes_for_slices = comb(T->order, 2);
    int num_modes_for_fibers = T->order; // comb(T->order, 1) = T->order

    timer *arrays_tm = timer_start("time_find_arrays");

    SliceMap **nnz_per_slice_m = new SliceMap *[num_modes_for_slices];
    FiberMap **nnz_per_fiber_m = new FiberMap *[num_modes_for_fibers];
    SliceMap **fibers_per_slice_m = new SliceMap *[num_modes_for_slices];
	
	mode_features **slice_mode_features = new mode_features *[num_modes_for_slices];
    mode_features **fps_mode_features = new mode_features *[num_modes_for_slices];
    mode_features **fiber_mode_features = new mode_features *[num_modes_for_fibers];
	
	mode_based_features *all = new mode_based_features{features, slice_mode_features, fiber_mode_features, fps_mode_features};
	
	TENSORSIZE_T curr_all_cnt;

    for (int i = 0; i < num_modes_for_slices; i++)
    {
        int s = floor((sqrt(1 + 8 * i) - 1.0) / 2.0);

		// edited part. 
        int ignored_dim_1 = s+1;
        int ignored_dim_2 = i - (s * (s + 1)) / 2;
		
		// int ignored_dim_1 = T->order - 2 - s;
        // int ignored_dim_2 = T->order - 1 - i + (s * (s + 1)) / 2;
		
		// printf("---------------------\n");
		// printf("i: %d\n", i);
		// printf("s: %d\n", s);
		// printf("ignored_dim_1: %d\n", ignored_dim_1);
		// printf("ignored_dim_2: %d\n", ignored_dim_2);
		// printf("---------------------\n");

        nnz_per_slice_m[i] = new SliceMap(T->indices, ignored_dim_1, ignored_dim_2, order);
        fibers_per_slice_m[i] = new SliceMap(T->indices, ignored_dim_1, ignored_dim_2, order);
		
		slice_mode_features[i] = new mode_features();
		fps_mode_features[i] = new mode_features();
		
		slice_mode_features[i]->ignored_dim1 = ignored_dim_1;
		slice_mode_features[i]->ignored_dim2 = ignored_dim_2;
		
		fps_mode_features[i]->ignored_dim1 = ignored_dim_1;
		fps_mode_features[i]->ignored_dim2 = ignored_dim_2;
		
		curr_all_cnt = 1;
		for (int j = 0; j < order; j++)
			if (j != ignored_dim_1 && j != ignored_dim_2)
				curr_all_cnt *= dim[j];
		
		slice_mode_features[i]->all_cnt = curr_all_cnt;
		fps_mode_features[i]->all_cnt = curr_all_cnt;
		
    }

    for (int i = 0; i < num_modes_for_fibers; i++)
    {
        nnz_per_fiber_m[i] = new FiberMap(T->indices, i, order);
		
		fiber_mode_features[i] = new mode_features();
		
		
		fiber_mode_features[i]->ignored_dim1 = i;
		fiber_mode_features[i]->ignored_dim2 = -1;
		
		curr_all_cnt = 1;
		for (int j = 0; j < order; j++)
			if (j != i)
				curr_all_cnt *= dim[j];
		
		fiber_mode_features[i]->all_cnt = curr_all_cnt;

    }
	

#pragma omp parallel
    {
#pragma omp for nowait
        for (int i = 0; i < num_modes_for_slices; i++)
        {
            calculate_nnz_per_slice_map(T, nnz_per_slice_m[i]);
        }

#pragma omp for
        for (int i = 0; i < num_modes_for_fibers; i++)
        {
			// printf("\n*************************\n");
			// printf("i: %d\n", i);
            calculate_nnz_per_fiber_and_fibers_per_slice_map(T, i, nnz_per_fiber_m[i], fibers_per_slice_m);
        }
    }

    timer_end(arrays_tm);
    timer *extract_final_tm = timer_start("time_extract_final");

    // Transforming the unordered_maps above to our usual arrays
    features->nnzSliceCnt = 0;
    features->nnzFiberCnt = 0;
    int fpsCnt = 0;

    for (int i = 0; i < num_modes_for_slices; i++)
    {
        features->nnzSliceCnt += nnz_per_slice_m[i]->map.size();
        fpsCnt += fibers_per_slice_m[i]->map.size();
    }

    for (int i = 0; i < num_modes_for_fibers; i++)
    {
        features->nnzFiberCnt += nnz_per_fiber_m[i]->map.size();
    }

	if(PRINT_DEBUG) printf(	"HASH-MAP METHOD\n");
    
#pragma omp parallel
    {
#pragma omp for nowait
        for (int i = 0; i < num_modes_for_slices; i++)
        {
			int nnz_per_slice_size = nnz_per_slice_m[i]->map.size();
            int *nnz_per_slice = new int[nnz_per_slice_size];

            std::transform(nnz_per_slice_m[i]->map.begin(), nnz_per_slice_m[i]->map.end(), nnz_per_slice, [](auto &kv)
                           { return kv.second; });
						   
		if(PRINT_DEBUG){
			printf("nnz_per_slice #%d, ", i);
			// print_vec(nnz_per_slice, nnz_per_slice_size);
			// printf("]\n nnz_per_slice_size : %d ", nnz_per_slice_size);
		}

			// wrong calculation fixed: give the slice count of only the current mode, not all !!!
            // extract_final_mode(slice_mode_features[i], features->nnzSliceCnt, nnz_per_slice, nnz_per_slice_m[i]->map.size());
			extract_final_mode(slice_mode_features[i], slice_mode_features[i]->all_cnt, nnz_per_slice, nnz_per_slice_size);
        }

#pragma omp for nowait
        for (int i = 0; i < num_modes_for_slices; i++)
        {
			int fibers_per_slice_size = fibers_per_slice_m[i]->map.size();
            int *fibers_per_slice = new int[ fibers_per_slice_size ];

            std::transform(fibers_per_slice_m[i]->map.begin(), fibers_per_slice_m[i]->map.end(), fibers_per_slice, [](auto &kv)
                           { return kv.second; });
						   
						   
				if(PRINT_DEBUG){
					printf("fibers_per_slice #%d, ", i);
					// print_vec(fibers_per_slice, fibers_per_slice_size);
					// printf("]\n fibers_per_slice_size : %d ", fibers_per_slice_size);
				}

            // extract_final_mode(fps_mode_features[i], features->nnzSliceCnt, fibers_per_slice, fibers_per_slice_m[i]->map.size());
			 extract_final_mode(fps_mode_features[i], fps_mode_features[i]->all_cnt, fibers_per_slice, fibers_per_slice_size );
        }

#pragma omp for
        for (int i = 0; i < num_modes_for_fibers; i++)
        {
			int nnz_per_fiber_size = nnz_per_fiber_m[i]->map.size();
            int *nnz_per_fiber = new int[ nnz_per_fiber_size ];

            std::transform(nnz_per_fiber_m[i]->map.begin(), nnz_per_fiber_m[i]->map.end(), nnz_per_fiber, [](auto &kv)
                           { return kv.second; });
						   
			if(PRINT_DEBUG){
					printf("nnz_per_fiber #%d, ", i);
					// print_vec(nnz_per_fiber, nnz_per_fiber_size);
					// printf("]\n nnz_per_fiber_size : %d ", nnz_per_fiber_size);
				}

            // extract_final_mode(fiber_mode_features[i], features->nnzFiberCnt, nnz_per_fiber, nnz_per_fiber_m[i]->map.size());
			extract_final_mode(fiber_mode_features[i], fiber_mode_features[i]->all_cnt, nnz_per_fiber, nnz_per_fiber_size );
        }
    }
    
    features->sparsity = calculate_sparsity(nnz, order, dim);
    features->nnz = T->nnz;


    timer_end(extract_final_tm);

    return all;
}


std::string all_mode_features_to_json(mode_based_features *features, tensor *T)
{
    std::string result = "{";

    int num_modes_for_slices, num_modes_for_fibers;
	
	int order = T->order;

    num_modes_for_slices = comb(order, 2);
    num_modes_for_fibers = order;

	result += "\"base\":" + base_features_to_json(features->base) ;
    // result += "\"base\":{" ;
	// result += base_features_to_json(features->base) ;
	result += ", \"slices\": [";

    for (int i = 0; i < num_modes_for_slices; i++){
        result += mode_features_to_json(features->slice[i]) + ",";
	}
	
	result += "], \"fibers\": [";

    for (int i = 0; i < num_modes_for_fibers; i++){
        result += mode_features_to_json_fibers(features->fiber[i]) + ",";
	}
	
	result += "], \"fibperslice\": [";

    for (int i = 0; i < num_modes_for_slices; i++){
        result += mode_features_to_json(features->fps[i]) + ",";
	}
	
	// result += "\"ORG_DIM\":" + org_dim_to_json(T) ;

    result += "]}\n";

    return result;
}

std::string all_mode_features_to_csv(mode_based_features *features, tensor *T)
{
    // std::string result = "BASE ";
	
	// if (PRINT_HEADER){
		// printf("\n HEADER: \n BASE ");
	// }

	// result +=  base_features_to_csv(features->base) ;
	
	std::string result =  base_features_to_csv(features->base) ;
	
	int order = T->order;

    int num_modes_for_slices = comb(order, 2);
    int num_modes_for_fibers = order;
	
	result += " SLICES ";
	
	if (PRINT_HEADER){
		printf(" SLICES ");
	}

    for (int i = 0; i < num_modes_for_slices; i++){
        result += mode_features_to_csv(features->slice[i]) ;
	}
	
	result += " FIBERS ";
	
	if (PRINT_HEADER){
		printf(" FIBERS ");
	}

    for (int i = 0; i < num_modes_for_fibers; i++){
        result += mode_features_to_csv_fibers(features->fiber[i]);
	}
	
	result += " FIBPERSLICE ";
	
	if (PRINT_HEADER){
		printf(" FIBPERSLICE ");
	}

    for (int i = 0; i < num_modes_for_slices; i++){
        result += mode_features_to_csv(features->fps[i]) ;
	}

	// result += "\"ORG_DIM\":" + org_dim_to_csv(T) ;
	
    result += "\n";
	
	if (PRINT_HEADER){
		printf(" \n");
	}

    return result;
}

std::string mode_features_to_json(mode_features *features)
{
    char buffer[10000];
    buffer[0] = '\0';

    char const *format = "{\"id1\" : %d,\"id2\" : %d,\"all_cnt\" : %llu,\"nz_cnt\" : %d, \"nz_density\" : %g, \"max\" : %d,\"min\" : %d, \"dev\" : %d, \"sum\" : %llu, \"avg\" : %g,  \"imbal\" : %g,\"stDev\" : %g,  \"cv\" : %g, \"avg_onlynz\" : %g, \"imbal_onlynz\" : %g, \"stDev_onlynz\" : %g, \"cv_onlynz\" : %g }";

    sprintf(buffer, format,
			features->ignored_dim1,
			features->ignored_dim2,
			features->all_cnt,
            features->nz_cnt,
			(features->nz_cnt + 0.0) / features->all_cnt,
            features->max,
            features->min,
			features->dev,
			features->sum,
            features->avg,
			(features->max + 0.0) / features->avg -1 ,
            features->stDev,
            features->cv,
			features->avg_onlynz,
			(features->max + 0.0) / features->avg_onlynz -1 ,
			features->stDev_onlynz,
			features->cv_onlynz);

    return std::string(buffer, buffer + strlen(buffer));
}

std::string mode_features_to_csv(mode_features *features)
{
    char buffer[1000];
    buffer[0] = '\0';

	if (PRINT_HEADER){
		printf (" id1 id2 all_cnt nz_cnt nz_density max min dev sum avg imbal stDev cv avg_onlynz imbal_onlynz stDev_onlynz cv_onlynz ");
	}

	char const *format = "\t %d \t %d \t %llu \t %d \t %g \t %d \t %d \t %d \t %llu \t %g \t %g \t %g \t %g \t %g \t %g \t %g \t %g ";

    sprintf(buffer, format,
			features->ignored_dim1,
			features->ignored_dim2,
			features->all_cnt,
            features->nz_cnt,
			(features->nz_cnt + 0.0) / features->all_cnt,
            features->max,
            features->min,
			features->dev,
			features->sum,
            features->avg,
			(features->max + 0.0) / features->avg -1 ,
            features->stDev,
            features->cv,
			features->avg_onlynz,
			(features->max + 0.0) / features->avg_onlynz -1 ,
			features->stDev_onlynz,
			features->cv_onlynz);
			
	// printf(" // features->avg = %g // \n", features->avg);

    return std::string(buffer, buffer + strlen(buffer));
}

std::string mode_features_to_json_fibers(mode_features *features)
{
    char buffer[10000];
    buffer[0] = '\0';

    char const *format = "{\"id\" : %d,\"all_cnt\" : %llu,\"nz_cnt\" : %d, \"nz_density\" : %g, \"max\" : %d,\"min\" : %d,\"dev\" : %d, \"sum\" : %llu, \"avg\" : %g,  \"imbal\" : %g,\"stDev\" : %g,  \"cv\" : %g, \"avg_onlynz\" : %g, \"imbal_onlynz\" : %g, \"stDev_onlynz\" : %g, \"cv_onlynz\" : %g }";

    sprintf(buffer, format,
			features->ignored_dim1,
			features->all_cnt,
            features->nz_cnt,
			(features->nz_cnt + 0.0) / features->all_cnt,
            features->max,
            features->min,
			features->dev,
			features->sum,
            features->avg,
			(features->max + 0.0) / features->avg -1 ,
            features->stDev,
            features->cv,
			features->avg_onlynz,
			(features->max + 0.0) / features->avg_onlynz -1 ,
			features->stDev_onlynz,
			features->cv_onlynz);

    return std::string(buffer, buffer + strlen(buffer));
}

std::string mode_features_to_csv_fibers(mode_features *features)
{
    char buffer[1000];
    buffer[0] = '\0';

	if (PRINT_HEADER){
		printf (" id all_cnt nz_cnt nz_density max min dev sum avg imbal stDev cv avg_onlynz imbal_onlynz stDev_onlynz cv_onlynz ");
	}

	char const *format = " \t %d \t %llu \t %d \t %g \t %d \t %d \t %d \t %llu \t %g \t %g \t %g \t %g \t %g \t %g \t %g \t %g ";

    sprintf(buffer, format,
			features->ignored_dim1,
			features->all_cnt,
            features->nz_cnt,
			(features->nz_cnt + 0.0) / features->all_cnt,
            features->max,
            features->min,
            features->dev,
			features->sum,
            features->avg,
			(features->max + 0.0) / features->avg -1 ,
            features->stDev,
            features->cv,
			features->avg_onlynz,
			(features->max + 0.0) / features->avg_onlynz -1 ,
			features->stDev_onlynz,
			features->cv_onlynz);

    return std::string(buffer, buffer + strlen(buffer));
}

std::string org_dim_to_json(tensor *T){
	char buffer[1000];
    buffer[0] = '\0';
	
	for (int i = 0; i <T->org_order; i++){
		sprintf(buffer + strlen(buffer), "\"org_dim_%d\" : %d, ", i, T->org_dim[i]);
	}
	
	return std::string(buffer, buffer + strlen(buffer));
}

std::string base_features_to_json(base_features *features)
{
    char buffer[1000];
    buffer[0] = '\0';

	sprintf(buffer, "{\"name\" : \"%s\", ", features->tensor_name);
	// sprintf(buffer, "{\"name\" : ");
	// strcat(buffer, features->tensor_name);
	// strcat(buffer, (features->tensor_name).c_str());
	sprintf(buffer + strlen(buffer), "\"order\" : %d, ", features->org_order);
	
	for (int i = 0; i <features->order; i++){
		sprintf(buffer + strlen(buffer), "\"dim_%d\" : %d, ", i, features->dim[i]);
	}
	
    char const *format = "\"nnz\" : %ld, \"sparsity\" : %g, \"sliceCnt\" : %llu, \"fiberCnt\" : %llu, \"nnzSliceCnt\":%d \"nnzFiberCnt\" : %d, \"slice_sparsity\" : %g, \"fiber_sparsity\" : %g, }";

    sprintf(buffer + strlen(buffer), format,
            features->nnz,
            features->sparsity,
            features->sliceCnt,
            features->fiberCnt,
			features->nnzSliceCnt,
            features->nnzFiberCnt,
            (0.0 + features->nnzSliceCnt) / features->sliceCnt,
			(0.0 + features->nnzFiberCnt) / features->fiberCnt 
		);
		
	


    return std::string(buffer, buffer + strlen(buffer));
}


std::string org_dim_to_csv(tensor *T){
	char buffer[1000];
    buffer[0] = '\0';
	
	for (int i = 0; i <T->org_order; i++){
		sprintf(buffer + strlen(buffer), " \t %d ", T->org_dim[i]);
		if (PRINT_HEADER){
			printf(" org_dim_%d ", i);
		}
	}
	
	return std::string(buffer, buffer + strlen(buffer));
}

std::string base_features_to_csv(base_features *features)
{
    char buffer[1000];
    buffer[0] = '\0';
	
	if (PRINT_HEADER){
		printf("\nname ");
	}

	sprintf(buffer, "%s ", features->tensor_name);
	
	if (PRINT_HEADER){
		printf(" order ");
	}
	
	sprintf(buffer + strlen(buffer), " \t %d ", features->org_order);
	
	
	
	for (int i = 0; i <features->order; i++){
		sprintf(buffer + strlen(buffer), " \t %d ", features->dim[i]);
		if (PRINT_HEADER){
			printf(" dim_%d ", i);
		}
	}
	

	if (PRINT_HEADER){
		printf(" nnz sparsity sliceCnt fiberCnt nnzSliceCnt nnzFiberCnt slice_sparsity fiber_sparsity ");
	}

    char const *format = " \t %ld \t %g \t %llu \t %llu \t %d \t %d \t %g \t %g ";

    sprintf(buffer + strlen(buffer), format,
            features->nnz,
            features->sparsity,
			features->sliceCnt,
            features->fiberCnt,
			features->nnzSliceCnt,
            features->nnzFiberCnt,
            (0.0 + features->nnzSliceCnt) / features->sliceCnt,
			(0.0 + features->nnzFiberCnt) / features->fiberCnt 
			);
			
	

    return std::string(buffer, buffer + strlen(buffer));
}



std::string tensor_features_to_json(tensor_features *features)
{

    char buffer[1000];
    buffer[0] = '\0';

    char const *format = "{\"fiberCnt\" : %llu,\"sliceCnt\" : %llu,\"nnzFiberCnt\" : %d,\"nnzSliceCnt\" : %d,\"maxNnzPerSlice\" : %d,\"minNnzPerSlice\" : %d,\"avgNnzPerSlice\" : %g,\"devNnzPerSlice\" : %d,\"stDevNnzPerSlice\" : %g,\"cvNnzPerSlice\" : %g, \"maxNnzPerFiber\" : %d,\"minNnzPerFiber\" : %d,\"avgNnzPerFiber\" : %g,\"devNnzPerFiber\" : %d,\"stDevNnzPerFiber\" : %g,\"cvNnzPerFiber\" : %g,\"maxFibersPerSlice\" : %d,\"minFibersPerSlice\" : %d,\"avgFibersPerSlice\" : %g,\"devFibersPerSlice\" : %d,\"stdDevFibersPerSlice\" : %g,\"cvFibersPerSlice\" : %g }";

    sprintf(buffer, format,
            features->fiberCnt,
            features->sliceCnt,
            features->nnzFiberCnt,
            features->nnzSliceCnt,
            features->maxNnzPerSlice,
            features->minNnzPerSlice,
            features->avgNnzPerSlice,
            features->devNnzPerSlice,
            features->stDevNnzPerSlice,
            features->cvNnzPerSlice,
            /*<ADJFEATURE>
            features->adjNnzPerSlice,
            </ADJFEATURE>*/
            features->maxNnzPerFiber,
            features->minNnzPerFiber,
            features->avgNnzPerFiber,
            features->devNnzPerFiber,
            features->stDevNnzPerFiber,
            features->cvNnzPerFiber,
            /*<ADJFEATURE>
            features->adjNnzPerFiber,
            </ADJFEATURE>*/

            features->maxFibersPerSlice,
            features->minFibersPerSlice,
            features->avgFibersPerSlice,
            features->devFibersPerSlice,
            features->stDevFibersPerSlice,
            features->cvFibersPerSlice
            /*<ADJFEATURE>
            features->adjFibersPerSlice
            </ADJFEATURE>*/
    );

    return std::string(buffer, buffer + strlen(buffer));
}

void print_tensor_features(tensor_features *features)
{
    // Printing results
    printf("\n");
    printf("%llu \t\t :  fiberCnt \n", features->fiberCnt);
    printf("%llu \t\t :  sliceCnt \n", features->sliceCnt);

    printf("%d \t\t :  nnzFiberCnt \n", features->nnzFiberCnt);
    printf("%d \t\t :  nnzSliceCnt \n", features->nnzSliceCnt);

    printf("%e \t\t :  sparsity \n", features->sparsity);

    printf("\n");
    // printf("avgNnzPerSlice :%g\n", avgNnzPerSlice);
    printf("%d \t\t :  maxNnzPerSlice \n", features->maxNnzPerSlice);
    printf("%d \t\t :  minNnzPerSlice \n", features->minNnzPerSlice);
    printf("%.4f \t\t :  avgNnzPerSlice \n", features->avgNnzPerSlice);
    printf("%d \t\t :  devNnzPerSlice \n", features->devNnzPerSlice);
    printf("%.4f \t\t :  stDevNnzPerSlice \n", features->stDevNnzPerSlice);
    printf("%.4f \t\t :  cvNnzPerSlice \n", features->cvNnzPerSlice);
    /* <ADJFEATURE>
    printf("%.4Lf \t\t :  adjNnzPerSlice \n", features->adjNnzPerSlice);
    </ADJFEATURE> */

    printf("\n");

    printf("%d \t\t :  maxNnzPerFiber \n", features->maxNnzPerFiber);
    printf("%d \t\t :  minNnzPerFiber \n", features->minNnzPerFiber);
    printf("%.4f \t\t :  avgNnzPerFiber \n", features->avgNnzPerFiber);
    printf("%d \t\t :  devNnzPerFiber \n", features->devNnzPerFiber);
    printf("%.4f \t\t :  stDevNnzPerFiber \n", features->stDevNnzPerFiber);
    printf("%.4f \t\t :  cvNnzPerFiber \n", features->cvNnzPerFiber);
    /* <ADJFEATURE>
    printf("%.4Lf \t\t :  adjNnzPerFiber \n", features->adjNnzPerFiber);
    </ADJFEATURE>*/

    printf("\n");

    printf("%d \t\t :  maxFibersPerSlice \n", features->maxFibersPerSlice);
    printf("%d \t\t :  minFibersPerSlice \n", features->minFibersPerSlice);
    printf("%.4f \t\t :  avgFibersPerSlice \n", features->avgFibersPerSlice);
    printf("%d \t\t :  devFibersPerSlice \n", features->devFibersPerSlice);
    printf("%.4f \t\t :  stdDevFibersPerSlice \n", features->stDevFibersPerSlice);
    printf("%.4f \t\t :  cvFibersPerSlice \n", features->cvFibersPerSlice);
    /* <ADJFEATURE>
    printf("%.4Lf \t\t :  adjFibersPerSlice \n", features->adjFibersPerSlice);
    </ADJFEATURE>*/
    printf("\n");
}

void print_csr_tensor(csr_tensor *T)
{
    printf("row_ptr: ");
    for (int i = 0; i < T->dim[1] + 2; i++)
    {
        printf("%d ", T->csr_rowptr[i]);
    }
    printf("\n");

    printf("cols: ");
    for (TENSORSIZE_T i = 0; i < T->nnz; i++)
    {
        printf("%d ", T->csr_cols[i]);
    }
    printf("\n");

    printf("values: ");
    for (TENSORSIZE_T i = 0; i < T->nnz; i++)
    {
        printf("%f ", T->csr_vals[i]);
    }
    printf("\n");
}

void print_csf_tensor(csf_tensor *T)
{
    printf("csf_inds1: ");
    for (int i = 0; i < T->inds_size[0] + 2; i++)
    {
        printf("%d ", T->csf_inds1[i]);
    }
    printf("\n");

    printf("csf_ptr1: ");
    for (int i = 0; i < T->ptr_size[0]; i++)
    {
        printf("%d ", T->csf_ptr1[i]);
    }
    printf("\n");

    printf("csf_inds2: ");
    for (int i = 0; i < T->inds_size[1]; i++)
    {
        printf("%d ", T->csf_inds2[i]);
    }
    printf("\n");

    printf("csf_ptr2: ");
    for (int i = 0; i < T->ptr_size[1]; i++)
    {
        printf("%d ", T->csf_ptr2[i]);
    }
    printf("\n");

    printf("csf_inds3: ");
    for (int i = 0; i < T->inds_size[2]; i++)
    {
        printf("%d ", T->csf_inds3[i]);
    }
    printf("\n");

    printf("csf_ptr3: ");
    for (int i = 0; i < T->ptr_size[2]; i++)
    {
        printf("%d ", T->csf_ptr3[i]);
    }
    printf("\n");

    printf("csf_vals: ");
    for (TENSORSIZE_T i = 0; i < T->nnz; i++)
    {
        printf("%f ", T->csf_vals[i]);
    }
    printf("\n");
}
