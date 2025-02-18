#ifndef NETWORK_H
#define NETWORK_H

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <winsock2.h>
#else
    #define INVALID_SOCKET -1
    #define SOCKET int
#endif

#include <vector>
class INetwork
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

    virtual bool  Connect(const char* pServerAddr, unsigned short nPort) = 0;
    virtual void  Disconnect() = 0;
    virtual bool  Connected() const = 0;
    virtual bool  CreateUDPSocket(unsigned short& nUDPPort, bool bBroadcast = false) = 0;

    virtual Response Receive(char* rtDataBuff, int nDataBufSize, bool bHeader, int timeoutMicroseconds, unsigned int* ipAddr = nullptr) = 0;
    virtual Response ReceiveUdpBroadcast(char* rtDataBuff, int nDataBufSize, int timeoutMicroseconds, unsigned int* ipAddr = nullptr) = 0;
    virtual bool  Send(const char* pSendBuf, int nSize) = 0;
    virtual bool  SendUDPBroadcast(const char* pSendBuf, int nSize, short nPort, unsigned int nFilterAddr = 0) = 0;
    virtual char* GetErrorString() = 0;
    virtual int   GetError() const = 0;
    virtual bool  IsLocalAddress(unsigned int nAddr) const = 0;
    virtual unsigned short GetUdpServerPort() = 0;
    virtual unsigned short GetUdpBroadcastServerPort() = 0;
    virtual ~INetwork() = default;
};

class CNetwork : public INetwork
{
public:
    CNetwork();
    ~CNetwork() override;
    bool  Connect(const char* pServerAddr, unsigned short nPort) override;
    void  Disconnect() override;
    bool  Connected() const override;
    bool  CreateUDPSocket(unsigned short &nUDPPort, bool bBroadcast = false) override;

    Response Receive(char* rtDataBuff, int nDataBufSize, bool bHeader, int timeoutMicroseconds, unsigned int *ipAddr = nullptr) override;
    Response ReceiveUdpBroadcast(char* rtDataBuff, int nDataBufSize, int timeoutMicroseconds, unsigned int *ipAddr = nullptr) override;
    bool  Send(const char* pSendBuf, int nSize) override;
    bool  SendUDPBroadcast(const char* pSendBuf, int nSize, short nPort, unsigned int nFilterAddr = 0) override;
    char* GetErrorString() override;
    int   GetError() const override;
    bool  IsLocalAddress(unsigned int nAddr) const override;
    unsigned short GetUdpServerPort() override;
    unsigned short GetUdpBroadcastServerPort() override;

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