#include "distprimecommon.h"

void addressCreate(struct sockaddr_in* address, uint32_t ip, in_port_t port)
{
	memset(address, '\0', sizeof(struct sockaddr_in));
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = htonl(ip);
	address->sin_port = htons(port);
}

void socketCreate(int* socket, int timeoutSeconds, bool broadcastEnable,
		bool reuseAddress)
{
	*socket = makeSocket(PF_INET, SOCK_DGRAM);

	if(timeoutSeconds > 0)
	{
		struct timeval tv;
		tv.tv_sec = timeoutSeconds;
		tv.tv_usec = 0;
		if(setsockopt(*socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)) < 0)
			ERR("setsockopt");
	}

	if(broadcastEnable)
	{
		int broadcastEnable=1;
		if(setsockopt(*socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, (socklen_t)sizeof(broadcastEnable)) < 0)
			ERR("setsocketopt");
	}

	if(reuseAddress)
	{
		int reuseAddr = 1;
		if(setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, (socklen_t)sizeof(reuseAddr)) < 0)
			ERR("setsockopt");
	}

}

void socketBind(int socket, struct sockaddr_in* addr)
{
	if(bind(socket, (struct sockaddr*)addr, sizeof(*addr)) < 0)
		ERR("bind");
}

// TODO: slightly too long
size_t socketSend(const int socket, const struct sockaddr_in* address,
		const xmlDocPtr doc)
{
	static char buffer[BUFSIZE_MAX];
	const int bufferLen = BUFSIZE_MAX;

	size_t written = xmlToString(doc, buffer, bufferLen);

	if(written == 0)
	{
#ifdef DEBUG_IO
		printf(" * wanted to send data, but xml to string conversion failed\n");
#endif
		return 0;
	}

	size_t sentBytes = 0;
#ifdef TEST_RETRY
	bool fake = 0 == rand()%4;
	if(fake)
	{
		// this simulates that the content was sent, but lost
		sentBytes = written;
	}
	else
	{ //ELSE start
#endif
		if((sentBytes = sendto(socket, buffer, /*bufferLen*/written, 0,
				(const struct sockaddr *)address, sizeof(struct sockaddr_in))) < 0)
		{
			ERR("sendto");
		}
#ifdef TEST_RETRY
	} //ELSE end
#endif

	if(sentBytes < written)
	{
#ifdef DEBUG_IO
		printf(" * wanted to send %u bytes but sent %u", written, sentBytes);
#endif
	}

#ifdef DEBUG_IO
#ifdef TEST_RETRY
	if(fake)
		printf(" * fake");
#endif
	printf(" * sent %d bytes to %s:%d\n", sentBytes,
		inet_ntoa(address->sin_addr), ntohs(address->sin_port));
#endif

	return sentBytes;
}

// TODO: slighlty too long
size_t socketReceive(const int socket, struct sockaddr_in* senderAddress,
		xmlDocPtr* doc)
{
	static char buffer[BUFSIZE_MAX];
	const int bufferLen = BUFSIZE_MAX;
	*doc = NULL;
	socklen_t senderAddressSize = (socklen_t)sizeof(*senderAddress);
	int receivedBytes = 0;
	memset(buffer, '\0', bufferLen * sizeof(char));
	if((receivedBytes = TEMP_FAILURE_RETRY(recvfrom(socket, buffer, bufferLen, 0,
			(struct sockaddr*)senderAddress, &senderAddressSize))) < 0)
	{
		switch(errno)
		{
		case EAGAIN:
			//receivedBytes = 0;
			return 0;
		default:
			ERR("recvfrom");
		}
	}
#ifdef DEBUG_IO
	printf(" * received %d bytes from %s:%d\n", receivedBytes,
		inet_ntoa(senderAddress->sin_addr), ntohs(senderAddress->sin_port));
#endif
	if(receivedBytes == 0)
		return 0;
#ifdef DEBUG
	printf("========\n");
	printf("%s\n", buffer);
	printf("========\n");
#endif
	*doc = stringToXml(buffer, bufferLen);
	if(*doc == NULL)
	{
#ifdef DEBUG_IO
		printf(" * wanted to receive data, but string to xml conversion failed\n");
#endif
		return 0;
	}
	return receivedBytes;
}

xmlDocPtr xmlDocCreate()
{
	return xmlNewDoc(XMLCHARS "1.0");
}

xmlNodePtr xmlNodeCreateMsg()
{
	return xmlNewNode(NULL, XMLCHARS "distprimemsg");
}

xmlNodePtr xmlNodeCreatePing()
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "ping");

	return node;
}

