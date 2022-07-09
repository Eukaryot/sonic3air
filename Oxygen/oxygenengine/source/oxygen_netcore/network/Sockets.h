/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class Sockets
{
public:
	static void startupSockets();
	static void shutdownSockets();

	static bool resolveToIP(const std::string& hostName, std::string& outIP);

public:
	static inline rmx::ErrorHandling::LoggerInterface* mLogger = nullptr;

private:
	static inline bool mIsInitialized = false;
};


struct SocketAddress
{
public:
	inline SocketAddress() :
		mHasSockAddr(false),
		mHasIpPort(false),
		mPort(0)
	{}

	inline SocketAddress(const std::string& ip, uint16 port) :
		mHasSockAddr(false),
		mHasIpPort(true),
		mIP(ip),
		mPort(port)
	{}

	inline const uint8* getSockAddr() const  { assureSockAddr();  return mSockAddr; }
	inline uint8* accessSockAddr() const	 { return mSockAddr; }

	inline const std::string& getIP() const	 { assureIpPort();  return mIP; }
	inline uint16 getPort() const			 { assureIpPort();  return mPort; }
	inline std::string toString() const		 { assureIpPort();  return mIP + ':' + std::to_string(mPort); }
	std::string toLoggedString() const;

	inline bool isValid() const  { return (mHasSockAddr || mHasIpPort); }

	inline void clear()
	{
		mHasSockAddr = false;
		mHasIpPort = false;
	}

	inline void set(const std::string& ip, uint16 port)
	{
		mHasSockAddr = false;
		mHasIpPort = true;
		mIP = ip;
	#ifdef _MSC_VER
		mIP = ip;	// Two assignments because there seems to be a weird compiler error that makes the first assignment not work
	#endif
		mPort = port;
	}

	inline void set(const uint8* sockAddr)
	{
		memcpy(mSockAddr, sockAddr, 128);
		mHasSockAddr = true;
		mHasIpPort = false;
	}

	inline void onSockAddrSet()
	{
		mHasSockAddr = true;
		mHasIpPort = false;
	}

	inline void writeTo(std::string& outIP, uint16& outPort) const
	{
		assureIpPort();
		outIP = mIP;
		outPort = mPort;
	}

	uint64 getHash() const;

	void assureSockAddr() const;
	void assureIpPort() const;

	bool operator==(const SocketAddress& other) const;
	inline bool operator!=(const SocketAddress& other) const { return !operator==(other); }

private:
	// Yes, everything is mutable here indeed...
	mutable bool mHasSockAddr = false;
	mutable bool mHasIpPort = false;
	mutable uint8 mSockAddr[128];	// Size of sockaddr_storage (pretty large, huh?)
	mutable std::string mIP;		// Usually an IPv4 as string, e.g. "123.45.67.89"
	mutable uint16 mPort = 0;
};


class TCPSocket
{
public:
	struct ReceiveResult
	{
		std::vector<uint8> mBuffer;
	};

public:
	TCPSocket();
	~TCPSocket();

	bool isValid() const;
	void close();

	const SocketAddress& getRemoteAddress();
	void swapWith(TCPSocket& other);

	bool setupServer(uint16 serverPort);
	bool acceptConnection(TCPSocket& outSocket);

	bool connectTo(const std::string& serverAddress, uint16 serverPort);

	bool sendData(const uint8* data, size_t length);
	bool sendData(const std::vector<uint8>& data);

	bool receiveBlocking(ReceiveResult& outReceiveResult);
	bool receiveNonBlocking(ReceiveResult& outReceiveResult);

private:
	bool receiveInternal(ReceiveResult& outReceiveResult);

private:
	struct Internal;
	Internal* mInternal = nullptr;
};


class UDPSocket
{
public:
	static const constexpr size_t MAX_DATAGRAM_SIZE = 0x8000;	// That's 32 KB (the actual limit is somewhat close to 64 KB, but let's play safe here)

	struct ReceiveResult
	{
		std::vector<uint8> mBuffer;
		SocketAddress mSenderAddress;
	};

public:
	~UDPSocket();

	bool isValid() const;
	void close();

	bool bindToPort(uint16 port);
	bool bindToAnyPort();

	bool sendData(const uint8* data, size_t length, const SocketAddress& destinationAddress);
	bool sendData(const std::vector<uint8>& data, const SocketAddress& destinationAddress);

	bool receiveBlocking(ReceiveResult& outReceiveResult);
	bool receiveNonBlocking(ReceiveResult& outReceiveResult);

private:
	bool receiveInternal(ReceiveResult& outReceiveResult);

private:
	struct Internal;
	Internal* mInternal = nullptr;
};
