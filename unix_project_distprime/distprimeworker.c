
#include "distprimecommon.h"
#include "workerdata.h"
#include "serverdata.h"

#define NETWORK_TIMEOUT 5 // seconds

void usage();
int main(int argc, char** argv);
void workerLoop(workerDataPtr myData, serverDataPtr myServerData);
void invokePrimesGenerator(processDataPtr process);
bool isPrime(int64_t number);
void generatePrimes(processDataPtr process);

void usage()
{
	fprintf(stderr, "USAGE: distprimeworker server_port subprocesses_count\n");
	fprintf(stderr, "  server_port must be a valid port number\n");
	fprintf(stderr, "  subprocesses_count must be a positive number\n");
	fprintf(stderr, "EXAMPLE: distprimeworker 8765 8\n");
	exitNormal();
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

void socketSendPrimes(const int socket, workerDataPtr worker,
		serverDataPtr server, processDataPtr process)
{
	//size_t i;
	//for(i = 0; i < worker->processes; ++i)
	//{
	//processDataPtr process = worker->processesData[i];

	//int primesCount = listLength(process->primes);

	//listElemPtr currPrime = process->primes->first;
	//int64_t primesFrag[50];

	//while(primesCount > 0)
	//{
	//size_t j;
	//size_t max = primesCount > 50 ? 50 : primesCount;
	//for(j = 0; j < max; ++j)
	//{
	//	primesFrag[j] = valueToPrime(currPrime->val);
	//	currPrime = currPrime->next;
	//}

	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);
	//int currCount = 50;
	//if(primesCount < 50) currCount = primesCount;
	xmlNodePtr processNode = xmlNodeCreateProcessData(process, true);

	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, workerNode);
	xmlAddChild(msgNode, processNode);

	// send data to server
	size_t bytesSent = socketSend(socket, &server->address, doc);
	xmlFreeDoc(doc);

	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no primes sent\n");
	//primesCount -= 50;
	//}
	//}
}

void socketSendStatus(const int socket, workerDataPtr worker,
		serverDataPtr server)
{
	xmlDocPtr doc = xmlDocCreate();
	xmlNodePtr msgNode = xmlNodeCreateMsg();
	xmlNodePtr workerNode = xmlNodeCreateWorkerData(worker);

	xmlDocSetRootElement(doc, msgNode);
	xmlAddChild(msgNode, workerNode);

	size_t i;
	for(i = 0; i < worker->processes; ++i)
	{
		xmlNodePtr processNode = xmlNodeCreateProcessData(worker->processesData[i], false);
		xmlNodeSetContent(processNode, XMLCHARS "");
		xmlAddChild(msgNode, processNode);
	}

	size_t bytesSent = socketSend(socket, &server->address, doc);
	xmlFreeDoc(doc);

	if(bytesSent == 0)
		fprintf(stderr, "ERROR: no status sent\n");
}

/*//void socketOutSendStatus(const int socketOut, const struct sockaddr_in* addrOut,
//		workerDataPtr worker)
//{
//	xmlDocPtr doc = commCreateDoc();
//	xmlNodePtr root = commCreateMsgNode();
//	xmlNodePtr workerdata = commCreateWorkerdataNode(worker);
//	xmlDocSetRootElement(doc, root);
//	xmlAddChild(root, workerdata);

//	char bufferOut[BUFSIZE_MAX];
//	int bytesOut = 0;
//	commXmlToString(doc, bufferOut, &bytesOut, BUFSIZE_MAX);

//	int bytesSent = 0;
//	while(bytesOut != bytesSent)
//		socketOutSend(socketOut, bufferOut, bytesOut, addrOut, &bytesSent);

//	xmlFreeDoc(doc);

//	if(bytesSent == 0)
//		fprintf(stderr, "ERROR: sending status\n");
//}*/

