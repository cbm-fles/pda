DEBUG=true
IPATH=/usr

all:
	find . -iname 'build_*' -exec make -C {} \;

install:
ifdef DESTDIR
	find . -iname 'build_*' -exec make -C {} install DESTDIR=$(DESTDIR) \;
else
	find . -iname 'build_*' -exec make -C {} install \;
endif

check: opt
	$(MAKE) PATH=$(PWD)/opt/bin/:$(PATH) -C ./test/
	$(MAKE) PATH=$(PWD)/opt/bin/:$(PATH) -C ./test/ clean
	$(MAKE) mrproper

check_build: opt
	$(MAKE) PATH=$(PWD)/opt/bin/:$(PATH) -C ./test/ build

nice: mrproper
	find . -name "patches" -prune -o -iname '*.c'   -exec uncrustify -c uncrust.cfg --no-backup -l C {} \;
	find . -name "patches" -prune -o -iname '*.h'   -exec uncrustify -c uncrust.cfg --no-backup -l C {} \;
	find . -name "patches" -prune -o -iname '*.inc' -exec uncrustify -c uncrust.cfg --no-backup -l C {} \;

opt:
	mkdir $(PWD)/opt/
	./configure --debug=$(DEBUG) --prefix=$(PWD)/opt/
	$(MAKE) install

dist: rpm

rpm: tarball
	./package/rpm/configure --version --prefix=$(IPATH)
	cp libpda-`cat VERSION`.tar.gz package/rpm/
	make -C package/rpm/
	cp ${HOME}/rpmbuild/RPMS/x86_64/libpda-`cat VERSION`*.rpm .
	make -C patches/linux_uio rpm
	cp ${HOME}/rpmbuild/RPMS/x86_64/pda_kadapter-`uname -r`*.rpm .

deb:
	make -C patches/linux_uio deb
	cp patches/pda-kernel-dkms_*.deb .

tarball: mrproper
	rm -rf libpda-`cat VERSION`.tar.gz
	tar -cf libpda-`cat VERSION`.tar *
	mkdir dist
	tar -xf libpda-`cat VERSION`.tar -C dist/
	rm -rf dist/libpda.cbp dist/libpda.layout
	-find ./dist -name '.svn' -exec rm -rf {} \; >> /dev/null
	rm -rf libpda-`cat VERSION`.tar
	mv dist libpda-`cat VERSION`
	tar -cf libpda-`cat VERSION`.tar libpda-`cat VERSION`
	gzip libpda-`cat VERSION`.tar
	rm -rf libpda-`cat VERSION`

mrproper: clean
	$(MAKE) PATH=$(PWD)/opt/bin/:$(PATH) -C ./test/ clean
	-rm -rf ./package/rpm/pda.spec
	-rm -rf patches/linux_uio/pda_kadapter.spec
	-rm -rf build_*
	-rm -rf opt
	-rm -rf dist
	-rm -rf doxygen
	-rm -rf *.tar
	-find . -iname 'a.out' -exec rm {} \;
	$(MAKE) -C package/rpm/ mrproper

clean:
	find . -iname 'build_*' -exec make -C {} clean \;
	rm -rf libpda*.tar.gz libpda*.rpm package/rpm/*.tar.gz
	rm -rf pda_kadapter*.rpm
	rm pda-kernel-dkms_*.deb

count: mrproper
	wc -l `find . -iname '*.c' && find . -iname '*.h' && find . -iname '*.inc'`	

doc:
	doxygen doxyfile.in
