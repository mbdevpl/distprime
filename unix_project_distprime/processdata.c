#include "processdata.h"

processDataPtr allocProcessData()
{
	processDataPtr process = (processDataPtr)malloc(sizeof(processData));
	if(process == 0)
		ERR("malloc");

	process->pipeRead = 0;
	process->pipeWrite = 0;
	process->pipeReadBuf = NULL;
	process->pipeReadBufCount = 0;
	process->primeFrom = 0;
	process->primeTo = 0;
	process->primeRange = 0;
	process->status = PROCSTATUS_COMPUTING;
	process->started = 0;
	process->finished = 0;
	process->primes = NULL;
	process->confirmed = NULL;

	return process;
}

void freeProcessData(processDataPtr process)
{
	if(process == NULL)
		return;
	process->pipeRead = 0;
	process->pipeWrite = 0;
	freePrimesList(process->primes);
	freePrimesList(process->confirmed);
	if(process->pipeReadBuf != NULL)
		free(process->pipeReadBuf);
	free(process);
}

void printProcessData(processDataPtr process)
{
	if(process == NULL)
		return;
	char temp[32];
	printf("prmieRange=[%" PRId64 ",%" PRId64 "] |primeRange|=%" PRId64 "",
		process->primeFrom, process->primeTo, process->primeRange);
	if(listLength(process->primes) > 0)
	{
		printf(" |primes|=%zu", listLength(process->primes));
		printf(" primes={");
		listElemPtr elem;
		for(elem = listElemGetFirst(process->primes); elem; elem = elem->next)
		{
			printf("%" PRId64 "", valueToPrime(elem->val));
			if(elem->next) printf(",");
		}
		printf("}");
	}
	if(listLength(process->confirmed) > 0)
	{
		printf(" |confirmed|=%zu", listLength(process->primes));
		printf(" confirmed={");
		listElemPtr elem;
		for(elem = listElemGetFirst(process->primes); elem; elem = elem->next)
		{
			printf("%" PRId64 "", valueToPrime(elem->val));
			if(elem->next) printf(",");
		}
		printf("}");
	}
	printf(" status=%d", process->status);
	timeToString(&process->started, temp, 32);
	printf(" started=%s", temp);
	timeToString(&process->finished, temp, 32);
	printf(" finished=%s", temp);
	if(process->pipeRead > 0 || process->pipeWrite > 0)
		printf(" pipes={in:%d,out:%d}", process->pipeRead, process->pipeWrite);
	printf("\n");
}

