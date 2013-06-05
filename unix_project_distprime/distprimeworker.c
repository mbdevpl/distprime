
#include "distprimecommon.h"

void usage();
int main(int argc, char** argv);
void processValidMsg(xmlDocPtr doc, const in_addr_t sender);
void invokePrimesGenerator(int64_t primeFrom, int64_t primeTo, const in_addr_t sender);
bool isPrime(int64_t number);
void generatePrimes(struct list** primes, int64_t primeFrom, int64_t primeTo);

static volatile int status = STATUS_IDLE;

static volatile in_port_t serverPort = 0;

static volatile int myPort = 0;

static volatile int myProcesses = 0;

void usage()
{
	fprintf(stderr, "USAGE: distprimeworker port subprocesses_count\n");
	exitNormal();
}

void socketOutSendHandshake(const int socketOut, const struct sockaddr_in* addrOut,
		int portIn, int processes)
{
	char bufferOut[BUFSIZE_MAX];
	int bytesOut = 0;

	xmlNodePtr workerNode = commCreateWorkerdataNode(portIn, processes);
	xmlNodePtr rootNode = commCreateMsgNode();
	xmlAddChild(rootNode, workerNode);
	xmlDocPtr doc = commCreateDoc();
	xmlDocSetRootElement(doc, rootNode);
	int bufferOutLen = 0;
	commXmlToString(doc, bufferOut, &bufferOutLen, BUFSIZE_MAX);
	xmlFreeDoc(doc);
	//printf("sending %s\n", bufferOut);

	// send handshake to server
	socketOutSend(socketOut, bufferOut, bufferOutLen, addrOut, &bytesOut);
}

void socketOutSendPrimes(const int socketOut, const struct sockaddr_in* addrOut,
		struct list* primes)
{
	int primesCount = listLength(&primes);

	struct list* currPrime = primes;
	int64_t primesFrag[50];

	while(primesCount > 0)
	{
		int i;
		int max = primesCount > 50 ? 50 : primesCount;
		for(i = 0; i < max; ++i)
		{
			primesFrag[i] = currPrime->val;
			currPrime = currPrime->next;
		}

		xmlDocPtr doc = commCreateDoc();
		xmlNodePtr root = commCreateMsgNode();
		xmlNodePtr workerNode = commCreateWorkerdataNode(myPort, myProcesses);
		int currCount = 50;
		if(primesCount < 50) currCount = primesCount;
		xmlNodePtr primesNode = commCreatePrimesNode(primesFrag, currCount);

		xmlDocSetRootElement(doc, root);
		xmlAddChild(root, workerNode);
		xmlAddChild(root, primesNode);

		// convert xml tree to plaintext
		int bufferOutLen = 0;
		char bufferOut[BUFSIZE_MAX];
		commXmlToString(doc, bufferOut, &bufferOutLen, BUFSIZE_MAX);
		xmlFreeDoc(doc);

		// send data to server
		int bytesOut = 0;
		socketOutSend(socketOut, bufferOut, bufferOutLen, addrOut, &bytesOut);

		primesCount -= 50;
	}
}

int main(int argc, char** argv)
{
	if(argc != 3)
		usage();

	const int port = atoi(argv[1]);
	const int processes = atoi(argv[2]);

	if(processes < 1 || port < 1024 || port > 65535)
		usage();

	printf("distprimeworker\n");
	printf("Distributed prime numbers generation worker\n\n");

	srand(getpid());

	printf("will generate using %d processes\n", processes);

	myPort = port;
	myProcesses = processes;

	int socketIn;
	struct sockaddr_in addrIn;
	in_port_t portIn = (in_port_t)(rand()%10001 + 5000);
	createSocketIn(&socketIn, &addrIn, INADDR_ANY, portIn, 5);

	printf("receiving on port %d\n", portIn);

	int socketOut;
	struct sockaddr_in addrOut;
	//uint32_t addrOut = INADDR_BROADCAST;
	in_port_t portOut = (in_port_t)port;
	createSocketOut(&socketOut, &addrOut, INADDR_BROADCAST, portOut);

	printf("broadcasting to port %d...\n", portOut);

	serverPort = portOut;

	// handshake
	socketOutSendHandshake(socketOut, &addrOut, portIn, processes);

	char bufferIn[BUFSIZE_MAX];
	struct sockaddr_in addrSender;
	int bytesIn = 0;

	while(true)
	{

		// wait for a response from server
		socketInReceive(socketIn, bufferIn, BUFSIZE_MAX, &addrSender, &bytesIn);

		if(bytesIn == 0)
		{
			switch(status)
			{
			case STATUS_IDLE:
				printf("no distprime server available...\n");
				exitNormal();
			case STATUS_COMPUTING:
			case STATUS_FINISHED:
			default:
				break;
			}
			continue;
		}

		xmlDocPtr doc = commStringToXml(bufferIn, bytesIn);

		int msgType = commGetMsgType(doc);

		switch(msgType)
		{
		case MSG_NULL:
			printf("received non-xml content\n");
			break;
		case MSG_INVALID:
			printf("received invalid xml content\n");
			break;
		case MSG_VALID:
			processValidMsg(doc, ntohl(addrSender.sin_addr.s_addr));
			break;
		default:
			break;
		}

		if(doc != NULL)
			xmlFreeDoc(doc);

		printf("====\n%s\n====\n", bufferIn);
	}

	closeSocket(socketOut);
	closeSocket(socketIn);
	xmlCleanupParser();

	return EXIT_SUCCESS;
}

