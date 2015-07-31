echo "-- Mounting mbed"
sudo mkdir /media/MBED
sleep 0.25
sudo mount /dev/sda /media/MBED
sleep 1.5
echo "-- If you see mbed's files here then it worked!"
sudo ls /media/MBED
