# Makefile for Program 2 - Chat Program
# written by Hugh Smith - April 2017
# editted by Shane Kent - Febuary 2018

CC = gcc
CFLAGS = -g -Wall
LIBS = 

FILE = 
ARCH = $(shell arch)
ifeq ("$(ARCH)", "i686")
	FILE = 32
endif

all: $(FILE) cclient$(FILE) server$(FILE)

32: rm -f *.o

cclient$(FILE): cclient.c networks.o gethostbyname6.o helper.o
	$(CC) $(CFLAGS) -o cclient$(FILE) cclient.c networks.o gethostbyname6.o helper.o $(LIBS)

server$(FILE): server.c networks.o gethostbyname6.o helper.o
	$(CC) $(CFLAGS) -o server$(FILE) server.c networks.o gethostbyname6.o helper.o $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f server cclient server32 cclient32 *.o
