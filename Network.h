#ifndef NETWORK_H
#define NETWORK_H

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
#else
	#define INVALID_SOCKET	-1
	#define SOCKET int
#endif

#include <vector>

class CNetwork
{
public:
    enum class ResponseType
    {
        success,
        timeout,
        disconnect,
        error
    };

    struct Response
    {
        int received;
        ResponseType type;

        Response(ResponseType type_, int received_) : received(received_), type(type_) {}
        operator bool() { return type == ResponseType::success; }
        operator ResponseType() { return type; }
    };

    CNetwork();
    ~CNetwork();
    bool  Connect(const char* pServerAddr, unsigned short nPort);
    void  Disconnect();
    bool  Connected() const;
    bool  CreateUDPSocket(unsigned short &nUDPPort, bool bBroadcast = false);

    Response Receive(char* rtDataBuff, int nDataBufSize, bool bHeader, int timeoutMicroseconds, unsigned int *ipAddr = nullptr);
    Response ReceiveUdpBroadcast(char* rtDataBuff, int nDataBufSize, int timeoutMicroseconds, unsigned int *ipAddr = nullptr);
    bool  Send(const char* pSendBuf, int nSize);
    bool  SendUDPBroadcast(const char* pSendBuf, int nSize, short nPort, unsigned int nFilterAddr = 0);
    char* GetErrorString();
    int   GetError() const;
    bool  IsLocalAddress(unsigned int nAddr) const;
    unsigned short GetUdpServerPort();
    unsigned short GetUdpBroadcastServerPort();

private:
    Response Receive(SOCKET socket, SOCKET udpSocket, char* rtDataBuff, int nDataBufSize, bool bHeader, int timeoutMicroseconds, unsigned int *ipAddr = nullptr);
    bool InitWinsock();
    void SetErrorString();
    unsigned short GetUdpServerPort(SOCKET nSocket);

private:
    SOCKET     mSocket;
    SOCKET     mUDPSocket;
    SOCKET     mUDPBroadcastSocket;
    char       mErrorStr[256];
    unsigned long mLastError;
};


#endif