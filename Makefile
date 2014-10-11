DEBUG=true
IPATH=/opt/pda

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
	cp pda-`cat VERSION`.tar.gz package/rpm/
	make -C package/rpm/
	make -C patches/linux_uio
	cp ${HOME}/rpmbuild/RPMS/x86_64/pda*.rpm .

tarball: mrproper
	rm -rf pda-`cat VERSION`.tar.gz
	tar -cf pda-`cat VERSION`.tar *
	mkdir dist
	tar -xf pda-`cat VERSION`.tar -C dist/
	rm -rf dist/pda.cbp dist/pda.layout
	-find ./dist -name '.svn' -exec rm -rf {} \; >> /dev/null
	rm -rf pda-`cat VERSION`.tar
	mv dist pda-`cat VERSION`
	tar -cf pda-`cat VERSION`.tar pda-`cat VERSION`
	gzip pda-`cat VERSION`.tar
	rm -rf pda-`cat VERSION`

mrproper: clean
	$(MAKE) PATH=$(PWD)/opt/bin/:$(PATH) -C ./test/ clean
	-rm -rf ./package/rpm/pda.spec
	-rm -rf build_*
	-rm -rf opt
	-rm -rf dist
	-rm -rf doxygen
	-rm -rf *.tar
	-find . -iname 'a.out' -exec rm {} \;
	$(MAKE) -C package/rpm/ mrproper

clean:
	find . -iname 'build_*' -exec make -C {} clean \;
	rm -rf pda*.tar.gz pda*.rpm

count: mrproper
	wc -l `find . -iname '*.c' && find . -iname '*.h' && find . -iname '*.inc'`	

doc:
	doxygen doxyfile.in
