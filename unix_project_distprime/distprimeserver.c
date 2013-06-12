
#include "distprimecommon.h"
#include "serverdata.h"

#define FIRSTLIMIT ((int64_t)4000000LL)

#define SERVER_PORT ((in_port_t)8765)

void usage();
int main(int argc, char** argv);
void serverLoop(serverDataPtr myData);
void socketSendPrimeRanges(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr ranges);
int64_t findRangeEndStartedByInList(const int64_t value,
		listPtr processes);
int64_t findRangeEndStartedBy(const int64_t value,
		processDataPtr* processes, size_t processesCount);
int64_t findRangeStartPrecededByInList(const int64_t value,
		listPtr processes);
int64_t findRangeStartPrecededBy(const int64_t value,
		processDataPtr* processes, size_t processesCount);
int64_t findRangeStart(serverDataPtr server, listPtr excludedRanges);
int64_t findRangeEndLimited(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, const int64_t rangeStart, const int64_t rangeLimit);
int64_t findRangeEnd(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, const int64_t rangeStart);
bool setPrimeRange(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, processDataPtr process);
void savePrimes(listPtr list, const char* filename);

void usage()
{
	fprintf(stderr, "USAGE: distprime prime_from prime_to output_file\n");
	fprintf(stderr, "  prime_from must be a positive number\n");
	fprintf(stderr, "  prime_to must not be smaller than prime_from\n");
	fprintf(stderr, "  output_file must be a location with write permissions\n");
	fprintf(stderr, "EXAMPLE:\n");
	fprintf(stderr, "  distprime 2 %lld log.txt\n", INT64_MAX);
	exitNormal();
}

void socketSendConfirmation(const int socket, serverDataPtr server,
		workerDataPtr worker, processDataPtr process)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr serverNode = xmlNodeCreateServerData(server);
	//xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlNodePtr processNode = xmlNodeCreateProcessData(process, true);
	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, serverNode);
	//xmlAddChild(msgNode, workerNode);
	xmlAddChild(msgNode, processNode);
	size_t bytesSent = socketSend(socket, &worker->address, doc);
	xmlFreeDoc(doc);
	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no confirmation sent\n");
}

listPtr createNewPrimeRanges(serverDataPtr server, workerDataPtr worker)
{
	listPtr ranges = listCreate();
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr range = allocProcessData();
		if(!setPrimeRange(server, ranges, worker->processes-1-i, range))
			break;
		listElemInsertEnd(ranges, (data_type)range);
	}
	return ranges;
}

void movePrimeRangesToWorker(listPtr ranges, workerDataPtr worker)
{
	listElemPtr elem = listElemGetFirst(ranges);
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		if(!elem)
		{
			worker->processesData[i] = NULL;
			continue;
		}
		worker->processesData[i] = allocProcessData();
		processDataPtr newProcess = worker->processesData[i];
		processDataPtr range = (processDataPtr)elem->val;
		newProcess->primeFrom = range->primeFrom;
		newProcess->primeTo = range->primeTo;
		newProcess->primeRange = range->primeRange;
		elem = elem->next;
	}
}

double calcProgress(serverDataPtr server)
{
	int64_t doneRange = 0;
	listElemPtr elem;
	for(elem = listElemGetFirst(server->processesDoneData); elem; elem = elem->next)
	{
		processDataPtr process = (processDataPtr)elem->val;
		doneRange += process->primeRange;
	}
	return doneRange * 100.0 / server->primeRange;
}

// returns true if all primes were scanned and results are available on given server
bool checkIfDone(serverDataPtr server)
{
	if(server->processesDone == 0)
		return false;

	int64_t current = server->primeFrom;
	int64_t next = 0;
	do
	{
		next = findRangeEndStartedByInList(current, server->processesDoneData);
		if(next > 0)
		{
			current = next + 1;
			continue;
		}
	}
	while(next > 0);
	if(current > server->primeTo)
		return true;
	return false;
}

