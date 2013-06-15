#include "distprimecommon.h"

#define WORKER_SELECT_TIMEOUT 5 // seconds

#define WORKER_SOCKET_TIMEOUT 1 // seconds

void usage();
int main(int argc, char** argv);
bool handleTimeout(const int socket, workerDataPtr myData,
		serverDataPtr myServerData, size_t timeoutCount);
void workerLoop(workerDataPtr myData, serverDataPtr myServerData);
bool handleMsg(const int socket, workerDataPtr myData, serverDataPtr myServerData,
		struct sockaddr_in* addressSenderPtr,
		serverDataPtr server, workerDataPtr worker, listPtr processesList);
int setupSocket(workerDataPtr worker);
int setupProcessesAndPipes(workerDataPtr worker);
void cleanupPipes(workerDataPtr myData);
bool updateStatusIfConfirmedAll(workerDataPtr worker);
bool updateStatusIfGeneratedAll(workerDataPtr worker);

// in all below socket(..) and received(...) functions:
// 'socket' is a socket used to send data,
// 'worker' is the complete state of this program
// 'server' is a set of data of the server (ex. its address/hash)
void socketSendHandshake(const int socket, workerDataPtr worker,
		serverDataPtr server);
void socketSendPrimes(const int socket, workerDataPtr worker,
		serverDataPtr server, processDataPtr process);
void socketSendRequest(const int socket, workerDataPtr worker,
		serverDataPtr server);
void socketSendStatus(const int socket, workerDataPtr worker,
		serverDataPtr server);
bool receivedPrimeRanges(workerDataPtr worker, listPtr ranges);
bool receivedConfirmation(workerDataPtr worker, listPtr processesList);

bool readPrimesFromPipe(workerDataPtr worker,
		serverDataPtr server, processDataPtr process);
void moveConfirmedPrimes(listPtr from, listPtr to, listPtr moved);

void invokePrimesGenerator(processDataPtr process);
bool isPrime(int64_t number);
void generatePrimes(processDataPtr process);

void usage()
{
	// server_port
	fprintf(stderr, "USAGE: distprimeworker subprocesses_count\n");
	//fprintf(stderr, "  server_port must be a valid port number\n");
	fprintf(stderr, "  subprocesses_count must be a positive number\n");
	fprintf(stderr, "EXAMPLE: distprimeworker 8\n");
	exitNormal();
}

int main(int argc, char** argv)
{
	printf("distprimeworker\n");
	printf("Distributed prime numbers discovery worker\n\n");

	if(argc != 2)
		usage();
	//const int port = atoi(argv[1]);
	const int processes = atoi(argv[1]);
	if(processes < 1 /*|| port < 1024 || port > 65535*/)
		usage();

	srand(getGoodSeed());

	setSigHandler(handlerSigchldDefault, SIGCHLD);

	workerDataPtr worker = allocWorkerData();
	worker->hash = getHash();
	worker->processes = processes;
	worker->processesData =
		(processDataPtr*)malloc(worker->processes * sizeof(processDataPtr));
	size_t i;
	for(i = 0; i < worker->processes; ++i)
		worker->processesData[i] = NULL;

	serverDataPtr server = allocServerData();
	server->address.sin_port = htons(SERVER_PORT);

	workerLoop(worker, server);

	while(TEMP_FAILURE_RETRY(wait(NULL)) >= 0)
		;
	freeServerData(server);
	freeWorkerData(worker);
	return EXIT_SUCCESS;
}

