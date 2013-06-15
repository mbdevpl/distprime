#include "distprimecommon.h"
#include "primerange.h"

#define SERVER_SOCKET_TIMEOUT 1

// after no communication occurs for given amount of time
// server requests the status (it counts as outgoing communication,
// so next status request is sent after the same amount of time
// if no response arrives)
#define PING_SENDEVERY 4 // seconds

// worker is banned after not sending anything
// to server for the given amount of time
#define PING_TIMEOUT 30 // seconds

void usage();
int main(int argc, char** argv);
void serverLoop(serverDataPtr myData);
void checkAndPurgeInactiveWorkers(const int socket, serverDataPtr myData);
bool handleMsg(const int socket, serverDataPtr myData,
		serverDataPtr server, workerDataPtr worker, listPtr processesList);

void socketSendPrimeRanges(const int socket, serverDataPtr server,
		workerDataPtr worker);
void socketSendConfirmation(const int socket, serverDataPtr server,
		workerDataPtr worker, processDataPtr process);
void socketSendStatusRequest(const int socket, serverDataPtr server,
		workerDataPtr worker);
bool socketRespondToHandshake(const int socket, serverDataPtr server,
		workerDataPtr worker/*, unsigned int workerIdSentLast*/);
bool socketRespondToRequest(const int socket, serverDataPtr server,
		workerDataPtr worker);
bool socketRespondToPrimes(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr processes);
bool receivedStatus(serverDataPtr server, workerDataPtr worker);

listPtr createNewPrimeRanges(serverDataPtr server, workerDataPtr worker);
void movePrimeRangesToWorker(listPtr ranges, workerDataPtr worker);
void removeConfirmedPrimes(listPtr list, listPtr confirmed);
double calcProgress(serverDataPtr server);
bool checkIfDone(serverDataPtr server);

void writeHeader(const char* filename);
void writeFooter(const char* filename);
void savePrimes(listPtr list, const char* filename);

void usage()
{
	fprintf(stderr, "USAGE: distprime prime_from prime_to output_file\n");
	fprintf(stderr, "  prime_from must be a positive number\n");
	fprintf(stderr, "  prime_to must not be smaller than prime_from\n");
	fprintf(stderr, "  output_file must be a location with write permissions\n");
	fprintf(stderr, "EXAMPLE:\n");
	fprintf(stderr, "  distprime 2 %" PRId64 " log.txt\n", (int64_t)INT64_MAX);
	exitNormal();
}

int main(int argc, char** argv)
{
	//testPrimeRanges();
	//exitNormal();

	printf("distprime\n");
	printf("Distributed prime numbers discovery server\n\n");

	if(argc != 4)
		usage();
	const int64_t primeFrom = (int64_t)atoll(argv[1]);
	const int64_t primeTo = (int64_t)atoll(argv[2]);
	char* outputFile = argv[3];
	if(primeFrom < 1 || primeTo < 1 || primeFrom > primeTo || outputFile == NULL)
		usage();

	srand(getGoodSeed());

	serverDataPtr server = allocServerData();

	server->primeFrom = primeFrom;
	server->primeTo = primeTo;
	server->primeRange = primeTo - primeFrom + 1;
	if(strncmp(outputFile, "/dev/null", 9) == 0)
		server->outputPath = NULL; // disable text output (for testing)
	else
		server->outputPath = outputFile;
	server->hash = getHash();

	serverLoop(server);

	freeServerData(server);
	return EXIT_SUCCESS;
}

