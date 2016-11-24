# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

# user-space tools for spec-fine-delay
M = $(shell /bin/pwd)/../kernel
DESTDIR ?= /usr/local

CFLAGS += -I$(M) -I$(ZIO)/include -Wno-trigraphs -Wall -ggdb -O2  $(EXTRACFLAGS)

CC ?= $(CROSS_COMPILE)gcc

progs := fau-trg-config
progs += fau-acq-time
progs += parport-burst

# we are not in the kernel, so we need to piggy-back on "make modules"
all modules: $(progs)

clean:
	rm -f $(progs) *.o *~

# make nothing for modules_install, but avoid errors
modules_install:

install:
	install -d $(DESTDIR)/bin
	install -D $(progs) $(DESTDIR)/bin

# we need this as we are out of the kernel
%: %.c
	$(CC) $(CFLAGS) $^ -o $@
