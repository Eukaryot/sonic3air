/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/Sockets.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#undef ERROR

	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")

#else
	// Use POSIX sockets
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <netdb.h>  // Needed for getaddrinfo() and freeaddrinfo()
	#include <unistd.h> // Needed for close()
	#include <fcntl.h>	// For fcntl(), obviously

	#define SOCKET int
	#define INVALID_SOCKET -1

#endif

#ifdef __vita__
	#define SOMAXCONN 4096
#endif


namespace
{
	void setSocketOptionGeneric(SOCKET socket, int level, int optname, void* ptr, size_t size)
	{
		const int result = ::setsockopt(socket, level, optname, (const char*)ptr, (int)size);
	#ifdef _WIN32
		RMX_ASSERT(result == 0, "setsockopt failed with error: " << WSAGetLastError());
	#else
		RMX_ASSERT(result == 0, "setsockopt failed with error: " << errno);
	#endif
	}

	void setSocketOptionBool(SOCKET socket, int level, int optname, bool enable)
	{
		int option = enable ? 1 : 0;
		setSocketOptionGeneric(socket, level, optname, &option, sizeof(option));
	}

	void setSocketOptionInt(SOCKET socket, int level, int optname, int value)
	{
		setSocketOptionGeneric(socket, level, optname, &value, sizeof(value));
	}

	void configureSocket(SOCKET socket, Sockets::ProtocolFamily protocolFamily)
	{
		// Allow re-use of the port
		setSocketOptionBool(socket, SOL_SOCKET, SO_REUSEADDR, true);

		if (protocolFamily >= Sockets::ProtocolFamily::IPv6)
		{
			// Optionally allow IPv4 + IPv6 dual stack support on the socket
			setSocketOptionBool(socket, IPPROTO_IPV6, IPV6_V6ONLY, protocolFamily != Sockets::ProtocolFamily::DualStack);
		}
	}
}


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

