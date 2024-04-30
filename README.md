# **Paralell Feature Extraction Algorithms for Sparse Tensors**

## **Documentation 📑** 


This is a repository that provides different implementations for extracting the same features from tensors. The input tensor format is COO (*.tns* file format [1]). 

### **Compilation & Running**

To compile and run our main code, use following commands, replace placeholders with corresponding values:

```
TO COMPILE:
make

USAGE: 
featen [options]                                                                                                                                               
-i input : input tensor path                                                                                                                                  
-o out : feature info output file                                                                                                                            
-m method : feature extraction method. Options:{map, sort, group, hybrid}      [DEFAULT : map]                                                                                                            
-c csv_out : feture out file to be in csv {1} or json {0} format  [DEFAULT : 1]                                                                                        
-d only3d : features to be in only 3 modes {1} or M modes {0} (only valid for MAP) [DEFAULT : 0] 

SIMPLEST USAGE:
./featen -i [input-tensor-path] -o [output-file-path]

EXAMPLE: 
./featen -i ../data_tensors/sample_small_3D.tns -o ../features/sample_small_3D_feat.txt
```

### **Extraction Methods**

All feature extraction methods find the features of the tensor exactly. Users can use whichever method they like. These methods provide different performances depending on the given tensor. 

For 3-mode tensors, all methods return the same set of features.
For M-mode tensors (M>3), only MAP returns the features of all M modes; whereas other methods return the features of the 3 modes with the largest sizes.

To use one, write the choice number of the method as a parameter (the placeholder : [algorithm choice]).

| Method | Description | 
| --------- | ------------|
| **MAP** | Uses `std::unordered_map` to calculate # of nnz per fiber & slice to do the extraction. |
| SORT | Sorts all the modes to make the calculation. |
| GROUP | Groups the slices and fibers according to their indices, similar to sorting except for the last mode. |
| HYBRID | Combination of sort and group |

**MAP** is the default feature extraction method when no choice is provided. 

### **Feature Set**

|Feature Name| Description |
|----|----|
|fiberCnt | # of all possible fibers (even if it has no nnz) |
|sliceCnt | # of all possible slices (even if it has no nnz) |
|nnzFiberCnt | # of fibers which has at least 1 nnz. |
|nnzSliceCnt | # of slices which has at least 1 nnz.  |
|maxNnzPerSlice | maximum # of nnz in a single slice |
|minNnzPerSlice |  minimum # of nnz in a single slice |
|avgNnzPerSlice |  average # of nnz in slices |
|devNnzPerSlice | difference between max & min nnzPerSlices |
|stDevNnzPerSlice | standard deviation of nnz numbers in slices |
|cvNnzPerSlice | coefficent of variation of nnz numbers in slices |
|adjNnzPerSlice | average adjacent difference between nnz numbers in slices |
|maxNnzPerFiber | maximum # of nnz in a single fiber |
|minNnzPerFiber |  minimum # of nnz in a single fiber |
|avgNnzPerFiber |  average # of nnz in fibers |
|devNnzPerFiber | difference between max & min nnzPerFibers |
|stDevNnzPerFiber | standard deviation of nnz numbers in fibers |
|cvNnzPerFiber | coefficent of variation of nnz numbers in fibers |
|adjNnzPerFiber | average adjacent difference between nnz numbers in fibers |
|maxFibersPerSlice | maximum # of fibers in a single slice |
|minFibersPerSlice | minimum # of fibers in a single slice |
|avgFibersPerSlice | average # of fibers in slices |
|devFibersPerSlice | difference between max & min fibersPerSlice  |
|stdDevFibersPerSlice | standard deviation of fiber numbers in slices |
|cvFibersPerSlice |  coefficent of variation of fiber numbers in slices |
|adjFibersPerSlice | average adjacent difference between fiber numbers in slices |