xmlNodePtr xmlNodeCreateProcessData(processDataPtr process, bool includePrimes)
{
	xmlNodePtr node = xmlNewNode(NULL, XMLCHARS "process");

	char temp[32];

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%" PRId64 "", process->primeFrom);
	xmlNewProp(node, XMLCHARS "primeFrom", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%" PRId64 "", process->primeTo);
	xmlNewProp(node, XMLCHARS "primeTo", XMLCHARS temp);

	memset(temp, '\0', 32 * sizeof(char));
	sprintf(temp, "%d", process->status);
	xmlNewProp(node, XMLCHARS "status", XMLCHARS temp);

	if(includePrimes && listLength(process->primes) > 0)
	{
		memset(temp, '\0', 32 * sizeof(char));
		sprintf(temp, "%zu", listLength(process->primes));
		xmlNewProp(node, XMLCHARS "primesCount", XMLCHARS temp);

		char buf[BUFSIZE_MAX];
		memset(buf, '\0', BUFSIZE_MAX * sizeof(char));
		primesToString(process->primes, buf, BUFSIZE_MAX);
		xmlNodeSetContent(node, XMLCHARS buf);
	}

	return node;
}

xmlNodePtr xmlNodeCreateProcessDataAltered(processDataPtr process,
		int64_t* fakePrimes, size_t fakePrimesCount)
{
	xmlNodePtr node = xmlNodeCreateProcessData(process, false);

	if(fakePrimesCount > 0)
	{
		char temp[32];

		memset(temp, '\0', 32 * sizeof(char));
		sprintf(temp, "%zu", fakePrimesCount);
		xmlNewProp(node, XMLCHARS "primesCount", XMLCHARS temp);

		char buf[BUFSIZE_MAX];
		memset(buf, '\0', BUFSIZE_MAX * sizeof(char));
		char* curr = buf;
		size_t i;
		for(i = 0; i < fakePrimesCount; ++i)
		{
			memset(temp, '\0', 32 * sizeof(char));
			sprintf(temp, "%" PRId64 "", fakePrimes[i]);
			size_t tempLen = strlen(temp);
			strncpy(curr, temp, tempLen);
			curr += tempLen;
			if(i == fakePrimesCount - 1)
				break;
			*curr = ',';
			curr += 1;
		}
		xmlNodeSetContent(node, XMLCHARS buf);
	}
	return node;
}

processDataPtr xmlNodeParseProcessData(xmlNodePtr node)
{
	processDataPtr process = allocProcessData();
	xmlAttrPtr attr = node->properties;
	for(;attr;attr=attr->next)
	{
		const char* attrName = CHARS attr->name;
		xmlChar* content = xmlNodeGetContent(attr->children);
		if(strncmp(attrName, "primeFrom", 10) == 0)
			process->primeFrom = strtoull(CHARS content, NULL, 10);
		else if(strncmp(attrName, "primeTo", 8) == 0)
			process->primeTo = strtoull(CHARS content, NULL, 10);
		else if(strncmp(attrName, "status", 7) == 0)
			process->status = atoi(CHARS content);
		if(content != NULL)
			free(content);
	}
	{
		xmlChar* content = xmlNodeGetContent(node);
		size_t contentLen = strlen(CHARS content);
		if(contentLen > 0)
			process->primes = stringToPrimes(CHARS content, contentLen);
		if(content != NULL)
			free(content);
	}
	process->primeRange = process->primeTo - process->primeFrom + 1;
	if(process->primeFrom >= 1 && process->primeTo >= 1
			&& process->primeFrom <= process->primeTo)
		return process;
	freeProcessData(process);
	return NULL;
}

processDataPtr matchProcess(processDataPtr match, listPtr list)
{
	if(listLength(list) == 0 || match == NULL)
		return NULL;
	listElemPtr e;
	for(e = listElemGetFirst(list); e; e = e->next)
	{
		processDataPtr p = (processDataPtr)e->val;
		if(p == NULL)
			continue;
		if(p->primeFrom == match->primeFrom && p->primeTo == match->primeTo)
			return match;
	}
	return NULL;
}

listPtr stringToPrimes(const char* buffer, int bufferLen)
{
	listPtr primes = listCreate();
	int i = 0;
	size_t count = 0;
	char* start = (char*)buffer;
	while(i < bufferLen)
	{
		char* nextChar = strchr(start,',');
		if(nextChar != NULL && nextChar-buffer+1 < bufferLen)
			*nextChar = '\0';
		int64_t prime = (int64_t)strtoull(start, NULL, 10);
		listElemInsertEndPrime(primes, prime);
		++count;
		if(nextChar == NULL)
			break;
		i += nextChar+1-start;
		start = nextChar+1;
	}
	return primes;
}

size_t primesToString(listPtr primes, char* buffer, const int bufferLen)
{
	if(primes == NULL)
		return 0;
	size_t primesCount = listLength(primes);
	if(primesCount == 0)
		return 0;

	char temp[20];

	int pos = 0;
	int len;
	listElemPtr elem;
	for(elem = primes->first; elem; elem = elem->next)
	{
		int64_t prime = valueToPrime(elem->val);
		// copy prime to temporary buffer
		memset(temp, '\0', 20 * sizeof(char));
		sprintf(temp, "%" PRId64 "", prime);
		len = strnlen(temp, 21);
		if(pos+len >= bufferLen)
			CERR("text buffer for primes is too small");
		strncpy(buffer+pos, temp, len);
		pos+=len;
		if(elem->next != NULL)
		{
			buffer[pos] = ',';
			++pos;
		}
	}
	return pos;
}

void freePrimesList(listPtr primes)
{
	if(primes == NULL)
		return;
	listElemPtr e;
	for(e = listElemGetFirst(primes); e; e = e->next)
		if(e->val != NULL)
			free(e->val);
	listFree(primes);
}

void listElemInsertEndPrime(listPtr primes, int64_t value)
{
	int64_t* prime = (int64_t*)malloc(sizeof(int64_t));
	*prime = value;
	listElemInsertEnd(primes, (data_type)prime);
}

time_t stringToTime(const char* buffer, const int bufferLen)
{
	struct tm timeData; // = getdate(buffer);
	strptime(buffer, "%Y/%m/%d %H:%M.%S", &timeData);
	return mktime(&timeData);
}

size_t timeToString(const time_t* time, char* buffer, const int bufferLen)
{
	memset(buffer, '\0', bufferLen * sizeof(char));
	struct tm *__restrict__ timeData = gmtime(time);
	size_t result = strftime(buffer, 32, "%Y/%m/%d %H:%M.%S", timeData);
	//free(local);
	return result;
}
