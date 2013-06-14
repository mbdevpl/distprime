#ifndef PROCESSDATA_H
#define PROCESSDATA_H

#include "defines.h"

#define valueToPrime(data_type_value) *((int64_t*)data_type_value)

struct _processData
{
	int pipeRead;
	int pipeWrite;
	char* pipeReadBuf; // of size BUFSIZE_MAX
	size_t pipeReadBufCount;
	int64_t primeFrom;
	int64_t primeTo;
	int64_t primeRange;
	// enum from PROCSTATUS_...
	int status;
	time_t started;
	time_t finished;
	// stores int64_t*
	listPtr primes;
	// list of confirmed primes
	listPtr confirmed;
};
typedef struct _processData processData;
typedef processData* processDataPtr;

processDataPtr allocProcessData();

void freeProcessData(processDataPtr process);

void printProcessData(processDataPtr process);

xmlNodePtr xmlNodeCreateProcessData(processDataPtr process, bool includePrimes);

xmlNodePtr xmlNodeCreateProcessDataAltered(processDataPtr process,
		int64_t* fakePrimes, size_t fakePrimesCount);

processDataPtr xmlNodeParseProcessData(xmlNodePtr node);

processDataPtr matchProcess(processDataPtr match, listPtr list);

listPtr stringToPrimes(const char* buffer, int bufferLen);

size_t primesToString(listPtr primes, char* buffer, int bufferLen);

void freePrimesList(listPtr primes);

void listElemInsertEndPrime(listPtr primes, int64_t value);

time_t stringToTime(const char* buffer, const int bufferLen);

size_t timeToString(const time_t* time, char* buffer, const int bufferLen);

#endif // PROCESSDATA_H
