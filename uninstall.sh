#!/bin/sh

# check permissions
if [ "$(id -u)" != "0" ]; then
  echo "Please run as root."
  exit
fi

# remove inserted module
rmmod translate

# remove device file
rm /dev/trans0
rm /dev/trans1