// TODO: maybe slightly too long
void serverLoop(serverDataPtr myData)
{
	int socket;
	struct sockaddr_in addrSender;
	addressCreate(&myData->address, INADDR_ANY, SERVER_PORT);
	socketCreate(&socket, SERVER_SOCKET_TIMEOUT, true, true);
	socketBind(socket, &myData->address);
	printf("Receiving on port %d.\n", SERVER_PORT);
	time_t printed = 0, now;
	writeHeader(myData->outputPath);
	while(true)
	{
		time(&now);
		checkAndPurgeInactiveWorkers(socket,  myData);
		// receive xml from the socket
		xmlDocPtr doc;
		if(socketReceive(socket, &addrSender, &doc) > 0)
		{
			// parse xml into distprime objects
			serverDataPtr server;
			workerDataPtr worker;
			listPtr processesList;
			if(processXml(doc, &server, &worker, &processesList) >= 0)
			{
				worker->address = addrSender;
				handleMsg(socket, myData, server, worker, processesList);
			}
			if(worker != NULL)
				freeWorkerData(worker);
			if(server != NULL)
				freeServerData(server);
			listElemPtr e;
			for(e = listElemGetFirst(processesList); e; e = e->next)
				freeProcessData((processDataPtr)e->val);
			listFree(processesList);
		}
		if(doc != NULL)
			xmlFreeDoc(doc);
		bool allDone = checkIfDone(myData);
		if(allDone || now - printed > 5)
		{
			// print progress information every 5 seconds
			printf("Progress: %6.2f%% active workers: %2zu primes discovered: %8" PRId64 "\n",
				calcProgress(myData), myData->workersActive, myData->primesCount);
			time(&printed);
		}
		if(allDone)
		{
			printf("All primes were scanned.\n");
			break;
		}
	}
	writeFooter(myData->outputPath);
	closeSocket(socket);
	xmlCleanupParser();
}

//send status requests to clients that checked-in long ago
void checkAndPurgeInactiveWorkers(const int socket, serverDataPtr myData)
{
	time_t now;
	time(&now);
	listElemPtr e;
	for(e = listElemGetFirst(myData->workersActiveData); e;)
	{
		workerDataPtr active = (workerDataPtr)e->val;
		time_t statusAge = now - active->statusSince;
		// wait 3 seconds before checking up on the worker,
		// and another 7 for response
		if(statusAge >= PING_TIMEOUT)
		{
			// being deaf has consequences
			listElemPtr temp = e;
			e = e->next;
			if(!listElemDetach(myData->workersActiveData, temp))
				CERR("could not remove worker from active list");
			myData->workersActive -= 1;
			freeWorkerData(active); // ok!
			free(temp);
			printf("Kicked inactive worker.\n");
			continue;
		}
		time_t nowDiff = now - active->now;
		//printf("%ld-%ld=%ld\n", now, active->now, nowDiff);
		if(nowDiff >= PING_SENDEVERY)
		{
			// it's time to send the ping
			active->now = now;
			socketSendStatusRequest(socket, myData, active);
		}
		e = e->next;
	}
}

bool handleMsg(const int socket, serverDataPtr myData,
		serverDataPtr server, workerDataPtr worker, listPtr processesList)
{
	bool msgHandled = false;
	if(server == NULL)
	{
		if(worker != NULL)
		{
			if(worker->id == 0)
				msgHandled = socketRespondToHandshake(socket, myData, worker);
			else
			{
				if(listLength(processesList) > 0)
					msgHandled = socketRespondToPrimes(socket, myData,
							worker, processesList);
				else
				{
					//respond to primes request
					workerDataPtr active = matchActiveWorker(myData, worker);
					if(active != NULL)
						msgHandled = socketRespondToRequest(socket, myData, active);
					else
						fprintf(stderr, "WARNING: registered worker not found\n");
				}
			}
		}
		else
			fprintf(stderr, "INFO: received message without worker data\n");
	}
	else
		msgHandled = receivedStatus(myData, worker);
	if(!msgHandled)
		fprintf(stderr, "ERROR: message was not handled\n");
	return msgHandled;
}

void socketSendPrimeRanges(const int socket, serverDataPtr server,
		workerDataPtr worker)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr serverNode = xmlNodeCreateServerData(server);
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, serverNode);
	xmlAddChild(msgNode, workerNode);
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr process = (processDataPtr)worker->processesData[i];
		if(process == NULL)
			continue;
		xmlNodePtr processNode = xmlNodeCreateProcessData(process, false);
		xmlAddChild(msgNode, processNode);
	}
	size_t bytesSent = socketSend(socket, &worker->address, doc);
	xmlFreeDoc(doc);

	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no primes range sent\n");
}

// sends confirmation about process, to the given worker, using given socket
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

void socketSendStatusRequest(const int socket, serverDataPtr server,
		workerDataPtr worker)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr serverNode = xmlNodeCreateServerData(server);
	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, serverNode);

	size_t bytesSent = socketSend(socket, &worker->address, doc);
	xmlFreeDoc(doc);
	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no request sent\n");
}

