
#include "distprimecommon.h"

#define FIRSTLIMIT 4000000LL

void usage();
int main(int argc, char** argv);
void processValidMsg(xmlDocPtr doc, const in_addr_t sender);
int64_t getPrimeRangeEnd(const int64_t rangeStart);
void savePrimes(struct list** head, const char* filename);

static volatile int64_t primeRangeFrom;

static volatile int64_t primeRangeTo;

static volatile int64_t primeSentLast;

static volatile char* outputFile;

void usage()
{
	fprintf(stderr, "USAGE: distprime prime_from prime_to output_file\n");
	exitNormal();
}

void socketOutSendPrimesRange(const int socketOut, const struct sockaddr_in* addrOut,
		int64_t primeFrom, int64_t primeTo)
{
	xmlDocPtr doc = commCreateDoc();
	xmlNodePtr root = commCreateMsgNode();
	xmlNodePtr primerange = commCreatePrimerangeNode(primeFrom, primeTo);
	xmlDocSetRootElement(doc, root);
	xmlAddChild(root, primerange);

	char bufferOut[BUFSIZE_MAX];
	int bytesOut = 0;
	commXmlToString(doc, bufferOut, &bytesOut, BUFSIZE_MAX);

	//memset(bufferOut, 0, BUFSIZE_MAX * sizeof(char));
	//sprintf(bufferOut, "distprime primeFrom=%lld primeTo=%lld", primeFrom, primeTo);

	int bytesSent = 0;
	while(bytesOut != bytesSent)
		socketOutSend(socketOut, bufferOut, bytesOut, addrOut, &bytesSent);

	xmlFreeDoc(doc);
}

int main(int argc, char** argv)
{
	if(argc != 4)
		usage();

	const int64_t primeFrom = (int64_t)atoll(argv[1]);
	const int64_t primeTo = (int64_t)atoll(argv[2]);
	/*const char**/ outputFile = argv[3];

	if(primeFrom < 1 || primeTo < 1 || primeFrom > primeTo)
		usage();

	printf("distprime\n");
	printf("Distributed prime numbers generation server\n\n");

//	{
//		int64_t i;
//		for(i = 1; i < 10; i += 1)
//			printf("[%lld,%lld]\n", i, getPrimeRangeEnd(i));
//		for(i = 1; i < 200000000; i += 10000000)
//			printf("[%lld,%lld]\n", i, getPrimeRangeEnd(i));
//	}

	srand(getpid());

	primeRangeFrom = primeFrom;
	primeRangeTo = primeTo;
	primeSentLast = primeRangeFrom - 1;

	int socketIn;
	struct sockaddr_in addrIn;
	in_port_t portIn = (in_port_t)8765;
	createSocketIn(&socketIn, &addrIn, INADDR_ANY, portIn, 5);

	printf("receiving on port %d\n", portIn);

	//if(listen(socket, 5) < 0)
	//	ERR("listen");

	//int descriptor = 0;
	//if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
	//	ERR("accept");

	char bufferIn[BUFSIZE_MAX];
	struct sockaddr_in addrSender;
	//socklen_t sender_addr_size = sizeof(sender_addr);

	while(true)
	{
		int bytesIn = 0;
		socketInReceive(socketIn, bufferIn, BUFSIZE_MAX, &addrSender, &bytesIn);

		if(bytesIn == 0)
		{
			printf("waiting for clients...\n");
			continue;
		}

		xmlDocPtr doc = commStringToXml(bufferIn, bytesIn);

		//commXmlToString(doc, bufferIn, &bytescount, BUFSIZE_MAX);
		//xmlFreeDoc(doc);
		//doc = commStringToXml(bufferIn, bytescount);

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

//		xmlNodePtr rootNode = xmlDocGetRootElement(doc);
//		printf("name = %s\n", (const char*)rootNode->name);
//		printf("children pointer = %d\n", (int)(void*)rootNode->children);
//		xmlNodePtr child = rootNode->last;
//		const char* childName = (const char*)child->name;
//		printf("child = %s\n", childName);
//		printf("child content = %s\n", (char*) xmlNodeGetContent(child));
//		printf("child is text = %d\n", xmlNodeIsText(child));
//		printf("root content = %s\n", (char*) xmlNodeGetContent(rootNode));
//		printf("root children count %ld\n", xmlChildElementCount(rootNode));
//		printf("root is text = %d\n", xmlNodeIsText(rootNode));

		if(msgType != MSG_VALID)
			printf("====\n%s\n====\n", bufferIn);

		if(doc != NULL)
			xmlFreeDoc(doc);
	}

	closeSocket(socketIn);
	xmlCleanupParser();

	return EXIT_SUCCESS;
}

