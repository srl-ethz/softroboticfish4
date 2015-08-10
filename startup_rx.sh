echo "- Startup script"
sudo /home/pi/fish/mbed/remountMbed.sh
#sudo python /home/pi/fish/mbed/resetMbed.py
echo "-- Starting video and low battery monitor"
sudo python /home/pi/fish/python/rx_controller.py &
echo "-- Starting serial monitor to record data"
sudo python /home/pi/fish/mbed/runProgram.py print=0 file='data.wp' &
echo "- Startup script complete"

