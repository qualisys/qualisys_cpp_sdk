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
    unsigned int pnFreqMultiplier = 999u;
    unsigned int pnFreqDivisor = 998u;
    unsigned int pnFreqTolerance = 997u;
    float pfNominalFrequency = 996.0f;
    bool pbNegativeEdge = true;
    unsigned int pnSignalShutterDelay = 995u;
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

    unsigned int nCameraID = 1u;
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

    unsigned int nCameraIndex = 0u;
    unsigned int nID = 1u;
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

TEST_CASE("SetCameraAutoExposureSettings")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    int nCameraID = 1;
    bool autoExposure = true;
    float compensation = 1.2345f;

    if (!protocol->SetCameraAutoExposureSettings(nCameraID, autoExposure, compensation))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetCameraAutoExposureSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraAutoExposureSettings")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    int nCameraID = 6;
    bool bAutoExposure = true;
    float fCompensation = 1.0f;

    if (!protocol->GetCameraAutoExposureSettings(nCameraID, &bAutoExposure, &fCompensation))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(false, bAutoExposure);
    CHECK_EQ(0.0f, fCompensation);
}

TEST_CASE("SetCameraVideoSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int nCameraID = 1u;
    const CRTProtocol::EVideoResolution eVideoResolution = CRTProtocol::EVideoResolution::VideoResolution1080p;
    const CRTProtocol::EVideoAspectRatio eVideoAspectRatio = CRTProtocol::EVideoAspectRatio::VideoAspectRatio4x3;
    const unsigned int pnVideoFrequency = 23u;
    const float pfVideoExposure = 0.123f;
    const float pfVideoFlashTime = 0.456f;

    if (!protocol->SetCameraVideoSettings(
        nCameraID, &eVideoResolution,
        &eVideoAspectRatio, &pnVideoFrequency,
        &pfVideoExposure, &pfVideoFlashTime))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetCameraVideoSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraVideoSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    unsigned int nCameraIndex = 0u;
    CRTProtocol::EVideoResolution eVideoResolution = CRTProtocol::EVideoResolution::VideoResolutionNone;
    CRTProtocol::EVideoAspectRatio eVideoAspectRatio = CRTProtocol::EVideoAspectRatio::VideoAspectRatioNone;
    unsigned int nVideoFrequency = 999u;
    unsigned int nCurrentExposure = 999u;
    unsigned int nMinExposure = 999u;
    unsigned int nMaxExposure = 999u;
    unsigned int nCurrentFlashTime = 999u;
    unsigned int nMinFlashTime = 999u;
    unsigned int nMaxFlashTime = 999u;

    protocol->GetCameraVideoSettings(
        nCameraIndex, eVideoResolution,
        eVideoAspectRatio, nVideoFrequency,
        nCurrentExposure, nMinExposure,
        nMaxExposure, nCurrentFlashTime,
        nMinFlashTime, nMaxFlashTime
    );

    CHECK_EQ(CRTProtocol::EVideoResolution::VideoResolution1080p, eVideoResolution);
    CHECK_EQ(CRTProtocol::EVideoAspectRatio::VideoAspectRatio16x9, eVideoAspectRatio);
    CHECK_EQ(25, nVideoFrequency);
    CHECK_EQ(500, nCurrentExposure);
    CHECK_EQ(5, nMinExposure);
    CHECK_EQ(39940, nMaxExposure);
    CHECK_EQ(500, nCurrentFlashTime);
    CHECK_EQ(0, nMinFlashTime);
    CHECK_EQ(500, nMaxFlashTime);
}

