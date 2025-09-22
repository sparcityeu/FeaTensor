# **FeaTensor: Parallel Feature Extraction Algorithms for Sparse Tensors**

## **Documentation 📑** 


This repository provides different implementations for extracting the same features from tensors. The input tensor format is COO (*.tns* file format). 

For more information on this tool, please refer to the paper:

T. Torun, A. Taweel, and D. Unat, A sparse tensor generator with efficient feature extraction, Frontiers in Applied Mathematics and Statistics, Volume 11 (2025), DOI: 10.3389/fams.2025.158903, 
https://doi.org/10.3389/fams.2025.158903,
https://www.frontiersin.org/journals/applied-mathematics-and-statistics/articles/10.3389/fams.2025.1589033/full.

### **Compilation & Running**

To compile and run our main code, use following commands, replace placeholders with corresponding values:

TO COMPILE:
```
make
```

USAGE: 
```
featen [options]                                                                                                                                               
-i input : input tensor path                                                                                                                                  
-o out : feature info output file                                                                                                                            
-m method : feature extraction method. Options:{map, sort, group, hybrid}  [DEFAULT : map]                                                                                                            
-c csv_out : feture out file to be in csv {1} or json {0} format  [DEFAULT : 1]                                                                                        
-d only3d : features to be in only 3 modes {1} or M modes {0} (only valid for MAP) [DEFAULT : 0]
```

SIMPLEST USAGE:
```
./featen -i [input-tensor-path] -o [output-file-path]
```

EXAMPLE: 
```
./featen -i ../data_tensors/sample_small_3D.tns -o ../features/sample_small_3D_feat.txt
```

### **Extraction Methods**

All feature extraction methods find the features of the tensor exactly. Users can use whichever method they like. These methods provide different performances depending on the given tensor. 

To use one, write the choice number of the method as a parameter (the placeholder : [algorithm choice]).

| Method | Description | 
| --------- | ------------|
| MAP | Uses hash table implemented with `std::unordered_map` to calculate # of nnz per fiber & slice to do the extraction. |
| SORT | Sorts all the indices along all modes to make the calculation. |
| GROUP | Groups the slices and fibers according to their indices, similar to CSF construction. |
| HYBRID | Uses sorting-based and grouping-based algorithms for different modes, depending on mode properties. |

For 3-mode tensors, all the methods extract all features along all modes.

For M-mode tensors with $M \geq 4$, the option named only-3-mode ( only3d = 1 ) extracts the features along only the modes with the three largest sizes.
This option is available for all methods in FeaTensor.
If this option is not used ( only3d = 0 ), then all the features along all modes are extracted, which is only available for the MAP method for $M \geq 4$.

### **Feature Set**

The program writes the tensor features into a file. 

It first prints the general features, whose meanings are:

```
name  order  sizes[] nnz sparsity sliceCnt fiberCnt nnzSliceCnt nnzFiberCnt slice_sparsity fiber_sparsity
```
The description of general features is shown in the table below.

|Feature Name| Description |
|----|----|
|name | Name of the tensor  |
|order | Order ( mode count) of the tensor  |
|sizes[] | Sizes of the tensor, which is an array of size order  |
|nnz | Number of nonzeros  |
|sparsity | Nonzero density of the tensor  |
|sliceCnt | # of all possible slices (even if it has no nnz) |
|fiberCnt | # of all possible fibers (even if it has no nnz) |
|nnzSliceCnt | # of nonzero slices (which has at least 1 nnz)  |
|nnzFiberCnt | # of nonzero fibers (which has at least 1 nnz) |
|slice_sparsity | Density of nonzero slices  |
|fiber_sparsity | Density of nonzero fibers  |


The program then prints the kind- and mode-dependent features for each mode and kind (nonzeros_per_slice, nonzeros_per_fibers, fibers_per_slice).

For features of nonzeros_per_slice and fibers_per_slice kinds, the output is in the form:
```
id1 id2 all_cnt nz_cnt nz_density max min dev sum avg imbal stDev cv avg_onlynz imbal_onlynz stDev_onlynz cv_onlynz
```
Here id1 and id2 represent the ignored dimensions of mode-(id1, id2) slice

For features of nonzeros_per_fiber kind, the output is in the form:
```
id all_cnt nz_cnt nz_density max min dev sum avg imbal stDev cv avg_onlynz imbal_onlynz stDev_onlynz cv_onlynz
```
Here id represents the ignored dimension of mode-(id) fiber

The description of kind- and mode-dependent features is shown in the table below.

|Feature Name| Description |
|----|----|
|all_cnt |  all count including empty |
|nz_cnt | nonzero count |
|nz_density | nonzero sparsity (nz_cnt / all_cnt) |
|max | maximum value |
|min | minimum value |
|dev | deviation (max – min) |
|sum | summation of values |
|avg | average value (sum / all_cnt) |
|imbal | imbalance : (max-avg)/max |
|stDev | standard deviation |
|cv | coefficient of variation (stDev/avg) |
|avg_onlynz | average by excluding empty (sum / nz_cnt) |
|imbal_onlynz | imbal by excluding empty (max- avg_onlynz)/max |
|stDev_onlynz | stDev by excluding empty |
|cv_onlynz | cv by excluding empty (stDev_onlynz  / avg_onlynz ) |

