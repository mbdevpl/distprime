#ifndef SERVERDATA_H
#define SERVERDATA_H

#include "defines.h"
#include "processdata.h"
#include "workerdata.h"

struct _serverData
{
	struct sockaddr_in address;
	unsigned long long hash;
	int64_t primeFrom;
	int64_t primeTo;
	// primeTo-primeFrom+1
	int64_t primeRange;
	// global count of confirmed primes
	int64_t primesCount;
	// together with worker's hash it is used to identify workers
	unsigned int workerIdSentLast;
	size_t workersActive;
	// stores workerDataPtr
	listPtr workersActiveData;
	size_t processesDone;
	// stores processDataPtr
	listPtr processesDoneData;
	char* outputPath;
};
typedef struct _serverData serverData;
typedef struct _serverData* serverDataPtr;

serverDataPtr allocServerData();

void freeServerData(serverDataPtr server);

void printServerData(serverDataPtr server);

xmlNodePtr xmlNodeCreateServerData(serverDataPtr server);

serverDataPtr xmlNodeParseServerData(xmlNodePtr node);

workerDataPtr matchActiveWorker(serverDataPtr server, workerDataPtr match);

#endif // SERVERDATA_H
