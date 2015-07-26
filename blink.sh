echo "none" | sudo tee /sys/class/leds/led0/trigger > /dev/null
echo "1" | sudo tee /sys/class/leds/led0/brightness > /dev/null
sleep 0.2s
echo "0" | sudo tee /sys/class/leds/led0/brightness > /dev/null
sleep 0.2s
echo "1" | sudo tee /sys/class/leds/led0/brightness > /dev/null
sleep 0.2s
echo "0" | sudo tee /sys/class/leds/led0/brightness > /dev/null
echo "mmc0" | sudo tee /sys/class/leds/led0/trigger > /dev/null

