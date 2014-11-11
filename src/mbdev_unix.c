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

#include "mbdev_unix.h"

/*!
 * \brief reverse string s in place
 *
 * used by itoa
 *
 * source: first edition of Kernighan and Ritchie's The C Programming Language, around page 60
 */
void reverse(char s[])
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}


/*!
 * \brief convert n to characters in s
 *
 * copied here because itoa is not a standard function
 *
 * source: first edition of Kernighan and Ritchie's The C Programming Language, around page 60
 */
const char* itoa(int n, char* s)
{
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;          /* make n positive */
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = n % 10 + '0';   /* get next digit */
	} while ((n /= 10) > 0);     /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
	return s;
}


const char* ltoa(long n, char* s)
{
	long i, sign;

	if ((sign = n) < 0)
		n = -n;
	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
	return s;
}


const char* lltoa(long long n, char* s)
{
	long long i, sign;

	if ((sign = n) < 0)
		n = -n;
	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
	return s;
}


/*!
 * \brief sets the handler for the specified signal
 *
 * copied from my unix programming classes materials
 *
 * \param f handler
 * \param sigNo signal number
 * \return -1 if error occured while setting up handler, 0 otherwise
 */
int setSigHandler( void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}


#define IFEQUAL_VARNAMETOSTR(x) else if(sig == x) returned = (#x)

/*!
 * \brief Converts a signal number to its string representation.
 */
const char* sigToStr(int sig)
{
	const char* returned;

	if(sig < 0) returned = "invalid signal";
	IFEQUAL_VARNAMETOSTR(SIGABRT);
	IFEQUAL_VARNAMETOSTR(SIGALRM);
	IFEQUAL_VARNAMETOSTR(SIGBUS);
	IFEQUAL_VARNAMETOSTR(SIGCHLD);
	IFEQUAL_VARNAMETOSTR(SIGCLD); // same as SIGCHLD
	IFEQUAL_VARNAMETOSTR(SIGCONT);
	IFEQUAL_VARNAMETOSTR(SIGFPE);
	IFEQUAL_VARNAMETOSTR(SIGINT);
	IFEQUAL_VARNAMETOSTR(SIGKILL);
	IFEQUAL_VARNAMETOSTR(SIGPIPE);
	IFEQUAL_VARNAMETOSTR(SIGPOLL);
	IFEQUAL_VARNAMETOSTR(SIGPROF);
	IFEQUAL_VARNAMETOSTR(SIGPWR);
	IFEQUAL_VARNAMETOSTR(SIGQUIT);
	IFEQUAL_VARNAMETOSTR(SIGRTMAX); // not a constant, cannot use switch
	IFEQUAL_VARNAMETOSTR(SIGRTMIN); // not a constant, cannot use switch
	IFEQUAL_VARNAMETOSTR(SIGSEGV);
	IFEQUAL_VARNAMETOSTR(SIGSTKFLT);
	IFEQUAL_VARNAMETOSTR(SIGSTKSZ);
	IFEQUAL_VARNAMETOSTR(SIGSTOP);
	IFEQUAL_VARNAMETOSTR(SIGSYS);
	IFEQUAL_VARNAMETOSTR(SIGTERM);
	IFEQUAL_VARNAMETOSTR(SIGTRAP);
	IFEQUAL_VARNAMETOSTR(SIGTSTP);
	IFEQUAL_VARNAMETOSTR(SIGTTIN);
	IFEQUAL_VARNAMETOSTR(SIGTTOU);
	IFEQUAL_VARNAMETOSTR(SIGUNUSED); // same as SIGSYS
	IFEQUAL_VARNAMETOSTR(SIGURG);
	IFEQUAL_VARNAMETOSTR(SIGUSR1);
	IFEQUAL_VARNAMETOSTR(SIGUSR2);
	IFEQUAL_VARNAMETOSTR(SIGVTALRM);
	IFEQUAL_VARNAMETOSTR(SIGWINCH);
	IFEQUAL_VARNAMETOSTR(SIGXCPU);
	IFEQUAL_VARNAMETOSTR(SIGXFSZ);
	else
	{
		if(sig >= SIGRTMIN && sig <= SIGRTMAX)
			returned = "custom signal";

		char* temp = (char*)malloc(4*sizeof(char));
		returned = itoa(sig, temp);
	}
	return returned;
}

#undef IFEQUAL_VARNAMETOSTR


void handlerSigchldDefault(int sig)
{
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(0==pid) return;
		if(0>=pid) {
			if(ECHILD==errno) return;
			ERR("waitpid:");
		}
	}
}

