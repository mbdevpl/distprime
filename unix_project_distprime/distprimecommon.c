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

void socketOutSend(const int socketOut, char* bufferOut, const int bufferLen,
							struct sockaddr_in* addrOut, int* sentBytes)
{
	*sentBytes = 0;
	if((*sentBytes = sendto(socketOut, bufferOut, bufferLen, 0,
			(struct sockaddr *)addrOut, sizeof(struct sockaddr_in))) < 0)
	{
		ERR("sendto");
	}

	printf("sent %d bytes to %d:%d\n", *sentBytes,
		ntohl(addrOut->sin_addr.s_addr), ntohs(addrOut->sin_port));
}

void createSocketIn(int* socketIn, struct sockaddr_in* addr,
		uint32_t addressIn, in_port_t portIn, const int timeoutSeconds)
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

void socketInReceive(const int socketIn, char* bufferIn, const int bufferLen,
							struct sockaddr_in* addrSender, int* receivedBytes)
{
	socklen_t addrSenderSize = (socklen_t)sizeof(*addrSender);
	*receivedBytes = 0;
	memset(bufferIn, '\0', bufferLen * sizeof(char));
	if((*receivedBytes = recvfrom(socketIn, bufferIn, bufferLen, 0,
			(struct sockaddr*)addrSender, &addrSenderSize)) < 0)
	{
		switch(errno)
		{
		case EAGAIN:
			*receivedBytes = 0;
			return;
		default:
			ERR("recvfrom");
		}
	}

	printf("received %d bytes from %s:%d\n", *receivedBytes,
		inet_ntoa(addrSender->sin_addr), ntohs(addrSender->sin_port));
}