/*//void socketOutSendStatusRequest(const int socketOut, workerDataPtr worker)
//{
//	xmlDocPtr doc = xmlDocCreate();
//	xmlNodePtr root = commCreateMsgNode();
//	xmlNodePtr request = commCreateStatusRequestNode();
//	xmlDocSetRootElement(doc, root);
//	xmlAddChild(root, request);

//	char bufferOut[BUFSIZE_MAX];
//	int bytesOut = 0;
//	commXmlToString(doc, bufferOut, &bytesOut, BUFSIZE_MAX);

//	int bytesSent = 0;
//	while(bytesOut != bytesSent)
//		socketOutSend(socketOut, bufferOut, bytesOut, &worker->address, &bytesSent);

//	xmlFreeDoc(doc);

//	if(bytesSent == 0)
//		fprintf(stderr, "ERROR: sending primesrange");
//}*/

int main(int argc, char** argv)
{
	printf("distprime\n");
	printf("Distributed prime numbers generation server\n\n");

	if(argc != 4)
		usage();
	const int64_t primeFrom = (int64_t)atoll(argv[1]);
	const int64_t primeTo = (int64_t)atoll(argv[2]);
	char* outputFile = argv[3];
	if(primeFrom < 1 || primeTo < 1 || primeFrom > primeTo || outputFile == NULL)
		usage();

	srand(getGoodSeed());

	//	{
	//		int64_t i;
	//		for(i = 1; i < 10; i += 1)
	//			printf("[%lld,%lld]\n", i, getPrimeRangeEnd(i));
	//		for(i = 1; i < 200000000; i += 10000000)
	//			printf("[%lld,%lld]\n", i, getPrimeRangeEnd(i));
	//	}

	serverDataPtr server = allocServerData();

	server->primeFrom = primeFrom;
	server->primeTo = primeTo;
	server->primeRange = primeTo - primeFrom + 1;
	server->outputPath = outputFile;
	server->hash = rand()%9000000+1000000;

	serverLoop(server);

	freeServerData(server);

	return EXIT_SUCCESS;
}

