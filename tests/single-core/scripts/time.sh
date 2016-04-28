#!/bin/bash

folder=$1

p50=$(tac $folder/app.log | egrep '50.0(\s*)Percentile' | grep -v 4096 | awk '{print $4}' | head -n1)
p95=$(tac $folder/app.log | egrep '95.0(\s*)Percentile' | grep -v 4096 | awk '{print $4}' | head -n1)
p99=$(tac $folder/app.log | egrep '99.0(\s*)Percentile' | grep -v 4096 | awk '{print $4}' | head -n1)
avg=$(tac $folder/app.log | egrep 'Average' | grep -v nan | awk '{print $5}' | head -n1)

echo -e "$avg\t$p50\t$p95\t$p99"