int main(int argc, char** argv)
{
	printf("distprimeworker\n");
	printf("Distributed prime numbers generation worker\n\n");

	if(argc != 3)
		usage();
	const int port = atoi(argv[1]);
	const int processes = atoi(argv[2]);
	if(processes < 1 || port < 1024 || port > 65535)
		usage();

	srand(getGoodSeed());

	setSigHandler(handlerSigchldDefault, SIGCHLD);

	workerDataPtr worker = allocWorkerData();
	worker->hash = rand()%9000000+1000000;
	worker->processes = processes;
	worker->processesData = (processDataPtr*)malloc(processes*sizeof(processDataPtr));

	serverDataPtr server = allocServerData();
	server->address.sin_port = htons((in_port_t)port);

	workerLoop(worker, server);

	while(TEMP_FAILURE_RETRY(wait(NULL)) >= 0)
		;

	freeServerData(server);
	freeWorkerData(worker);

	return EXIT_SUCCESS;
}

void moveConfirmedPrimes(listPtr from, listPtr to, listPtr moved)
{
#ifdef DEBUG_CONFIRMATION
	printf("moveConfirmedPrimes(from, to, moved)\n");
	printf("  |from|=%u |to|=%u |moved|=%u\n",
		listLength(from), listLength(to), listLength(moved));
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
#endif
}

bool updateStatusIfConfirmedAll(workerDataPtr worker)
{
	if(worker->status == STATUS_IDLE)
	return false;
	size_t i;
	bool allFinished = true;
	for(i = 0; i < worker->processes; ++i)
	{
		if(worker->processesData[i]->status != PROCSTATUS_CONFIRMED)
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
		if(worker->processesData[i]->status != PROCSTATUS_UNCONFIRMED
				&& worker->processesData[i]->status != PROCSTATUS_CONFIRMED)
			allAtLeastConfirming = false;
		if(worker->processesData[i]->status != PROCSTATUS_CONFIRMED)
			allConfirmed = false;
	}
	if(!allAtLeastConfirming || allConfirmed)
		return false;
	worker->status = STATUS_CONFIRMING;
	return true;
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
		myProcess->primes = listCreate();
	}
	return descriptorSetMax;
}

// returns socket descriptor number
int setupSocket(workerDataPtr worker)
{
	int socket = 0;
	addressCreate(&worker->address, INADDR_ANY, (in_port_t)(rand()%10001 + 5000));
	socketCreate(&socket, NETWORK_TIMEOUT, true, true);
	socketBind(socket, &worker->address);
	return socket;
}