void serverLoop(serverDataPtr myData)
{
	unsigned int workerIdSentLast = 0;
	int socket;
	struct sockaddr_in addrSender;

	addressCreate(&myData->address, INADDR_ANY, SERVER_PORT);
	socketCreate(&socket, 1, true, true);
	socketBind(socket, &myData->address);
	printf("SRV: receiving on port %d\n", SERVER_PORT);

	//if(listen(socket, 5) < 0)
	//	ERR("listen");
	//int descriptor = 0;
	//if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
	//	ERR("accept");

	while(true)
	{
		// receive xml from the socket
		xmlDocPtr doc;
		if(socketReceive(socket, &addrSender, &doc) == 0)
			continue;

		// parse xml into distprime objects
		serverDataPtr server;
		workerDataPtr worker;
		listPtr processesList;
		if(processXml(doc, &server, &worker, &processesList) < 0)
			continue;

		bool msgHandled = false;
		if(server == NULL)
		{
			if(worker != NULL)
			{
				if(worker->id == 0)
				{
					if(worker->processes > 0)
					{
						if(listLength(processesList) == 0)
						{
							listPtr ranges = createNewPrimeRanges(myData, worker);
							if(listLength(ranges) == worker->processes)
							{
								// received broadcast
								workerDataPtr newWorker = allocWorkerData();

								newWorker->address = addrSender;
								newWorker->hash = worker->hash;
								++workerIdSentLast;
								newWorker->id = workerIdSentLast;
								newWorker->processes = worker->processes;
								newWorker->processesData = (processDataPtr*)malloc(newWorker->processes * sizeof(processDataPtr));
								newWorker->status = STATUS_GENERATING;
								time(&newWorker->now);
								newWorker->statusSince = newWorker->now;
								movePrimeRangesToWorker(ranges, newWorker);
								myData->workersActive += 1;
								listElemInsertEnd(myData->workersActiveData, (data_type)newWorker);

								socketSendPrimeRanges(socket, myData, newWorker, ranges);
								msgHandled = true;
							}
							else if(listLength(ranges) == 0)
							{
								// received broadcast, but all prime ranges are allocated
								msgHandled = true;
							}
							else
								fprintf(stderr, "ERROR: wrong number of prime ranges to send\n");
							listFree(ranges);
						}
						else
							fprintf(stderr, "ERROR: unregistered worker sent too much information\n");
					}
					else
						fprintf(stderr, "ERROR: worker does not want to work\n");
				}
				else
				{
					if(listLength(processesList) > 0)
					{
						// received a list of primes
						bool recognizedWorker = false;
						bool sentConfirmation = false;
						listElemPtr elem;
						for(elem = listElemGetFirst(myData->workersActiveData); elem; elem = elem->next)
						{
							workerDataPtr active = (workerDataPtr)elem->val;
							if(active->hash == worker->hash)
							{
								listElemPtr e;
								for(e = listElemGetFirst(processesList); e; e = e->next)
								{
									processDataPtr proc = (processDataPtr)e->val;
									if(sentConfirmation)
									{
										freeProcessData(proc);
										continue;
									}
									size_t i;
									for(i = 0; i < active->processes; ++i)
									{
										processDataPtr activeProcess = active->processesData[i];
										if(activeProcess == NULL)
											continue;
										if(activeProcess->primeFrom == proc->primeFrom)
										{
											activeProcess->status = proc->status;
											if(activeProcess->primes == NULL)
												activeProcess->primes = listCreate();
											listMerge(activeProcess->primes, proc->primes);
											proc->primes = NULL;
											//printf("TESTU\n");
											//listElemPtr e2;
											//for(e2 = listElemGetFirst(proc->primes); e2; e2 = e2->next)
											//	listElemInsertEnd(myData->primes, e2->val);
											socketSendConfirmation(socket, myData, active, activeProcess);

											if(activeProcess->confirmed == NULL)
												activeProcess->confirmed = listCreate();
											listMerge(activeProcess->confirmed, activeProcess->primes);
											activeProcess->primes = listCreate();

											sentConfirmation = true;
											if(activeProcess->status == PROCSTATUS_UNCONFIRMED
													&& listLength(activeProcess->primes) == 0)
											{
												activeProcess->status = PROCSTATUS_CONFIRMED;
												active->processesData[i] = NULL;
												time(&activeProcess->finished);
												myData->processesDone += 1;
												listElemInsertEnd(myData->processesDoneData, (data_type)activeProcess);
											}
											break;
										}
									}
									freeProcessData(proc);
								}
								recognizedWorker = true;
								break;
							}
						}
						if(recognizedWorker && sentConfirmation)
						{
							//savePrimes(myData->primes, myData->outputPath);
							//listClear(myData->primes);
							msgHandled = true;
						}
						else
						{
							if(!recognizedWorker)
								fprintf(stderr, "ERROR: did not recognize worker\n");
							if(!sentConfirmation)
								fprintf(stderr, "ERROR: did not send any confirmation\n");
						}
					}
					else
					{
						bool foundWorker = false;
						listElemPtr e1;
						for(e1 = listElemGetFirst(myData->workersActiveData); e1; e1 = e1->next)
						{
							workerDataPtr w1 = (workerDataPtr)e1->val;
							if(w1->hash == worker->hash && w1->id == worker->id)
							{
								foundWorker = true;
								bool workerReady = true;
								size_t i;
								for(i = 0; i < w1->processes; ++i)
								{
									if(w1->processesData[i] != NULL)
									{
										workerReady = false;
										break;
									}
								}
								if(workerReady)
								{
									// received subsequent request
									listPtr ranges = createNewPrimeRanges(myData, w1);
									if(listLength(ranges) == w1->processes)
									{
										// received broadcast
										//workerDataPtr newWorker = allocWorkerData();
										//newWorker->address = addrSender;
										//newWorker->hash = worker->hash;
										//++workerIdSentLast;
										//newWorker->id = workerIdSentLast;
										//newWorker->processes = worker->processes;
										//newWorker->processesData = (processDataPtr*)malloc(newWorker->processes * sizeof(processDataPtr));
										w1->status = STATUS_GENERATING;
										time(&w1->now);
										w1->statusSince = w1->now;
										movePrimeRangesToWorker(ranges, w1);
										//myData->workersActive += 1;
										//listElemInsertEnd(myData->workersActiveData, (data_type)newWorker);

										socketSendPrimeRanges(socket, myData, w1, ranges);
										msgHandled = true;
									}
									else if(listLength(ranges) == 0)
									{
										// received broadcast, but all prime ranges are allocated
										msgHandled = true;
									}
									else
										fprintf(stderr, "ERROR: wrong number of prime ranges to send\n");
									listFree(ranges);
								}
								else
									fprintf(stderr, "ERROR: worker not ready\n");
							}
							if(foundWorker)
								break;
						}
						if(!foundWorker)
							fprintf(stderr, "ERROR: registered worker not found\n");
						//if(!workerReady)
						//	if(!foundWorker)
					}
				}
				freeWorkerData(worker);
				worker = NULL;
			}
			else
				fprintf(stderr, "ERROR: received message without worker data\n");
		}
		else
		{
			fprintf(stderr, "INFO: discarded message with server data\n");
			freeServerData(server);
			server = NULL;
		}
		if(worker != NULL)
		{
			freeWorkerData(worker);
			worker = NULL;
		}

		if(doc != NULL)
			xmlFreeDoc(doc);

		if(!msgHandled)
			fprintf(stderr, "ERROR: message was not handled\n");

		printf("progress: %3.2f%% active workers: %3u primes discovered: %8u\n",
			calcProgress(myData), myData->workersActive, listLength(myData->primes));

		if(checkIfDone(myData))
		{
			printf("all primes were scanned\n");
			break;
		}
	}

	closeSocket(socket);
	xmlCleanupParser();
}

