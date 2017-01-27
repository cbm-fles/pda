include arch.mk
include gen.mk

LDNAME = libpda
SRCS   = $(wildcard src/*.c)
OBJS   = $(patsubst src/%.c,src/%.o,$(SRCS))
LARCH  = $(patsubst src/%.c,%.lo,$(SRCS))

all: libpda.so

libpda.so: $(OBJS)
	echo "BUILD PREFIX $(PREFIX)"
	libtool --mode=link --tag=CC \
	$(CC) $(LDFLAGS) -o $(LDNAME).la $(LARCH) \
	-rpath $(PREFIX)/lib/ -version-info $(LD_CUR):$(LD_REV):$(LD_AGE)
	cp .libs/libpda.so .

%.o: %.c
	libtool --mode=compile --tag=CC $(CC) $(CFLAGS) $(INCLUDE) -c $<
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

install: libpda.so gendirs
	@echo "-= install libs =-  $(PREFIX) $(DESTDIR)"
	libtool --mode=install --tag=CC cp $(LDNAME).la $(DESTDIR)/lib/$(LDNAME).la
	libtool --finish $(DESTDIR)/lib/
	@echo -= install header files =- 
	cp -r -L include/* $(DESTDIR)/include
	@echo -= install bin stuff =-
	cp -r -L scripts/* $(DESTDIR)/bin
	echo "CC=\"$(CC)\" "                              >  $(DESTDIR)/bin/pda-config.inc
	echo "LDFLAGS=\"$(LDFLAGS)\" "                    >> $(DESTDIR)/bin/pda-config.inc
	echo "CFLAGS=\"$(CFLAGS)\" "                      >> $(DESTDIR)/bin/pda-config.inc
	echo "VERSION=\"$(LD_CUR).$(LD_REV).$(LD_AGE)\" " >> $(DESTDIR)/bin/pda-config.inc
	@echo -= install shared stuff =-
	cp -r -L share/pda/* $(DESTDIR)/share/pda

gendirs:
	mkdir -p $(DESTDIR)/lib
	mkdir -p $(DESTDIR)/include
	mkdir -p $(DESTDIR)/bin
	mkdir -p $(DESTDIR)/share
	mkdir -p $(DESTDIR)/share/pda

uninstall:
	-rm    $(PREFIX)/lib/$(LDNAME).so.$(LDMAJOR)
	-rm    $(PREFIX)/lib/$(LDNAME).so
	-rm    $(PREFIX)/lib/$(LDNAME).so.$(LD_VERSION)
	-rm    $(PREFIX)/lib/$(LDNAME).a
	-rm -r $(PREFIX)/include/pda/
	-rm    $(PREFIX)/include/pda.h
	-rm    $(PREFIX)/bin/pda-config*

clean:
	echo $(INCLUDE)
	@for f in $(CLEANUP_FILES); do \
		find . -iname '*'$$f -exec echo "rm -rf {}" \; ; \
		find . -iname '*'$$f -exec rm -rf {} \; ; \
	done
	-rm -rf $(LDNAME).so.$(LD_VERSION) $(LDNAME).a
	-rm -rf .libs libpda.so
