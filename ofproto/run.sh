make clean;
make;
#set -x;

./hypercut -p 100 -r /home/xflow/Algo_Development/filter_sets/acl1_100.txt -t /home/xflow/Algo_Development/filter_sets/acl1_100_trace.txt

#./hypercut -r /home/xflow/Algo_Development/filter_sets/fw1_100.txt -t /home/xflow/Algo_Development/filter_sets/fw1_100_trace.txt

#./nova

#sudo ./rx
#./mtt
#./hal

#./hypercut -s 1 -b 16 -d -p 2 -r ../filter_sets/fw1_100.txt -t ../filter_sets/fw1_100_trace.txt
#./hypercut -s 4 -b 16 -d -p 1 -r ../filter_sets/acl1_100.txt -t ../filter_sets/acl1_100_trace.txt
#./hypercut -s 0 -b 100 -r MyFilters -t MyFilters_trace
#valgrind --leak-check=yes ./hypercut -s 0 -b 100 -r MyFilters -t MyFilters_trace


#./hypercut -s 1 -b 4 -p 100 -r acl1_1.txt -t acl1_1_trace.txt
#./hypercut -s 1 -b 4 -p 100 -r acl1_10.txt -t acl1_10_trace.txt
#./hypercut -s 1 -b 16 -r $FILTERS/acl1_10.txt -t $FILTERS/acl1_10_trace.txt


#for i in "acl1" "fw1" "ipc1"
#do
#  for j in "100" "1K" "5K" "10K"
#  do
#  echo '=============================================='
#  echo ${i}_${j}.txt
#  echo '=============================================='
#  ./hypercut -s 1 -b 16 -r $FILTERS/${i}_${j}.txt -t $FILTERS/${i}_${j}_trace.txt
#  done
#done
