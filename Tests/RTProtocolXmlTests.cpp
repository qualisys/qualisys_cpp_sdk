#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
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

    TestContext CreateTestContext()
    {
        auto networkDummy = new qualisys_cpp_sdk::test_utils::DummyXmlNetwork{};

        auto protocol = std::make_unique<CRTProtocol>();

        protocol->OverrideNetwork(networkDummy);

        if (!protocol->Connect(""))
        {
            FAIL(protocol->GetErrorString());
        }

        return { std::move(protocol), networkDummy };
    }
}

TEST_CASE("SetExtTimeBaseSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    bool pbEnabled = true;
    CRTProtocol::ESignalSource peSignalSource = CRTProtocol::ESignalSource::SourceVideoSync;
    bool pbSignalModePeriodic = true;
    unsigned int pnFreqMultiplier = 999;
    unsigned int pnFreqDivisor = 998;
    unsigned int pnFreqTolerance = 997;
    float pfNominalFrequency = 996.0f;
    bool pbNegativeEdge = true;
    unsigned int pnSignalShutterDelay = 995;
    float pfNonPeriodicTimeout = 994.0f;

    if (!protocol->SetExtTimeBaseSettings(
        &pbEnabled,
        &peSignalSource,
        &pbSignalModePeriodic,
        &pnFreqMultiplier,
        &pnFreqDivisor,
        &pnFreqTolerance,
        &pfNominalFrequency,
        &pbNegativeEdge,
        &pnSignalShutterDelay,
        &pfNonPeriodicTimeout))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetExtTimeBaseSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("SetGeneralSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

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

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetGeneralSettingsTest, network->ReadSentData().data()));
}


TEST_CASE("GetGeneralSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);
    
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