void processValidMsg(xmlDocPtr doc, const in_addr_t senderAddress)
{
	int workerProcesses = -1;
	int workerPort = -1;
	int primeCount = -1;

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
		case MSGPART_WORKERDATA:
			{
				xmlAttrPtr attr = part->properties;
				for(;attr;attr=attr->next)
				{
					const char* attrName = CHARS attr->name;
					if(strncmp(attrName, "port", 4) == 0)
						workerPort = atoi(CHARS xmlNodeGetContent(attr->children));
					else if(strncmp(attrName, "processes", 9) == 0)
						workerProcesses = atoi(CHARS xmlNodeGetContent(attr->children));
					//printf("found attr %s\n", attrName);
				}
				if(workerProcesses < 0 || workerPort < 0)
					break;
				partHandled = true;
				printf(" added new worker port=%d processes=%d\n", workerPort, workerProcesses);
			} break;
		case MSGPART_PRIME:
			{
				xmlAttrPtr attr = part->properties;
				for(;attr;attr=attr->next)
				{
					const char* attrName = CHARS attr->name;
					if(strncmp(attrName, "count", 4) == 0)
						primeCount = atoi(CHARS xmlNodeGetContent(attr->children));
				}
				if(primeCount < 0)
					break;
				printf(" received %d primes\n", primeCount);

				char* contents = (char*)xmlNodeGetContent(part);
				struct list* primes = NULL;
				struct list* lastPrime = NULL;
				int contentsLen = strnlen(contents, BUFSIZE_MAX);
				if(contentsLen == BUFSIZE_MAX || contentsLen == 0)
					break;
				int i = 0;
				//char* currentChar = contents;
				while(i < contentsLen)
				{
					char* nextChar = strchr(contents,',');
					//int len = nextChar - currentChar;
					//char fragment[len+1];
					//fragment[len] = '\0';
					//strncpy(fragment, currentChar,
					if(nextChar != NULL)
						*nextChar = '\0';
					int64_t prime = atoll(contents);
					if(primes == NULL)
					{
						primes = createElem(prime);
						lastPrime = primes;
					}
					else
					{
						insertAfter(&lastPrime, 1, prime);
						lastPrime = lastPrime->next;
					}
					if(nextChar == NULL)
						break;
					i += nextChar+1-contents;
					contents = nextChar+1;
				}
				printf("parsed primes list\n");
				printLnLen(&primes);
				partHandled = true;
				savePrimes(&primes,(const char*)outputFile);
				freeList(&primes);
			} break;
		default:
			break;
		}
		if(!partHandled)
			printf("message part was outside of protocol\n");
	}

	if(workerProcesses < 0 || workerPort < 0)
		return;

	int socketOut;
	struct sockaddr_in addrOut;
	//in_port_t portOut = (in_port_t)rand()%10001 + 10000;
	createSocketOut(&socketOut, &addrOut, senderAddress, workerPort);

	printf("sending prmies range\n");

	int64_t from = primeSentLast + 1;
	int64_t to = getPrimeRangeEnd(from);

	socketOutSendPrimesRange(socketOut, &addrOut, from, to);

	primeSentLast = to;

	closeSocket(socketOut);
}

int64_t getPrimeRangeEnd(const int64_t rangeStart)
{
	if(rangeStart < 1 || rangeStart < primeRangeFrom)
		return -1LL;
	if(rangeStart == 1)
	{
		if(FIRSTLIMIT > primeRangeTo)
			return primeRangeTo;
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

	//const double limit2 = sqrt((double)FIRSTLIMIT) * 0.5 * FIRSTLIMIT;
	//printf("limit2=%f\n", limit2);

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
		return -1LL;

	int64_t rangeEnd;

	if(delta > 0)
		rangeEnd = (int64_t)llround((-b+sqrt(delta))/(2*a)) + rangeStart;
	else if(delta == 0)
		rangeEnd = -b/(2*a) + rangeStart;

	if(rangeEnd > primeRangeFrom)
		rangeEnd = primeRangeFrom;

	return rangeEnd;
}

void savePrimes(struct list** head, const char* filename)
{
	FILE* file = fopen(filename, "a");
	struct list*temp;
	fprintf(file, "primes");
	for(temp=*head;temp;)
	{
		fprintf(file, " %lld",temp->val);
		//if(
		(temp=temp->next);
		//&&(temp!=*head))
		//	printf(", ");
		if(temp==*head)return;
	}
	fprintf(file, "\n");
	fclose(file);
}