void socketSendPrimeRanges(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr ranges)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr serverNode = xmlNodeCreateServerData(server);
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, serverNode);
	xmlAddChild(msgNode, workerNode);
	//size_t i, count = listLength(ranges);
	//for(i = 0; i < count; ++i)
	listElemPtr elem;
	for(elem = listElemGetFirst(ranges); elem; elem = elem->next)
	{
		processDataPtr process = (processDataPtr)elem->val;
		xmlNodePtr processNode = xmlNodeCreateProcessData(process, false);
		xmlAddChild(msgNode, processNode);
	}
	size_t bytesSent = socketSend(socket, &worker->address, doc);
	xmlFreeDoc(doc);

	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no primes range sent\n");
}

// in a set of processes, finds the end of range started by given value
int64_t findRangeEndStartedByInList(const int64_t value,
		listPtr processes)
{
#ifdef DEBUG_RANGES
	printf("findRangeEndStartedByInList(value, processes)\n");
	printf("  value=%llu\n", value);
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
	printf("  value=%llu\n", value);
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
	printf("  value=%llu\n", value);
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
	printf("  minimum=%llu\n", minimum);
#endif
	return minimum;
}

int64_t findRangeStartPrecededBy(const int64_t value,
		processDataPtr* processes, size_t processesCount)
{
#ifdef DEBUG_RANGES
	printf("findRangeStartPrecededBy(value, processes, processesCount)\n");
	printf("  value=%llu\n", value);
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
	printf("  minimum=%llu\n", minimum);
#endif
	return minimum;
}

