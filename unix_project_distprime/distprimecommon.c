#include "distprimecommon.h"

void createSocketOut(int* socketOut, struct sockaddr_in* addr,
		uint32_t addressOut, in_port_t portOut)
{
	*socketOut = makeSocket(PF_INET, SOCK_DGRAM);

	if(addressOut == INADDR_BROADCAST)
	{
		int broadcastEnable=1;
		if(setsockopt(*socketOut, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, (socklen_t)sizeof(broadcastEnable)) < 0)
			ERR("setsocketopt");
	}

	memset(addr, '\0', sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(addressOut);
	addr->sin_port = htons(portOut);
}

void socketOutSend(const int socketOut, char* bufferOut, const int bufferLen,
		const struct sockaddr_in* addrOut, int* sentBytes)
{
	//xmlDocPtr doc;

	*sentBytes = 0;
	if((*sentBytes = sendto(socketOut, bufferOut, bufferLen, 0,
			(const struct sockaddr *)addrOut, sizeof(struct sockaddr_in))) < 0)
	{
		ERR("sendto");
	}

	if(*sentBytes < bufferLen)
		printf(" * wanted to send %d bytes but sent less", bufferLen);

	printf(" * sent %d bytes to %s:%d\n", *sentBytes,
		inet_ntoa(addrOut->sin_addr), ntohs(addrOut->sin_port));
}

void createSocketIn(int* socketIn, struct sockaddr_in* addr,
		uint32_t addressIn, in_port_t portIn, const int timeoutSeconds)
{
	*socketIn = makeSocket(PF_INET,SOCK_DGRAM);

	int reuseAddr = 1;
	if(setsockopt(*socketIn, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, (socklen_t)sizeof(reuseAddr)) < 0)
		ERR("setsockopt");

	if(timeoutSeconds > 0)
	{
		struct timeval tv;
		tv.tv_sec = timeoutSeconds;
		tv.tv_usec = 0;
		if(setsockopt(*socketIn, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)) < 0)
			ERR("setsockopt");
	}

	memset(addr, '\0', sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(addressIn);
	addr->sin_port = htons(portIn);

	if(bind(*socketIn, (struct sockaddr*)addr, sizeof(*addr)) < 0)
		ERR("bind");
}

void socketInReceive(const int socketIn, char* bufferIn, const int bufferLen,
		struct sockaddr_in* addrSender, int* receivedBytes)
{
	socklen_t addrSenderSize = (socklen_t)sizeof(*addrSender);
	*receivedBytes = 0;
	memset(bufferIn, '\0', bufferLen * sizeof(char));
	if((*receivedBytes = recvfrom(socketIn, bufferIn, bufferLen, 0,
			(struct sockaddr*)addrSender, &addrSenderSize)) < 0)
	{
		switch(errno)
		{
		case EAGAIN:
			*receivedBytes = 0;
			return;
		default:
			ERR("recvfrom");
		}
	}

	printf(" * received %d bytes from %s:%d\n", *receivedBytes,
		inet_ntoa(addrSender->sin_addr), ntohs(addrSender->sin_port));
}

xmlDocPtr commCreateDoc()
{
	return xmlNewDoc(XMLCHARS "1.0");
}

xmlNodePtr commCreateMsgNode()
{
	return xmlNewNode(NULL, XMLCHARS "distprimemsg");
}

xmlNodePtr commCreateWorkerdataNode(int port, int processes)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "workerdata");

	char temp[32];

	//memset(temp, '\0', 32 * sizeof(char));
	//itoa(processes, temp);
	//xmlNewProp(workerNode, XMLCHARS "ip", XMLCHARS "127.0.0.1");

	memset(temp, '\0', 32 * sizeof(char));
	itoa(port, temp);
	xmlNewProp(node, XMLCHARS "port", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	itoa(processes, temp);
	xmlNewProp(node, XMLCHARS "processes", XMLCHARS temp);

	return node;
}

xmlNodePtr commCreatePrimerangeNode(int from, int to)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "primerange");

	char temp[32];

	memset(temp, '\0', 32 * sizeof(char));
	itoa(from, temp);
	xmlNewProp(node, XMLCHARS "from", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	itoa(to, temp);
	xmlNewProp(node, XMLCHARS "to", XMLCHARS temp);

	return node;
}

xmlNodePtr commCreatePrimesNode(const int64_t* primes, const int primesCount)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "prime");

	char temp[20];

	memset(temp, '\0', 20 * sizeof(char));
	itoa(primesCount, temp);
	xmlNewProp(node, XMLCHARS "count", XMLCHARS temp);

	char tempArr[20*primesCount];
	memset(tempArr, '\0', 20*primesCount * sizeof(char));

	int i;
	int pos = 0;
	int len;
	for(i = 0; i < primesCount; ++i)
	{
		memset(temp, '\0', 20 * sizeof(char));
		lltoa(primes[i], temp);
		len = strlen(temp);
		strncpy(tempArr+pos, temp, len);
		pos+=len;
		if(i == primesCount-1)
			break;
		tempArr[pos] = ',';
		++pos;
	}
	xmlNodeSetContentLen(node, XMLCHARS tempArr, strlen(tempArr));

	return node;
}

xmlDocPtr commStringToXml(char* buffer, int bufferLen)
{
	xmlDocPtr doc = xmlReadMemory(buffer, bufferLen, "noname.xml", NULL,
			XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);
	return doc;
}

void commXmlToString(xmlDocPtr doc, char* buffer, int* writtenBufferLen, int bufferLen)
{
	xmlChar* xmlBuf;
	int xmlBufSize;

	// everything in one line
	//xmlDocDumpMemory(doc, &xmlBuf, &xmlBufSize);
	// fancy indentation
	xmlDocDumpFormatMemory(doc, &xmlBuf, &xmlBufSize, 1);

	memset(buffer, '\0', bufferLen * sizeof(char));
	if(xmlBufSize >= bufferLen)
	{
		*writtenBufferLen = 0;
		return;
	}

	sprintf(buffer, "%s\n", CHARS xmlBuf);
	*writtenBufferLen = xmlBufSize;
}

int commGetMsgType(xmlDocPtr doc)
{
	if(doc == NULL)
		return MSG_NULL;

	xmlNodePtr root = xmlDocGetRootElement(doc);

	if(root == NULL
		|| strncmp(CHARS root->name, "distprimemsg", 12) != 0
		|| xmlChildElementCount(root) == 0)
		return MSG_INVALID;

	return MSG_VALID;
}

int commGetMsgpartType(xmlNodePtr part)
{
	if(part == NULL)
		return MSGPART_NULL;

	const char* partName = CHARS part->name;
	if(strncmp(partName, "workerdata", 10) == 0)
		return MSGPART_WORKERDATA;
	else if(strncmp(partName, "primerange", 10) == 0)
		return MSGPART_PRIMERANGE;
	else if(strncmp(partName, "prime", 10) == 0)
		return MSGPART_PRIME;

	return MSGPART_INVALID;
}