bool socketRespondToHandshake(const int socket, serverDataPtr server,
		workerDataPtr worker/*, unsigned int workerIdSentLast*/)
{
	bool result = false;
	if(worker->processes == 0)
	{
		fprintf(stderr, "INFO: worker does not want to work\n");
		return true;
	}
	listPtr ranges = createNewPrimeRanges(server, worker);
	if(listLength(ranges) > 0/*== worker->processes*/)
	{
		// received broadcast
		workerDataPtr newWorker = allocWorkerData();
		newWorker->address = worker->address;
		newWorker->hash = worker->hash;
		server->workerIdSentLast += 1; //++workerIdSentLast;
		newWorker->id = server->workerIdSentLast; //workerIdSentLast;
		newWorker->processes = worker->processes; //listLength(ranges) wrong
		newWorker->processesData =
			(processDataPtr*)malloc(newWorker->processes * sizeof(processDataPtr));
		size_t i;
		for(i = 0; i < newWorker->processes; ++i)
			newWorker->processesData[i] = NULL;
		newWorker->status = STATUS_GENERATING;
		time(&newWorker->now);
		newWorker->statusSince = newWorker->now;
		movePrimeRangesToWorker(ranges, newWorker);
		listFree(ranges);
		server->workersActive += 1;
		listElemInsertEnd(server->workersActiveData, (data_type)newWorker);
		socketSendPrimeRanges(socket, server, newWorker);
		result = true;
	}
	else if(listLength(ranges) == 0)
		result = true; // received broadcast, but all prime ranges are allocated
	return result;
}

bool socketRespondToRequest(const int socket, serverDataPtr server,
		workerDataPtr worker)
{
	bool result = false;
	// cleanup after previous round, save primes to HDD
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr activeProcess = worker->processesData[i];
		if(activeProcess == NULL)
			continue;
		if(activeProcess->status < PROCSTATUS_UNCONFIRMED)
			return true; //CERR("worker not ready");
		activeProcess->status = PROCSTATUS_CONFIRMED;
		time(&activeProcess->finished);
		// move process from active client to "done" list
		listElemInsertEnd(server->processesDoneData, (data_type)activeProcess);
		server->processesDone += 1;
		size_t len = listLength(activeProcess->confirmed);
		server->primesCount += len;
		// save all primes to text file, and increase counter
		if(len > 0)
			savePrimes(activeProcess->confirmed, server->outputPath);
		listClear(activeProcess->confirmed);
		worker->processesData[i] = NULL;
	}
	// received subsequent request
	listPtr ranges = createNewPrimeRanges(server, worker);
	if(listLength(ranges) > 0/*== worker->processes*/)
	{
		// received prime range request
		worker->status = STATUS_GENERATING;
		time(&worker->now);
		worker->statusSince = worker->now;
		movePrimeRangesToWorker(ranges, worker);
		listFree(ranges);
		socketSendPrimeRanges(socket, server, worker);
		result = true;
	}
	else if(listLength(ranges) == 0)
		result = true; // received request, but all prime ranges are allocated
	return result;
}

// TODO: maybe slightly too long
bool socketRespondToPrimes(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr processes)
{
	// received a list of primes
	bool sentConfirmation = false;
	workerDataPtr active = matchActiveWorker(server, worker);
	if(active == NULL)
		return true;
	listElemPtr e;
	for(e = listElemGetFirst(processes); e; e = e->next)
	{
		processDataPtr proc = (processDataPtr)e->val;
		size_t i;
		for(i = 0; i < active->processes; ++i)
		{
			processDataPtr activeProcess = active->processesData[i];
			if(activeProcess == NULL)
				continue;
			else if(activeProcess->primeFrom == proc->primeFrom
					&& activeProcess->primeTo == proc->primeTo)
			{
				// received a list of primes from an active worker
				// send confirmation
				socketSendConfirmation(socket, server, active, proc);
				sentConfirmation = true;
				// update status only if it indicates progress
				if(proc->status > activeProcess->status)
					activeProcess->status = proc->status;
				time(&worker->now);
				worker->statusSince = worker->now;
				if(activeProcess->confirmed == NULL)
					activeProcess->confirmed = proc->primes;
				else
				{
					// resolve duplicates
					removeConfirmedPrimes(proc->primes, activeProcess->confirmed);
					listMerge(activeProcess->confirmed, proc->primes);
				}
				proc->primes = NULL;
			}
			if(sentConfirmation)
				break;
		}
		if(sentConfirmation)
			break;
	}

	if(sentConfirmation)
		return true;
	fprintf(stderr, "ERROR: did not send any confirmation\n");
	return false;
}

