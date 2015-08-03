echo "-- Killing fish processes"
sudo ps -ef | grep "fish" | awk '{print $2}' | xargs sudo kill

