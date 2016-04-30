#!/bin/bash

# Get the directory of the benchmark
bashDir=$(dirname "$0")
pushd $bashDir > /dev/null
bashDir=$(pwd)
popd > /dev/null

RunBenchmark() {
    benchmarkList=$1
    sizeList=$2
    speed=$3
    trafficDist=$4

    resultDir=$bashDir/results/$trafficDist
    mkdir -p $resultDir

    cd $resultDir && touch tmp.yaml

    # Set the directory of the build
    buildDir="$bashDir/../.."

    # Build the measurement framework
    echo "> Building: measure-pkt"
    cd $buildDir && make clean && make -j$(nproc) >/dev/null 2>&1;

    echo "> Running: benchmarks."
    for benchmark in $benchmarkList; do
        for size in $sizeList; do
            name=$(basename "$benchmark")
            ext="${name##*.}"
            name="${name%.*}"

            expDir=$resultDir/$name/$size/
            logFile=$expDir/app.log

            mkdir -p $expDir

            echo "> Running: $name ($size)"
            sed -E "s/(\\s+)size: ([0-9]+)/\\1size: $size/" $benchmark > $resultDir/tmp.yaml

            # Run the measurement framework locally and on the remote server
            cd $buildDir && sudo ./build/l2fwd -- $resultDir/tmp.yaml >$logFile >&1 &
            sleep 5

            echo "> Running: remote traffic."
            ssh omid@mm -- ~/measure-pkt-d1-to-d1/run.sh $trafficDist $speed 27263000 >/dev/null 2>&1
            sleep 5
            sudo killall l2fwd
            mv $buildDir/*log $expDir/
        done
    done
}

for dist in 0.5 0.75 1.1 1.25 1.5 1.75 def; do
    RunBenchmark "$(ls $bashDir/01*yaml | grep -v pqueue | grep -v cuckoo | grep -v simple)" "131072 262144 524288 1048576 2097152 4194304 8388608" 200 $dist
    RunBenchmark "$(ls $bashDir/01*yaml | grep 'simple')" "16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608" 200 $dist
    RunBenchmark "$(ls $bashDir/01*yaml | grep 'cuckoo')" "131072 262144 524288 1048576 2097152 4194304" 200 $dist
    RunBenchmark "$(ls $bashDir/01*yaml | grep 'pqueue')" "8192 16384 32768 65536 131072 262144" 500 $dist
done


