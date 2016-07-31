#!/bin/bash
date=$1
if [[ -z $date ]];
then
    echo "Usage: $0 YYYY/MM/DD"
    exit;
fi
mkdir -p captures/${date}/analysis
find captures/${date}/*/video0.jpeg | ./find_most_distinct -o=captures/${date}/analysis

