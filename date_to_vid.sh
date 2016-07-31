directory_list=`ls captures/$1/*/video0.jpeg | sed -e "s/video0.jpeg//"`
mkdir scratch
i=0
for directory in $directory_list
do
    pretty_i=`printf %05d $i`
    convert ${directory}/video0.jpeg ${directory}/video1.jpeg +append scratch/video_${pretty_i}.jpeg
    rc=$?; if [[ $rc == 0 ]]; then i=$((i+1)); fi
    echo $pretty_i
done
ffmpeg -framerate 10 -i scratch/video_%05d.jpeg -c:v libx264 -profile:v high -crf 20 -pix_fmt yuv420p output.mp4
rm -rf scratch/
