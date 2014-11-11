#ifndef DISTPRIMECOMMON_H
#define DISTPRIMECOMMON_H

#include "defines.h"

#include "processdata.h"
#include "workerdata.h"
#include "serverdata.h"

#define SERVER_PORT ((in_port_t)2000)

void addressCreate(struct sockaddr_in* address, uint32_t ip, in_port_t port);
void socketCreate(int* socket, int timeoutSeconds, bool broadcastEnable,
		bool reuseAddress);
void socketBind(int socket, struct sockaddr_in* addr);
size_t socketSend(const int socket, const struct sockaddr_in* address,
		const xmlDocPtr doc);
size_t socketReceive(const int socket, struct sockaddr_in* senderAddress,
		xmlDocPtr* doc);

xmlDocPtr xmlDocCreate();
xmlNodePtr xmlNodeCreateMsg();
xmlNodePtr xmlNodeCreatePing();

int processXml(xmlDocPtr doc, serverDataPtr* server, workerDataPtr* worker,
		listPtr* processes);
void preprocessXml(xmlDocPtr doc, listPtr contentTypes, listPtr content);

xmlDocPtr stringToXml(const char* buffer, const int bufferLen);
size_t xmlToString(xmlDocPtr doc, char* buffer, const int bufferLen);

int commGetMsgType(xmlDocPtr doc);
int commGetMsgpartType(xmlNodePtr part);

#endif // DISTPRIMECOMMON_H
