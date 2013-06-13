#include "distprimecommon.h"
#include "primerange.h"

#define SERVER_PORT ((in_port_t)8765)

void usage();
int main(int argc, char** argv);
void serverLoop(serverDataPtr myData);

void socketSendPrimeRanges(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr ranges);
void socketSendConfirmation(const int socket, serverDataPtr server,
		workerDataPtr worker, processDataPtr process);
void socketSendStatusRequest(const int socket, serverDataPtr server,
		workerDataPtr worker);
bool socketRespondToHandshake(const int socket, serverDataPtr server,
		workerDataPtr worker, unsigned int workerIdSentLast);
bool socketRespondToRequest(const int socket, serverDataPtr server,
		workerDataPtr worker);
bool socketRespondToPrimes(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr processes);
bool receivedStatus(serverDataPtr server, workerDataPtr worker);

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
	fprintf(stderr, "  distprime 2 %lld log.txt\n", INT64_MAX);
	exitNormal();
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
		if(elem)
		{
			worker->processesData[i] = allocProcessData();
			processDataPtr newProcess = worker->processesData[i];
			processDataPtr range = (processDataPtr)elem->val;
			newProcess->primeFrom = range->primeFrom;
			newProcess->primeTo = range->primeTo;
			newProcess->primeRange = range->primeRange;
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
	listElemPtr e1;
	for(e1 = listElemGetFirst(list); e1; e1 = e1->next)
	{
		listElemPtr e2;
		for(e2 = listElemGetFirst(confirmed); e2; e2 = e2->next)
		{
			if(valueToPrime(e1->val) != valueToPrime(e2->val))
				continue;
			if(!listElemDetach(list, e1))
				CERR("could not remove a confirmed prime");
			listElemFree(e1);
			break;
		}
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
	//testPrimeRanges();
	//exitNormal();

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

	serverDataPtr server = allocServerData();

	server->primeFrom = primeFrom;
	server->primeTo = primeTo;
	server->primeRange = primeTo - primeFrom + 1;
	if(strncmp(outputFile, "/dev/null", 9) == 0)
		server->outputPath = NULL; // disable text output (for testing)
	else
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
	printf("Receiving on port %d.\n", SERVER_PORT);

	//if(listen(socket, 5) < 0)
	//	ERR("listen");
	//int descriptor = 0;
	//if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
	//	ERR("accept");

	time_t printed = 0;
	time_t now;
	writeHeader(myData->outputPath);
	while(true)
	{
		time(&now);
		//send status requests to clients that checked-in long ago
		{
			listElemPtr e1;
			for(e1 = listElemGetFirst(myData->workersActiveData); e1;)
			{
				workerDataPtr active = (workerDataPtr)e1->val;
				time_t statusAge = now - active->statusSince;
				// wait 3 seconds before checking up on the worker,
				// and another 7 for response
				if(statusAge >= 10)
				{
					//else
					//{
					// being deaf has consequences
					listElemPtr temp = e1;
					e1 = e1->next;
					if(!listElemDetach(myData->workersActiveData, temp))
						CERR("could not remove worker from active list");
					myData->workersActive -= 1;
					freeWorkerData(active);
					free(temp);
					continue;
					//}
				}
				time_t nowDiff = now - active->now;
				//printf("%ld-%ld=%ld\n", now, active->now, nowDiff);
				if(nowDiff >= 3)
				{
					//if(active->now == active->statusSince)
					//{
					// it's time to send the ping
					active->now = now;
					socketSendStatusRequest(socket, myData, active);
					//}
				}
				e1 = e1->next;
			}
		}

		// receive xml from the socket
		xmlDocPtr doc;
		if(socketReceive(socket, &addrSender, &doc) > 0)
		{
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
								worker->address = addrSender;
								msgHandled = socketRespondToHandshake(socket, myData,
										worker, workerIdSentLast);
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
							msgHandled = socketRespondToPrimes(socket, myData,
									worker, processesList);
						}
						else
						{
							bool foundWorker = false;
							workerDataPtr active = matchActiveWorker(myData, worker);
							if(active != NULL)
							{
								foundWorker = true;
								bool workerReady = true;
								size_t i;
								for(i = 0; i < active->processes; ++i)
								{
									if(active->processesData[i] != NULL)
									{
										workerReady = false;
										break;
									}
								}
								if(workerReady)
								{
									msgHandled = socketRespondToRequest(socket, myData, active);
								}
								else
									fprintf(stderr, "ERROR: worker not ready\n");
							}
							if(!foundWorker)
								fprintf(stderr, "ERROR: registered worker not found\n");
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
				// received status message
				msgHandled = receivedStatus(myData, worker);
			}

			if(worker != NULL)
			{
				freeWorkerData(worker);
				worker = NULL;
			}

			if(server != NULL)
			{
				freeServerData(server);
				server = NULL;
			}

			if(!msgHandled)
				fprintf(stderr, "ERROR: message was not handled\n");
		}

		if(doc != NULL)
			xmlFreeDoc(doc);


		bool allDone = checkIfDone(myData);

		if(allDone || now - printed > 5)
		{
			// print progress information every 5 seconds
			printf("Progress: %6.2f%% active workers: %2u primes discovered: %8lld\n",
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
		workerDataPtr worker, unsigned int workerIdSentLast)
{
	bool result = false;
	listPtr ranges = createNewPrimeRanges(server, worker);
	if(listLength(ranges) > 0/*== worker->processes*/)
	{
		// received broadcast
		workerDataPtr newWorker = allocWorkerData();
		newWorker->address = worker->address;
		newWorker->hash = worker->hash;
		++workerIdSentLast;
		newWorker->id = workerIdSentLast;
		newWorker->processes = worker->processes; //listLength(ranges) wrong
		newWorker->processesData =
			(processDataPtr*)malloc(newWorker->processes * sizeof(processDataPtr));
		newWorker->status = STATUS_GENERATING;
		time(&newWorker->now);
		newWorker->statusSince = newWorker->now;
		movePrimeRangesToWorker(ranges, newWorker);
		server->workersActive += 1;
		listElemInsertEnd(server->workersActiveData, (data_type)newWorker);
		socketSendPrimeRanges(socket, server, newWorker, ranges);
		result = true;
	}
	else if(listLength(ranges) == 0)
	{
		// received broadcast, but all prime ranges are allocated
		result = true;
	}
	else
		fprintf(stderr, "ERROR: wrong number of prime ranges to send\n");
	listFree(ranges);
	return result;
}

bool socketRespondToRequest(const int socket, serverDataPtr server,
		workerDataPtr worker)
{
	bool result = false;
	// received subsequent request
	listPtr ranges = createNewPrimeRanges(server, worker);
	if(listLength(ranges) == worker->processes)
	{
		// received prime range request
		worker->status = STATUS_GENERATING;
		time(&worker->now);
		worker->statusSince = worker->now;
		movePrimeRangesToWorker(ranges, worker);
		socketSendPrimeRanges(socket, server, worker, ranges);
		result = true;
	}
	else if(listLength(ranges) == 0)
	{
		// received request, but all prime ranges are allocated
		result = true;
	}
	else
		fprintf(stderr, "ERROR: wrong number of prime ranges to send\n");
	listFree(ranges);
	return result;
}

bool socketRespondToPrimes(const int socket, serverDataPtr server,
		workerDataPtr worker, listPtr processes)
{
	// received a list of primes
	bool recognizedWorker = false;
	bool sentConfirmation = false;
	workerDataPtr active = matchActiveWorker(server, worker);
	if(active != NULL)
	{
		recognizedWorker = true;
		listElemPtr e;
		for(e = listElemGetFirst(processes); e; e = e->next)
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
				{
					// TODO: find this process in server->processesDoneData
					// check if all new primes are in confirmed
					// send confirmation
					// if confirmation sent, break loop:
					//if(sentConfirmation)
					//	break;
					continue;
				}
				if(activeProcess->primeFrom == proc->primeFrom)
				{
					// received a list of primes from a worker
					activeProcess->status = proc->status;
					if(activeProcess->primes == NULL)
						activeProcess->primes = listCreate();
					listMerge(activeProcess->primes, proc->primes);
					proc->primes = NULL;
					socketSendConfirmation(socket, server, active, activeProcess);

					if(activeProcess->confirmed == NULL)
						activeProcess->confirmed = listCreate();
					removeConfirmedPrimes(activeProcess->primes, activeProcess->confirmed);
					// save all primes to text file, and increase counter
					{
						size_t len = listLength(activeProcess->primes);
						if(len > 0)
						{
							savePrimes(activeProcess->primes, server->outputPath);
							server->primesCount += len;
						}
					}
					listMerge(activeProcess->confirmed, activeProcess->primes);
					activeProcess->primes = listCreate();

					sentConfirmation = true;
					if(activeProcess->status == PROCSTATUS_UNCONFIRMED
							&& listLength(activeProcess->primes) == 0)
					{
						activeProcess->status = PROCSTATUS_CONFIRMED;
						active->processesData[i] = NULL;
						time(&activeProcess->finished);
						server->processesDone += 1;
						listElemInsertEnd(server->processesDoneData, (data_type)activeProcess);
					}
					break;
				}
			}
			freeProcessData(proc);
		}
	}
	if(recognizedWorker && sentConfirmation)
		return true;
	if(!recognizedWorker)
		fprintf(stderr, "ERROR: did not recognize worker\n");
	if(!sentConfirmation)
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
		fprintf(file, " %lld", valueToPrime(temp->val));
		temp = temp->next;
	}
	fprintf(file, "\n");
	fclose(file);
}
