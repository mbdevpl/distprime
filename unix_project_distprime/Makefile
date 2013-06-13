# distprime

# this project requires libxml2 library and headers
# in Ubuntu install libxml2-dev package

INCLUDES=-I/usr/include/libxml2
# LIBS=-L/usr/lib/i386-linux-gnu

all: server worker

mbdev_unix.o: mbdev_unix.c mbdev_unix.h
	gcc -Wall -c mbdev_unix.c

listfunctions.o: listfunctions.c listfunctions.h
	gcc -Wall -c listfunctions.c

distprimecommon.o: distprimecommon.c distprimecommon.h processdata.h workerdata.h serverdata.h defines.h
	gcc -Wall $(INCLUDES) -c distprimecommon.c

# structures
processdata.o: processdata.c processdata.h
	gcc -Wall $(INCLUDES) -c processdata.c

workerdata.o: workerdata.c workerdata.h
	gcc -Wall $(INCLUDES) -c workerdata.c

serverdata.o: serverdata.c serverdata.h
	gcc -Wall $(INCLUDES) -c serverdata.c

# prime ranges
primerange.o: primerange.c primerange.h
	gcc -Wall $(INCLUDES) -c primerange.c

# server
distprimeserver.o: distprimeserver.c distprimecommon.h primerange.h
	gcc -Wall $(INCLUDES) -c distprimeserver.c

# worker
distprimeworker.o: distprimeworker.c distprimecommon.h
	gcc -Wall $(INCLUDES) -c distprimeworker.c

server: mbdev_unix.o listfunctions.o processdata.o workerdata.o serverdata.o distprimecommon.o distprimeserver.o primerange.o
	gcc -Wall $(INCLUDES) -o distprime mbdev_unix.o listfunctions.o processdata.o workerdata.o serverdata.o distprimecommon.o primerange.o distprimeserver.o -lm -lxml2
#  $(LIBS)

worker: mbdev_unix.o listfunctions.o processdata.o workerdata.o serverdata.o distprimecommon.o distprimeworker.o
	gcc -Wall $(INCLUDES) -o distprimeworker mbdev_unix.o listfunctions.o processdata.o workerdata.o serverdata.o distprimecommon.o distprimeworker.o -lm -lxml2
# $(LIBS)

.PHONY: clean

clean:
	rm distprime distprimeworker mbdev_unix.o listfunctions.o distprimecommon.o processdata.o workerdata.o serverdata.o primerange.o distprimeserver.o distprimeworker.o