// finds the minimum value not researched for primality
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
				current = next + 1;
		}
		while(next > 0);
		return current;
	}
	else if(server->workersActive > 0 || server->processesDone > 0)
	{
		// some primes scanned or some scanning currently going on

		do
		{
			next = findRangeEndStartedByInList(current, server->processesDoneData);
			if(next > 0)
			{
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
				current = next + 1;
				continue;
			}

			next = findRangeEndStartedByInList(current, excludedRanges);
			if(next > 0)
			{
				current = next + 1;
				continue;
			}
#ifdef DEBUG_RANGES
	printf("findRangeStart(server) status:\n");
	printf("  next=%lld", next);
	printf(" current=%lld\n", current);
	printServerData(server);
#endif
		}
		while(next > 0);

		return current;
	}

	fprintf(stderr, "ERROR: didn't get the proper prime range start\n");
	return 0LL;
}

int64_t findRangeEndLimited(serverDataPtr server, listPtr excludedRanges,
		size_t reserveMore, const int64_t rangeStart, const int64_t rangeLimit)
{
#ifdef DEBUG_RANGES
	printf("findRangeEndLimited(rangeStart, rangeLimit)\n");
	printf("  rangeStart=%llu\n", rangeStart);
	printf("  rangeLimit=%llu\n", rangeLimit);
#endif
	if(rangeStart < 1 || rangeStart > rangeLimit)
	{
		fprintf(stderr, "ERROR: didn't get the proper prime range end\n");
		return 0LL;
	}
	if(rangeStart == 1)
	{
		if(FIRSTLIMIT > rangeLimit)
			return rangeLimit;
		return FIRSTLIMIT;
	}

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
	// for 444444416228682746, which cannot be stored in 64bit signed var
	const double sqrtOfStart = sqrt((double)rangeStart);
	if(sqrtOfStart >= limit)
		return rangeStart;

	double a = 1.0 / (2.0*sqrtOfStart);
	double b = sqrtOfStart;
	double c = -limit;
	double delta = (b*b)-(4*a*c);

	// x_min
	//double x = -(sqrtOfStart*sqrtOfStart);
	//double x_min = x;

	//double fx_min = x*x*a + x+b + c;

	//printf("sqrtOfStart=%f\n", sqrtOfStart);
	//printf("a=%f\n", a);
	//printf("b=%f\n", b);
	//printf("c=%f\n", c);
	//printf("delta=%f\n", delta);

	//double rangeEnd = (-b+sqrt(delta))/(2*a);
	//const double test = (0.6666667) * pow(rangeEnd + rangeStart, 1.5)
	//	- (0.6666667) * pow(rangeStart, 1.5);
	//printf("test=%f\n", test);

	if(delta < 0)
	{
		fprintf(stderr, "ERROR: didn't get the proper prime range end\n");
		return 0LL;
	}

	int64_t rangeEnd;

	if(delta > 0)
		rangeEnd = (int64_t)llround((-b+sqrt(delta))/(2*a)) + rangeStart;
	else if(delta == 0)
		rangeEnd = -b/(2*a) + rangeStart;

	if(rangeEnd > rangeLimit)
		rangeEnd = rangeLimit;

	return rangeEnd;
}

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
		return findRangeEndLimited(server, excludedRanges,
			reserveMore, rangeStart, server->primeTo);
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

		minimum = findRangeEndLimited(server, excludedRanges,
			reserveMore, rangeStart, minimum - 1);

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
	if(process->primeFrom == 0)
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

void savePrimes(listPtr list, const char* filename)
{
	if(list == NULL)
		return;
	FILE* file = NULL;
	if((file = fopen(filename, "a")) == NULL)
		ERR("fopen");
	fprintf(file, "primes");
	listElemPtr temp = listElemGetFirst(list);
	while(temp)
	{
		fprintf(file, " %llu", valueToPrime(temp->val));
		temp = temp->next;
	}
	fprintf(file, "\n");
	fclose(file);
}
