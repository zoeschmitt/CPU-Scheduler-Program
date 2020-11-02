#!/bin/bash
filename='results.txt'
if [ -f $filename ]; then
    rm "$filename"
fi
for ((i = 10; i < 31; i++)); do
    ./sim 1 $i 0.04 0.01
done
for ((i = 10; i < 31; i++)) do 
    ./sim 2 $i 0.04 0.01
done
for ((i = 10; i < 31; i++)) do 
    ./sim 3 $i 0.04 0.01
done
