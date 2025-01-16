#include "../Network.h"
#include <windows.h>

class DebugStreamBuffer : public std::streambuf {
protected:
    virtual int overflow(int c = EOF) override {
        if (c != EOF) {
            char ch = static_cast<char>(c);
            OutputDebugStringA(&ch);
        }
        return c;
    }

    virtual std::streamsize xsputn(const char* s, std::streamsize count) override {
        std::string str(s, count);
        OutputDebugStringA(str.c_str());
        return count;
    }
};

class DebugOStream : public std::ostream {
private:
    DebugStreamBuffer debugBuffer;
public:
    DebugOStream() : std::ostream(&debugBuffer) {}
};

// Declare a global instance of DebugOStream
DebugOStream debugOut;


class DummyXmlReceiverNetwork : public INetwork {
    bool mConnected = false;
    std::stringstream mStringStream;
public:
    bool Connect(const char* pServerAddr, unsigned short nPort) override
    {
        mConnected = true;
        return mConnected;
    }
    void Disconnect() override
    {
    }

    bool Connected() const override
    {
        return mConnected;
    }

    bool CreateUDPSocket(unsigned short& nUDPPort, bool bBroadcast) override
    {
        return true;
    }

    Response Receive(char* rtDataBuff, int nDataBufSize, bool bHeader, int timeoutMicroseconds,
        unsigned* ipAddr) override
    {
        auto x = mStringStream.readsome(rtDataBuff, nDataBufSize);

        return Response{
            x > 0 ? ResponseType::success : ResponseType::timeout,
            static_cast<int>(x),
        };
    }
    Response ReceiveUdpBroadcast(char* rtDataBuff, int nDataBufSize, int timeoutMicroseconds,
        unsigned* ipAddr) override
    {
        auto x = mStringStream.readsome(rtDataBuff, nDataBufSize);

        return Response{
            x > 0 ? ResponseType::success : ResponseType::timeout,
            static_cast<int>(x),
        };
    }

    bool Send(const char* pSendBuf, int nSize) override
    {
        return true;
    }
    bool SendUDPBroadcast(const char* pSendBuf, int nSize, short nPort, unsigned nFilterAddr) override
    {
        return true;
    }
    char* GetErrorString() override
    {
        return nullptr;
    }

    int GetError() const override
    {
        return 0;
    }

    bool IsLocalAddress(unsigned nAddr) const override
    {
        return true;
    }

    unsigned short GetUdpServerPort() override
    {
        return 0;
    }
    unsigned short GetUdpBroadcastServerPort() override
    {
        return 0;
    }

    void SetXmlData(const char* str)
    {
        mStringStream.clear();
        mStringStream.write(str, static_cast<long long>(strlen(str)) + 1);
    }
};