#ifndef WORKERDATA_H
#define WORKERDATA_H

#include "defines.h"
#include "processdata.h"

struct _workerData
{
	struct sockaddr_in address;
	unsigned long long hash;
	unsigned int id;
	size_t processes;
	processDataPtr* processesData;
	// enum from STATUS_...
	int status;
	time_t statusSince;
	time_t now;
};
typedef struct _workerData workerData;
typedef struct _workerData* workerDataPtr;

workerDataPtr allocWorkerData();

void freeWorkerData(workerDataPtr worker);

void printWorkerData(workerDataPtr worker);

xmlNodePtr xmlNodeCreateWorkerData(workerDataPtr worker);

workerDataPtr xmlNodeParseWorkerData(xmlNodePtr node);

unsigned long long getHash();

#endif // WORKERDATA_H
