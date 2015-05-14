obj-m += ring_buf.o
#obj-m += post_ring_buf.o
#obj-m += pull_ring_buf.o
#obj-m += timer.o
#obj-m += kthread.o
obj-m += uart_driver_x86.o
obj-m += pca_tdma_code.o
obj-m += pull_ring_buf.o
obj-m += test.o


#CFLAGS_MODULE=-Werror

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

#	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