// value returned indicated if server should be kept alive
bool handleTimeout(const int socket, workerDataPtr myData,
		serverDataPtr myServerData, size_t timeoutCount)
{
	bool keepAlive = false;
	pid_t pid = getpid();
	switch(myData->status)
	{
	case STATUS_IDLE:
		if(timeoutCount < 3)
		{
			keepAlive = true;
			if(myData->id == 0)
			{
				printf("P%d: Server not available...\n", pid);
				socketSendHandshake(socket, myData, myServerData);
			}
			else
			{
				printf("P%d: Server not responding...\n", pid);
				socketSendRequest(socket, myData, myServerData);
			}
		}
		else
		{
			keepAlive = false;
			if(myData->id == 0)
				printf("P%d: Server not available: timeout.\n", pid);
			else
				printf("P%d: Server not responding anymore: timeout.\n", pid);
		}
		break;
	case STATUS_GENERATING:
		keepAlive = true;
		//printf("P%d: Generating primes...\n", pid);
		break;
	case STATUS_CONFIRMING:
		if(timeoutCount < 3)
		{
			// send primes for all processes that need it
			keepAlive = true;
			printf("P%d: Cannot confirm results...\n", pid);
			size_t i;
			for(i = 0; i < myData->processes; ++i)
			{
				processDataPtr myProcess = myData->processesData[i];
				if(myProcess->status != PROCSTATUS_IDLE)
					socketSendPrimes(socket, myData, myServerData, myProcess);
			}
		}
		else
		{
			keepAlive = false;
			printf("P%d: Cannot confirm results: timeout.\n", pid);
		}
		break;
	default:
		keepAlive = false;
		break;
	}
	return keepAlive;
}

