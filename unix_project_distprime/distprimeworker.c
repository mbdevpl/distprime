
#include "mbdev_unix.h"
#include "listfunctions.h"

#include <stdbool.h> // bool true false

void usage();
int main(int argc, char** argv);

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

	srand(getpid());

	int socket;
	struct sockaddr_in addr;
	in_port_t port = (in_port_t)atoi(argv[1]);

	//setSigHandler(SIG_IGN,SIGPIPE);
	socket = makeSocket(PF_INET, SOCK_DGRAM);

	int broadcastEnable=1;
	if(setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
		ERR("setsocketopt");

	memset(&addr, '\0', sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = port; //(in_port_t)(rand()%10001 + 10000);
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	printf("broadcasting from port %d...\n",addr.sin_port);

	char buffer[100];
	memset(buffer, 0, 100 * sizeof(char));
	sprintf(buffer, "hello\n");

	while(true)
	{
		int bytescount = 0;
		if((bytescount = sendto(socket, buffer, 100, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) < 0)
			ERR("sendto");

		printf("sent %d bytes\n", bytescount);

		milisleepFor(1000);
	}

	printf("will generate using %d processes\n", atoi(argv[2]));

	closeSocket(socket);

	return EXIT_SUCCESS;
}
