echo "-- Unmounting mbed"
sudo umount /media/MBED
sleep 0.25
echo "--- Removing as a file"
sudo rm /media/MBED*
echo "--- Removing as a directory"
sudo rmdir /media/MBED*
