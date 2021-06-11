KDIR    := /lib/modules/$(shell uname -r)/build
MOD_VERSION := $(shell cat uio_pci_dma.h | grep UIO_PCI_DMA_VERSION | cut -d\" -f2)
VERSION     := $(shell uname -r)-$(shell cat uio_pci_dma.h | grep UIO_PCI_DMA_VERSION | cut -d\" -f2)

build: prepare
	KDIR=$(KDIR) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) make -C build/
	cp build/uio_pci_dma.ko .
	rm -rf build

prepare: clean
	mkdir build
	cp Makefile_dma.temp build/Makefile
	cp uio_pci_dma.* build/

install: udev dkms

udev:
	install 99-pda.rules /etc/udev/rules.d/

manual_install: build udev
	cp      uio_pci_dma.ko /lib/modules/$(shell uname -r)/kernel/drivers/uio/
	depmod -a

rpminstall: build
	mkdir -p             $(INSTALL_MOD_PATH)/lib/modules/$(shell uname -r)/kernel/drivers/uio/
	cp uio_pci_dma.ko    $(INSTALL_MOD_PATH)/lib/modules/$(shell uname -r)/kernel/drivers/uio/
	mkdir -p             $(INSTALL_MOD_PATH)/etc/udev/rules.d/
	cp 99-pda.rules      $(INSTALL_MOD_PATH)/etc/udev/rules.d/

rpm: pack
	./configure --version
	mkdir -p ${HOME}/rpmbuild
	mkdir -p ${HOME}/rpmbuild/BUILD ${HOME}/rpmbuild/BUILDROOT ${HOME}/rpmbuild/RPMS ${HOME}/rpmbuild/SOURCES ${HOME}/rpmbuild/SPECS ${HOME}/rpmbuild/SRPMS
	mv *.tar.gz ${HOME}/rpmbuild/SOURCES
	rpmbuild -ba pda_kadapter.spec

pack:
	rm -rf pda_kadapter-${VERSION}
	mkdir pda_kadapter-${VERSION}
	cp Makefile Makefile_dma.temp uio_pci_dma.c uio_pci_dma.h 99-pda.rules pda_kadapter-${VERSION}/
	tar -cf pda_kadapter-${VERSION}.tar pda_kadapter-${VERSION}
	gzip pda_kadapter-${VERSION}.tar
	rm -rf pda_kadapter-${VERSION}

dkms:
	mkdir -p /usr/src/uio_pci_dma-${MOD_VERSION}
	cp uio_pci_dma.c uio_pci_dma.h dkms.conf /usr/src/uio_pci_dma-${MOD_VERSION}/
	cp Makefile.dkms /usr/src/uio_pci_dma-${MOD_VERSION}/Makefile
	dkms install uio_pci_dma/${MOD_VERSION}

clean:
	rm -rf test
	rm -rf uio_pci_dma.ko
	rm -rf build
	rm -rf *.tar.gz *.spec *.tmp