void workerLoop(workerDataPtr myData, serverDataPtr myServerData)
{
	// setup
	int socket = setupSocket(myData);
	fd_set descriptorSet;
	int descriptorSetMax = setupProcessesAndPipes(myData);
	if(socket > descriptorSetMax)
		descriptorSetMax = socket;
	// broadcasting handshake
	socketSendHandshake(socket, myData, myServerData);
	printf("P%d: Broadcasting to port %d...\n",
		getpid(), ntohs(myServerData->address.sin_port));
	// stores address of last packet sender
	struct sockaddr_in addressSender;
	bool keepAlive = true;
	while(keepAlive)
	{
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

		if(myData->status == STATUS_IDLE && myData->id > 0)
		{
			xmlDocPtr doc = xmlDocCreate();
			xmlNodePtr msgNode = xmlNodeCreateMsg();
			xmlNodePtr workerNode = xmlNodeCreateWorkerData(myData);
			xmlDocSetRootElement(doc, msgNode);
			xmlAddChild(msgNode, workerNode);
			int bytesSent = socketSend(socket, &myServerData->address, doc);
			xmlFreeDoc(doc);
			if(bytesSent == 0)
				fprintf(stderr, "ERROR: no primes range request sent\n");
		}

		struct timeval descriptorSetTimeout;
		descriptorSetTimeout.tv_sec = 5;
		descriptorSetTimeout.tv_usec = 0;

		// select all ready descriptors
		int descriptorsToRead = 0;
		if(TEMP_FAILURE_RETRY(descriptorsToRead = select(descriptorSetMax + 1,
				&descriptorSet, NULL, NULL, &descriptorSetTimeout)) < 0)
			ERR("select()");

		if(descriptorsToRead == 0)
		{
			pid_t pid = getpid();
			switch(myData->status)
			{
			case STATUS_IDLE:
				printf("P%d: Server not available: timeout.\n", pid);
				keepAlive = false;
				break;
			case STATUS_GENERATING:
				printf("P%d: Generating primes...\n", pid);
				keepAlive = true;
				break;
			case STATUS_CONFIRMING:
				printf("P%d: Cannot confirm results: timeout.\n", pid);
				keepAlive = false;
				break;
			default:
				break;
			}
			continue;
		}

		size_t i;
		for(i = 0; i < myData->processes; ++i)
		{
			processDataPtr myProcess = myData->processesData[i];
			int pipe = myProcess->pipeRead;
			if(FD_ISSET(pipe, &descriptorSet))
			{
				FD_CLR(pipe, &descriptorSet);

				char buffer[BUFSIZE_MAX];
				memset(buffer, '\0', BUFSIZE_MAX * sizeof(char));
				size_t bytesRead = readUntil(pipe, buffer, BUFSIZE_MAX, '\n');
				if(bytesRead < 0)
					ERR("read");
				else if(bytesRead > 0)
				{
					printf(" * read %u bytes from pipe\n", bytesRead);
#ifdef DEBUG
					printf("--------\n%s\n--------\n", buffer);
#endif
					listPtr newPrimes = stringToPrimes(buffer, bytesRead);
					listMerge(myProcess->primes, newPrimes);

					if(myServerData->address.sin_port > 0)
					{
						if(valueToPrime(listElemGetLast(myProcess->primes)->val) == 0)
						{
							listElemRemoveLast(myProcess->primes);
							time(&myProcess->finished);
							myProcess->status = PROCSTATUS_UNCONFIRMED;
						}
						// send results back to the server
						socketSendPrimes(socket, myData, myServerData, myProcess);
						if(updateStatusIfGeneratedAll(myData))
							printf("P%d: Waiting for confirmation...\n", getpid());
					}
					else
						fprintf(stderr, "ERROR: server address unknown but primes ready\n");
				}
				else
					fprintf(stderr, "ERROR: read pipe but it was empty\n");
			}
		}

		if(FD_ISSET(socket, &descriptorSet))
		{
			FD_CLR(socket, &descriptorSet);

			// receive xml from the socket
			xmlDocPtr doc;
			socketReceive(socket, &addressSender, &doc);

			// parse xml into distprime objects
			serverDataPtr server;
			workerDataPtr worker;
			listPtr processesList;
			processXml(doc, &server, &worker, &processesList);

			bool msgHandled = false;
			if(server != NULL)
			{
				if(worker != NULL)
				{
					if(myData->hash == worker->hash)
					{
						if(myData->id == 0)
						{
							// received worker id from the server
							myData->id = worker->id;
							myServerData->address = addressSender;
							myServerData->hash = server->hash;
							//printf("updated my id to %u\n", myData->id);
							//printf("set server hash to %u\n", myServerData->hash);
						}
						else
							fprintf(stderr, "ERROR, received id but already have id\n");
					}
					else
						fprintf(stderr, "ERROR, unauthorized worker data sent\n");
					freeWorkerData(worker);
					worker = NULL;
				}

				if(myServerData->hash == server->hash)
				{
					if(listLength(processesList) > 0)
					{
						//processDataPtr first = (processDataPtr)processesList->first->val;
						//if(listLength(first->primes) == 0)
						//{
						if(myData->status == STATUS_IDLE)
						{
							if(listLength(processesList) == myData->processes)
							{
							// received new orders
								size_t index = 0;
								listElemPtr elem;
								for(elem = processesList->first; elem; elem = elem->next)
								{
									processDataPtr process = (processDataPtr)elem->val;
									processDataPtr myProcess = myData->processesData[index];
									time(&myProcess->started);
									myProcess->primeFrom = process->primeFrom;
									myProcess->primeTo = process->primeTo;
									myProcess->primeRange = process->primeRange;
									myProcess->status = PROCSTATUS_COMPUTING;
									invokePrimesGenerator(myProcess);
									++index;
									freeProcessData(process);
								}
								if(index < myData->processes)
									fprintf(stderr, "ERROR: not all processes launched\n");

								myData->status = STATUS_GENERATING;
								msgHandled = true;
							}
							else
								fprintf(stderr, "ERROR: incorrect number of processes\n");
						}
						else if(myData->status == STATUS_GENERATING
							|| myData->status == STATUS_CONFIRMING)
						{
							// received confirmation
							bool movedPrimes = false;
							listElemPtr elem;
							for(elem = processesList->first; elem; elem = elem->next)
							{
								processDataPtr process = (processDataPtr)elem->val;
								for(i = 0; i < myData->processes; ++i)
								{
									processDataPtr myProcess = myData->processesData[i];
									if(process->primeFrom == myProcess->primeFrom
										&& process->primeTo == myProcess->primeTo)
									{
										if(myProcess->confirmed == NULL)
											myProcess->confirmed = listCreate();
										moveConfirmedPrimes(myProcess->primes,
											myProcess->confirmed, process->primes);
										movedPrimes = true;
										if(myProcess->status == PROCSTATUS_UNCONFIRMED
											&& listLength(myProcess->primes) == 0)
											myProcess->status = PROCSTATUS_CONFIRMED;
										break;
									}
								}
							}
							if(updateStatusIfConfirmedAll(myData))
								printf("P%d: All results were confirmed.\n", getpid());
							if(movedPrimes)
								msgHandled = true;
							else
								fprintf(stderr, "ERROR: did not move any primes\n");
						}
						//}
						//else
						//{
						//}
					}
					else if(listLength(processesList) == 0)
					{
						// received ping request
						socketSendStatus(socket, myData, myServerData);
						msgHandled = true;
					}
				}
				else
					fprintf(stderr, "INFO: message from unauthorized server\n");
				freeServerData(server);
				server = NULL;
			}
			else
				fprintf(stderr, "ERROR: no server info in the message\n");


			if(doc != NULL)
				xmlFreeDoc(doc);

			if(!msgHandled)
				fprintf(stderr, "ERROR: last message was not handled\n");
		}
	}

	closeSocket(socket);
	xmlCleanupParser();
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

		//if(TEMP_FAILURE_RETRY(close(pipe)))
		//	ERR("close");

		//printf("closed pipe\n");

		//sleep(60);
		//printf("exit!\n");
		exitNormal();
	}
}

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

