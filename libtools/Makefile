# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

LIBADC = ../lib/
ZIO ?= ../zio

CFLAGS = -Wall -g -ggdb -I$(LIBADC) -I$(ZIO)/include -I../kernel $(EXTRACFLAGS)
LDFLAGS = -L$(LIBADC) -lfmcadc -lpthread -lrt

DEMOS := fald-simple-acq fald-acq fald-trg-cfg
DEMOS += fald-simple-get-conf
DEMOS += fald-test
DEMOS += fald-bad-clock
DEMOS += acq-program



all: demo

demo: $(DEMOS)

install:
	install -d $(DESTDIR)/bin
	install -D $(DEMOS) $(DESTDIR)/bin

%: %.c $(LIBADC)/libfmcadc.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(DEMOS) *.o *~

.PHONY: all, clean
