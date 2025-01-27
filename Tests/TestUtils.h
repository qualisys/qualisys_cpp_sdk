#pragma once

#include <Network.h>
#include <RTProtocol.h>

#include <doctest/doctest.h>
#include <tinyxml2.h>
#include <optional>
#include <sstream>
#include <vector>

namespace qualisys_cpp_sdk::tests::utils
{
    struct MessageFilter
    {
        std::string startWith;
        std::string response;
        CRTPacket::EPacketType packetType;

        bool Compare(const std::string& message) const
        {
            return message.rfind(startWith, 0) == 0;
        }
    };

    class DummyXmlNetwork : private INetwork
    {
        bool connected = false;
        std::stringstream stringStream;
        std::stringstream outputStream;
        std::vector<MessageFilter> messageAndResponses;

        bool Connect(const char*, unsigned short) override
        {
            QueueResponse("QTM RT Interface connected", CRTPacket::EPacketType::PacketCommand);
            PrepareResponse("Version ", std::string{
                                "Version set to " + std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION)
                            }, CRTPacket::EPacketType::PacketCommand);
            connected = true;
            return connected;
        }

        void Disconnect() override {}

        bool Connected() const override
        {
            return connected;
        }

        bool CreateUDPSocket(unsigned short&, bool) override
        {
            return true;
        }

        Response Receive(char* rtDataBuff, int nDataBufSize, bool bHeader, int, unsigned*) override
        {
            auto start = stringStream.tellg();
            auto sizeRead = stringStream.readsome(rtDataBuff, bHeader ? 8 : nDataBufSize);
            stringStream.seekg(start + sizeRead);
            return Response{
                sizeRead > 0 ? ResponseType::success : ResponseType::timeout,
                static_cast<int>(sizeRead),
            };
        }

        Response ReceiveUdpBroadcast(char*, int, int, unsigned*) override
        {
            return Response{
                ResponseType::timeout,
                0,
            };
        }

        bool Send(const char* pSendBuf, int nSize) override
        {
            std::string sendText(pSendBuf + 8, nSize - 8);

            for (const auto x : messageAndResponses)
            {
                if (x.Compare(sendText))
                {
                    QueueResponse(x.response.data(), x.packetType);
                    break;
                }
            }
            outputStream.str(std::string{});
            outputStream.write(sendText.data(), static_cast<long long>(sendText.length() + 1));
            return true;
        }

        bool SendUDPBroadcast(const char*, int, short, unsigned) override
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

        bool IsLocalAddress(unsigned) const override
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

    public:
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
            header[4] = static_cast<char>(p);

            stringStream.write(header, headerSize);
            stringStream.write(str, dataSize);
        }

        void PrepareResponse(const std::string& message, const std::string& response,
                             const CRTPacket::EPacketType responsePacketType)
        {
            messageAndResponses.push_back(MessageFilter{
                message,
                response,
                responsePacketType
            });
        }

        std::string ReadSentData() const
        {
            return outputStream.str();
        }

        INetwork* GetInterfacePtr()
        {
            return this;
        }
    };

    inline bool CompareXmlIgnoreWhitespace(const char* expected, const char* actual)
    {
        tinyxml2::XMLDocument docExpected, docActual;

        if (auto error = docExpected.Parse(expected); error != tinyxml2::XML_SUCCESS)
        {
            return false;
        }

        if (auto error = docActual.Parse(actual); error != tinyxml2::XML_SUCCESS)
        {
            return false;
        }

        tinyxml2::XMLPrinter printer1, printer2;

        docExpected.Print(&printer1);

        docActual.Print(&printer2);

        return strcmp(printer1.CStr(), printer2.CStr()) == 0;
    }

    struct TestContext
    {
        std::unique_ptr<CRTProtocol> mRTProtocol;
        DummyXmlNetwork* mNetwork;
    };

    inline TestContext CreateTestContext()
    {
        auto networkDummy = new DummyXmlNetwork{};

        auto protocol = std::make_unique<CRTProtocol>();

        protocol->OverrideNetwork(networkDummy->GetInterfacePtr());

        if (!protocol->Connect(""))
        {
            FAIL(protocol->GetErrorString());
        }

        return {std::move(protocol), networkDummy};
    }
}