void generatePrimes(processDataPtr process)
{
	pid_t pid = getpid();
	int64_t n = 0;
	const int64_t nMax = (process->primeRange-1) / 15 + 1;
	char out[BUFSIZE_MAX];
	size_t outCount;

	fprintf(stdout, "P%d: started, range=[%lld,%lld]\n",
		pid, process->primeFrom, process->primeTo);
	int64_t i;
	int64_t checked = 0;
	for(i = process->primeFrom; i <= process->primeTo; ++i)
	{
		if(isPrime(i))
			listElemInsertEndPrime(process->primes, i);
		++checked;
		++n;

		if(n == nMax)
		{
			n = 0;
			double percent = (100.0 * checked) / process->primeRange;
			fprintf(stdout, "P%d: computing: %3.2f%% %lld/%lld\n",
				pid, percent, checked, process->primeRange);
		}

		if(listLength(process->primes) >= 500)
		{
			outCount = primesToString(process->primes, out, BUFSIZE_MAX);
			out[outCount++] = '\n';
			bulk_write(process->pipeWrite, out, outCount);
			listClear(process->primes);
		}
	}
	fprintf(stdout, "P%d: done\n", pid);
	listElemInsertEndPrime(process->primes, 0);

	outCount = primesToString(process->primes, out, BUFSIZE_MAX);
	out[outCount++] = '\n';
	bulk_write(process->pipeWrite, out, outCount);
}