// TODO: TOO LONG
void workerLoop(workerDataPtr myData, serverDataPtr myServerData)
{
	// setup
	int socket = setupSocket(myData);
	printf("P%d: Receiving on port %d...\n",
		getpid(), ntohs(myData->address.sin_port));
	fd_set descriptorSet;
	int descriptorSetMax = setupProcessesAndPipes(myData);
	if(socket > descriptorSetMax)
		descriptorSetMax = socket;
	// broadcasting handshake
	printf("P%d: Broadcasting to port %d...\n",
		getpid(), ntohs(myServerData->address.sin_port));
	socketSendHandshake(socket, myData, myServerData);
	// stores address of last packet sender
	struct sockaddr_in addressSender;
	bool keepAlive = true;
	size_t timeoutCount = 0;
	time_t primesSent = 0;
	time(&primesSent);
	while(keepAlive)
	{
		time(&myData->now);
		// reset the descriptor set
		FD_ZERO(&descriptorSet);
		FD_SET(socket, &descriptorSet);
		if(myData->status == STATUS_GENERATING)
		{
			size_t i;
			for(i = 0; i < myData->processes; ++i)
				if(myData->processesData[i]->finished == 0)
					FD_SET(myData->processesData[i]->pipeRead, &descriptorSet);
		}

		if(timeoutCount == 0)
		{
			switch(myData->status)
			{
			case STATUS_IDLE:
				if(myData->id > 0)
					socketSendRequest(socket, myData, myServerData);
				break;
			case STATUS_CONFIRMING:
				{
					if(myData->now - primesSent > WORKER_SELECT_TIMEOUT)
					{
						size_t i;
						for(i = 0; i < myData->processes; ++i)
						{
							processDataPtr myProcess = myData->processesData[i];
							if(myProcess->status != PROCSTATUS_IDLE)
								socketSendPrimes(socket, myData, myServerData, myProcess);
						}
						time(&primesSent);
					}
				} break;
			default:
				break;
			}
		}

		struct timeval descriptorSetTimeout;
		descriptorSetTimeout.tv_sec = WORKER_SELECT_TIMEOUT;
		descriptorSetTimeout.tv_usec = 0;

		// select all ready descriptors
		int descriptorsToRead = 0;
		if(TEMP_FAILURE_RETRY(descriptorsToRead = select(descriptorSetMax + 1,
				&descriptorSet, NULL, NULL, &descriptorSetTimeout)) < 0)
			ERR("select()");

		if(descriptorsToRead == 0)
		{
			++timeoutCount;
			keepAlive = handleTimeout(socket, myData, myServerData, timeoutCount);
			continue;
		}
		timeoutCount = 0;

		size_t i;
		for(i = 0; i < myData->processes; ++i)
		{
			processDataPtr myProcess = myData->processesData[i];
			int pipe = myProcess->pipeRead;
			if(FD_ISSET(pipe, &descriptorSet))
			{
				FD_CLR(pipe, &descriptorSet);
				//printf("%u\n", listLength(myProcess->primes));
				readPrimesFromPipe(myData, myServerData, myProcess);
				// send results back to the server
				socketSendPrimes(socket, myData, myServerData, myProcess);
				if(updateStatusIfGeneratedAll(myData))
					printf("P%d: Waiting for confirmation...\n", getpid());
			}
		}

		if(FD_ISSET(socket, &descriptorSet))
		{
			FD_CLR(socket, &descriptorSet);

			// receive xml from the socket
			xmlDocPtr doc;
			if(socketReceive(socket, &addressSender, &doc) > 0)
			{
				// parse xml into distprime objects
				serverDataPtr server;
				workerDataPtr worker;
				listPtr processesList;
				if(processXml(doc, &server, &worker, &processesList) >= 0)
					handleMsg(socket, myData, myServerData,
							&addressSender, server, worker, processesList);
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
		}
	}
	// close all pipes
	cleanupPipes(myData);
	closeSocket(socket);
	xmlCleanupParser();
}

// TODO: too long
bool handleMsg(const int socket, workerDataPtr myData, serverDataPtr myServerData,
		struct sockaddr_in* addressSenderPtr,
		serverDataPtr server, workerDataPtr worker, listPtr processesList)
{
	bool msgHandled = false;
	if(server == NULL)
		return true;
	if(worker != NULL)
	{
		if(myData->hash == worker->hash)
		{
			if(myData->id == 0)
			{
				// received worker id from the server
				myData->id = worker->id;
				myServerData->address = *addressSenderPtr;
				myServerData->hash = server->hash;
			}
			else if(myData->id == worker->id)
				; // nothing
			else
				fprintf(stderr, "ERROR, received id but already have id\n");
		}
		else
			fprintf(stderr, "ERROR, unauthorized worker data sent\n");
	}
	if(myServerData->hash == server->hash)
	{
		if(listLength(processesList) > 0)
		{
			if(myData->status == STATUS_IDLE)
				msgHandled = receivedPrimeRanges(myData, processesList);
			else if(myData->status == STATUS_GENERATING
				|| myData->status == STATUS_CONFIRMING)
				msgHandled = receivedConfirmation(myData, processesList);
		}
		else if(listLength(processesList) == 0)
		{
			// received status request
			socketSendStatus(socket, myData, myServerData);
			msgHandled = true;
		}
	}
	else
		fprintf(stderr, "INFO: message from unauthorized server\n");
	if(!msgHandled)
		fprintf(stderr, "ERROR: last message was not handled\n");
	return msgHandled;
}

// returns socket descriptor number
int setupSocket(workerDataPtr worker)
{
	int socket = 0;
	addressCreate(&worker->address, INADDR_ANY, (in_port_t)(rand()%10001 + 5000));
	socketCreate(&socket, WORKER_SOCKET_TIMEOUT, true, true);
	socketBind(socket, &worker->address);
	return socket;
}

// returns highest allocated reading pipe descriptor number
int setupProcessesAndPipes(workerDataPtr worker)
{
	int descriptorSetMax = 0;
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		int pipes[2];
		if(pipe(pipes)<0)
			ERR("pipe");
		//addFlags(pipes[0], O_NONBLOCK);
		if(pipes[0] > descriptorSetMax)
			descriptorSetMax = pipes[0];
		worker->processesData[i] = allocProcessData();
		processDataPtr myProcess = worker->processesData[i];
		myProcess->pipeRead = pipes[0];
		myProcess->pipeWrite = pipes[1];
		myProcess->pipeReadBuf = (char*)malloc(BUFSIZE_MAX*sizeof(char));
		memset(myProcess->pipeReadBuf, '\0', BUFSIZE_MAX);
		myProcess->pipeReadBufCount = 0;
		myProcess->primes = listCreate();
	}
	return descriptorSetMax;
}

void cleanupPipes(workerDataPtr myData)
{
	size_t i;
	for(i = 0; i < myData->processes; ++i)
	{
		processDataPtr myProcess = myData->processesData[i];
		closefifo(myProcess->pipeRead);
		myProcess->pipeRead = 0;
		closefifo(myProcess->pipeWrite);
		myProcess->pipeWrite = 0;
	}
}

