
#include "distprimecommon.h"

#define IDLE 1
#define GENERATING 2

void usage();
int main(int argc, char** argv);
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
	//struct list* lastPrime = NULL;

	int socketIn;
	struct sockaddr_in addrIn;
	in_port_t portIn = (in_port_t)8765;
	createSocketIn(&socketIn, &addrIn, INADDR_ANY, portIn, 5);

	printf("receiving on port %d\n", portIn);

	//if(listen(socket, 5) < 0)
	//	ERR("listen");

	//int descriptor = 0;
	//if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
	//	ERR("accept");

	int socketOut;
	struct sockaddr_in addrOut;
	in_port_t portOut = (in_port_t)rand()%10001 + 10000;
	createSocketOut(&socketOut, &addrOut, INADDR_BROADCAST, portOut);

	char bufferIn[100];
	struct sockaddr_in addrSender;
	//socklen_t sender_addr_size = sizeof(sender_addr);

	char bufferOut[100];

	while(true)
	{
		int bytescount = 0;
		socketInReceive(socketIn, bufferIn, 100, &addrSender, &bytescount);

		if(bytescount == 0)
		{
			printf("waiting for clients...\n");
			continue;
		}

		//memset(bufferIn, 0, 100 * sizeof(char));
		//if(TEMP_FAILURE_RETRY(recvfrom(socketIn, bufferIn, 100, 0, (struct sockaddr*)&sender_addr, &sender_addr_size) < 0))
		//	ERR("recvfrom");
		//printf("received packet from %s:%d\n", inet_ntoa(addrSender.sin_addr), ntohs(sender_addr.sin_port));
		//printf("Data: %s\n", bufferIn);

		if(strncmp(bufferIn, "distprimeworker ", 16) != 0)
			continue;

		char* buffer = bufferIn;
		buffer += 16;
		if(strncmp(buffer, "processes=", 10) == 0)
		{
			buffer += 10;

			char* ch = strchr(buffer, ' ');
			int len = ch-(buffer);
			char procs[len+1];
			strncpy(procs, buffer, len);
			procs[len] = 0;
			int processesCount = atoi(procs);

			if(strncmp(buffer+len+1, "port=", 5) == 0)
			{
				int portNew = atoi(buffer+len+1+5);

				printf(" added new client port=%d processesCount=%d\n", portNew, processesCount);

				milisleepFor(1000);

				printf("sending prmies range\n");

				addrOut.sin_port = htons(portNew);
				memset(bufferOut, 0, 100 * sizeof(char));
				sprintf(bufferOut, "distprime primeFrom=%lld primeTo=%lld", primeFrom, primeTo);

				socketOutSend(socketOut, bufferOut, 50, &addrOut, &bytescount);
//				if((bytescount = sendto(socketOut, bufferOut, 50, 0, (struct sockaddr *)&addrOut, sizeof(struct sockaddr_in))) < 0)
//					ERR("sendto");
//				printf("sent %d bytes to %d:%d\n", bytescount,
//					ntohl(addrOut.sin_addr.s_addr), ntohs(addrOut.sin_port));
			}
		}
		else if(strncmp(buffer, "status ", 7) == 0)
		{
			buffer += 7;

			int status = atoi(buffer);
			switch(status)
			{
			case IDLE:
				break;
			case GENERATING:
				break;
			}
		}
		else if(strncmp(buffer, "primes ", 10) == 0)
		{
			//
		}
	}

	closeSocket(socketOut);
	closeSocket(socketIn);

	savePrimes(&primes, outputFile, "primes");

	freeList(&primes);

	return EXIT_SUCCESS;
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
