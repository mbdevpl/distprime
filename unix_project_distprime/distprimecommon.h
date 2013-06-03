#ifndef DISTPRIMECOMMON_H
#define DISTPRIMECOMMON_H

#include "listfunctions.h"
#include "mbdev_unix.h"

#include <stdbool.h> // bool true false
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void createSocketOut(int* socketOut, struct sockaddr_in* addr,
		uint32_t addressOut, in_port_t portOut);

void socketOutSend(const int socketOut, char* bufferOut, const int bufferLen,
		struct sockaddr_in* addrOut, int* sentBytes);

void createSocketIn(int* socketIn, struct sockaddr_in* addr,
		uint32_t addressIn, in_port_t portIn, const int timeoutSeconds);

void socketInReceive(const int socketIn, char* bufferIn, const int bufferLen,
		struct sockaddr_in* addrSender, int* receivedBytes);

#endif // DISTPRIMECOMMON_H