bool updateStatusIfConfirmedAll(workerDataPtr worker)
{
	if(worker->status == STATUS_IDLE)
	return false;
	size_t i;
	bool allFinished = true;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr process = (processDataPtr)worker->processesData[i];
		if(process->status == PROCSTATUS_IDLE)
			continue;
		if(process->status != PROCSTATUS_CONFIRMED)
			allFinished = false;
	}
	if(!allFinished)
		return false;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr myProcess = worker->processesData[i];
		myProcess->primeFrom = 0;
		myProcess->primeTo = 0;
		myProcess->primeRange = 0;
		myProcess->started = 0;
		myProcess->finished = 0;
		listElemPtr e;
		for(e = listElemGetFirst(myProcess->primes); e; e = e->next)
			if(e->val != NULL)
				free((int64_t*)e->val);
		listClear(myProcess->primes);
	}
	worker->status = STATUS_IDLE;
	return true;
}

bool updateStatusIfGeneratedAll(workerDataPtr worker)
{
	if(worker->status == STATUS_CONFIRMING)
		return false;
	bool allAtLeastConfirming = true;
	bool allConfirmed = true;
	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		processDataPtr process = (processDataPtr)worker->processesData[i];
		if(process->status == PROCSTATUS_IDLE)
			continue;
		if(process->status != PROCSTATUS_UNCONFIRMED
				&& process->status != PROCSTATUS_CONFIRMED)
			allAtLeastConfirming = false;
		if(process->status != PROCSTATUS_CONFIRMED)
			allConfirmed = false;
	}
	if(!allAtLeastConfirming || allConfirmed)
		return false;
	worker->status = STATUS_CONFIRMING;
	return true;
}

void socketSendHandshake(const int socket, workerDataPtr worker,
		serverDataPtr server)
{
	struct sockaddr_in addressBroadcast;
	addressCreate(&addressBroadcast, INADDR_BROADCAST, ntohs(server->address.sin_port));

	// send xml
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlAddChild(msgNode, workerNode);
	xmlDocSetRootElement(doc, msgNode);
	int bytesSent = socketSend(socket, &addressBroadcast, doc);
	xmlFreeDoc(doc);

	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no broadcast sent\n");
}

// used for short primes list
bool socketSendPrimesAll(const int socket, workerDataPtr worker,
		serverDataPtr server, processDataPtr process)
{
	if(listLength(process->primes) > PRIMEMAXCOUNT)
		return false;
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlNodePtr processNode = xmlNodeCreateProcessData(process, true);

	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, workerNode);
	xmlAddChild(msgNode, processNode);

	// send data to server
	size_t bytesSent = socketSend(socket, &server->address, doc);
	xmlFreeDoc(doc);
	if(bytesSent == 0)
		fprintf(stderr, "WARNING: didn't send all primes\n");
	return true;
}

// used for long primes list
void socketSendPrimesFragment(const int socket, workerDataPtr worker,
		serverDataPtr server, processDataPtr process, int64_t* frag, size_t fragLen)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlNodePtr processNode = xmlNodeCreateProcessDataAltered(process, frag, fragLen);

	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, workerNode);
	xmlAddChild(msgNode, processNode);

	// send data to server
	size_t bytesSent = socketSend(socket, &server->address, doc);
	xmlFreeDoc(doc);
	if(bytesSent == 0)
		fprintf(stderr, "WARNING: didn't send primes list section\n");
}

