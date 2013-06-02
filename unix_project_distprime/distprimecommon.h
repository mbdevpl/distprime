#ifndef DISTPRIMECOMMON_H
#define DISTPRIMECOMMON_H

#include "mbdev_unix.h"

void createSocketOut(int* socketOut, struct sockaddr_in* addr,
							uint32_t addressOut, in_port_t portOut);

void createSocketIn(int* socketIn, struct sockaddr_in* addr,
						  uint32_t addressIn, in_port_t portIn, int timeoutSeconds);

#endif // DISTPRIMECOMMON_H
