KDIR := /lib/modules/$(shell uname -r)/build

all: prepare
	KDIR=$(KDIR) make -C build/
	cp build/interrupt.ko .
	rm -rf build

prepare: clean
	mkdir build
	cp Makefile_dma.temp build/Makefile
	cp interrupt.* build/

install: all
	cp interrupt.ko /lib/modules/$(shell uname -r)/kernel/drivers/uio/
	depmod -a

clean:
	rm -rf test
	rm -rf interrupt.ko
	rm -rf build
