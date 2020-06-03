obj-m := ku_ipc.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	gcc -o ku_app_writer ku_app_writer.c
	gcc -o ku_app_reader ku_app_reader.c

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm ku_app_writer ku_app_reader