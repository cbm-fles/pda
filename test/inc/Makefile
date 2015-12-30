SRCS=$(wildcard *.c)
BINS=$(patsubst %.c,%,$(SRCS))

all:
	./gen_c.sh
	$(MAKE) bin

bin: $(BINS)

%: %.c
	$(CC) -I`pda-config --include` $< -o $@

clean:
	rm -rf $(BINS)
	rm -rf $(SRCS)
