#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <tinyxml2.h>
#include "../Network.h"
#include "../RTProtocol.h"
#include "TestUtils.h"
#include "XmlTestData.h"

namespace
{
    struct TestContext
    {
        std::unique_ptr<CRTProtocol> mRTProtocol;
        qualisys_cpp_sdk::test_utils::DummyXmlNetwork* mNetwork;
    };

    TestContext CreateTestContext(const char* xmlData)
    {
        auto networkDummy = new qualisys_cpp_sdk::test_utils::DummyXmlNetwork{};

        auto protocol = std::make_unique<CRTProtocol>();

        protocol->OverrideNetwork(networkDummy);

        if (!protocol->Connect(""))
        {
            FAIL(protocol->GetErrorString());
        }

        networkDummy->QueueResponse(xmlData, CRTPacket::EPacketType::PacketXML);
        networkDummy->theRealBool = true;

        return { std::move(protocol), networkDummy };
    }
}

TEST_CASE("SetGeneralSettingsTest")
{
    auto [protocol, network] = CreateTestContext(qualisys_cpp_sdk::xml_test_data::generalSettingsXml/*nullptr*/);

    unsigned int captureFrequency = 1;
    float captureTime = 999.0f;
    bool startOnExtTrig = true;
    bool startOnTrigNO = true;
    bool startOnTrigNC = true;
    bool startOnTrigSoftware = true;
    CRTProtocol::EProcessingActions peProcessingActions = CRTProtocol::EProcessingActions::ProcessingGazeVector;
    CRTProtocol::EProcessingActions peRtProcessingActions = CRTProtocol::EProcessingActions::ProcessingExportMatlabFile;
    CRTProtocol::EProcessingActions peReprocessingActions = CRTProtocol::EProcessingActions::ProcessingTwinSystemMerge;

    if (!protocol->SetGeneralSettings(
        &captureFrequency, &captureTime,
        &startOnExtTrig, &startOnTrigNO, &startOnTrigNC, &startOnTrigSoftware,
        &peProcessingActions, &peRtProcessingActions, &peReprocessingActions)
        )
    {
        FAIL(protocol->GetErrorString());
    }

    auto sentData = network->ReadSentData();
    const char test = 0;
}

TEST_CASE("GetGeneralSettingsTest")
{
    auto [protocol, _/*network*/] = CreateTestContext(qualisys_cpp_sdk::xml_test_data::generalSettingsXml);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

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
    CHECK_EQ(10.0f, fCaptureTime);
    CHECK_EQ(false, bStartOnExtTrig);
    CHECK_EQ(false, startOnTrigNO);
    CHECK_EQ(false, startOnTrigNC);
    CHECK_EQ(false, startOnTrigSoftware);
    CHECK_EQ(CRTProtocol::EProcessingActions::ProcessingTracking3D | CRTProtocol::EProcessingActions::ProcessingSplineFill, eProcessingActions);
    CHECK_EQ(CRTProtocol::EProcessingActions::ProcessingTracking3D, eRtProcessingActions);
    CHECK_EQ(CRTProtocol::EProcessingActions::ProcessingTracking3D | CRTProtocol::EProcessingActions::ProcessingSplineFill, eReprocessingActions);
}
