# unix_lab3_fix

all: server
#client

mbdev_unix.o: mbdev_unix.c mbdev_unix.h
	gcc -Wall -c mbdev_unix.c

listfunctions.o: listfunctions.c listfunctions.h
	gcc -Wall -c listfunctions.c

distprimeserver.o: distprimeserver.c
	gcc -Wall -c distprimeserver.c

#autoportclient.o: autoportclient.c
#	gcc -Wall -c autoportclient.c

server: mbdev_unix.o listfunctions.o distprimeserver.o
	gcc -Wall -o distprime mbdev_unix.o listfunctions.o distprimeserver.o

#client: mbdev_unix.o autoportclient.o
#	gcc -Wall -o client mbdev_unix.o autoportclient.o

.PHONY: clean

clean:
	rm server mbdev_unix.o listfunctions.o distprimeserver.o
# client autoportclient.o
