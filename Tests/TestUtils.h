#include "../Network.h"
namespace qualisys_cpp_sdk
{
    namespace test_utils
    {
        class DummyXmlNetwork : public INetwork {
            bool mConnected = false;
            std::stringstream mStringStream;
            std::stringstream mOutputStream;
        public:
            bool theRealBool = false;
            bool Connect(const char* pServerAddr, unsigned short nPort) override
            {
                auto versionString = std::string{ "Version set to " + std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION) };
                QueueResponse("QTM RT Interface connected", CRTPacket::EPacketType::PacketCommand);
                QueueResponse(versionString.data(), CRTPacket::EPacketType::PacketCommand);
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
                auto start = mStringStream.tellg();
                auto sizeRead = mStringStream.readsome(rtDataBuff, bHeader ? 8 : nDataBufSize);
                mStringStream.seekg(start + sizeRead);
                return Response{
                    sizeRead > 0 ? ResponseType::success : ResponseType::timeout,
                    static_cast<int>(sizeRead),
                };
            }

            Response ReceiveUdpBroadcast(char* rtDataBuff, int nDataBufSize, int timeoutMicroseconds,
                unsigned* ipAddr) override
            {
                return Response{
                    ResponseType::timeout,
                    0,
                };
            }

            bool Send(const char* pSendBuf, int nSize) override
            {
                if (theRealBool)
                {
                    mOutputStream.str(std::string());
                    mOutputStream.write(pSendBuf + 8, nSize - 8); // Ignore first 8 bytes / header data

                    mStringStream.str(std::string());
                    QueueResponse("Setting parameters succeeded", CRTPacket::EPacketType::PacketCommand);
                }

                return true;
            }

            std::string ReadSentData()
            {
                return mOutputStream.str();
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

            void QueueResponse(const char* str, CRTPacket::EPacketType p)
            {
                auto dataSize = static_cast<long long>(strlen(str) + 1);
                constexpr auto headerSize = 8;
                auto size = dataSize + headerSize;

                char header[headerSize];
                header[3] = static_cast<char>((size >> 24) & 0xff);
                header[2] = static_cast<char>((size >> 16) & 0xff);
                header[1] = static_cast<char>((size >> 8) & 0xff);
                header[0] = static_cast<char>(size & 0xff);

                header[7] = 0;
                header[6] = 0;
                header[5] = 0;
                header[4] = p;

                mStringStream.write(header, headerSize);
                mStringStream.write(str, dataSize);
            }
        };
    }
}