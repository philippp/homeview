DATA_DIR="/var/code/homeview/captures/"
DATESTRING=$(date +%Y/%m/%d/%H%M%S)
DATESTRING_PRETTY=$(date +"%Y/%m/%d %H:%M:%S")
DESTINATION_DIR=${DATA_DIR}${DATESTRING}
mkdir -p $DESTINATION_DIR

# Grab a network snapshot
nmap -sn -oX ${DESTINATION_DIR}/network.xml 192.168.1.1/24
xsltproc ${DESTINATION_DIR}/network.xml -o ${DESTINATION_DIR}/network.html

# Take some photos with USB Cameras and label them.
streamer -f jpeg -o ${DESTINATION_DIR}/video1.jpeg -c /dev/video1
convert ${DESTINATION_DIR}/video1.jpeg  -fill white  -undercolor '#00000080'  -gravity South -annotate +0+5 " ${DATESTRING_PRETTY} " ${DESTINATION_DIR}/video1.jpeg
streamer -f jpeg -o ${DESTINATION_DIR}/video0.jpeg -c /dev/video0
convert ${DESTINATION_DIR}/video0.jpeg  -fill white  -undercolor '#00000080'  -gravity South -annotate +0+5 " ${DATESTRING_PRETTY} " ${DESTINATION_DIR}/video0.jpeg

# Done processing -- update the latest data symlink.
rm ${DATA_DIR}/latest
ln -s $DESTINATION_DIR ${DATA_DIR}/latest
