#!/bin/bash
date=$1
timestamps=$(ls ../captures/${date})
last_timestamp=""
for ts in ${timestamps}; do
    if [[ -z  $last_timestamp  ]]; then
	last_timestamp=$ts
	continue
    fi
    ./find_most_distinct -l=../captures/${date}/${ts}/video0.jpeg -r=../captures/${date}/${last_timestamp}/video0.jpeg -o=../captures/${date}/${ts}/diff0.jpeg -t=manifest.txt
    last_timestamp=${ts}
done
