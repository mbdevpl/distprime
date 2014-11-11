#include "serverdata.h"

serverDataPtr allocServerData()
{
	serverDataPtr server = (serverDataPtr)malloc(sizeof(serverData));
	if(server == 0)
		ERR("malloc");

	memset(&server->address, 0, sizeof(struct sockaddr_in));
	server->hash = 0ULL;
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
	listElemPtr e;
	for(e = listElemGetFirst(server->workersActiveData); e; e = e->next)
		if(e->val != NULL) freeWorkerData((workerDataPtr)e->val);
	listFree(server->workersActiveData);
	for(e = listElemGetFirst(server->processesDoneData); e; e = e->next)
		if(e->val != NULL) freeProcessData((processDataPtr)e->val);
	listFree(server->processesDoneData);
	free(server);
}

void printServerData(serverDataPtr server)
{
	if(server == NULL)
		return;
	printf("address=%s:%d", inet_ntoa(server->address.sin_addr),
		ntohs(server->address.sin_port));
	printf(" hash=%llu", server->hash);
	if(server->primeFrom > 0 || server->primeTo > 0)
		printf(" prmieRange=[%" PRId64 ",%" PRId64 "] |primeRange|=%" PRId64 "",
			server->primeFrom, server->primeTo, server->primeRange);
	if(server->workersActive > 0 || server->processesDone > 0)
	{
		printf(" workersActive=%zu", server->workersActive);
		printf(" processesDone=%zu", server->processesDone);
	}
	printf("\n");
}

xmlNodePtr xmlNodeCreateServerData(serverDataPtr server)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "server");

	char temp[32];

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%llu", server->hash);
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
		xmlChar* content = xmlNodeGetContent(attr->children);
		if(strncmp(attrName, "hash", 5) == 0)
			server->hash = strtoull(CHARS content, NULL, 10);
		if(content != NULL)
			free(content);
	}
	if(server->hash >= HASH_MIN && server->hash <= HASH_MAX)
		return server;
	freeServerData(server);
	return NULL;

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
