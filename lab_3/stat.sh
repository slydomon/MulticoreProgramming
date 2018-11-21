#!/bin/bash

for points in {2..4}
do
   for threads in {8..1}
   do
   		./lab3 -d "$points" -n "$threads" -t 10  -l -10 -u 10 >> stat.txt &&
   		echo "finish." 
   done
done

echo "please check the stat.txt file for output."