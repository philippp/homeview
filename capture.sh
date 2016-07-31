#!/bin/sh
DATA_DIR="/var/code/homeview/captures/"
PERIOD_SECONDS=30

snapshot () {
    DATESTRING=$(date +%Y/%m/%d/%H%M%S)
    DATESTRING_PRETTY=$(date +"%Y/%m/%d %H:%M:%S")
    DESTINATION_DIR=${DATA_DIR}${DATESTRING}
    mkdir -p $DESTINATION_DIR
    
    # Grab a network snapshot
    nmap -sn -oX ${DESTINATION_DIR}/network.xml 192.168.1.1/24
    xsltproc ${DESTINATION_DIR}/network.xml -o ${DESTINATION_DIR}/network.html
    
    # Take some photos with USB Cameras and label them.
    ffmpeg -f video4linux2 -i /dev/video1 -vframes 4 -q:v 1 -video_size 1024x768 /tmp/video%3d.jpeg
    mv /tmp/video004.jpeg ${DESTINATION_DIR}/video1.jpeg
    rm /tmp/video00*.jpeg
    convert ${DESTINATION_DIR}/video1.jpeg  -fill white  -undercolor '#00000080'  -gravity South -annotate +0+5 " ${DATESTRING_PRETTY} " ${DESTINATION_DIR}/video1.jpeg

    ffmpeg -f video4linux2 -i /dev/video0 -vframes 4 -q:v 1 -video_size 1024x768 /tmp/video%3d.jpeg
    mv /tmp/video004.jpeg ${DESTINATION_DIR}/video0.jpeg
    rm /tmp/video00*.jpeg
    convert ${DESTINATION_DIR}/video0.jpeg  -fill white  -undercolor '#00000080'  -gravity South -annotate +0+5 " ${DATESTRING_PRETTY} " ${DESTINATION_DIR}/video0.jpeg
    sudo chown -R philippp:philippp ${DESTINATION_DIR}
    # Done processing -- update the latest data symlink.
    rm ${DATA_DIR}/latest
    ln -s $DESTINATION_DIR ${DATA_DIR}/latest
}

next_time=`date +%s`
while true
do
    next_time=$((((`date +%s` / ${PERIOD_SECONDS})+1)*${PERIOD_SECONDS}))
    snapshot
    sleep_time=$((next_time - `date +%s`))
    echo "Sleeping for ${sleep_time} seconds..."
    sleep ${sleep_time}
done
