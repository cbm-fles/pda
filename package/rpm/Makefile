all: clean prepare
	rpmbuild -ba pda.spec

prepare:
	mkdir -p ${HOME}/rpmbuild
	mkdir -p ${HOME}/rpmbuild/BUILD ${HOME}/rpmbuild/BUILDROOT ${HOME}/rpmbuild/RPMS ${HOME}/rpmbuild/SOURCES ${HOME}/rpmbuild/SPECS ${HOME}/rpmbuild/SRPMS
	cp *.tar.gz ${HOME}/rpmbuild/SOURCES

clean:
	rm -rf rpmbuild 

mrproper: clean
	rm -rf pda.spec *.tar.gz
