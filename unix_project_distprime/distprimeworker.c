
#include "distprimecommon.h"

void usage();
int main(int argc, char** argv);
bool isPrime(int64_t number);
void generatePrimes(struct list** primes, int64_t primeFrom, int64_t primeTo);

void usage()
{
	fprintf(stderr, "USAGE: distprimeworker port subprocesses_count\n");
	exitNormal();
}

int main(int argc, char** argv)
{
	if(argc != 3)
		usage();

	printf("distprimeworker\n");
	printf("Distributed prime numbers generation worker\n\n");

	int port = (in_port_t)atoi(argv[1]);
	int processes = atoi(argv[2]);
	printf("will generate using %d processes\n", processes);

	srand(getpid());

	int socketOut;
	struct sockaddr_in addrOut;
	//uint32_t addrOut = INADDR_BROADCAST;
	in_port_t portOut = port;
	createSocketOut(&socketOut, &addrOut, INADDR_BROADCAST, portOut);

	printf("broadcasting to port %d...\n", portOut);

	int socketIn;
	struct sockaddr_in addrIn;
	in_port_t portIn = (in_port_t)(rand()%10001 + 5000);
	createSocketIn(&socketIn, &addrIn, INADDR_ANY, portIn, 5);

	char bufferIn[100];
	struct sockaddr_in addrSender;

	char bufferOut[100];

	while(true)
	{
		memset(bufferOut, 0, 100 * sizeof(char));
		sprintf(bufferOut, "distprimeworker processes=%d port=%d\n", processes, portIn);
		int bytescount = 0;

		socketOutSend(socketOut, bufferOut, 60, &addrOut, &bytescount);

//		if((bytescount = sendto(socketOut, bufferOut, 60, 0, (struct sockaddr *)&addrOut, sizeof(struct sockaddr_in))) < 0)
//			ERR("sendto");
//		printf("sent %d bytes to %d:%d\n", bytescount,
//			ntohl(addrOut.sin_addr.s_addr), ntohs(addrOut.sin_port));

		socketInReceive(socketIn, bufferIn, 100, &addrSender, &bytescount);

		if(bytescount == 0)
		{
			printf("no distprime server available...\n");
			exitNormal();
		}

//		memset(bufferIn, 0, 100 * sizeof(char));
//		if(TEMP_FAILURE_RETRY(recvfrom(socketIn, bufferIn, 100, 0, (struct sockaddr*)&sender_addr, &sender_addr_size) < 0))
//			ERR("recvfrom");
		printf("received %s\n", bufferIn);

		struct list* primes = NULL;
		generatePrimes(&primes, 2000000, 3000000);
		printLnLen(&primes);

		// receive

//		int pid = fork();
//		if(pid < 0)
//			ERR("fork");

//		switch(pid)
//		{
//		case 0:
//			exitNormal();
//			break;
//		default:
//			break;
//		}
	}


	closeSocket(socketOut);
	closeSocket(socketIn);

	return EXIT_SUCCESS;
}

bool isPrime(int64_t number)
{
	if (number <= 1)
		return false;
	double a = sqrt((double)number);
	int64_t number_sqrt = (int64_t)ceil(a);
	int64_t i;
	for (i = 2; i <= number_sqrt; ++i)
	{
		if (number % i == 0)
			return false;
	}
	return true;
}

void generatePrimes(struct list** primes, int64_t primeFrom, int64_t primeTo)
{
	struct list* lastPrime;
	int n;
	int64_t i;
	int64_t range = primeTo - primeFrom + 1;
	for(i=primeFrom; i<=primeTo; ++i)
	{
		if(isPrime(i))
		{
			if(*primes == NULL)
			{
				*primes = createElem((int)i);
				lastPrime = *primes;
			}
			else
			{
				insertAfter(&lastPrime, 1, (int)i);
				lastPrime = lastPrime->next;
			}
		}

		if(n == 1000000)
		{
			n = 0;
			printf("%lldM/%lldM\n", (i-primeFrom) / 1000000, range / 1000000);
		}
		++n;
	}
}
