# distprime

all: server worker

mbdev_unix.o: mbdev_unix.c mbdev_unix.h
	gcc -Wall -c mbdev_unix.c

listfunctions.o: listfunctions.c listfunctions.h
	gcc -Wall -c listfunctions.c

distprimecommon.o: distprimecommon.c distprimecommon.h
	gcc -Wall -c distprimecommon.c

distprimeserver.o: distprimeserver.c
	gcc -Wall -c distprimeserver.c

distprimeworker.o: distprimeworker.c
	gcc -Wall -c distprimeworker.c

server: mbdev_unix.o listfunctions.o distprimecommon.o distprimeserver.o
	gcc -Wall -o distprime mbdev_unix.o listfunctions.o distprimecommon.o distprimeserver.o -lm

worker: mbdev_unix.o listfunctions.o distprimecommon.o distprimeworker.o
	gcc -Wall -o distprimeworker mbdev_unix.o listfunctions.o distprimecommon.o distprimeworker.o -lm

.PHONY: clean

clean:
	rm distprime distprimeworker mbdev_unix.o listfunctions.o distprimecommon.o distprimeserver.o distprimeworker.o
