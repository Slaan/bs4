#!/bin/sh

# check permissions
if [ "$(id -u)" != "0" ]; then
  echo "Please run as root."
  exit
fi

# remove existing modules
rmmod translate

# insert our module
# with parameter
insmod build/translate.ko
# add device file
major=$(awk -v mod="$device" '$2==mod{print $1}' /proc/devices)
mknod /dev/trans0 c $major 0
mknod /dev/trans1 c $major 1
# change device permissions
sudo chmod 777 /dev/trans0
sudo chmod 777 /dev/trans1
