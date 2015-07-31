echo "Unmounting mbed"
sudo umount /media/MBED
sudo rm /media/MBED/*
sudo rm /media/MBED
echo "Shutting down"
sudo shutdown -h now