TEST_CASE("SetCameraSyncOutSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int nCameraID = 7u;
    const unsigned int portNumber = 1u;
    const CRTProtocol::ESyncOutFreqMode peSyncOutMode = CRTProtocol::ESyncOutFreqMode::ModeSystemLiveTime;
    const unsigned int pnSyncOutValue = 15u;
    const float pfSyncOutDutyCycle = 22.0f;
    const bool pbSyncOutNegativePolarity = true;

    if (!protocol->SetCameraSyncOutSettings(
        nCameraID, portNumber, &peSyncOutMode,
        &pnSyncOutValue, &pfSyncOutDutyCycle,
        &pbSyncOutNegativePolarity))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetCameraSyncOutSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraSyncOutSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    unsigned int nCameraIndex = 7u;
    unsigned int portNumber = 1u;
    CRTProtocol::ESyncOutFreqMode eSyncOutMode = CRTProtocol::ESyncOutFreqMode::ModeDivisor;
    unsigned int nSyncOutValue = 99u;
    float fSyncOutDutyCycle = 99.0f;
    bool bSyncOutNegativePolarity = false;

    protocol->GetCameraSyncOutSettings(
        nCameraIndex, portNumber, eSyncOutMode,
        nSyncOutValue, fSyncOutDutyCycle,
        bSyncOutNegativePolarity
    );

    CHECK_EQ(CRTProtocol::ESyncOutFreqMode::ModeMultiplier, eSyncOutMode);
    CHECK_EQ(1, nSyncOutValue);
    CHECK_EQ(50.0f, fSyncOutDutyCycle);
    CHECK_EQ(true, bSyncOutNegativePolarity);
}

TEST_CASE("SetCameraLensControlSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int nCameraID = 1u;
    const float focus = 99.0f;
    const float aperture = 98.0f;

    if (!protocol->SetCameraLensControlSettings(nCameraID, focus, aperture))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetCameraLensControlSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraLensControlSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    const unsigned int nCameraIndex = 0u;
    float focus = 0.0f;
    float aperture = 0.0f;

    protocol->GetCameraLensControlSettings(nCameraIndex, &focus, &aperture);

    CHECK_EQ(99.0f, focus);
    CHECK_EQ(98.0f, aperture);
}

TEST_CASE("SetCameraAutoWhiteBalanceTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int nCameraID = 1u;
    const bool enable = true;

    if (!protocol->SetCameraAutoWhiteBalance(
        nCameraID, enable))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetCameraAutoWhiteBalanceTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraAutoWhiteBalanceTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::xml_test_data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    const unsigned int nCameraIndex = 0u;
    bool autoWhiteBalanceEnable = false;

    protocol->GetCameraAutoWhiteBalance(nCameraIndex, &autoWhiteBalanceEnable);

    CHECK_EQ(0, nCameraIndex);
    CHECK_EQ(true, autoWhiteBalanceEnable);
}

TEST_CASE("SetImageSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int nCameraID = 1u;
    const bool bEnable = true;
    const CRTPacket::EImageFormat eFormat = CRTPacket::EImageFormat::FormatRawGrayscale;
    const unsigned int nWidth = 99u;
    const unsigned int nHeight = 98u;
    const float fLeftCrop = 97.0f;
    const float fTopCrop = 96.0f;
    const float fRightCrop = 95.0f;
    const float fBottomCrop = 94.0f;

    if (!protocol->SetImageSettings(
        nCameraID, &bEnable, &eFormat,
        &nWidth, &nHeight, &fLeftCrop,
        &fTopCrop, &fRightCrop, &fBottomCrop))
    {
        FAIL(protocol->GetErrorString());
    }

    using namespace qualisys_cpp_sdk::test_utils;
    CHECK_EQ(true, CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::xml_test_data::SetImageSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetImageCameraTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters Image", qualisys_cpp_sdk::xml_test_data::GetImageSettingsTest, CRTPacket::PacketXML);

    bool dataAvailable = false;
    if (!protocol->ReadImageSettings(dataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(true, dataAvailable);

    unsigned int id = 0xffffffff;
    bool enabled = false;
    CRTPacket::EImageFormat format = CRTPacket::EImageFormat::FormatRawBGR;
    unsigned int width;
    unsigned int height;
    float cropLeft;
    float cropTop;
    float cropRight;
    float cropButton;

    if (!protocol->GetImageCamera(
    0, id, enabled, format, width, height, cropLeft, cropTop, cropRight, cropButton
    )) {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(1, id);
    CHECK_EQ(true, enabled);
    CHECK_EQ(CRTPacket::EImageFormat::FormatJPG, format);
    CHECK_EQ(1920, width);
    CHECK_EQ(1088, height);
    CHECK_EQ(0.0, cropLeft);
    CHECK_EQ(0.0, cropTop);
    CHECK_EQ(1.0, cropRight);
    CHECK_EQ(1.0, cropButton);
}

TEST_CASE("SetGeneralSettingsTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    unsigned int captureFrequency = 1u;
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
