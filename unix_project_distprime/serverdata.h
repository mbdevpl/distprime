#ifndef SERVERDATA_H
#define SERVERDATA_H

#include "defines.h"
#include "processdata.h"
#include "workerdata.h"

struct _serverData
{
	struct sockaddr_in address;
	unsigned int hash;
	int64_t primeFrom;
	int64_t primeTo;
	// primeTo-primeFrom+1
	int64_t primeRange;
	// stores int64_t*
	listPtr primes;
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

#endif // SERVERDATA_H
