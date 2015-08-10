DIR="/home/pi/fish/"
find $DIR -type f |  while read filename; do dos2unix $filename; done