void socketSendPrimes(const int socket, workerDataPtr worker,
		serverDataPtr server, processDataPtr process)
{
	size_t primesCount = listLength(process->primes);
	if(primesCount <= PRIMEMAXCOUNT)
	{
		socketSendPrimesAll(socket, worker, server, process);
	}
	else
	{
		//if(primesCount > PRIMEMAXCOUNT)
		//	CERR("can't send that many primes without risking fragmentation");
		listElemPtr currPrime = listElemGetFirst(process->primes);
		int64_t frag[PRIMEMAXCOUNT];
		size_t j;
		int statusBackup = process->status;
		if(statusBackup == PROCSTATUS_UNCONFIRMED)
			process->status = PROCSTATUS_COMPUTING; // to avoid false completion message
		while(primesCount > 0)
		{
			size_t max = primesCount > PRIMEMAXCOUNT ? PRIMEMAXCOUNT : primesCount;
			for(j = 0; j < max; ++j)
			{
				frag[j] = valueToPrime(currPrime->val);
				currPrime = currPrime->next;
			}
			if(primesCount == max)
				process->status = statusBackup;

			socketSendPrimesFragment(socket, worker, server, process, frag, j);

			primesCount -= max;
		}
		process->status = statusBackup;
	}
}

void socketSendRequest(const int socket, workerDataPtr worker,
		serverDataPtr server)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, workerNode);
	int bytesSent = socketSend(socket, &server->address, doc);
	xmlFreeDoc(doc);
	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no primes range request sent\n");
}

void socketSendStatus(const int socket, workerDataPtr worker,
		serverDataPtr server)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	xmlNodePtr serverNode = xmlNodeCreateServerData(server);
	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, workerNode);
	xmlAddChild(msgNode, serverNode);
	if(worker->status != STATUS_IDLE)
	{
		size_t i;
		for(i = 0; i < worker->processes; ++i)
		{
			processDataPtr process = (processDataPtr)worker->processesData[i];
			if(process->status == PROCSTATUS_IDLE)
				continue;
			xmlNodePtr processNode = xmlNodeCreateProcessData(process, false);
			xmlAddChild(msgNode, processNode);
		}
	}
	size_t bytesSent = socketSend(socket, &server->address, doc);
	xmlFreeDoc(doc);
	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no status sent\n");
}

// TODO: maybe slightly too long
bool receivedPrimeRanges(workerDataPtr worker, listPtr ranges)
{
	// if any range has wrong status, reutrn true
	listElemPtr e;
	for(e = listElemGetFirst(ranges); e; e = e->next)
	{
		processDataPtr range = (processDataPtr)e->val;
		if(range == NULL)
			CERR("received null range");
		if(range->status != PROCSTATUS_COMPUTING)
			return true;
	}
	if(listLength(ranges) == 0)
	{
		fprintf(stderr, "ERROR: incorrect number of processes\n");
		return false;
	}
	// received new orders
	size_t index = 0;
	listElemPtr elem;
	for(elem = listElemGetFirst(ranges); elem; elem = elem->next)
	{
		processDataPtr range = (processDataPtr)elem->val;
		processDataPtr myProcess = worker->processesData[index];
		time(&myProcess->started);
		myProcess->primeFrom = range->primeFrom;
		myProcess->primeTo = range->primeTo;
		myProcess->primeRange = range->primeRange;
		myProcess->status = PROCSTATUS_COMPUTING;
		invokePrimesGenerator(myProcess);
		++index;
	}
	if(index < worker->processes)
	{
		// set remaining processes to stubs
		for(;index < worker->processes; ++index)
		{
			processDataPtr myProcess = worker->processesData[index];
			myProcess->primeFrom = 0;
			myProcess->primeTo = 0;
			myProcess->primeRange = 0;
			myProcess->status = PROCSTATUS_IDLE;
		}
	}
	worker->status = STATUS_GENERATING;
	return true;
}