// TODO: TOO LONG
int processXml(xmlDocPtr doc, serverDataPtr* server, workerDataPtr* worker,
		listPtr* processes)
{
	if(doc == NULL)
		return -1;
	listPtr contentTypes = listCreate();
	listPtr content = listCreate();
	preprocessXml(doc, contentTypes, content);

	*server = NULL;
	*worker = NULL;
	*processes = listCreate();

	listElemPtr elemType = listElemGetFirst(contentTypes);
	listElemPtr elem = listElemGetFirst(content);
	while(elemType)
	{
		switch((long int)elemType->val)
		{
		case MSGPART_PROCESSDATA:
			listElemInsertEnd(*processes, elem->val);
#ifdef DEBUG_NETWORK
			printProcessData((processDataPtr)elem->val);
#endif
			break;
		case MSGPART_WORKERDATA:
			if(*worker != NULL)
				return -1;
			*worker = (workerDataPtr)elem->val;
#ifdef DEBUG_NETWORK
			printWorkerData(*worker);
#endif
			break;
		case MSGPART_SERVERDATA:

			if(*server != NULL)
				return -1;
			*server = (serverDataPtr)elem->val;
#ifdef DEBUG_NETWORK
			printServerData(*server);
#endif
			break;
		default:
			fprintf(stderr, "ERROR: unsupported content type resulted from preprocessing\n");
			break;
		}
		elemType = elemType->next;
		elem = elem->next;
	}
	listFree(contentTypes);
	listFree(content);
	if(listLength(*processes) == 0)
	{
		listFree(*processes);
		*processes = NULL;
	}
	if(*server == NULL && *worker == NULL)
		return -1;
	return 0;
}

// TODO: TOO LONG
void preprocessXml(xmlDocPtr doc, listPtr contentTypes, listPtr content)
{
	if(doc == NULL)
		return;
	int msgType = commGetMsgType(doc);
	switch(msgType)
	{
	case MSG_NULL:
		fprintf(stderr, "INFO: received non-xml content\n");
		break;
	case MSG_INVALID:
		fprintf(stderr, "INFO: received invalid xml content\n");
		break;
	case MSG_VALID:
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
					fprintf(stderr, "WARNING: message contains invalid part\n");
					break;
				case MSGPART_PROCESSDATA:
					{
						processDataPtr process = xmlNodeParseProcessData(part);
						if(process == NULL)
							break;
						listElemInsertEnd(contentTypes, (data_type)MSGPART_PROCESSDATA);
						listElemInsertEnd(content, (data_type)process);
						partHandled = true;
					} break;
				case MSGPART_WORKERDATA:
					{
						workerDataPtr worker = xmlNodeParseWorkerData(part);
						if(worker == NULL)
							break;
						listElemInsertEnd(contentTypes, (data_type)MSGPART_WORKERDATA);
						listElemInsertEnd(content, (data_type)worker);
						partHandled = true;
					} break;
				case MSGPART_SERVERDATA:
					{
						serverDataPtr server = xmlNodeParseServerData(part);
						if(server == NULL)
							break;
						listElemInsertEnd(contentTypes, (data_type)MSGPART_SERVERDATA);
						listElemInsertEnd(content, (data_type)server);
						partHandled = true;
					} break;
				default:
					break;
				}
				if(!partHandled)
					fprintf(stderr, "WARNING: message part was outside of the protocol\n");
			}
		}
		break;
	default:
		break;
	}
}

xmlDocPtr stringToXml(const char* buffer, const int bufferLen)
{
	xmlDocPtr doc = xmlReadMemory(buffer, bufferLen, "noname.xml", NULL,
		XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);
	return doc;
}

size_t xmlToString(xmlDocPtr doc, char* buffer, const int bufferLen)
{
	xmlChar* xmlBuf;
	size_t xmlBufSize;

	// everything in one line
	//xmlDocDumpMemory(doc, &xmlBuf, &xmlBufSize);
	// fancy indentation
	xmlDocDumpFormatMemory(doc, &xmlBuf, (int*)&xmlBufSize, 1);

	memset(buffer, '\0', bufferLen * sizeof(char));
	if(xmlBufSize >= bufferLen)
		return 0;

	sprintf(buffer, "%s\n", CHARS xmlBuf);
	free(xmlBuf);

	return xmlBufSize;
}

int commGetMsgType(xmlDocPtr doc)
{
	if(doc == NULL)
		return MSG_NULL;

	xmlNodePtr root = xmlDocGetRootElement(doc);

	if(root == NULL
		|| strncmp(CHARS root->name, "distprimemsg", 13) != 0
		|| xmlChildElementCount(root) == 0)
		return MSG_INVALID;

	return MSG_VALID;
}

int commGetMsgpartType(xmlNodePtr part)
{
	if(part == NULL)
		return MSGPART_NULL;

	const char* partName = CHARS part->name;
	if(strncmp(partName, "process", 8) == 0)
		return MSGPART_PROCESSDATA;
	else if(strncmp(partName, "worker", 7) == 0)
		return MSGPART_WORKERDATA;
	else if(strncmp(partName, "server", 7) == 0)
		return MSGPART_SERVERDATA;
	//else if(strncmp(partName, "ping", 4) == 0)
	//	return MSGPART_PING;

	return MSGPART_INVALID;
}
