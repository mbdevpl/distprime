#include "primerange.h"

// in a set of processes, finds the end of range started by given value
int64_t findRangeEndStartedByInList(const int64_t value,
		listPtr processes)
{
#ifdef DEBUG_RANGES
	printf("findRangeEndStartedByInList(value, processes)\n");
	printf("  value=%lld\n", value);
	printf("  |processes|=%u\n", listLength(processes));
#endif
	listElemPtr elem;
	for(elem = listElemGetFirst(processes); elem; elem = elem->next)
	{
		processDataPtr process = (processDataPtr)elem->val;
		if(process == NULL)
			continue;
		if(process->primeFrom == value)
			return process->primeTo;
	}
	return 0LL;
}

int64_t findRangeEndStartedBy(const int64_t value,
		processDataPtr* processes, size_t processesCount)
{
#ifdef DEBUG_RANGES
	printf("findRangeEndStartedBy(value, processes, processesCount)\n");
	printf("  value=%lld\n", value);
	printf("  processes=");
	size_t ii;
	for(ii = 0; ii < processesCount; ++ii)
		printProcessData(processes[ii]);
	printf("  processesCount=%u\n", processesCount);
#endif
	size_t i;
	for(i = 0; i < processesCount; ++i)
	{
		processDataPtr process = processes[i];
		if(process == NULL)
			continue;
		if(process->primeFrom == value)
			return process->primeTo;
	}
	return 0LL;
}

// in a set of processes, finds the minimum number that starts the primes range
// but is greater than given value
int64_t findRangeStartPrecededByInList(const int64_t value,
		listPtr processes)
{
#ifdef DEBUG_RANGES
	printf("findRangeStartPrecededByInList(value, processes)\n");
	printf("  value=%lld\n", value);
	printf("  |processes|=%u\n", listLength(processes));
#endif
	int64_t minimum = 0;
	listElemPtr elem;
	for(elem = listElemGetFirst(processes); elem; elem = elem->next)
	{
		processDataPtr process = (processDataPtr)elem->val;
		if(process == NULL)
			continue;
		if(process->primeFrom > value && (minimum == 0 || process->primeFrom < minimum))
			minimum = process->primeFrom;
	}
#ifdef DEBUG_RANGES
	printf("findRangeStartPrecededByInList(value, processes) returns:\n");
	printf("  minimum=%lld\n", minimum);
#endif
	return minimum;
}

int64_t findRangeStartPrecededBy(const int64_t value,
		processDataPtr* processes, size_t processesCount)
{
#ifdef DEBUG_RANGES
	printf("findRangeStartPrecededBy(value, processes, processesCount)\n");
	printf("  value=%lld\n", value);
	printf("  processes=");
	size_t ii;
	for(ii = 0; ii < processesCount; ++ii)
		printProcessData(processes[ii]);
	printf("  processesCount=%u\n", processesCount);
#endif
	int64_t minimum = 0LL;
	size_t i;
	for(i = 0; i < processesCount; ++i)
	{
		processDataPtr process = processes[i];
		if(process == NULL)
			continue;
		if(process->primeFrom > value && (minimum == 0 || process->primeFrom < minimum))
			minimum = process->primeFrom;

	}
#ifdef DEBUG_RANGES
	printf("findRangeStartPrecededBy(value, processes, processesCount) returns:\n");
	printf("  minimum=%lld\n", minimum);
#endif
	return minimum;
}

// finds the minimum value not researched for primality
// TODO: TOO LONG
int64_t findRangeStart(serverDataPtr server, listPtr excludedRanges)
{
#ifdef DEBUG_RANGES
	printf("findRangeStart(server)\n");
	printf("  server=");
	printServerData(server);
#endif
	int64_t current = server->primeFrom;
	int64_t next = 0;
	if(server->workersActive == 0 && server->processesDone == 0)
	{
		do
		{
			next = findRangeEndStartedByInList(current, excludedRanges);
			if(next > 0)
			{
				if(next == INT64_MAX)
					return 0;
				current = next + 1;
			}
		}
		while(next > 0);
		return current;
	}
	else if(server->workersActive > 0 || server->processesDone > 0)
	{
		// some primes scanned or some scanning currently going on

		do
		{
#ifdef DEBUG_RANGES
			printf("findRangeStart(server) status:\n");
			//printf("  next=%lld", next);
			printf("  current=%lld\n", current);
			//printServerData(server);
#endif
			next = findRangeEndStartedByInList(current, server->processesDoneData);
			if(next > 0)
			{
				if(next == INT64_MAX)
					return 0;
				current = next + 1;
				continue;
			}

			listElemPtr elem;
			for(elem = listElemGetFirst(server->workersActiveData); elem; elem = elem->next)
			{
				workerDataPtr worker = (workerDataPtr)elem->val;
				next = findRangeEndStartedBy(current, worker->processesData, worker->processes);
				if(next > 0)
					break;
			}
			if(next > 0)
			{
				if(next == INT64_MAX)
					return 0;
				current = next + 1;
				continue;
			}

			next = findRangeEndStartedByInList(current, excludedRanges);
			if(next > 0)
			{
				if(next == INT64_MAX)
					return 0;
				current = next + 1;
				continue;
			}
		}
		while(next > 0);

		return current;
	}

	fprintf(stderr, "ERROR: didn't get the proper prime range start\n");
	return 0LL;
}

