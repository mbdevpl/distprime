#ifndef PROCESSDATA_H
#define PROCESSDATA_H

#include "defines.h"

#define valueToPrime(data_type_value) *((int64_t*)data_type_value)

struct _processData
{
	int pipeRead;
	int pipeWrite;
	int64_t primeFrom;
	int64_t primeTo;
	int64_t primeRange;
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

processDataPtr xmlNodeParseProcessData(xmlNodePtr node);

listPtr stringToPrimes(const char* buffer, int bufferLen);

size_t primesToString(listPtr primes, char* buffer, int bufferLen);

void listElemInsertEndPrime(listPtr primes, int64_t value);

time_t stringToTime(const char* buffer, const int bufferLen);

size_t timeToString(const time_t* time, char* buffer, const int bufferLen);

#endif // PROCESSDATA_H
