#ifndef PRIMERANGE_HPP
#define PRIMERANGE_HPP

#include "processdata.h"
#include "workerdata.h"
#include "serverdata.h"

// the reference range is from 1 to FIRSTLIMIT, and the value
// of the integral of the reference range is used to estimate further ranges
// therefore increasing this value will in general increase allocated ranges
#define FIRSTLIMIT ((int64_t)20000000LL)

// argument reserveMore is currently not used, but can be used
// to decrease every allocated primes range by a constant amount

// 'excludedRanges' provides additional exclusions, independent
// of the current state of server - this variable is used to allocate
// multiple ranges of primes before actually making any changes in the server

int64_t findRangeEndStartedByInList(const int64_t value,
		listPtr processes);
int64_t findRangeEndStartedBy(const int64_t value,
		processDataPtr* processes, size_t processesCount);
int64_t findRangeStartPrecededByInList(const int64_t value,
		listPtr processes);
int64_t findRangeStartPrecededBy(const int64_t value,
		processDataPtr* processes, size_t processesCount);
int64_t findRangeStart(serverDataPtr server, listPtr excludedRanges);
int64_t findRangeEndLimited(/*serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, */const int64_t rangeStart, const int64_t rangeLimit);
int64_t findRangeEnd(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, const int64_t rangeStart);
bool setPrimeRange(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, processDataPtr process);

void testPrimeRanges();

#endif // PRIMERANGE_HPP
