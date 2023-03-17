#!/bin/bash

dat=`date +"%F-%T"`;

for run in 1 2
do
echo Run $run is started  $'\n';
./../fest_omp/extract_fest_omp ../../data_tensors/freebase_music.tns 3 23343790 23344784 166 99546551 >> ../results/result_fest_omp_$dat.txt
./../fest_omp/extract_fest_omp ../../data_tensors/freebase_sampled.tns 3 38954435 38955429 532 139920771 >> ../results/result_fest_omp_$dat.txt
echo Run $run is ended $'\n';
done

#./tensor_features/fest_omp/extract_fest_omp tensors/1998darpa.tns 3 22476 22476 23776223 28436033 >> tugba_results/tt_omp_$dat.result
#./tensor_features/fest_omp/extract_fest_omp tensors/nell2.tns 3 12092 9184 28818 76879419 >> tugba_results/tt_omp_$dat.result
#./tensor_features/fest_omp/extract_fest_omp tensors/nell1.tns 3 2902330 2143368 25495389 143599552 >> tugba_results/tt_omp_$dat.result

#./extract_tf ../tensors/nell1.tns 3 2902330 2143368 25495389 143599552 > features/$dat-nell1-serial.tns.txt
#./extract_tf ../tensors/nell2.tns 3 12092 9184 28818 76879419 > features/$dat-nell2-serial.tns.txt
~                                                                              
