#include "workerdata.h"

workerDataPtr allocWorkerData()
{
	workerDataPtr worker = (workerDataPtr)malloc(sizeof(workerData));
	if(worker == 0)
		ERR("malloc");

	memset(&worker->address, 0, sizeof(struct sockaddr_in));
	worker->hash = 0;
	worker->id = 0;
	worker->processes = 0;
	worker->processesData = NULL;
	worker->status = STATUS_IDLE;
	time(&worker->statusSince);
	worker->now = worker->statusSince;

	return worker;
}

void freeWorkerData(workerDataPtr worker)
{
	worker->hash = 0ULL;
	worker->id = 0;
	if(worker->processesData != NULL)
	{
		size_t i;
		for(i = 0; i < worker->processes; ++i)
			if(worker->processesData[i] != NULL)
				freeProcessData(worker->processesData[i]);
		free(worker->processesData);
	}
	free(worker);
}

void printWorkerData(workerDataPtr data)
{
	if(data == NULL)
		return;
	printf("address=%s:%d", inet_ntoa(data->address.sin_addr),
		ntohs(data->address.sin_port));
	printf(" hash=%llu id=%u", data->hash, data->id);
	printf(" processes=%zu", data->processes);
	//printf(" primes=[%lld,%lld]", data->primeFrom, data->primeTo);

	char temp[32];

	printf(" status=%d", data->status);

	timeToString(&data->statusSince, temp, 32);
	printf(" statusSince=%s", temp);
	printf("\n");
}

xmlNodePtr xmlNodeCreateWorkerData(workerDataPtr worker)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "worker");

	char temp[32];

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%llu", worker->hash);
	xmlNewProp(node, XMLCHARS "hash", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%u", worker->id);
	xmlNewProp(node, XMLCHARS "id", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%zu", worker->processes);
	xmlNewProp(node, XMLCHARS "processes", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%d", worker->status);
	xmlNewProp(node, XMLCHARS "status", XMLCHARS temp);

	timeToString(&worker->statusSince, temp, 32);
	xmlNewProp(node, XMLCHARS "statusSince", XMLCHARS temp);

	return node;
}

workerDataPtr xmlNodeParseWorkerData(xmlNodePtr node)
{
	workerDataPtr worker = allocWorkerData();
	xmlAttrPtr attr = node->properties;
	for(;attr;attr=attr->next)
	{
		const char* attrName = CHARS attr->name;
		xmlChar* content = xmlNodeGetContent(attr->children);
		if(strncmp(attrName, "processes", 10) == 0)
			worker->processes = strtoul(CHARS content, NULL, 10);
		else if(strncmp(attrName, "status", 7) == 0)
			worker->status = atoi(CHARS content);
		else if(strncmp(attrName, "statusSince", 12) == 0)
			worker->statusSince = atoi(CHARS content);
		else if(strncmp(attrName, "hash", 5) == 0)
			worker->hash = strtoull(CHARS content, NULL, 10);
		else if(strncmp(attrName, "id", 3) == 0)
			worker->id = strtoul(CHARS content, NULL, 10);
		if(content != NULL)
			free(content);
	}
	if(worker->processes >= 1 && worker->hash >= HASH_MIN && worker->hash <= HASH_MAX)
		return worker;
	freeWorkerData(worker);
	return NULL;
}

unsigned long long getHash()
{
	return rand()%(HASH_MAX-HASH_MIN+1)+HASH_MIN;
}
