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

distprimecommon.o: distprimecommon.c distprimecommon.h
	gcc -Wall $(INCLUDES) -c distprimecommon.c

distprimeserver.o: distprimeserver.c
	gcc -Wall $(INCLUDES) -c distprimeserver.c

distprimeworker.o: distprimeworker.c
	gcc -Wall $(INCLUDES) -c distprimeworker.c

server: mbdev_unix.o listfunctions.o distprimecommon.o distprimeserver.o
	gcc -Wall $(INCLUDES) -o distprime mbdev_unix.o listfunctions.o distprimecommon.o distprimeserver.o -lm -lxml2
#  $(LIBS)

worker: mbdev_unix.o listfunctions.o distprimecommon.o distprimeworker.o
	gcc -Wall $(INCLUDES) -o distprimeworker mbdev_unix.o listfunctions.o distprimecommon.o distprimeworker.o -lm -lxml2
# $(LIBS)

.PHONY: clean

clean:
	rm distprime distprimeworker mbdev_unix.o listfunctions.o distprimecommon.o distprimeserver.o distprimeworker.o
