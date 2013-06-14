#include "serverdata.h"

serverDataPtr allocServerData()
{
	serverDataPtr server = (serverDataPtr)malloc(sizeof(serverData));
	if(server == 0)
		ERR("malloc");

	memset(&server->address, 0, sizeof(struct sockaddr_in));
	server->hash = 0;
	server->primeFrom = 0;
	server->primeTo = 0;
	server->primeRange = 0;
	server->primesCount = 0;
	server->workerIdSentLast = 0;
	server->workersActive = 0;
	server->workersActiveData = listCreate();
	server->processesDone = 0;
	server->processesDoneData = listCreate();

	return server;
}

void freeServerData(serverDataPtr server)
{
	listFree(server->workersActiveData);
	listFree(server->processesDoneData);
	free(server);
}

void printServerData(serverDataPtr server)
{
	if(server == NULL)
		return;
	printf("address=%s:%d", inet_ntoa(server->address.sin_addr),
		ntohs(server->address.sin_port));
	printf(" hash=%u", server->hash);
	if(server->primeFrom > 0 || server->primeTo > 0)
		printf(" prmieRange=[%lld,%lld] |primeRange|=%lld",
			server->primeFrom, server->primeTo, server->primeRange);
	if(server->workersActive > 0 || server->processesDone > 0)
	{
		printf(" workersActive=%u", server->workersActive);
		printf(" processesDone=%u", server->processesDone);
	}
	printf("\n");
}

xmlNodePtr xmlNodeCreateServerData(serverDataPtr server)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "server");

	char temp[32];

	//memset(temp, '\0', 32 * sizeof(char));
	//sprintf(temp, "%u", server->workers);
	//itoa(server->workers, temp);
	//xmlNewProp(node, XMLCHARS "workers", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%u", server->hash);
	//itoa(server->hash, temp);
	xmlNewProp(node, XMLCHARS "hash", XMLCHARS temp);

	return node;
}

serverDataPtr xmlNodeParseServerData(xmlNodePtr node)
{
	serverDataPtr server = allocServerData();

	xmlAttrPtr attr = node->properties;
	for(;attr;attr=attr->next)
	{
		const char* attrName = CHARS attr->name;
		//if(strncmp(attrName, "workers", 8) == 0)
		//	server->workersActive = strtoul(CHARS xmlNodeGetContent(attr->children), NULL, 10);
		//else
		if(strncmp(attrName, "hash", 5) == 0)
			server->hash = strtoul(CHARS xmlNodeGetContent(attr->children), NULL, 10);
	}

	if(server->hash < 1000000)
	{
		//freeServerData(server);
		return NULL;
	}

	return server;
}

workerDataPtr matchActiveWorker(serverDataPtr server, workerDataPtr match)
{
	listElemPtr elem;
	for(elem = listElemGetFirst(server->workersActiveData); elem; elem = elem->next)
	{
		workerDataPtr active = (workerDataPtr)elem->val;
		if(active->hash == match->hash && active->id == match->id
			&& active->address.sin_port == active->address.sin_port)
			return active;
	}
	return NULL;
}
