#ifndef WORKERDATA_H
#define WORKERDATA_H

#include "defines.h"
#include "processdata.h"

struct _workerData
{
	struct sockaddr_in address;
	unsigned int hash;
	unsigned int id;
	size_t processes;
	processDataPtr* processesData;
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

#endif // WORKERDATA_H
