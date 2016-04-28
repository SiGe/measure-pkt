#!/bin/bash

graphName="graph.gp"

# Get the directory of the benchmark
bashDir=$(dirname "$0")
pushd $bashDir > /dev/null
bashDir=$(pwd)
popd > /dev/null

resultsFile=$1
fields=$(cat $resultsFile | awk '{print $1}' | sort | uniq)

cat graph.tpl > $graphName
cat <<EOF >>$graphName
# Get the directory of the benchmark
set xlabel "Size (MB)"
set ylabel "Time per packet (ns)"
set key top right
set logscale x
set mxtics 10
# set yrange [10:100]
EOF

count=1
columnToExtract="\$$2"

printf "plot " >>$graphName
for field in $fields; do
    cat $resultsFile | egrep "$field(\s+)" | sort -nk2 > ${field}-tmp.csv
    title=$(echo $field | cut -d'-' -f4-)
    #printf "'${field}-tmp.csv' using (\$2*5*4/(1024*1024)):(\$7/2.8) title '${field}' w lp ls $count," >>$graphName
    if [[ $field == *"pqueue"* ]]; then
        printf "'${field}-tmp.csv' using (\$2*132*4/(1024*1024)):($columnToExtract) title '${title}' w lp ls $count," >>$graphName
    elif [[ $field == *"cuckoo"* ]]; then
        printf "'${field}-tmp.csv' using (\$2*5*2*4/(1024*1024)):($columnToExtract) title '${title}' w lp ls $count," >>$graphName
    else
        printf "'${field}-tmp.csv' using (\$2*5*4/(1024*1024)):($columnToExtract) title '${title}' w lp ls $count," >>$graphName
    fi
    count=$(echo "$count + 1" | bc)
done

gnuplot $graphName
rm *-tmp.csv
rm $graphName

