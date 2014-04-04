#ifdef _WIN32
#include <assert.h>
#include <iostream>
#include <WinSock2.h>
#include "UDPSocket.h"

using namespace std;

void sleep(int millis) {
	Sleep(millis);
}

UDPSocket::UDPSocket() :
	socketFD(0)
{}

bool UDPSocket::open(uint16_t port) {
	assert(!isOpen());
	socketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketFD <= 0) {
		cerr << "Could not create socket" << endl;
		socketFD = 0;
		return false;
	}

	if (port != ANY_PORT) {
		// bind to port
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons((unsigned short) port);
		if (bind(socketFD, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0) {
			printf("failed to bind socket\n");
			close();
			return false;
		}
	}

	return true;
}

bool UDPSocket::connect(string ip, uint16_t port) {
	assert(isOpen());

	struct sockaddr target;
	struct sockaddr_in *target4 = (sockaddr_in *) &target;

	target4->sin_family = AF_INET;
	target4->sin_port = htons(port);
	target4->sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	memset(target4->sin_zero, 0, sizeof(target4->sin_zero));

	if (::connect(socketFD, &target, sizeof(target)) < 0) {
		cout << "Unable to connect!" << endl;
		close();
		return false;
	}
	return true;
}

bool UDPSocket::connect(uint32_t ip, uint16_t port) {
	assert(isOpen());
	struct sockaddr target;
	struct sockaddr_in *target4 = (sockaddr_in *) &target;

	target4->sin_family = AF_INET;
	target4->sin_port = htons(port);
	target4->sin_addr.S_un.S_addr = htonl(ip);
	memset(target4->sin_zero, 0, sizeof(target4->sin_zero));

	if (::connect(socketFD, &target, sizeof(target)) < 0) {
		cout << "Unable to connect!" << endl;
		close();
		return false;
	}
	return true;
}

int UDPSocket::send(const Address &to, const uint8_t *data, int len) {
	assert(data);
	assert(len > 0);
	assert(len <= MAX_UDP_SIZE);
	assert(isOpen());
	assert(to.GetAddress() != 0);
	assert(to.GetPort() != 0);

	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(to.GetAddress());
	address.sin_port = htons((unsigned short) to.GetPort());

	int sent_bytes = sendto(socketFD, (const char*)data, len, 0, (sockaddr*)&address, sizeof(sockaddr_in));
	assert(sent_bytes == len);

	return sent_bytes;
}

int UDPSocket::receive(Address &from, uint8_t *data, int maxLen) {
	assert(data);
	assert(maxLen > 0);
	assert(isOpen());
	sockaddr_in fromsa;
	int fromLength = sizeof(fromsa);

	int received_bytes = recvfrom(socketFD, (char*)data, maxLen, 0, (sockaddr*)&fromsa, &fromLength);

	if (received_bytes <= 0)
		return 0;

	unsigned int address = ntohl(fromsa.sin_addr.s_addr);
	unsigned short port = ntohs(fromsa.sin_port);

	from = Address(address, port);

	return received_bytes;
}

void UDPSocket::close() {
	if (isOpen()) {
		closesocket(socketFD);
		socketFD = 0;
	}
}

UDPSocket::~UDPSocket() {
	close();
}

#endif