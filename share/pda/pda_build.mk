OBJS=$(patsubst %.c,%.o,$(SOURCES))

CC=`pda-config --cc --ccopts`
LD=`pda-config --cc --ld`

binary: objects
	$(LD) *.o `pda-config --ldpath` -o ${BINARY}

static: objects
	$(LD) *.o `pda-config --sdpath` -o ${BINARY}

objects: $(OBJS)

%.o: %.c
	gcc -c -I`pda-config --include --ccopts ` $< -o $@

cleaner:
	rm -rf ${BINARY}
	rm -rf *.o