bool receivedStatus(serverDataPtr server, workerDataPtr worker)
{
	bool result = false;
	bool recognizedWorker = false;
	workerDataPtr active = matchActiveWorker(server, worker);
	if(active != NULL)
	{
		recognizedWorker = true;
		active->status = worker->status;
		time(&active->now);
		active->statusSince = active->now;
		result = true;
	}
	if(!recognizedWorker)
		fprintf(stderr, "ERROR: received status message from unknown worker\n");
	return result;
}

listPtr createNewPrimeRanges(serverDataPtr server, workerDataPtr worker)
{
	listPtr ranges = listCreate();
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr range = allocProcessData();
		if(!setPrimeRange(server, ranges, worker->processes-1-i, range))
		{
			freeProcessData(range);
			break;
		}
		listElemInsertEnd(ranges, (data_type)range);
	}
	if(listLength(ranges) == 0)
	{
		listFree(ranges);
		ranges = NULL;
	}
	return ranges;
}

void movePrimeRangesToWorker(listPtr ranges, workerDataPtr worker)
{
	listElemPtr elem = listElemGetFirst(ranges);
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		if(elem)
		{
			processDataPtr range = (processDataPtr)elem->val;
			if(worker->processesData[i] != NULL)
				freeProcessData(worker->processesData[i]);
			worker->processesData[i] = range;
			elem->val = NULL;
			//worker->processesData[i] = allocProcessData();
			//processDataPtr newProcess = worker->processesData[i];
			//newProcess->primeFrom = range->primeFrom;
			//newProcess->primeTo = range->primeTo;
			//newProcess->primeRange = range->primeRange;
			//freeProcessData(range);
			elem = elem->next;
		}
		else
		{
			worker->processesData[i] = NULL; //allocProcessData();
			//newProcess->primeFrom = 0;
			//newProcess->primeTo = 0;
			//newProcess->primeRange = 0;
			//newProcess->status = PROCSTATUS_IDLE;
		}
	}
}

void removeConfirmedPrimes(listPtr list, listPtr confirmed)
{
	if(listLength(confirmed) == 0 || listLength(list) == 0)
		return;
	listElemPtr e1 = listElemGetFirst(list);
	while(e1)
	{
		bool found = false;
		listElemPtr e2;
		for(e2 = listElemGetFirst(confirmed); e2; e2 = e2->next)
		{
			if(valueToPrime(e1->val) != valueToPrime(e2->val))
				continue;
			found = true;
			break;
		}
		if(!found)
		{
			e1 = e1->next;
			continue;
		}
		listElemPtr temp = e1;
		e1 = e1->next;
		if(!listElemDetach(list, temp))
			CERR("could not remove a confirmed prime");
		free((int64_t*)temp->val);
		listElemFree(temp);
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
			if(next == server->primeTo)
				return true;
			//if(next == INT64_MAX)
			//	return true;
			current = next + 1;
			continue;
		}
	}
	while(next > 0);
	//if(current > server->primeTo)
	//	return true;
	return false;
}

void writeHeader(const char* filename)
{
	if(filename == NULL)
		return;
	FILE* file = NULL;
	if((file = fopen(filename, "a")) == NULL)
		ERR("fopen");
	fprintf(file, "==  distprime started  ==\n");
	fprintf(file, "== ");
	// save timestamp
	{
		time_t now;
		time(&now);
		char temp[32];
		timeToString(&now, temp, 32);
		fprintf(file, "%s", temp);
	}
	fprintf(file, " ==\n");
	fclose(file);
}

void writeFooter(const char* filename)
{
	if(filename == NULL)
		return;
	FILE* file = NULL;
	if((file = fopen(filename, "a")) == NULL)
		ERR("fopen");
	fprintf(file, "====\n\n");
	fclose(file);
}

void savePrimes(listPtr list, const char* filename)
{
	if(list == NULL || filename == NULL)
		return;
	FILE* file = NULL;
	if((file = fopen(filename, "a")) == NULL)
		ERR("fopen");
	// save timestamp
//	{
//		time_t now;
//		time(&now);
//		char temp[32];
//		timeToString(&now, temp, 32);
//		fprintf(file, "%s -", temp);
//	}
	listElemPtr temp = listElemGetFirst(list);
	while(temp)
	{
		fprintf(file, " %" PRId64 "", valueToPrime(temp->val));
		temp = temp->next;
	}
	fprintf(file, "\n");
	fclose(file);
}
