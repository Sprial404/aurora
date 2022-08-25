CC      = gcc
CFLAGS  = -g -Wall -Wextra -pedantic -Werror -O2 -std=c11 -Wno-unused-parameter
LDFLAGS =
LDLIBS  =
PREFIX  = /usr/local

aurora: aurora.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $<
