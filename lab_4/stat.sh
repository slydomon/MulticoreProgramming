#!/bin/bash

for size in {10..15}
do
   #for threads in {8..3}
   #do
         LEN=$(($size*$size))
         LEN=100
   		./lab4 -n "8" -r "$size" -c "$size" -l "$LEN" >> stat.txt &&
   		echo "finish." 
   #done
done

echo "please check the stat.txt file for output."