BINARY=bar_read_write
TARGET=static # binary, static, objects

SOURCES= \
bar_read_write.c

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

