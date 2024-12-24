CC=gcc
SOURCES= ./lib/utils.c
CFLAGS= -g -Wall

build:	build_server build_client

build_server: 	$(SOURCES)
					$(CC) -o ./serv ./server.c $(SOURCES) $(CFLAGS)
build_client: 	$(SOURCES)
					$(CC) -o ./dev ./device.c $(SOURCES) $(CFLAGS)
