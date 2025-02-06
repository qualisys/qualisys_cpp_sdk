#include "Data/General.h"
#include "ParametersTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

TEST_CASE("SetExtTimeBaseSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    bool enabled = true;
    CRTProtocol::ESignalSource signalSource = CRTProtocol::ESignalSource::SourceVideoSync;
    bool signalModePeriodic = true;
    unsigned int freqMultiplier = 999u;
    unsigned int freqDivisor = 998u;
    unsigned int freqTolerance = 997u;
    float nominalFrequency = 996.0f;
    bool negativeEdge = true;
    unsigned int signalShutterDelay = 995u;
    float nonPeriodicTimeout = 994.0f;

    if (!protocol->SetExtTimeBaseSettings(
        &enabled,
        &signalSource,
        &signalModePeriodic,
        &freqMultiplier,
        &freqDivisor,
        &freqTolerance,
        &nominalFrequency,
        &negativeEdge,
        &signalShutterDelay,
        &nonPeriodicTimeout))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetExtTimeBaseSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetExtTimeBaseSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

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
    CHECK_EQ(1000, nFreqTolerance);
    CHECK_EQ(-1.0f, fNominalFrequency); // nominal frequency of "None" becomes -1
    CHECK_EQ(true, bNegativeEdge);
    CHECK_EQ(0, nSignalShutterDelay);
    CHECK_EQ(10.0f, fNonPeriodicTimeout);
}

TEST_CASE("SetExtTimestampSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    CRTProtocol::SSettingsGeneralExternalTimestamp timestampSettings;
    timestampSettings.enabled = true;
    timestampSettings.frequency = 999;
    timestampSettings.type = CRTProtocol::ETimestampType::Timestamp_IRIG;

    if (!protocol->SetExtTimestampSettings(timestampSettings))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetExtTimestampSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetExtTimestampSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    CRTProtocol::SSettingsGeneralExternalTimestamp timestampSettings;
    timestampSettings.enabled = true;
    timestampSettings.frequency = 999;
    timestampSettings.type = CRTProtocol::ETimestampType::Timestamp_IRIG;
    protocol->GetExtTimestampSettings(timestampSettings);

    CHECK_EQ(false, timestampSettings.enabled);
    CHECK_EQ(0, timestampSettings.frequency);
    CHECK_EQ(CRTProtocol::ETimestampType::Timestamp_SMPTE, timestampSettings.type);
}

TEST_CASE("SetCameraSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    unsigned int cameraID = 1u;
    CRTProtocol::ECameraMode mode = CRTProtocol::ECameraMode::ModeMarkerIntensity;
    float markerExposure = 999.0f;
    float markerThreshold = 998.0f;
    int orientation = 1;

    if (!protocol->SetCameraSettings(
        cameraID,
        &mode,
        &markerExposure,
        &markerThreshold,
        &orientation))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetCameraSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

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
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    int cameraID = 1;
    bool autoExposure = true;
    float compensation = 1.2345f;

    if (!protocol->SetCameraAutoExposureSettings(cameraID, autoExposure, compensation))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetCameraAutoExposureSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraAutoExposureSettings")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    int cameraID = 6;
    bool bAutoExposure = true;
    float fCompensation = 1.0f;

    if (!protocol->GetCameraAutoExposureSettings(cameraID, &bAutoExposure, &fCompensation))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(false, bAutoExposure);
    CHECK_EQ(0.0f, fCompensation);
}

TEST_CASE("SetCameraVideoSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int cameraID = 1u;
    const CRTProtocol::EVideoResolution eVideoResolution = CRTProtocol::EVideoResolution::VideoResolution1080p;
    const CRTProtocol::EVideoAspectRatio eVideoAspectRatio = CRTProtocol::EVideoAspectRatio::VideoAspectRatio4x3;
    const unsigned int videoFrequency = 23u;
    const float videoExposure = 0.123f;
    const float videoFlashTime = 0.456f;

    if (!protocol->SetCameraVideoSettings(
        cameraID, &eVideoResolution,
        &eVideoAspectRatio, &videoFrequency,
        &videoExposure, &videoFlashTime))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetCameraVideoSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraVideoSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

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
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int cameraID = 7u;
    const unsigned int portNumber = 1u;
    const CRTProtocol::ESyncOutFreqMode syncOutMode = CRTProtocol::ESyncOutFreqMode::ModeSystemLiveTime;
    const unsigned int syncOutValue = 15u;
    const float syncOutDutyCycle = 22.0f;
    const bool syncOutNegativePolarity = true;

    if (!protocol->SetCameraSyncOutSettings(
        cameraID, portNumber, &syncOutMode,
        &syncOutValue, &syncOutDutyCycle,
        &syncOutNegativePolarity))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetCameraSyncOutSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraSyncOutSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

    if (!protocol->ReadGeneralSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    unsigned int nCameraIndex = 7u;
    unsigned int portNumber = 1u;
    CRTProtocol::ESyncOutFreqMode syncOutMode = CRTProtocol::ESyncOutFreqMode::ModeDivisor;
    unsigned int syncOutValue = 99u;
    float syncOutDutyCycle = 99.0f;
    bool syncOutNegativePolarity = false;

    protocol->GetCameraSyncOutSettings(
        nCameraIndex, portNumber, syncOutMode,
        syncOutValue, syncOutDutyCycle,
        syncOutNegativePolarity
    );

    CHECK_EQ(CRTProtocol::ESyncOutFreqMode::ModeMultiplier, syncOutMode);
    CHECK_EQ(1, syncOutValue);
    CHECK_EQ(50.0f, syncOutDutyCycle);
    CHECK_EQ(true, syncOutNegativePolarity);
}

TEST_CASE("SetCameraLensControlSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int cameraID = 1u;
    const float focus = 99.0f;
    const float aperture = 98.0f;

    if (!protocol->SetCameraLensControlSettings(cameraID, focus, aperture))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetCameraLensControlSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraLensControlSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

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
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int cameraID = 1u;
    const bool enable = true;

    if (!protocol->SetCameraAutoWhiteBalance(
        cameraID, enable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetCameraAutoWhiteBalanceTest, network->ReadSentData().data()));
}

TEST_CASE("GetCameraAutoWhiteBalanceTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

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

TEST_CASE("SetGeneralSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    unsigned int captureFrequency = 1u;
    float captureTime = 999.0f;
    bool startOnExtTrig = true;
    bool startOnTrigNO = true;
    bool startOnTrigNC = true;
    bool startOnTrigSoftware = true;
    CRTProtocol::EProcessingActions processingActions = CRTProtocol::EProcessingActions::ProcessingGazeVector;
    CRTProtocol::EProcessingActions rtProcessingActions = CRTProtocol::EProcessingActions::ProcessingExportMatlabFile;
    CRTProtocol::EProcessingActions reprocessingActions = CRTProtocol::EProcessingActions::ProcessingTwinSystemMerge;

    if (!protocol->SetGeneralSettings(
        &captureFrequency, &captureTime,
        &startOnExtTrig, &startOnTrigNO, &startOnTrigNC, &startOnTrigSoftware,
        &processingActions, &rtProcessingActions, &reprocessingActions))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetGeneralSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetGeneralSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters General", qualisys_cpp_sdk::tests::data::GetGeneralSettingsTest, CRTPacket::PacketXML);

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

