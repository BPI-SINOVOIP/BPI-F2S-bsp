

CC = arm-none-linux-gnueabi-gcc
CFLAGS = -O2 -Wall -D_GNU_SOURCE -static -I../../include -I../../../../../include
LDFLAGS = -static

SOURCES    = hdmitx_test.c
OBJECTS    = ${SOURCES:.c=.o}

OUT    = hdmitx_test.x

.PHONY: clean

all: $(OUT)
	@echo Build DONE.

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS) *~ $$$$~* *.bak core config.log $(OUT)