// TODO: TOO LONG
int64_t findRangeEndLimited(/*serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, */const int64_t rangeStart, const int64_t rangeLimit)
{
#ifdef DEBUG_RANGES
	printf("findRangeEndLimited(rangeStart, rangeLimit)\n");
	printf("  rangeStart=%lld\n", rangeStart);
	printf("  rangeLimit=%lld\n", rangeLimit);
#endif
	if(rangeStart < 1 || rangeStart > rangeLimit)
	{
		fprintf(stderr, "ERROR: prime range start %lld is invalid, so range end cannot be found\n", rangeStart);
		return 0LL;
	}
	//if(rangeStart == 1)
	//{
	//	if(FIRSTLIMIT > rangeLimit)
	//		return rangeLimit;
	//	return FIRSTLIMIT;
	//}

	// time needed to generate primes from range [a,b] is equal
	// integral(sqrt(b))-integral(sqrt(a))
	// integral(sqrt(x)) = (2/3)*(n^(3/2))
	// inverse: ((3/2)^(2/3))*(n^(2/3))
	const double limit = (0.6666667) * pow((double)FIRSTLIMIT, 1.5)
		- (0.6666667) * pow(1.0, 1.5);
	// for N=10mln
	//const double fixed = 21081851067.7891955466-0.666666666666;
	//printf("limit=%f\n", limit);

	// if number is extremely large, range consists of only one number
	// when limit is calculated for 10mln, it happens
	// for 444444416228682746, which can still be stored in 64bit signed var
	const double sqrtOfStart = sqrt((double)rangeStart);
	if(sqrtOfStart >= limit)
	{
#ifdef DEBUG_RANGES
		printf("findRangeEndLimited(rangeStart, rangeLimit) returns:\n");
		printf("  rangeStart=%lld (sqrt(rangeStart) > limit)\n", rangeStart);
#endif
		return rangeStart;
	}

	double a = 1.0 / (2.0*sqrtOfStart);
	double b = sqrtOfStart;
	double c = -limit;
	double delta = (b*b)-(4*a*c);

	// x_min
	//double x = -(sqrtOfStart*sqrtOfStart);
	//double x_min = x;

	//double fx_min = x*x*a + x+b + c;

#ifdef DEBUG_RANGES
	printf("findRangeEndLimited(rangeStart, rangeLimit) status:\n");
	printf("  sqrtOfStart=%f\n", sqrtOfStart);
	printf("  a=%f", a);
	printf(" b=%f", b);
	printf(" c=%f", c);
	printf(" delta=%f\n", delta);
#endif

	//double rangeEnd = (-b+sqrt(delta))/(2*a);
	//const double test = (0.6666667) * pow(rangeEnd + rangeStart, 1.5)
	//	- (0.6666667) * pow(rangeStart, 1.5);
	//printf("test=%f\n", test);

	if(delta < 0)
	{
		fprintf(stderr, "ERROR: prime range end formula is wrong\n");
		return 0LL;
	}

	int64_t rangeEnd;

	if(delta > 0)
		rangeEnd = (int64_t)llround((sqrt(delta)-b)/(2*a)) /*+ rangeStart*/;
	else if(delta == 0)
		rangeEnd = (int64_t)llround((-b)/(2*a)) /*+ rangeStart*/;

#ifdef DEBUG_RANGES
	printf("findRangeEndLimited(rangeStart, rangeLimit) status:\n");
	printf("  rangeEnd=%lld\n", rangeEnd);
#endif

	//if(reserveMore > 0)
	//	rangeEnd -= reserveMore;

	if(rangeEnd < 0)
		rangeEnd = 0;

if(rangeStart < FIRSTLIMIT)
{
	double ratio = (FIRSTLIMIT-rangeStart)+FIRSTLIMIT/2;
	ratio /= (FIRSTLIMIT+FIRSTLIMIT);
	ratio *= rangeEnd;
	rangeEnd = (int64_t)llround(ratio);
}

	if(rangeEnd > rangeLimit - rangeStart)
		rangeEnd = rangeLimit;
	else
		rangeEnd += rangeStart;

	//if(rangeEnd > rangeLimit)
	//	rangeEnd = rangeLimit;

#ifdef DEBUG_RANGES
		printf("findRangeEndLimited(rangeStart, rangeLimit) returns:\n");
		printf("  rangeEnd=%lld\n", rangeEnd);
#endif
	return rangeEnd;
}

