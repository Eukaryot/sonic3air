/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/Sockets.h"

#ifdef _WIN32
	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#undef ERROR

	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")

#else
	// Use POSIX sockets
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>  // Needed for getaddrinfo() and freeaddrinfo()
	#include <unistd.h> // Needed for close()
	#include <fcntl.h>	// For fcntl(), obviously

	#define SOCKET int
	#define INVALID_SOCKET -1

#endif


void Sockets::startupSockets()
{
	if (mIsInitialized)
		return;

#ifdef _WIN32
	WSADATA wsaData;
	const int result = ::WSAStartup((WORD)0x0202, &wsaData);
	RMX_CHECK(result == 0, "WSAStartup failed with error: " << result, );
#endif
	mIsInitialized = true;
}

void Sockets::shutdownSockets()
{
	if (!mIsInitialized)
		return;

#ifdef _WIN32
	::WSACleanup();
#endif
	mIsInitialized = false;
}


bool SocketAddress::operator==(const SocketAddress& other) const
{
	assureIpPort();
	other.assureIpPort();
	return (mIP == other.mIP && mPort == other.mPort);
}

uint64 SocketAddress::getHash() const
{
	assureSockAddr();
	return rmx::getMurmur2_64(mSockAddr, 16) ^ ((uint64)mPort << 48);
}

void SocketAddress::assureSockAddr() const
{
	if (!mHasSockAddr)
	{
		memset(&mSockAddr, 0, sizeof(mSockAddr));
		bool success = false;
		{
			// IPv6
			sockaddr_in6& addr = *reinterpret_cast<sockaddr_in6*>(&mSockAddr);
			addr.sin6_family = AF_INET6;
			addr.sin6_port = htons(mPort);
			success = (1 == inet_pton(addr.sin6_family, mIP.c_str(), &addr.sin6_addr));
		}
		if (!success)
		{
			// IPv4
			sockaddr_in& addr = *reinterpret_cast<sockaddr_in*>(&mSockAddr);
			addr.sin_family = AF_INET;
			addr.sin_port = htons(mPort);
			inet_pton(addr.sin_family, mIP.c_str(), &addr.sin_addr);
		}
		mHasSockAddr = true;
	}
}

void SocketAddress::assureIpPort() const
{
	if (!mHasIpPort)
	{
		if (mHasSockAddr)
		{
			char myIP[512];
			inet_ntop(reinterpret_cast<sockaddr_storage&>(mSockAddr).ss_family, &(reinterpret_cast<sockaddr_in&>(mSockAddr).sin_addr), myIP, sizeof(myIP));
			mIP = myIP;
			mPort = ntohs(reinterpret_cast<sockaddr_in&>(mSockAddr).sin_port);
		}
		else
		{
			mIP.clear();
			mPort = 0;
		}
		mHasIpPort = true;
	}
}


struct TCPSocket::Internal
{
	SOCKET mSocket = INVALID_SOCKET;
};


TCPSocket::~TCPSocket()
{
	if (nullptr != mInternal)
	{
		close();
		delete mInternal;
	}
}

bool TCPSocket::isValid() const
{
	return (nullptr != mInternal && mInternal->mSocket >= 0);
}

bool TCPSocket::connectTo(const std::string& serverAddress, uint16 serverPort)
{
	if (nullptr == mInternal)
	{
		mInternal = new Internal();
	}
	else
	{
		close();
	}

	// Resolve the server address
	addrinfo* addressInfos = nullptr;
	{
		addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;	// Needed for TCP
		hints.ai_protocol = IPPROTO_TCP;	// Use TCP

		const int result = getaddrinfo(serverAddress.c_str(), std::to_string(serverPort).c_str(), &hints, &addressInfos);
		RMX_CHECK(result == 0, "getaddrinfo failed with error: " << result, return false);
	}

	// Try all addresses until one of them works
	for (addrinfo* ptr = addressInfos; ptr != nullptr; ptr = ptr->ai_next)
	{
		// Create a socket
		mInternal->mSocket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		RMX_CHECK(mInternal->mSocket >= 0, "socket failed with error: " << mInternal->mSocket, return false);

		// Connect to server
		const int result = ::connect(mInternal->mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result >= 0)
		{
			// Success!
			break;
		}

		// Try next address
		close();
	}

	::freeaddrinfo(addressInfos);
	return (mInternal->mSocket >= 0);
}