void processValidMsg(xmlDocPtr doc, const in_addr_t sender)
{
	xmlNodePtr root = xmlDocGetRootElement(doc);
	xmlNodePtr part = root->children;
	for(;part;part=part->next)
	{
		int partType = commGetMsgpartType(part);

		bool partHandled = false;
		switch(partType)
		{
		case MSGPART_NULL:
		case MSGPART_INVALID:
			printf("message contains invalid part\n");
			break;
		case MSGPART_PRIMERANGE:
			{
				int64_t primeFrom = -1;
				int64_t primeTo = -1;

				xmlAttrPtr attr = part->properties;
				for(;attr;attr=attr->next)
				{
					const char* attrName = CHARS attr->name;

					if(strncmp(attrName, "from", 4) == 0)
						primeFrom = (int64_t)atoll(CHARS xmlNodeGetContent(attr->children));
					else if(strncmp(attrName, "to", 2) == 0)
						primeTo = (int64_t)atoll(CHARS xmlNodeGetContent(attr->children));
				}

				if(primeFrom < 0 || primeTo < 0 || primeFrom > primeTo)
					break;

				partHandled = true;

				// start new process that will generate primes
				invokePrimesGenerator(primeFrom, primeTo, sender);
			} break;
		default:
			break;
		}
		if(!partHandled)
			printf("received message outside of protocol\n");
	}
}

void invokePrimesGenerator(int64_t primeFrom, int64_t primeTo, const in_addr_t sender)
{
	int pid = fork();
	if(pid < 0)
		ERR("fork");

	switch(pid)
	{
	case 0:
		{
			struct list* primes = NULL;
			generatePrimes(&primes, primeFrom, primeTo);
			printLnLen(&primes);

			int socketOut;
			struct sockaddr_in addrOut;
			createSocketOut(&socketOut, &addrOut, sender, serverPort);

			printf("sending primes to server...\n");
			socketOutSendPrimes(socketOut, &addrOut, primes);

			closeSocket(socketOut);

			exitNormal();
		}
		break;
	default:
		printf("started primes generator process, range=[%lld,%lld]\n", primeFrom, primeTo);
		status = STATUS_COMPUTING;
		break;
	}
}

bool isPrime(int64_t number)
{
	if (number <= 1)
		return false;
	double a = sqrt((double)number);
	int64_t number_sqrt = (int64_t)ceil(a);
	int64_t i;
	for (i = 2; i <= number_sqrt; ++i)
	{
		if (number % i == 0)
			return false;
	}
	return true;
}

void generatePrimes(struct list** primes, int64_t primeFrom, int64_t primeTo)
{
	struct list* lastPrime;
	int n = 0;
	int64_t i;
	int64_t checked = 0;
	int64_t range = primeTo - primeFrom + 1;
	for(i=primeFrom; i<=primeTo; ++i)
	{
		if(isPrime(i))
		{
			if(*primes == NULL)
			{
				*primes = createElem((int)i);
				lastPrime = *primes;
			}
			else
			{
				insertAfter(&lastPrime, 1, (int)i);
				lastPrime = lastPrime->next;
			}
		}
		++checked;
		++n;

		if(n == 1000000)
		{
			n = 0;
			printf("%lld%% %lldM/%lldM\n",
				(100*checked)/range, checked/1000000, range/1000000);
		}
	}
	printf("100%%\n");
}