// TODO: TOO LONG
int64_t findRangeEnd(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, const int64_t rangeStart)
{
#ifdef DEBUG_RANGES
	printf("findRangeEnd(server, rangeStart)\n");
	printf("  server=");
	printServerData(server);
	printf("  rangeStart=%lld\n", rangeStart);
#endif
	if(server->workersActive == 0 && server->processesDone == 0)
		return findRangeEndLimited(/*server, excludedRanges,
			reserveMore, */rangeStart, server->primeTo);
	else if(server->workersActive > 0 || server->processesDone > 0)
	{
		// some primes scanned or some scanning currently going on
		int64_t minimum = server->primeTo + 1;
		int64_t next;

		next = findRangeStartPrecededByInList(rangeStart, server->processesDoneData);
		if(next != 0 && next < minimum)
			minimum = next;

		listElemPtr elem;
		for(elem = listElemGetFirst(server->workersActiveData); elem; elem = elem->next)
		{
			workerDataPtr worker = (workerDataPtr)elem->val;
			next = findRangeStartPrecededBy(rangeStart, worker->processesData, worker->processes);
			if(next != 0 && next < minimum)
				minimum = next;
		}

		next = findRangeStartPrecededByInList(rangeStart, excludedRanges);
		if(next != 0 && next < minimum)
			minimum = next;

		minimum = findRangeEndLimited(/*server, excludedRanges,
			reserveMore, */rangeStart, minimum - 1);
#ifdef DEBUG_RANGES
		printf("findRangeEnd(server, rangeStart) returns:\n");
		printf("  minimum=%lld\n", minimum);
#endif
		return minimum;
	}
	fprintf(stderr, "ERROR: didn't get the proper prime range end\n");
	return 0LL;
}

bool setPrimeRange(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, processDataPtr process)
{
#ifdef DEBUG_RANGES
	printf("setPrimeRange(server, process)\n");
	printf("  server=");
	printServerData(server);
	printf("  process=");
	printProcessData(process);
#endif
	process->primeFrom = findRangeStart(server, excludedRanges);
	if(process->primeFrom == 0 || process->primeFrom > server->primeTo)
		return false;
	process->primeTo = findRangeEnd(server, excludedRanges, reserveMore, process->primeFrom);
	if(process->primeTo == 0 || process->primeFrom > process->primeTo)
		return false;
	process->primeRange = process->primeTo - process->primeFrom + 1;
#ifdef DEBUG_RANGES
	printf("setPrimeRange(server, process) returns:\n");
	printf("  process=");
	printProcessData(process);
#endif
	return true;
}

void testPrimeRanges()
{
	int64_t i, j, k;

	for(i = 1; i <= INT64_MAX; i *= 10)
	{
		j = findRangeEndLimited(i, INT64_MAX);
		k = j - i + 1;
		printf("[%19lld,%19lld] %8lld\n", i, j, k);
		if(i >= 900000000000000000LL)
			break;
	}
	//for(i = 2; i <= INT64_MAX;  i *= 10)
	//{
	//	j = findRangeEndLimited(i, INT64_MAX);
	//	k = j - i + 1;
	//	printf("[%19lld,%19lld] %8lld\n", i, j, k);
	//	if(i >= 900000000000000000LL)
	//		break;
	//}
	for(i = 5; i <= INT64_MAX;  i *= 10)
	{
		j = findRangeEndLimited(i, INT64_MAX);
		k = j - i + 1;
		printf("[%19lld,%19lld] %8lld\n", i, j, k);
		if(i >= 900000000000000000LL)
			break;
	}
	for(i = INT64_MAX - 99LL; i <= INT64_MAX - 97LL;)
	{
		j = findRangeEndLimited(i, INT64_MAX);
		k = j - i + 1;
		printf("[%19lld,%19lld] %8lld\n", i, j, k);
		if(j == INT64_MAX)
			break;
		i = j + 1;
	}
}
