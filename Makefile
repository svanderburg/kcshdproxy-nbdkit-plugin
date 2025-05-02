CC = gcc
CFLAGS = -Wall $(shell pkg-config --cflags nbdkit)
INSTALL = install
PREFIX = /usr
LIBDIR = $(PREFIX)/lib

all: kcshdproxy-plugin.so

kcshdproxy-plugin.so: kcshdproxy-plugin.c mbrsize.c
	$(CC) $(CFLAGS) -fPIC -shared $^ -o $@

querymbr: searchmbr.c
	$(CC) $< -o $@

install: all
	$(INSTALL) -d $(LIBDIR)
	$(INSTALL) kcshdproxy-plugin.so $(LIBDIR)

clean:
	rm -f *.so
	rm -f searchmbr