unsigned int getGoodSeed()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec*1000 + time.tv_usec/1000)*3 + getpid()*11;
}

/*!
 * \brief Exits the process with EXIT_FAILURE and message.
 */
void exitWithError(const char* errorMsg)
{
	perror(errorMsg);
	exit(EXIT_FAILURE);
}


/*!
 * \brief Exits the process normallly (i.e. with EXIT_SUCCESS).
 */
void exitNormal()
{
	exit(EXIT_SUCCESS);
}


/*!
 * \brief Sleeps for given number of seconds.
 * \param t time in seconds
 */
void sleepFor(int t)
{
	for(;t>0;t=sleep(t));
}


void milisleepFor(int milisecs)
{
	struct timespec req;
	req.tv_sec = milisecs > 0 ? (long)(milisecs/1000) : 0L;
	req.tv_nsec = ((long)(milisecs % 1000)) * 1000000L;
	struct timespec rem;
	rem.tv_sec = 0;
	rem.tv_nsec = 0;
	do
	{
		//printf("%ld s, %ld ns\n", req.tv_sec, req.tv_nsec);
		//printf("%ld s, %ld ns\n", rem.tv_sec, rem.tv_nsec);
		if(nanosleep(&req, &rem)<0)
		{
			switch(errno)
			{
			case EFAULT: exitWithError("nanosleep error when copying data");
			case EINVAL: exitWithError("nanosleep out of bounds error");
			case EINTR: continue;
			}
		}
	}
	while(rem.tv_sec > 0 || rem.tv_nsec > 0);
}


size_t bulk_read(int fd, char* buf, size_t count)
{
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		//printf("\"%s\", count=%d\n", buf, c);
		if(c<0) return c;
		if(c==0) return len; //EOF
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len;
}

size_t readUntil(int fd, char* buf, size_t count, char marker)
{
	int c;
	size_t len = 0;
	do
	{
		c = TEMP_FAILURE_RETRY(read(fd,buf,count));
		//printf("\"%s\", count=%d\n", buf, c);
		if(c<0) return c;
		if(c==0) return len; //EOF
		len += c;
		count -= c;
		if(count > 0)
		{
			if(strchr(buf, marker) != NULL)
				return len;
			//if(strchr(buf, '\0') != NULL)
			//	return len;
		}
		buf += c;
	}
	while(count > 0);
	return len;
}

size_t bulk_write(int fd, char* buf, size_t count)
{
	int c;
	size_t len=0;
	//printf("TEST3\n");
	do{
		if(strlen(buf) < count)
			CERR("strlen less than count");
		//printf("TEST %d %u %u\n", fd, strlen(buf), count);
		c=TEMP_FAILURE_RETRY(write(fd,buf,count));
		//printf("%s\n", buf);
		if(c<0) return c;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	//printf("TEST4\n");
	return len;
}


void addFlags(int descriptor, int addedFlags)
{
	int flags = fcntl(descriptor, F_GETFL);
	flags |= addedFlags;
	fcntl(descriptor, F_SETFL, flags);
}


void makefifo(char* arg)
{
	if(mkfifo(arg, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
	{
		if(errno!=EEXIST)
		{
			perror("Create fifo");
			exit(EXIT_FAILURE);
		}
	}
}


int openfifo(char* arg, int flags)
{
	int fifo;
	if((fifo=TEMP_FAILURE_RETRY(open(arg,flags)))<0)
	{
		perror("Open fifo");
		exit(EXIT_FAILURE);
	}
	return fifo;
}


void closefifo(int fifo)
{
	if(TEMP_FAILURE_RETRY(close(fifo)) < 0)
	{
		perror("Close fifo");
		exit(EXIT_FAILURE);
	}
}


void unlinkfifo(char* arg)
{
	if(unlink(arg) < 0)
	{
		perror("Remove fifo");
		exit(EXIT_FAILURE);
	}
}


struct sockaddr_in make_address(char* address, uint16_t port)
{
	struct sockaddr_in addr;
	struct hostent* hostinfo;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	hostinfo = gethostbyname(address);
	if(hostinfo == NULL)
		HERR("gethostbyname");
	addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;

	return addr;
}

int makeSocket(int domain, int type)
{
	int sock;
	sock = socket(domain, type, 0);
	if(sock < 0)
		ERR("socket");
	return sock;
}

void closeSocket(int socket)
{
	if(TEMP_FAILURE_RETRY(close(socket)) < 0)
	{
		perror("Close socket");
		exit(EXIT_FAILURE);
	}
}
