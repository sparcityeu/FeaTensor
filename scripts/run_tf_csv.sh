 
#!/bin/bash 

./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_map.txt -m map -c 1 >> ../features/runtime_all_methods_$1.txt;                                                                                                                    
./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_sort.txt -m sort -c 1 >> ../features/runtime_all_methods_$1.txt;                                                                                                                  
./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_fragment.txt -m fragment -c 1 >> ../features/runtime_all_methods_$1.txt;                                                                                                          
./featen -i ../data_tensors/$1.tns -o ../features/feat_$1_hybrid.txt -m hybrid -c 1 >> ../features/runtime_all_methods_$1.txt