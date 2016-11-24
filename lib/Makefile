# This is not a kbuild Makefile. It is a plain Makefile so it can be copied

# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

ZIO ?= ../zio

LIB = libfmcadc.a
LOBJ := route.o
LOBJ += init.o
LOBJ += boards.o
LOBJ += config-zio.o
LOBJ += buffer-zio.o
LOBJ += lib.o
LOBJ += fmc-adc-100m14b4cha.o
CFLAGS = -Wall -ggdb -O2 -I../kernel -I$(ZIO)/include $(EXTRACFLAGS)
LDFLAGS = -L. -lfmcadc

CC ?= $(CROSS_COMPILE)gcc

modules all: $(LIB)


%: %.c $(LIB)
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -o $@

$(LIB): $(LOBJ)
	ar r $@ $^

clean:
	rm -f $(LIB) .depend *.o *~

.depend: Makefile $(wildcard *.c *.h ../*.h)
	$(CC) $(CFLAGS) -M $(LOBJ:.o=.c) -o $@

install modules_install:

-include .depend

