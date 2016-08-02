#!/bin/bash
date=$1
ts_prefix=$2
if [[ -z $date ]];
then
    echo "Usage: $0 YYYY/MM/DD [timestamp_prefix]"
    exit;
fi
analysis_dir="captures/${date}/analysis"
if [[ ! -z $ts_prefix ]];
then
    analysis_dir=${analysis_dir}_${ts_prefix}
fi
mkdir -p ${analysis_dir}
find captures/${date}/${ts_prefix}*/video0.jpeg | ./find_most_distinct -o=${analysis_dir}
i=1
echo "<html>" > ${analysis_dir}/index.html
for diffline in `cat ${analysis_dir}/manifest.csv`;
do
    echo $diffline
    file_a=$(echo $diffline | cut -d"," -f2)
    file_b=$(echo $diffline | cut -d"," -f3)
    score=$(echo $diffline | cut -d"," -f4)
    diff_file=$(echo $diffline | cut -d"," -f5)
    echo "<center><img src='/${diff_file}'/><br/>${score}<br/><img src='/${file_a}'/><img src='/${file_b}'/>" > ${analysis_dir}/diff_${i}.html
    echo "<a href='/${analysis_dir}/diff_${i}.html'>Diff ${i}: ${score}</a><br/>" >> ${analysis_dir}/index.html
    i=$(($i+1))
done
echo "</html>" >> ${analysis_dir}/index.html 
