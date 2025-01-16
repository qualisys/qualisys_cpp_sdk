#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <tinyxml2.h>
#include "../Network.h"
#include "../RTProtocol.h"
#include "TestUtils.h"
#include "XmlTestData.h"

namespace
{
    std::unique_ptr<CRTProtocol> CreateConnectedWithData(const char* xmlData)
    {
        auto networkDummy = new qualisys_cpp_sdk::test_utils::DummyXmlReceiverNetwork{};

        auto protocol = std::make_unique<CRTProtocol>();

        protocol->OverrideNetwork(networkDummy);

        if (!protocol->Connect(""))
        {
            FAIL(protocol->GetErrorString());
        }

        networkDummy->QueueResponse(xmlData, CRTPacket::EPacketType::PacketXML);

        if (!protocol->ReadGeneralSettings())
        {
            FAIL(protocol->GetErrorString());
        }

        return protocol;
    }
}

TEST_CASE("GetGeneralSettingsTest") {

    auto protocol = CreateConnectedWithData(qualisys_cpp_sdk::xml_test_data::generalSettingsXml);

    unsigned int nCaptureFrequency;
    float fCaptureTime;
    bool bStartOnExtTrig;
    bool startOnTrigNO; 
    bool startOnTrigNC;
    bool startOnTrigSoftware;
    CRTProtocol::EProcessingActions eProcessingActions;
    CRTProtocol::EProcessingActions eRtProcessingActions;
    CRTProtocol::EProcessingActions eReprocessingActions;

    protocol->GetGeneralSettings(
    nCaptureFrequency,
    fCaptureTime,
    bStartOnExtTrig,
    startOnTrigNO,
    startOnTrigNC,
    startOnTrigSoftware,
    eProcessingActions,
    eRtProcessingActions,
    eReprocessingActions
    );

    CHECK_EQ(100, nCaptureFrequency);
}
