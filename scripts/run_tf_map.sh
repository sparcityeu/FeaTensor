 
#!/bin/bash 

./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_map_ndim_corrected.txt >> ../results/featen_corrected_results/runtime_featen_map_ndim.txt;                                                                                                                    
#./featen -i ../data_tensors/$1.tns -o ../features/$1_feat_last.txt -m sort -c 1 >> ../results/runtime_sort_last_$1.txt                                                                                                                  
#./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_fragment.txt -m fragment -c 1 >> ../results/runtime_all_methods_$1.txt;                                                                                                          
#./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_hybrid.txt -m hybrid -c 1 >> ../results/runtime_all_methods_$1.txt