bool Sockets::resolveToIP(const std::string& hostName, std::string& outIP, bool useIPv6)
{
#if defined(__EMSCRIPTEN__) || defined(__vita__)
	// Just return the input
	outIP = hostName;
	return true;
#else

	// Resolve host name to an IP
	addrinfo* addrInfo = nullptr;
	addrinfo hintsAddrInfo = {};
	if (::getaddrinfo(hostName.c_str(), nullptr, &hintsAddrInfo, &addrInfo) == 0)
	{
		addrinfo* firstAddrInfo = addrInfo;

		// Return either IPv4 or IPv6, depending on requested protocol family
		const int aiFamily = useIPv6 ? AF_INET6 : AF_INET;

		for (addrInfo = firstAddrInfo; nullptr != addrInfo; addrInfo = addrInfo->ai_next)
		{
			if (aiFamily == addrInfo->ai_family)
			{
				sockaddr_storage* sockAddrStorage = reinterpret_cast<sockaddr_storage*>(addrInfo->ai_addr);
				if (nullptr != sockAddrStorage)
				{
					char nodeBuffer[NI_MAXHOST] = {};
					char serviceBuffer[NI_MAXSERV] = {};
					if (::getnameinfo(reinterpret_cast<sockaddr*>(sockAddrStorage), sizeof(sockaddr_storage), nodeBuffer, sizeof(nodeBuffer), serviceBuffer, sizeof(serviceBuffer), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
					{
						// Got a result
						outIP = nodeBuffer;
						return true;
					}
					break;
				}
			}
		}
		::freeaddrinfo(firstAddrInfo);
	}

	// Failed
	return false;
#endif
}


bool SocketAddress::operator==(const SocketAddress& other) const
{
	assureIpPort();
	other.assureIpPort();
	return (mIP == other.mIP && mPort == other.mPort);
}

std::string SocketAddress::toLoggedString() const
{
	assureIpPort();
	if (mPreventIPLogging)
	{
		// Do not log the IP itself
		return "[IP]:" + std::to_string(mPort);
	}
	else
	{
		return mIP + ':' + std::to_string(mPort);
	}
}

uint64 SocketAddress::getHash() const
{
	assureSockAddr();
	return rmx::getMurmur2_64(mSockAddr, 16);
}

void SocketAddress::assureSockAddr() const
{
	if (!mHasSockAddr)
	{
		memset(&mSockAddr, 0, sizeof(mSockAddr));
		bool success = false;
	#if !defined(PLATFORM_SWITCH)	// The IPv6 part won't compile on Switch, but isn't really needed there anyways
		{
			// IPv6
			sockaddr_in6& addr = *reinterpret_cast<sockaddr_in6*>(&mSockAddr);
			addr.sin6_family = AF_INET6;
			addr.sin6_port = htons(mPort);
			success = (1 == inet_pton(addr.sin6_family, mIP.c_str(), &addr.sin6_addr));
		}
	#endif
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
			const auto addressFamily = reinterpret_cast<sockaddr_storage&>(mSockAddr).ss_family;
			if (addressFamily == AF_INET)
			{
				// IPv4
				inet_ntop(addressFamily, &(reinterpret_cast<sockaddr_in&>(mSockAddr).sin_addr), myIP, sizeof(myIP));
				mPort = ntohs(reinterpret_cast<sockaddr_in&>(mSockAddr).sin_port);
			}
		#if !defined(PLATFORM_SWITCH)	// The IPv6 part won't compile on Switch, but isn't really needed there anyways
			else
			{
				// IPv6
				inet_ntop(addressFamily, &(reinterpret_cast<sockaddr_in6&>(mSockAddr).sin6_addr), myIP, sizeof(myIP));
				mPort = ntohs(reinterpret_cast<sockaddr_in6&>(mSockAddr).sin6_port);
			}
		#endif
			mIP = myIP;
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
	SocketAddress mRemoteAddress;
#ifndef _WIN32
	bool mIsBlockingSocket = true;
#endif
};


TCPSocket::TCPSocket()
{
}

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
	return (nullptr != mInternal && mInternal->mSocket != INVALID_SOCKET);
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

	// Reset to defaults
	*mInternal = Internal();
}

const SocketAddress& TCPSocket::getRemoteAddress()
{
	if (nullptr != mInternal)
		return mInternal->mRemoteAddress;
	static SocketAddress EMPTY;
	return EMPTY;
}

void TCPSocket::swapWith(TCPSocket& other)
{
	std::swap(mInternal, other.mInternal);
}

bool TCPSocket::setupServer(uint16 serverPort, Sockets::ProtocolFamily protocolFamily)
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
	hints.ai_family = (protocolFamily >= Sockets::ProtocolFamily::IPv6) ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* addr = nullptr;
	const std::string portAsString = std::to_string(serverPort);
	int result = ::getaddrinfo(nullptr, portAsString.c_str(), &hints, &addr);
	if (result != 0)
	{
	#ifdef _WIN32
		RMX_ERROR("getaddrinfo failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("getaddrinfo failed with error: " << result, );
	#endif
		return false;
	}

	// Create a socket
	mInternal->mSocket = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (mInternal->mSocket == INVALID_SOCKET)
	{
	#ifdef _WIN32
		RMX_ERROR("socket failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("socket failed with error: " << result, );
	#endif
		close();
		return false;
	}

	configureSocket(mInternal->mSocket, protocolFamily);

	// Bind socket
	result = ::bind(mInternal->mSocket, addr->ai_addr, (int)addr->ai_addrlen);
	if (result != 0)
	{
	#ifdef _WIN32
		RMX_ERROR("bind failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("bind failed with error: " << result, );
	#endif
		close();
		return false;
	}

	// Setup for listening to incoming connections
	result = ::listen(mInternal->mSocket, SOMAXCONN);
	if (result != 0)
	{
	#ifdef _WIN32
		RMX_ERROR("listen failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("listen failed with error: " << result, );
	#endif
		close();
		return false;
	}

	#ifdef _WIN32
		// Switch socket to non-blocking (especially for sending)
		u_long mode = 1;
		ioctlsocket(mInternal->mSocket, FIONBIO, &mode);
	#endif

	return true;
}

bool TCPSocket::acceptConnection(TCPSocket& outSocket)
{
	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(mInternal->mSocket, &socketSet);
	timeval timeout { 0, 0 };
	const int result = ::select(0, &socketSet, nullptr, nullptr, &timeout);
	if (result < 0)
	{
	#ifdef _WIN32
		RMX_ERROR("select failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("select failed with error: " << result, );
	#endif
		return false;
	}

	if (result == 0)
	{
		// No incoming connection
		return false;
	}

	// Accept connection, creating a new socket
	if (nullptr == outSocket.mInternal)
	{
		outSocket.mInternal = new Internal();
	}
	else
	{
		outSocket.close();
	}

	sockaddr_storage& senderAddr = *reinterpret_cast<sockaddr_storage*>(outSocket.mInternal->mRemoteAddress.accessSockAddr());
	socklen_t senderAddrSize = sizeof(sockaddr_storage);

	outSocket.mInternal->mSocket = ::accept(mInternal->mSocket, (sockaddr*)&senderAddr, &senderAddrSize);
	if (outSocket.mInternal->mSocket < 0)
	{
	#ifdef _WIN32
		RMX_ERROR("accept failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("accept failed with error: " << outSocket.mInternal->mSocket, );
	#endif
		outSocket.mInternal->mSocket = INVALID_SOCKET;
		return false;
	}

	outSocket.mInternal->mRemoteAddress.onSockAddrSet();
	return true;
}

bool TCPSocket::connectTo(const std::string& serverAddress, uint16 serverPort, Sockets::ProtocolFamily protocolFamily)
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
		hints.ai_family = (protocolFamily >= Sockets::ProtocolFamily::IPv6) ? AF_INET6 : AF_INET;
		hints.ai_socktype = SOCK_STREAM;	// Needed for TCP
		hints.ai_protocol = IPPROTO_TCP;	// Use TCP

		const int result = getaddrinfo(serverAddress.c_str(), std::to_string(serverPort).c_str(), &hints, &addressInfos);
		RMX_CHECK(result == 0, "getaddrinfo failed with error: " << result, return false);
	}

	// Try all addresses until one of them works
	for (addrinfo* ptr = addressInfos; nullptr != ptr; ptr = ptr->ai_next)
	{
		// Create a socket
		int result = (int)::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		RMX_CHECK(result >= 0, "socket failed with error: " << result, return false);
		mInternal->mSocket = (SOCKET)result;

		configureSocket(mInternal->mSocket, protocolFamily);

		// Connect to server
		result = ::connect(mInternal->mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result >= 0)
		{
			// Success!
			break;
		}

		// Try next address
		close();
	}

	::freeaddrinfo(addressInfos);
	return (mInternal->mSocket != INVALID_SOCKET);
}

bool TCPSocket::sendData(const uint8* data, size_t length)
{
	if (!isValid())
		return false;

	const int result = ::send(mInternal->mSocket, (const char*)data, (int)length, 0);
	return (result >= 0);
}

bool TCPSocket::sendData(const std::vector<uint8>& data)
{
	if (data.empty())
		return false;
	return sendData(&data[0], data.size());
}

bool TCPSocket::receiveBlocking(ReceiveResult& outReceiveResult)
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

bool TCPSocket::receiveNonBlocking(ReceiveResult& outReceiveResult)
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

#else
	if (mInternal->mIsBlockingSocket)
	{
		// Set to non-blocking
		const int flags = fcntl(mInternal->mSocket, F_GETFL, 0);
		fcntl(mInternal->mSocket, F_SETFL, flags | O_NONBLOCK);
		mInternal->mIsBlockingSocket = false;
	}
	receiveInternal(outReceiveResult);

#endif

	// Return true as there was no error
	return true;
}

bool TCPSocket::receiveInternal(ReceiveResult& outReceiveResult)
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
			outReceiveResult.mBuffer.clear();
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
	return (nullptr != mInternal && mInternal->mSocket != INVALID_SOCKET);
}

void UDPSocket::close()
{
	if (!isValid())
		return;

#ifdef _WIN32
	int result = shutdown(mInternal->mSocket, SD_BOTH);
	if (result == 0)
	{
		result = closesocket(mInternal->mSocket);
	}
#else
	int result = shutdown(mInternal->mSocket, SHUT_RDWR);
	if (result == 0)
	{
		result = ::close(mInternal->mSocket);
	}
#endif

	// Reset to defaults
	*mInternal = Internal();
}

bool UDPSocket::bindToPort(uint16 port, Sockets::ProtocolFamily protocolFamily)
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
	hints.ai_family = (protocolFamily >= Sockets::ProtocolFamily::IPv6) ? AF_INET6 : AF_INET;
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
	result = (int)::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
	if (result < 0)
	{
	#ifdef _WIN32
		RMX_ERROR("socket failed with error: " << WSAGetLastError(), );
	#else
		RMX_ERROR("socket failed with error: " << result, );
	#endif
		::freeaddrinfo(addressInfo);
		return false;
	}
	mInternal->mSocket = (SOCKET)result;

	configureSocket(mInternal->mSocket, protocolFamily);

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
	setSocketOptionInt(mInternal->mSocket, SOL_SOCKET, SO_SNDBUF, bufsize);
	setSocketOptionInt(mInternal->mSocket, SOL_SOCKET, SO_RCVBUF, bufsize);
	return true;
}

bool UDPSocket::bindToAnyPort(Sockets::ProtocolFamily protocolFamily)
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
	mInternal->mSocket = ::socket((protocolFamily >= Sockets::ProtocolFamily::IPv6) ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (mInternal->mSocket < 0)
	{
		RMX_ERROR("socket failed with error: " << mInternal->mSocket, );
		return false;
	}

	configureSocket(mInternal->mSocket, protocolFamily);

	// Note that the local port stays unknown this way
	mInternal->mLocalPort = 0;

	// Setup socket options
	int bufsize = 0x20000;
	setSocketOptionInt(mInternal->mSocket, SOL_SOCKET, SO_SNDBUF, bufsize);
	setSocketOptionInt(mInternal->mSocket, SOL_SOCKET, SO_RCVBUF, bufsize);
	return true;
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
	RMX_LOG_INFO("sendto failed with error: " << errorCode);
#else
	const int errorCode = errno;
	RMX_LOG_INFO("sendto failed with error: " << errorCode);
#endif
	return false;
}

bool UDPSocket::sendData(const std::vector<uint8>& data, const SocketAddress& destinationAddress)
{
	if (data.empty())
		return false;
	return sendData(&data[0], data.size(), destinationAddress);
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

#else
	if (mInternal->mIsBlockingSocket)
	{
		// Set to non-blocking
		const int flags = fcntl(mInternal->mSocket, F_GETFL, 0);
		fcntl(mInternal->mSocket, F_SETFL, flags | O_NONBLOCK);
		mInternal->mIsBlockingSocket = false;
	}
	receiveInternal(outReceiveResult);

#endif

	// Return true as there was no error
	return true;
}

bool UDPSocket::receiveInternal(ReceiveResult& outReceiveResult)
{
	size_t bytesRead = 0;
	while (true)
	{
		// TODO: Reading a datagram in multiple chunks does not work, at least on Windows, so this whole while-loop is kind of pointless...
		const constexpr size_t CHUNK_SIZE = MAX_DATAGRAM_SIZE;
		outReceiveResult.mBuffer.resize(bytesRead + CHUNK_SIZE);

		sockaddr_storage& senderAddr = *reinterpret_cast<sockaddr_storage*>(outReceiveResult.mSenderAddress.accessSockAddr());
		socklen_t senderAddrSize = sizeof(sockaddr_storage);
		const int result = ::recvfrom(mInternal->mSocket, (char*)&outReceiveResult.mBuffer[bytesRead], CHUNK_SIZE, 0, (sockaddr*)&senderAddr, &senderAddrSize);
		if (result < 0)
		{
			outReceiveResult.mBuffer.clear();
		#ifdef _WIN32
			const int errorCode = WSAGetLastError();
			if (errorCode == WSAECONNRESET)		// Ignore this error, see https://stackoverflow.com/questions/30749423/is-winsock-error-10054-wsaeconnreset-normal-with-udp-to-from-localhost
				return true;
			RMX_ERROR("recv failed with error: " << errorCode, );
			if (errorCode == WSAENETRESET)		// Ignore this error as well (it happens occasionally on the server)
				return true;
		#else
			// This is only an error for blocking sockets
			if (mInternal->mIsBlockingSocket)
			{
				RMX_ERROR("recv failed with error: " << result, );
			}
		#endif
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
