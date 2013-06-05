#ifndef DISTPRIMECOMMON_H
#define DISTPRIMECOMMON_H

#include "listfunctions.h"
#include "mbdev_unix.h"

#include <stdbool.h> // bool true false
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE_MAX 1024

#define MSG_NULL 1
#define MSG_INVALID 2
#define MSG_VALID 3

#define MSGPART_NULL MSG_NULL
#define MSGPART_INVALID MSG_INVALID
#define MSGPART_WORKERDATA 11
#define MSGPART_PRIMERANGE 12
#define MSGPART_PRIME 13

#define STATUS_IDLE 1
#define STATUS_COMPUTING 2
#define STATUS_FINISHED 3

#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#define MY_ENCODING "UTF-8"
#define XMLCHARS BAD_CAST
#define CHARS (const char*)

void createSocketOut(int* socketOut, struct sockaddr_in* addr,
		uint32_t addressOut, in_port_t portOut);

void socketOutSend(const int socketOut, char* bufferOut, const int bufferLen,
		const struct sockaddr_in* addrOut, int* sentBytes);

void createSocketIn(int* socketIn, struct sockaddr_in* addr,
		uint32_t addressIn, in_port_t portIn, const int timeoutSeconds);

void socketInReceive(const int socketIn, char* bufferIn, const int bufferLen,
		struct sockaddr_in* addrSender, int* receivedBytes);

xmlDocPtr commCreateDoc();

xmlNodePtr commCreateMsgNode();

xmlNodePtr commCreateWorkerdataNode(int port, int processes);

xmlNodePtr commCreatePrimerangeNode(int from, int to);

xmlNodePtr commCreatePrimesNode(const int64_t* primes, const int primesCount);

xmlDocPtr commStringToXml(char* buffer, int bufferLen);

void commXmlToString(xmlDocPtr doc, char* buffer, int* writtenBufferLen, int bufferLen);

int commGetMsgType(xmlDocPtr doc);

int commGetMsgpartType(xmlNodePtr part);

#endif // DISTPRIMECOMMON_H
