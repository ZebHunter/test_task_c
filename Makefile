name = dmp
obj-m = $(name).o
PWD = $(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build M="$(PWD)" modules

load:
	sudo insmod $(name).ko

unload:
	sudo rmmod -f $(name).ko

clean:
	make -C /lib/modules/$(shell uname -r)/build M="$(PWD)" clean

reload: unload clean all load