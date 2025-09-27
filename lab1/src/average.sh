#!/bin/bash

count=$#
sum=0

for number in "$@"
do
     sum=$(echo "$sum + $number" | bc -l)
done

average=$(echo "scale=2; $sum / $count" | bc -l)

echo "Среднее арифметическое: $average"
