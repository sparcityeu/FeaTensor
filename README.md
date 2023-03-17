# **Paralell Feature Extraction Algorithms for Sparse Tensors**

## **Documentation 📑** 


This is a repository that provides different implementations for extracting the same features from tensors. The input tensor format is COO (*.tns* file format [1]). 

### **Compilation & Running**

To compile and run our main code, use following commands, replace placeholders with corresponding values:

```
make
./featen -i [input-tensor-path] -o [output-file-path] -m [method-choice (optional)]
```

### **Extraction Algorithms**

All of the following algorithms output the same set of features. So, one can use whichever method they like. These methods provide different performances depending on the given tensor. 

To use one, write the choice number of the algorithm as parameter when running main (the placeholder : [algorithm choice]).

| Algorithm | Description | 
| --- | ---------------|
| FRAGMENT | For each mode, sorts the tensor except the last dimension, calculates feature after slightly modifying this structure. |
| MAP | Uses `std::unordered_map` to calculate # of nnz per fiber & slice to do the extraction. |
| SORT | Sorts all the modes to make the calculation. ***|
| HYBRID | Combination of sort and fragment |

*** SORT is the default feature extraction method when no choice is provided.

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