bool receivedConfirmation(workerDataPtr worker, listPtr processesList)
{
	bool result = false;
	bool movedPrimes = false;
	size_t i;
	listElemPtr elem;
	for(elem = listElemGetFirst(processesList); elem; elem = elem->next)
	{
		processDataPtr process = (processDataPtr)elem->val;
		for(i = 0; i < worker->processes; ++i)
		{
			processDataPtr myProcess = worker->processesData[i];
			if(process->primeFrom == myProcess->primeFrom
				&& process->primeTo == myProcess->primeTo)
			{
				if(myProcess->confirmed == NULL)
					myProcess->confirmed = listCreate();
				moveConfirmedPrimes(myProcess->primes,
					myProcess->confirmed, process->primes);
				movedPrimes = true;
				listElemPtr e;
				for(e = listElemGetFirst(myProcess->confirmed); e; e = e->next)
					if(e->val != NULL)
						free((int64_t*)e->val);
				listClear(myProcess->confirmed);
				if(myProcess->status == PROCSTATUS_UNCONFIRMED
					&& listLength(myProcess->primes) == 0)
					myProcess->status = PROCSTATUS_CONFIRMED;
				break;
			}
		}
	}
	if(updateStatusIfConfirmedAll(worker))
		printf("P%d: All results were confirmed.\n", getpid());
	if(movedPrimes)
		result = true;
	else
		fprintf(stderr, "ERROR: did not move any primes\n");
	return result;
}

// server and worker not needed
// TODO: TOO LONG
bool readPrimesFromPipe(workerDataPtr worker,
		serverDataPtr server, processDataPtr process)
{
	char buffer[BUFSIZE_MAX];
	memset(buffer, '\0', BUFSIZE_MAX * sizeof(char));
	size_t bytesRead = readUntil(process->pipeRead, buffer, BUFSIZE_MAX, '\n');
	if(bytesRead < 0)
		ERR("read");
	else if(bytesRead > 0)
	{
#ifdef DEBUG_IO
		printf(" * read %u bytes from pipe\n", bytesRead);
#endif
#ifdef DEBUG
		printf("--------\n%s\n--------\n", buffer);
#endif
		char* start = buffer;
		char* newline = NULL;
		do
		{
			newline = strchr(start, '\n');
			if(newline == NULL)
				break;
			size_t len = newline-start+1;
			listPtr newPrimes = NULL;
			if(process->pipeReadBufCount == 0)
				newPrimes = stringToPrimes(start, len);
			else
			{
				// copy frag to pipeBuf
				char* header = process->pipeReadBuf + process->pipeReadBufCount;
				if(process->pipeReadBufCount + len > BUFSIZE_MAX)
					CERR("temporary pipe buffer buffer is too small");
				strncpy(header, start, len);
				process->pipeReadBufCount += len;
				process->pipeReadBuf[process->pipeReadBufCount] = '\0';
				newPrimes = stringToPrimes(process->pipeReadBuf, process->pipeReadBufCount);
				//clear pipeBuf
				process->pipeReadBuf[0] = '\0';
				process->pipeReadBufCount = 0;
			}
			if(process->primes == NULL)
				CERR("primes list is NULL");
			listMerge(process->primes, newPrimes);
			start += len;
		}
		while(newline != NULL);
		//copy remaining fragment to temp buf
		if(newline == NULL && start != NULL)
		{
			size_t fraglen = strnlen(start, bytesRead);
			if(fraglen > 0)
			{
				if(process->pipeReadBufCount + fraglen > BUFSIZE_MAX)
					CERR("temporary pipe buffer buffer is too small");
				strncpy(process->pipeReadBuf + process->pipeReadBufCount, start, fraglen);
				process->pipeReadBufCount += fraglen;
			}
		}
		if(server->address.sin_port > 0)
		{
			if(valueToPrime(listElemGetLast(process->primes)->val) == 0)
			{
				free(listElemGetLast(process->primes)->val);
				listElemRemoveLast(process->primes);
				time(&process->finished);
				process->status = PROCSTATUS_UNCONFIRMED;
			}
			return true;
		}
		else
			fprintf(stderr, "ERROR: server address unknown but primes ready\n");
	}
	else
		fprintf(stderr, "ERROR: read pipe but it was empty\n");
	return false;
}

