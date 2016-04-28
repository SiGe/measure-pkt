#!/bin/bash

# Get the directory of the benchmark
bashDir=$(dirname "$0")
pushd $bashDir > /dev/null
bashDir=$(pwd)
popd > /dev/null

resultsDir=$bashDir/../results

for exp in $(ls $resultsDir | grep 01); do
    for expSize in $(ls $resultsDir/$exp); do
        expDir=$resultsDir/$exp/$expSize;
        acc=$($bashDir/acc.sh $expDir)
        time=$($bashDir/time.sh $expDir)
        echo -e "$exp\t$expSize\t$acc\t$time";
    done
done
