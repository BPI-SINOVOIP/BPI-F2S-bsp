CROSS = arm-none-linux-gnueabi-

OUT := iop_test

iop_test: iop_test.o
	$(CROSS)gcc -o $@ $^ -static
	$(CROSS)strip $@

%.o: %.c
	$(CROSS)gcc -c -o $@ $<

clean:
	@rm -f *.o $(OUT)

