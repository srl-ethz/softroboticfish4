echo "-- Killing fish processes"
sudo ps -ef | grep "fish/python" | awk '{print $2}' | xargs sudo kill


