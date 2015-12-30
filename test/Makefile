TESTS=           \
inc              \
operator         \
buffer           \
buffer_fork      \
buffer_free      \
buffer_list      \
buffer_reconnect \
buffer_locking   \
buffer_wrapmap   \
sglist           \
interrupts       \
bar_read_write   \
bar_perf


run: build
	@for T in $(TESTS); do \
		echo -e "\n#--- running test $$T --------"; \
		./$$T/run.sh ; \
	done

build: clean
	@for T in $(TESTS); do \
		echo -e "\n#--- building test $$T --------"; \
		$(MAKE) -C ./$$T/ ; \
	done	

clean:
	@for T in $(TESTS); do \
		echo -e "\n#--- running clean $$T -------"; \
		$(MAKE) -i -C ./$$T/ clean; \
	done
	rm -rf bar*.bin