void TCPSocket::close()
{
	if (!isValid())
		return;

#ifdef _WIN32
	int status = ::shutdown(mInternal->mSocket, SD_BOTH);
	if (status == 0)
	{
		status = ::closesocket(mInternal->mSocket);
	}
#else
	int status = shutdown(mInternal->mSocket, SHUT_RDWR);
	if (status == 0)
	{
		status = ::close(mInternal->mSocket);
	}
#endif

	mInternal->mSocket = INVALID_SOCKET;
}

bool TCPSocket::sendData(const uint8* data, size_t length)
{
	if (!isValid())
		return false;

	const int result = ::send(mInternal->mSocket, (const char*)data, (int)length, 0);
	return (result >= 0);
}

bool TCPSocket::receiveBlocking(ReceiveResult& outReceiveResult)
{
	size_t bytesRead = 0;
	while (true)
	{
		const constexpr size_t CHUNK_SIZE = 0x1000;
		outReceiveResult.mBuffer.resize(bytesRead + CHUNK_SIZE);

		const int result = ::recv(mInternal->mSocket, (char*)&outReceiveResult.mBuffer[bytesRead], CHUNK_SIZE, 0);
		if (result > 0)
		{
			bytesRead += result;
			if (result < CHUNK_SIZE)
			{
				// Done
				outReceiveResult.mBuffer.resize(bytesRead);
				return true;
			}
			// Otherwise continue
		}
		else if (result == 0)
		{
			// Done
			outReceiveResult.mBuffer.resize(bytesRead);
			return true;
		}
		else
		{
		#ifdef _WIN32
			const int errorCode = WSAGetLastError();
			if (errorCode == WSAECONNRESET)		// Ignore this error, see https://stackoverflow.com/questions/30749423/is-winsock-error-10054-wsaeconnreset-normal-with-udp-to-from-localhost
				return true;
			RMX_ERROR("recv failed with error: " << errorCode, );
		#else
			RMX_ERROR("recv failed with error: " << result, );
		#endif
			return false;
		}
	}
}


struct UDPSocket::Internal
{
	SOCKET mSocket = INVALID_SOCKET;
	uint16 mLocalPort = 0;
#ifndef _WIN32
	bool mIsBlockingSocket = true;
#endif
};


UDPSocket::~UDPSocket()
{
	if (nullptr != mInternal)
	{
		close();
		delete mInternal;
	}
}

bool UDPSocket::isValid() const
{
	return (nullptr != mInternal && mInternal->mSocket >= 0);
}

bool UDPSocket::bindToPort(uint16 port)
{
	if (nullptr == mInternal)
	{
		mInternal = new Internal();
	}
	else
	{
		close();
	}

	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	addrinfo* addressInfo = nullptr;
	int result = ::getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &addressInfo);
	if (result != 0)
	{
		RMX_ERROR("getaddrinfo failed with error: " << result, );
		return false;
	}

	// Create a socket
	mInternal->mSocket = ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
	if (mInternal->mSocket < 0)
	{
		RMX_ERROR("socket failed with error: " << mInternal->mSocket, );
		::freeaddrinfo(addressInfo);
		return false;
	}

	// Setup the socket
	result = ::bind(mInternal->mSocket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
	if (result < 0)
	{
	#ifdef _WIN32
		RMX_ERROR("bind failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("bind failed with error: " << result, );
	#endif
		::freeaddrinfo(addressInfo);
		close();
		return false;
	}

	::freeaddrinfo(addressInfo);
	mInternal->mLocalPort = port;

	// Setup socket options
	int bufsize = MAX_DATAGRAM_SIZE * 8;	// This would be 256 KB, enough to hold multiple large datagrams
	::setsockopt(mInternal->mSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize));
	::setsockopt(mInternal->mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize));
	return true;
}

bool UDPSocket::bindToAnyPort()
{
	if (nullptr == mInternal)
	{
		mInternal = new Internal();
	}
	else
	{
		close();
	}

	// Create a socket
	mInternal->mSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (mInternal->mSocket < 0)
	{
		RMX_ERROR("socket failed with error: " << mInternal->mSocket, );
		return false;
	}

	// Note that the local port stays unknown this way
	mInternal->mLocalPort = 0;

	// Setup socket options
	int bufsize = 0x20000;
	::setsockopt(mInternal->mSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize));
	::setsockopt(mInternal->mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize));
	return true;
}

