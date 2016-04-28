#!/bin/bash


# Get the directory of the benchmark
bashDir=$(dirname "$0")
pushd $bashDir > /dev/null
bashDir=$(pwd)
popd > /dev/null

resultDir=$bashDir/results
mkdir -p $resultDir

# Set the directory of the build
buildDir="$bashDir/../.."

# Build the measurement framework
echo "> Building: measure-pkt"
cd $buildDir && make clean && make -j$(nproc) >/dev/null 2>&1;

echo "> Running: benchmarks."
for benchmark in $(ls $bashDir/01*); do
    name=$(basename "$benchmark")
    ext="${name##*.}"
    name="${name%.*}"

    logFile=$resultDir/${name}.log

    echo "> Running: $name"

    # Run the measurement framework locally and on the remote server
    cd $buildDir && sudo ./build/l2fwd -- $benchmark >$logFile >&1 &

    echo "> Running: remote traffic."
    ssh omid@mm -- ~/measure-pkt-d1-to-d1/run.sh 100 27263000 >/dev/null 2>&1
    sudo killall l2fwd
    mv $buildDir/*log $resultDir/
done
