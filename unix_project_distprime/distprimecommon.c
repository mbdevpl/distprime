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

void createSocketIn(int* socketIn, struct sockaddr_in* addr,
						  uint32_t addressIn, in_port_t portIn, int timeoutSeconds)
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