void UDPSocket::close()
{
	if (!isValid())
		return;

#ifdef _WIN32
	int status = shutdown(mInternal->mSocket, SD_BOTH);
	if (status == 0)
	{
		status = closesocket(mInternal->mSocket);
	}
#else
	int status = shutdown(mInternal->mSocket, SHUT_RDWR);
	if (status == 0)
	{
		status = ::close(mInternal->mSocket);
	}
#endif

	mInternal->mSocket = INVALID_SOCKET;
	mInternal->mLocalPort = 0;
}

bool UDPSocket::sendData(const uint8* data, size_t length, const SocketAddress& destinationAddress)
{
	if (!isValid())
		return false;

	const int result = ::sendto(mInternal->mSocket, (const char*)data, (int)length, 0, (sockaddr*)destinationAddress.getSockAddr(), (int)sizeof(sockaddr_storage));
	if (result >= 0)
		return true;

#ifdef _WIN32
	const int errorCode = WSAGetLastError();
	std::cout << "sendto failed with error: " << errorCode << std::endl;
#endif
	return false;
}

bool UDPSocket::sendData(const std::vector<uint8>& data, const SocketAddress& destinationAddress)
{
	if (data.empty())
		return false;
	return sendData(&data[0], data.size(), destinationAddress);
}

bool UDPSocket::receiveNonBlocking(ReceiveResult& outReceiveResult)
{
	outReceiveResult.mBuffer.clear();
	if (!isValid())
		return false;

#ifdef _WIN32
	// Check if there's pending data at all
	uint32 pendingDataSize = 0;
	if (::ioctlsocket(mInternal->mSocket, FIONREAD, (u_long*)(&pendingDataSize)) == 0)
	{
		if (pendingDataSize > 0)
		{
			return receiveInternal(outReceiveResult);
		}
	}

	// Return true as there was no error
	return true;

#else
	if (mInternal->mIsBlockingSocket)
	{
		// Set to non-blocking
		const int flags = fcntl(mInternal->mSocket, F_GETFL, 0);
		fcntl(mInternal->mSocket, F_SETFL, flags | O_NONBLOCK);
		mInternal->mIsBlockingSocket = false;
	}

	return receiveInternal(outReceiveResult);
#endif
}

bool UDPSocket::receiveBlocking(ReceiveResult& outReceiveResult)
{
	outReceiveResult.mBuffer.clear();
	if (!isValid())
		return false;

#ifndef _WIN32
	if (!mInternal->mIsBlockingSocket)
	{
		// Set to blocking
		const int flags = fcntl(mInternal->mSocket, F_GETFL, 0);
		fcntl(mInternal->mSocket, F_SETFL, flags & ~O_NONBLOCK);
		mInternal->mIsBlockingSocket = true;
	}
#endif

	return receiveInternal(outReceiveResult);
}

bool UDPSocket::receiveInternal(ReceiveResult& outReceiveResult)
{
	size_t bytesRead = 0;
	while (true)
	{
		// TODO: Reading a datagram in multiple chunks does not work, at least on Windows, so this whole while-loop is kind of pointless...
		const constexpr size_t CHUNK_SIZE = MAX_DATAGRAM_SIZE;
		outReceiveResult.mBuffer.resize(bytesRead + CHUNK_SIZE);

		sockaddr_storage& senderAddress = *reinterpret_cast<sockaddr_storage*>(outReceiveResult.mSenderAddress.accessSockAddr());
		socklen_t senderAddressSize = sizeof(sockaddr_storage);
		const int result = ::recvfrom(mInternal->mSocket, (char*)&outReceiveResult.mBuffer[bytesRead], CHUNK_SIZE, 0, (sockaddr*)&senderAddress, &senderAddressSize);
		if (result < 0)
		{
		#ifdef _WIN32
			const int errorCode = WSAGetLastError();
			if (errorCode == WSAECONNRESET)		// Ignore this error, see https://stackoverflow.com/questions/30749423/is-winsock-error-10054-wsaeconnreset-normal-with-udp-to-from-localhost
				return true;
			RMX_ERROR("recv failed with error: " << errorCode, );
		#else
			// This is only an error for blocking sockets
			if (mInternal->mIsBlockingSocket)
			{
				RMX_ERROR("recv failed with error: " << result, );
			}
		#endif
			outReceiveResult.mBuffer.clear();
			return false;
		}

		outReceiveResult.mSenderAddress.onSockAddrSet();
		if (result > 0)
		{
			bytesRead += result;
			if (result >= CHUNK_SIZE)
			{
				// There's more to read
				continue;
			}
		}

		// Done
		outReceiveResult.mBuffer.resize(bytesRead);
		return true;
	}
}
