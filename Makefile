# check permissions
if [ "$(id -u)" != "0" ]; then
  echo "Please run as root."
  exit
fi

obj-m += hello-1.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/src modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/src clean
