#include "mbdev_unix.h"

void usage()
{
	fprintf(stderr, "USAGE: distprime prime_from prime_to output_file\n");
	exitNormal();
}

int isPrime(int64_t number)
{
	return 0;
}

int main(int argc, char** argv)
{
	if(argc != 4)
		usage();

	srand(getpid());

	int port_num = rand()%10001 + 10000;

	int64_t primeFrom = (int64_t)atoll(argv[1]);
	int64_t primeTo = (int64_t)atoll(argv[2]);
	char* outputFile = argv[3];

	int64_t i;
	for(i=primeFrom; i<=primeTo; ++i)
	{
		if(isPrime(i))
		{
		}
	}

	printf("server starting at %d",port_num);

	return EXIT_SUCCESS;
}
