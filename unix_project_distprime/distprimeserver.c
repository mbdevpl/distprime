
#include "listfunctions.h"
#include "mbdev_unix.h"

//#include <stdint.h> /uint64_t
#include <stdbool.h> // bool true false
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void usage();
int main(int argc, char** argv);
bool isPrime(int64_t number);
void savePrimes(struct list**head, char* filename, const char* intro);

void usage()
{
	fprintf(stderr, "USAGE: distprime prime_from prime_to output_file\n");
	exitNormal();
}

int main(int argc, char** argv)
{
	if(argc != 4)
		usage();

	printf("distprime\n");
	printf("Distributed prime numbers generation server\n\n");

	srand(getpid());

	int64_t primeFrom = (int64_t)atoll(argv[1]);
	int64_t primeTo = (int64_t)atoll(argv[2]);
	char* outputFile = argv[3];

	struct list* primes = NULL;
	struct list* lastPrime = NULL;

	int socket = 0;
	int port = rand()%8001 + 2000;
	struct sockaddr_in addr;
	struct sockaddr_in sender_addr;

	printf("starting at port %d\n", port);

	socket = makeSocket(PF_INET,SOCK_DGRAM);

	int reuseAddr = 1;
	if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0)
		ERR("setsockopt");

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	while(port < 10000)
	{
		addr.sin_port = htons(port);
		if(bind(socket, (struct sockaddr*)&addr, sizeof(addr)) == 0)
			break;
		printf("error");
		++port;
	}
	if(socket == 0)
	{
		addr.sin_port = htons(port);
		if(bind(socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
			ERR("bind");
	}

	printf("receiving on port %d\n",port);

	//if(listen(socket, 5) < 0)
	//	ERR("listen");

	//int descriptor = 0;
	//if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
	//	ERR("accept");

	char buffer[100];
	memset(buffer, 0, 100 * sizeof(char));
	socklen_t sender_addr_size = sizeof(sender_addr);
	if(recvfrom(socket, buffer, 100, 0, (struct sockaddr*)&sender_addr, &sender_addr_size) < 0)
		ERR("recvfrom");

	printf("received packet from %s:%d\nData: %s\n\n",
		inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);

	closeSocket(socket);

	int n;
	int64_t i;
	int64_t range = primeTo - primeFrom + 1;
	for(i=primeFrom; i<=primeTo; ++i)
	{
		if(isPrime(i))
		{
			if(primes == NULL)
			{
				primes = createElem((int)i);
				lastPrime = primes;
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
	//printf("\n");
	printLnLen(&primes);

	savePrimes(&primes, outputFile, "primes");

	freeList(&primes);

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

void savePrimes(struct list**head, char* filename, const char* intro)
{
	FILE* file = fopen(filename, "a");
	struct list*temp;
	fprintf(file, intro);
	for(temp=*head;temp;)
	{
		fprintf(file, " %lld",temp->val);
		//if(
		(temp=temp->next);
		//&&(temp!=*head))
		//	printf(", ");
		if(temp==*head)return;
	}
	fprintf(file, "\n");
	fclose(file);
}
