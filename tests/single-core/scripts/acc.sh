#!/bin/bash

# Get the directory of the benchmark
bashDir=$(dirname "$0")
pushd $bashDir > /dev/null
bashDir=$(pwd)
popd > /dev/null

folder=$1
groundtruth=$bashDir/../results/groundtruth

truePositives=`for i in {0..12}; do
    next=$(echo "$i + 1" | bc)
    gt_f=$(printf %04d $i)
    ac_f=$(printf %04d $next)
    grep -F -x -f <(cat $groundtruth/${gt_f}.log| cut -d" " -f1 | sort) <(cat ${folder}/*${ac_f}.log | cut -d" " -f1 | sort)
done | wc -l`

relevantElements=$(cat $groundtruth/*log | wc -l)
selectedElements=$(cat ${folder}/*00*log | wc -l)

precision=$(echo "scale=3;100*$truePositives/$relevantElements" | bc)
recall=$(echo "scale=3;100*$truePositives/$selectedElements" | bc)

echo -e "$precision\t$recall"