void moveConfirmedPrimes(listPtr from, listPtr to, listPtr moved)
{
#ifdef DEBUG_CONFIRMATION
	printf("moveConfirmedPrimes(from, to, moved)\n");
	printf("  |from|=%u |to|=%u |moved|=%u\n",
		listLength(from), listLength(to), listLength(moved));
	listPrintStatistics(from, stdout);
	listPrintErrors(from, stdout);
	listPrintStatistics(to, stdout);
	listPrintErrors(to, stdout);
	printf("\n");
#endif
	listElemPtr e1;
	listElemPtr e2;
	for(e1 = listElemGetFirst(moved); e1; e1 = e1->next)
	{
		int64_t val1 = valueToPrime(e1->val);
		for(e2 = listElemGetFirst(from); e2; e2 = e2->next)
		{
			if(val1 == valueToPrime(e2->val))
			{
#ifdef DEBUG_CONFIRMATION
				printf("  move\n");
#endif
				listElemMove(e2, from, to);
				break;
			}
		}
	}
#ifdef DEBUG_CONFIRMATION
	printf("moveConfirmedPrimes(from, to, moved) returns:\n");
	printf("  |from|=%u |to|=%u\n", listLength(from), listLength(to));
	listPrintStatistics(from, stdout);
	listPrintStatistics(to, stdout);
	printf("\n");
#endif
}

// start a new process that will generate primes
void invokePrimesGenerator(processDataPtr process)
{
	int pid = fork();
	if(pid < 0)
		ERR("fork");

	if(pid == 0)
	{
		generatePrimes(process);
		freeProcessData(process);
		exitNormal();
	}
}

// checks if a given number is a prime
// invoked only by subprocesses
bool isPrime(int64_t number)
{
	if(number <= 1)
		return false;
	if(number == 2)
		return true;
	if(number % 2 == 0)
		return false;
	double a = sqrt((double)number);
	int64_t number_sqrt = (int64_t)ceil(a);
	int64_t i;
	for(i = 3; i <= number_sqrt; ++i)
	{
		if(number % i == 0)
			return false;
	}
	return true;
}

// copies primes to pipe and deletes the origin,
// 'out' is only used to not allocate memory over and over again,
// it is assumed to be a buffer with length >= BUFSIZE_MAX
// invoked only by subprocesses
size_t movePrimesToPipe(processDataPtr process, char* out)
{
	size_t outCount = primesToString(process->primes, out, BUFSIZE_MAX);
	if(outCount >= BUFSIZE_MAX)
		CERR("too many primes to write at once");
	out[outCount++] = '\n';
	out[outCount] = '\0';
	size_t written = bulk_write(process->pipeWrite, out, outCount);
	listElemPtr e;
	for(e = listElemGetFirst(process->primes); e; e = e->next)
		if(e->val != NULL)
			free((int64_t*)e->val);
	listClear(process->primes);
	return written;
}

// main computational procedure
// invoked only by subprocesses
void generatePrimes(processDataPtr process)
{
	pid_t pid = getpid();
	int64_t n = 0;
	const int64_t nMax = (process->primeRange-1) / 10 + 1;
	char out[BUFSIZE_MAX];

	fprintf(stdout, "P%d: started, range=[%" PRId64 ",%" PRId64 "]\n",
		pid, process->primeFrom, process->primeTo);
	int64_t i = process->primeFrom;
	int64_t checked = 0;
	int64_t discovered = 0;
	if(i <= process->primeTo)
		while(true)
		{
			if(isPrime(i))
			{
				listElemInsertEndPrime(process->primes, i);
				++discovered;
			}
			++checked;
			++n;
			if(n == nMax || i == process->primeTo)
			{
				n = 0;
				double percent = (100.0 * checked) / process->primeRange;
				fprintf(stdout, "P%d: computing: %5.1f%% %8" PRId64 "/%" PRId64 " found %7" PRId64 " primes\n",
					pid, percent, checked, process->primeRange, discovered);
			}
			if(listLength(process->primes) == PRIMEMAXCOUNT)
				movePrimesToPipe(process, out);
			else if(listLength(process->primes) > PRIMEMAXCOUNT)
				CERR("primes count larger than PRIMEMAXCOUNT");
			if(i == process->primeTo)
				break;
			++i;
		}
	fprintf(stdout, "P%d: done\n", pid);
	listElemInsertEndPrime(process->primes, 0);
	movePrimesToPipe(process, out);
}
