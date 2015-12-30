BINARY=buffer_wrapmap
TARGET=static # binary, static, objects

SOURCES= \
buffer_wrapmap.c

#-------------------------------------------------------

hook:
	$(MAKE) INCLUDEFILE=`pda-config --build` all 

clean:
	$(MAKE) INCLUDEFILE=`pda-config --build` cleaner 

include $(INCLUDEFILE)

all: $(TARGET)

#-------------------------------------------------------
# you can include your own rules here!
#-------------------------------------------------------

