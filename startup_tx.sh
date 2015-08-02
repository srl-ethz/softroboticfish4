#!/bin/sh

echo "--- killing existing fish processes"
sudo ps -ef | grep "fish" | awk '{print $2}' | xargs sudo kill

echo "--- removing state files"
sudo rm /home/pi/fish/python/fishAlive.txt*
sudo rm /home/pi/fish/python/fishStateCurrent*
sudo rm /home/pi/fish/python/fishStatePending*

echo "--- starting master heartbeat"
sudo python /home/pi/fish/python/heartbeat_pi_nofish.py &
sleep 5
echo "--- starting fish control"
sudo python /home/pi/fish/python/fishcontrol.py &
sleep 2
echo "--- starting fish control player"
sudo python /home/pi/fish/python/fishcontrol_playwav.py &
sleep 5
echo "--- done with startup"
