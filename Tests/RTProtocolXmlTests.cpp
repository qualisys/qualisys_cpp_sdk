#include "../Network.h"
#include "../RTProtocol.h"

#include "TestUtils.h"
#include "XmlTestData.h"

#include <doctest/doctest.h>
#include <tinyxml2.h>

#include <memory>
#include <utility>

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

TEST_CASE("GetExtTimeBaseSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    bool bEnabled;
    CRTProtocol::ESignalSource eSignalSource;
    bool bSignalModePeriodic;
    unsigned int nFreqMultiplier;
    unsigned int nFreqDivisor;
    unsigned int nFreqTolerance;
    float fNominalFrequency;
    bool bNegativeEdge;
    unsigned int nSignalShutterDelay;
    float fNonPeriodicTimeout;

    protocol->GetExtTimeBaseSettings(
        bEnabled,
        eSignalSource,
        bSignalModePeriodic,
        nFreqMultiplier,
        nFreqDivisor,
        nFreqTolerance,
        fNominalFrequency,
        bNegativeEdge,
        nSignalShutterDelay,
        fNonPeriodicTimeout);

    volatile char breaker = 1;

    CHECK_EQ(false, bEnabled);
    CHECK_EQ(CRTProtocol::ESignalSource::SourceControlPort, eSignalSource);
    CHECK_EQ(true, bSignalModePeriodic);
    CHECK_EQ(1, nFreqMultiplier);
    CHECK_EQ(1, nFreqDivisor);
    CHECK_EQ(1000 , nFreqTolerance);
    CHECK_EQ(-1.0f, fNominalFrequency); // nominal frequency of "None" becomes -1
    CHECK_EQ(true, bNegativeEdge);
    CHECK_EQ(0, nSignalShutterDelay);
    CHECK_EQ(10.0f, fNonPeriodicTimeout);
}

TEST_CASE("SetExtTimestampSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    CRTProtocol::SSettingsGeneralExternalTimestamp timestampSettings;
    timestampSettings.bEnabled = true;
    timestampSettings.nFrequency = 999;
    timestampSettings.nType = CRTProtocol::ETimestampType::Timestamp_IRIG;

    if (!protocol->SetExtTimestampSettings(timestampSettings))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetExtTimestampSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetExtTimestampSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    CRTProtocol::SSettingsGeneralExternalTimestamp timestampSettings;
    timestampSettings.bEnabled = true;
    timestampSettings.nFrequency = 999;
    timestampSettings.nType = CRTProtocol::ETimestampType::Timestamp_IRIG;
    protocol->GetExtTimestampSettings(timestampSettings);

    CHECK_EQ(false, timestampSettings.bEnabled);
    CHECK_EQ(0, timestampSettings.nFrequency);
    CHECK_EQ(CRTProtocol::ETimestampType::Timestamp_SMPTE, timestampSettings.nType);
}

TEST_CASE("SetCameraSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    unsigned int nCameraID = 1;
    CRTProtocol::ECameraMode peMode = CRTProtocol::ECameraMode::ModeMarkerIntensity;
    float pfMarkerExposure = 999.0f;
    float pfMarkerThreshold = 998.0f;
    int pnOrientation = 1;

    if (!protocol->SetCameraSettings(
        nCameraID,
        &peMode,
        &pfMarkerExposure,
        &pfMarkerThreshold,
        &pnOrientation))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetCameraSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    unsigned int nCameraIndex = 0;
    unsigned int nID = 1;
    CRTProtocol::ECameraModel eModel;
    bool bUnderwater;
    bool bSupportsHwSync;
    unsigned int nSerial;
    CRTProtocol::ECameraMode eMode;

    protocol->GetCameraSettings(
        nCameraIndex,
        nID,
        eModel,
        bUnderwater,
        bSupportsHwSync,
        nSerial,
        eMode);

    CHECK_EQ(CRTProtocol::ECameraModel::ModelMiqusHybrid, eModel);
    CHECK_EQ(false, bUnderwater);
    CHECK_EQ(false, bSupportsHwSync);
    CHECK_EQ(21310, nSerial);
    CHECK_EQ(CRTProtocol::ECameraMode::ModeMarker, eMode);
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
        &peProcessingActions, &peRtProcessingActions, &peReprocessingActions))
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
