	#!/bin/sh
module="translate"
device="trans"
mode="664"

	# delete before inserting it again
/sbin/rmmod ./$module.ko
	# invoke insmod with all arguments we got
	# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod ./$module.ko $* || exit 1

	# remove stale nodes
rm -f /dev/${device}[0-1]
	#grep und cut in einem aus der Aufgabe4
major=$(awk -v mod="$device" '$2==mod{print $1}' /proc/devices)
echo $major
mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1

	# give appropriate group/permissions, and change the group.
	# Not all distributions have staff, some have "wheel" instead.
group="staff"
grep -q '^staff:' /etc/group || group="wheel"
chgrp $group /dev/${device}[0-1]
chmod $mode /dev/${device}[0-1]

