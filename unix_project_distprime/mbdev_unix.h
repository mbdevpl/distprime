/**
    Copyright 2013 Mateusz Bysiek
    mb@mbdev.pl

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#ifndef MBDEV_UNIX_H
#define MBDEV_UNIX_H

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h> // bool true false
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef TEMP_FAILURE_RETRY
/*!
 * \brief Used to uninterrupted read/write.
 *
 * Evaluate x, and repeat as long as it returns -1 with `errno' set to EINTR.
 *
 * copied from unistd.h, LGPL license
 */
# define TEMP_FAILURE_RETRY(expression) \
	(__extension__ \
	({ long int __result; \
	do __result = (long int) (expression); \
	while (__result == -1L && errno == EINTR); \
	__result; }))
#endif

#ifndef ERR
// errno error
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__), \
	perror(source),kill(0,SIGTERM), \
	exit(EXIT_FAILURE))
#endif

// h_errno error
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__), \
	exit(EXIT_FAILURE))

// custom error
#define CERR(source) (fprintf(stderr,"%s at %s:%d\n",source,__FILE__,__LINE__), \
	exit(EXIT_FAILURE))

// custom output
#define DEB (fprintf(stderr,"HELLO at %s:%d\n",__FILE__,__LINE__))
#define DEBOUT(msg) (fprintf(stderr,"%s at %s:%d\n",msg,__FILE__,__LINE__))

void reverse(char* s);
const char* itoa(int n, char* s);
const char* ltoa(long n, char* s);
const char* lltoa(long long n, char* s);

int setSigHandler( void (*f)(int), int sigNo);
const char* sigToStr(int sig);
void handlerSigchldDefault(int sig);

unsigned int getGoodSeed();

void exitWithError(const char* errorMsg);
void exitNormal();

void sleepFor(int t);
void milisleepFor(int t);

size_t bulk_read(int fd, char* buf, size_t count);
size_t readUntil(int fd, char* buf, size_t count, char marker);
size_t bulk_write(int fd, char* buf, size_t count);

void addFlags(int descriptor, int addedFlags);

void makefifo(char* arg);
int openfifo(char* arg, int flags);
void unlinkfifo(char* arg);
void closefifo(int fifo);

struct sockaddr_in make_address(char* address, uint16_t port);
int makeSocket(int domain, int type);
void closeSocket(int socket);

#endif // MBDEV_UNIX_H
