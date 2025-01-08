obj-m += miniptm_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	insmod miniptm_module.ko
	dmesg | tail -n 10
	
full_test:
	#make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	insmod miniptm_module.ko
	dmesg | tail -n 10
