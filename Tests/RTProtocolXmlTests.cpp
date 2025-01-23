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

namespace
{
    bool Verify3DLabels(const std::vector<CRTProtocol::SSettings3DLabel>& labels3D)
    {
        std::vector<std::string> expectedLabels = {
            "VF_LeftShoulder", "VF_Spine", "VF_RightShoulder", "VF_HeadL", "VF_HeadTop", "VF_HeadR", "VF_HeadFront",
            "VF_LShoulderTop", "VF_LShoulderBack", "VF_LArm", "VF_LElbowOut", "VF_LWristOut", "VF_LWristIn", "VF_LHandOut",
            "VF_RShoulderTop", "VF_RShoulderBack", "VF_RArm", "VF_RElbowOut", "VF_RWristOut", "VF_RWristIn", "VF_RHandOut",
            "VF_Chest", "VF_SpineTop", "VF_BackL", "VF_BackR", "VF_WaistLFront", "VF_WaistLBack", "VF_WaistRBack",
            "VF_WaistRFront", "VF_LThigh", "VF_LKneeOut", "VF_LShin", "VF_LAnkleOut", "VF_LHeelBack", "VF_LForefootOut",
            "VF_LToeTip", "VF_LForefootIn", "VF_RThigh", "VF_RKneeOut", "VF_RShin", "VF_RAnkleOut", "VF_RHeelBack",
            "VF_RForefootOut", "VF_RToeTip", "VF_RForefootIn", "Eye Tracker Layout 3 - 1", "Eye Tracker Layout 3 - 2",
            "Eye Tracker Layout 3 - 3", "Eye Tracker Layout 3 - 4", "Eye Tracker Layout 3 - 5", "Eye Tracker Layout 3 - 6",
            "Table - 1", "Table - 2", "Table - 3", "Table - 4", "Screen2 - 1", "Screen2 - 2", "Screen2 - 3", "Screen2 - 4",
            "Cup - 1", "Cup - 2", "Cup - 3", "Cup - 4", "Cup - 5", "Phone - 1", "Phone - 2", "Phone - 3", "Phone - 4",
            "Screen - 1","Screen - 2","Screen - 3", "Screen - 4"
        };

        if (expectedLabels.size() != labels3D.size())
        {
            return false;
        }

        for (int i = 0; i < labels3D.size(); i++)
        {
            if (labels3D[i].oName != expectedLabels[i])
            {
                return false;
            }
        }

        return true;
    }

    bool Verify3DBones(const std::vector<CRTProtocol::SSettingsBone>& bones)
    {
        std::vector<std::tuple<std::string, std::string>> expectedFromAndToBoneNames = {
            {"VF_WaistLBack", "VF_WaistRBack"}, {"VF_LWristOut", "VF_LWristIn"}, {"VF_RWristOut", "VF_RWristIn"},
            {"VF_LWristOut", "VF_LHandOut"}, {"VF_LToeTip", "VF_LForefootIn"}, {"VF_RToeTip", "VF_RForefootIn"},
            {"VF_RForefootOut", "VF_RToeTip"}, {"VF_RAnkleOut", "VF_RHeelBack"}, {"VF_RWristOut", "VF_RHandOut"},
            {"VF_LForefootOut", "VF_LToeTip"}, {"VF_LAnkleOut", "VF_LHeelBack"}, {"VF_RArm", "VF_RElbowOut"},
            {"VF_RKneeOut", "VF_RShin"}, {"VF_LKneeOut", "VF_LShin"}, {"VF_LShoulderTop", "VF_LShoulderBack"},
            {"VF_HeadL", "VF_HeadTop"}, {"VF_RShoulderTop", "VF_RShoulderBack"}, {"VF_HeadTop", "VF_HeadR"},
            {"VF_LArm", "VF_LElbowOut"}, {"VF_HeadTop", "VF_HeadFront"}, {"VF_RThigh", "VF_RKneeOut"},
            {"VF_LThigh", "VF_LKneeOut"}, {"VF_WaistLFront", "VF_WaistRFront"}, {"VF_WaistLFront", "VF_WaistLBack"},
            {"VF_LElbowOut", "VF_LWristOut"}, {"VF_RElbowOut", "VF_RWristOut"}, {"VF_RShin", "VF_RAnkleOut"},
            {"VF_LShin", "VF_LAnkleOut"}, {"VF_WaistRFront", "VF_RThigh"}, {"VF_WaistLFront", "VF_LThigh"},
            {"VF_WaistRBack", "VF_WaistRFront"}, {"VF_BackL", "VF_BackR"}, {"VF_RShoulderBack", "VF_RArm"},
            {"VF_Chest", "VF_SpineTop"}, {"VF_LShoulderBack", "VF_LArm"}, {"VF_SpineTop", "VF_BackL"},
            {"VF_RHeelBack", "VF_RForefootIn"}, {"VF_SpineTop", "VF_BackR"}, {"VF_WaistLBack", "VF_LKneeOut"},
            {"VF_WaistRBack", "VF_RKneeOut"}, {"VF_Chest", "VF_BackL"}, {"VF_Chest", "VF_BackR"},
            {"VF_HeadR", "VF_HeadFront"}, {"VF_LShoulderTop", "VF_LElbowOut"}, {"VF_RShoulderTop", "VF_RElbowOut"},
            {"VF_LKneeOut", "VF_LAnkleOut"}, {"VF_LHeelBack", "VF_LForefootOut"}, {"VF_LHeelBack", "VF_LForefootIn"},
            {"VF_RKneeOut", "VF_RAnkleOut"}, {"VF_RHeelBack", "VF_RForefootOut"}, {"VF_HeadL", "VF_HeadFront"}
        };

        if (expectedFromAndToBoneNames.size() != bones.size())
        {
            return false;
        }

        for (int i = 0; i < bones.size(); i++)
        {
            if (bones[i].fromName != std::get<0>(expectedFromAndToBoneNames[i]) ||
                bones[i].toName != std::get<1>(expectedFromAndToBoneNames[i]))
            {
                return false;
            }
        }

        return true;
    }
}

TEST_CASE("GetSettings3DTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters 3D", qualisys_cpp_sdk::xml_test_data::Get3DSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->Read3DSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CRTProtocol::EAxis axisUpwards = CRTProtocol::EAxis::ZNeg;
    std::string calibrationTime = "";
    std::vector<CRTProtocol::SSettings3DLabel> labels3D;
    std::vector<CRTProtocol::SSettingsBone> bones;

    protocol->Get3DSettings(axisUpwards, calibrationTime, labels3D, bones);

    CHECK_EQ(CRTProtocol::EAxis::ZPos, axisUpwards);
    CHECK_EQ("2019-09-17 16:00:43", calibrationTime);
    CHECK_EQ(true, Verify3DLabels(labels3D));
    CHECK_EQ(true, Verify3DBones(bones));
}

namespace
{
    bool Settings6DMeshEqualityOperator(const CRTProtocol::SSettings6DMesh& lhs, const CRTProtocol::SSettings6DMesh& rhs)
    {
        if (lhs.name != rhs.name ||
            lhs.opacity != rhs.opacity ||
            lhs.position.fX != rhs.position.fX ||
            lhs.position.fY != rhs.position.fY ||
            lhs.position.fZ != rhs.position.fZ ||
            lhs.rotation.fX != rhs.rotation.fX ||
            lhs.rotation.fY != rhs.rotation.fY ||
            lhs.rotation.fZ != rhs.rotation.fZ ||
            lhs.scale != rhs.scale)
        {
            return false;
        }
        return true;
    }

    bool OriginEqualityOperator(const CRTProtocol::SOrigin& lhs, const CRTProtocol::SOrigin& rhs)
    {
        if (lhs.position.fX != rhs.position.fX ||
            lhs.position.fY != rhs.position.fY ||
            lhs.position.fZ != rhs.position.fZ ||
            lhs.relativeBody != rhs.relativeBody ||
            lhs.rotation[0] != rhs.rotation[0] ||
            lhs.rotation[1] != rhs.rotation[1] ||
            lhs.rotation[2] != rhs.rotation[2] ||
            lhs.rotation[3] != rhs.rotation[3] ||
            lhs.rotation[4] != rhs.rotation[4] ||
            lhs.rotation[5] != rhs.rotation[5] ||
            lhs.rotation[6] != rhs.rotation[6] ||
            lhs.rotation[7] != rhs.rotation[7] ||
            lhs.rotation[8] != rhs.rotation[8] ||
            lhs.type != rhs.type)
        {
            return false;
        }

        return true;
    }
    
    bool BodyPointsVectorEqualityOperator(const std::vector<CRTProtocol::SBodyPoint>& lhs, const std::vector<CRTProtocol::SBodyPoint>& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (int i = 0; i < lhs.size(); i++)
        {
            if (lhs[i].name != rhs[i].name ||
                lhs[i].fX != rhs[i].fX ||
                lhs[i].fX != rhs[i].fX ||
                lhs[i].fX != rhs[i].fX ||
                lhs[i].virtual_ != rhs[i].virtual_ ||
                lhs[i].physicalId != rhs[i].physicalId)
            {
                return false;
            }
        }


        return true;
    }

    bool VerifySettings6DOF(const std::vector<CRTProtocol::SSettings6DOFBody>& settings6DOF)
    {
        std::vector<std::string> expectedNames = {
            "refined", "Table", "Screen2",
            "Cup", "Phone", "Screen1"
        };
        std::vector<std::uint32_t> expectedColors = { 65280, 16711680, 16711935, 65535, 16776960, 127 };
        std::vector<float> expectedMaxResiduals = { 20.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
        std::vector<float> expectedBoneLengthTolerances = { 20.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
        CRTProtocol::SSettings6DMesh arqusMesh{ "Arqus.obj", {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.00999999978f, 1.0f };
        CRTProtocol::SSettings6DMesh emptyMesh{ "", {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f };
        std::vector<CRTProtocol::SSettings6DMesh> expectedMeshes = { arqusMesh, emptyMesh, emptyMesh, emptyMesh, emptyMesh, emptyMesh };
        CRTProtocol::SOrigin defaultOrigin{
            CRTProtocol::EOriginType::GlobalOrigin, 1, CRTProtocol::SPoint{ 0.0f, 0.0f, 0.0f },
            { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f }
        };
        std::vector<std::vector<CRTProtocol::SBodyPoint>> expectedPoints = {
            {
            CRTProtocol::SBodyPoint{ "Layout 3 - 1", -67.9271545f, 29.8100796f, -1.59297097f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 2", -95.0230560f, 1.74239099f, -19.5413551f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 3", -90.0171738f, 19.8888149f, -57.4946556f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 4", 68.3161087f, 29.1735229f, -1.82902002f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 5", 95.0272446f, 1.47444904f, -19.1718693f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 6", 89.6240387f, 20.6907444f, -57.4501305f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Table - 1", 801.421997f, 29.8100796f, 2.14241600f, false, 0 },
            CRTProtocol::SBodyPoint{ "Table - 2", 777.315125f, -413.341278f, 4.12720299f, false, 0 },
            CRTProtocol::SBodyPoint{ "Table - 3", -778.145569f, 412.610626f, -2.27826405f, false, 0 },
            CRTProtocol::SBodyPoint{ "Table - 4", -800.591614f, -368.158539f, -3.99135494f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Screen2 - 1", 177.895599f, -40.2035255f, -276.061218f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen2 - 2", -176.929245f, 41.0591507f, 276.110107f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen2 - 3", -111.494820f, 129.602188f, -283.761566f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen2 - 4", 110.528465f, -130.457809f, 283.712677f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Cup - 1", -16.4094849f, 43.2384529f, -7.37500811f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 2", 41.2375832f, 9.39587307f, -14.9948711f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 3", -6.34368896f, -32.7491074f, -24.3911552f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 4", 27.2115021f, -35.5109825f, 31.2080975f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 5", -45.6959114f, 15.6257639f, 15.5529385f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Phone - 1", -39.9740448f, -68.1382675f, -0.101278998f, false, 0 },
            CRTProtocol::SBodyPoint{ "Phone - 2", 43.3482971f, 64.0306702f, -2.40792704f, false, 0 },
            CRTProtocol::SBodyPoint{ "Phone - 3", -31.9067173f, 75.0699539f, 1.21390605f, false, 0 },
            CRTProtocol::SBodyPoint{ "Phone - 4", 28.5324669f, -70.9623566f, 1.29529905f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Screen - 1", -395.500610f, 5.13963413f, -195.481262f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen - 2", 392.106415f, -2.13840294f, 196.007095f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen - 3", 402.905762f, 26.1865349f, -178.268143f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen - 4", -399.511597f, -29.1877670f, 177.742310f, false, 0 }
            }
        };

        CHECK_EQ(settings6DOF.size(), 6);

        for (int i = 0; i < settings6DOF.size(); i++)
        {
            CHECK_EQ(settings6DOF[i].name, expectedNames[i]);
            CHECK_EQ(settings6DOF[i].enabled, true);
            CHECK_EQ(settings6DOF[i].color, expectedColors[i]);
            CHECK_EQ(settings6DOF[i].filterPreset, "No filter");
            CHECK_EQ(settings6DOF[i].maxResidual, expectedMaxResiduals[i]);
            CHECK_EQ(settings6DOF[i].minMarkersInBody, 3);
            CHECK_EQ(settings6DOF[i].boneLengthTolerance, expectedBoneLengthTolerances[i]);
            CHECK_EQ(Settings6DMeshEqualityOperator(settings6DOF[i].mesh, expectedMeshes[i]), true);
            CHECK_EQ(OriginEqualityOperator(settings6DOF[i].origin, defaultOrigin), true);
            CHECK_EQ(BodyPointsVectorEqualityOperator(settings6DOF[i].points, expectedPoints[i]), true);
        }

        return true;
    }
}

TEST_CASE("GetSettings6DOFTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters 6D", qualisys_cpp_sdk::xml_test_data::Get6DSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;
    
    if (!protocol->Read6DOFSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SSettings6DOFBody> settings6DOF;

    protocol->Get6DOFBodySettings(settings6DOF);

    CHECK_EQ(true, VerifySettings6DOF(settings6DOF));
}

TEST_CASE("GetSettingsGazeVectorTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters GazeVector", qualisys_cpp_sdk::xml_test_data::GetGazeVectorSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadGazeVectorSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SGazeVector> gazeVectorSettings;
    protocol->GetGazeVectorSettings(gazeVectorSettings);

    CHECK_EQ("Gaze vector 1 (L)", gazeVectorSettings[0].name);
    CHECK_EQ(240.0f, gazeVectorSettings[0].frequency);
    CHECK_EQ(true, gazeVectorSettings[0].hwSync);
    CHECK_EQ(false, gazeVectorSettings[0].filter);
    CHECK_EQ("Gaze vector 1 (R)", gazeVectorSettings[1].name);
    CHECK_EQ(240.0f, gazeVectorSettings[1].frequency);
    CHECK_EQ(true, gazeVectorSettings[1].hwSync);
    CHECK_EQ(false, gazeVectorSettings[1].filter);
}

TEST_CASE("GetSettingsEyeTrackerTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters EyeTracker", qualisys_cpp_sdk::xml_test_data::GetEyeTrackerSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadEyeTrackerSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SEyeTracker> eyeTrackerSettings;
    protocol->GetEyeTrackerSettings(eyeTrackerSettings);

    CHECK_EQ(1, eyeTrackerSettings.size());
    CHECK_EQ("EyeTrackerDevice", eyeTrackerSettings[0].name);
    CHECK_EQ(240.0f, eyeTrackerSettings[0].frequency);
    CHECK_EQ(true, eyeTrackerSettings[0].hwSync);
}

namespace
{
    bool VerifySettingsAnalog(const std::vector<CRTProtocol::SAnalogDevice>& analogSettings)
    {
        std::vector<std::string> expectedLabels = { "fx", "fy", "fz", "mx", "my", "mz", "trigger", "aux", "zero", "sync" };
        std::vector<std::string> expectedUnits = { "newtons", "newtons", "newtons", "newtonmetre", "newtonmetre", "newtonmetre", "", "", "", "" };

        CHECK_EQ(1, analogSettings.size());

        CHECK_EQ(1u, analogSettings[0].nDeviceID);
        CHECK_EQ(10u, analogSettings[0].nChannels);
        CHECK_EQ("Force plate 1", analogSettings[0].oName);
        for (int i = 0; i < analogSettings[0].voLabels.size(); i++)
        {
            CHECK_EQ(expectedLabels[i], analogSettings[0].voLabels[i]);
            CHECK_EQ(expectedUnits[i], analogSettings[0].voUnits[i]);
        }
        CHECK_EQ(100u, analogSettings[0].nFrequency);
        CHECK_EQ("", analogSettings[0].oUnit);
        CHECK_EQ(0.0f, analogSettings[0].fMinRange);
        CHECK_EQ(0.0f, analogSettings[0].fMaxRange);

        return true;
    }
}

TEST_CASE("GetSettingsAnalogTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters Analog", qualisys_cpp_sdk::xml_test_data::GetAnalogSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadAnalogSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SAnalogDevice> analogSettings;
    protocol->GetAnalogSettings(analogSettings);

    CHECK_EQ(true, VerifySettingsAnalog(analogSettings));
}

namespace
{
    bool VerifyForceSettings(const CRTProtocol::SSettingsForce& forceSettings)
    {
        float defaultCalibrationMatrix[12][12];
        for (int i = 0; i < 12; i++)
        {
            for (int j = 0; j < 12; j++)
            {
                defaultCalibrationMatrix[i][j] = -107374176.0f;
            }
        }

        float fVal = -107374176.0f;

        std::vector<CRTProtocol::SForcePlate> expectedPlates = {
            CRTProtocol::SForcePlate {
                1, 1, "Kistler", "Force-plate 1", 1000, 600.0f, 400.0f,
                { CRTProtocol::SPoint { 4.66907978, 3.01687002, 1.71506000 },
                  CRTProtocol::SPoint { 3.84404993, 394.877991, 1.08413005 },
                  CRTProtocol::SPoint { 593.901978, 395.980011, -0.707184970 },
                  CRTProtocol::SPoint { 594.333008, 2.39286995, -0.577087998 }
                }, CRTProtocol::SPoint { 120.0f, 200.0f, 63.0f },
                { CRTProtocol::SForceChannel { 1, -263.227173 },
                  CRTProtocol::SForceChannel { 2, -262.536102 },
                  CRTProtocol::SForceChannel { 3, -262.536102 },
                  CRTProtocol::SForceChannel { 4, -262.123199 },
                  CRTProtocol::SForceChannel { 5, -513.610657 },
                  CRTProtocol::SForceChannel { 6, -515.729736 },
                  CRTProtocol::SForceChannel { 7, -512.557678 },
                  CRTProtocol::SForceChannel { 8, -512.557678 },
                }, false,
                {
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal }
                }, 6, 6
            },
            CRTProtocol::SForcePlate {
                2, 1, "Kistler", "Force-plate 2", 1000, 600.0f, 400.0f,
                { CRTProtocol::SPoint { 606.708984, 2.52706003, -0.719672978 },
                  CRTProtocol::SPoint { 608.502014, 397.065002, -0.878275990 },
                  CRTProtocol::SPoint { 1200.28003, 396.040985, -2.18829012 },
                  CRTProtocol::SPoint { 1200.19995, 3.80520010, -1.57172000 }
                }, CRTProtocol::SPoint { 120.0f, 200.0f, 63.0f },
                { CRTProtocol::SForceChannel { 9, -263.227173 },
                  CRTProtocol::SForceChannel { 10, -262.536102 },
                  CRTProtocol::SForceChannel { 11, -262.605042 },
                  CRTProtocol::SForceChannel { 12, -262.881195 },
                  CRTProtocol::SForceChannel { 13, -513.083618 },
                  CRTProtocol::SForceChannel { 14, -512.557678 },
                  CRTProtocol::SForceChannel { 15, -513.083618 },
                  CRTProtocol::SForceChannel { 16, -513.347046 },
                }, false,
                {
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal },
                 { fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal, fVal }
                }, 6, 6
            }
        };

        CHECK_EQ("N", forceSettings.oUnitForce);
        CHECK_EQ("mm", forceSettings.oUnitLength);
        CHECK_EQ(2, forceSettings.vsForcePlates.size());

        for (int i = 0; i < forceSettings.vsForcePlates.size(); i++)
        {
            CHECK_EQ(expectedPlates[i].nID, forceSettings.vsForcePlates[i].nID);
            CHECK_EQ(expectedPlates[i].nAnalogDeviceID , forceSettings.vsForcePlates[i].nAnalogDeviceID);
            CHECK_EQ(expectedPlates[i].oType , forceSettings.vsForcePlates[i].oType);
            CHECK_EQ(expectedPlates[i].oName , forceSettings.vsForcePlates[i].oName);
            CHECK_EQ(expectedPlates[i].nFrequency , forceSettings.vsForcePlates[i].nFrequency);
            CHECK_EQ(expectedPlates[i].fLength , forceSettings.vsForcePlates[i].fLength);
            CHECK_EQ(expectedPlates[i].fWidth , forceSettings.vsForcePlates[i].fWidth);
            for (int j = 0; j < 4; j++)
            {
                CHECK_EQ(expectedPlates[i].asCorner[j].fX, forceSettings.vsForcePlates[i].asCorner[j].fX);
                CHECK_EQ(expectedPlates[i].asCorner[j].fY , forceSettings.vsForcePlates[i].asCorner[j].fY);
                CHECK_EQ(expectedPlates[i].asCorner[j].fZ , forceSettings.vsForcePlates[i].asCorner[j].fZ);
            }
            CHECK_EQ(expectedPlates[i].sOrigin.fX , forceSettings.vsForcePlates[i].sOrigin.fX);
            CHECK_EQ(expectedPlates[i].sOrigin.fY , forceSettings.vsForcePlates[i].sOrigin.fY);
            CHECK_EQ(expectedPlates[i].sOrigin.fZ , forceSettings.vsForcePlates[i].sOrigin.fZ);
            for (int j = 0; j < 8; j++)
            {
                CHECK_EQ(((8 * i) + ( j + 1 )), forceSettings.vsForcePlates[i].vChannels[j].nChannelNumber);
                CHECK_EQ(expectedPlates[i].vChannels[j].fConversionFactor, forceSettings.vsForcePlates[i].vChannels[j].fConversionFactor);
            }
            CHECK_EQ(expectedPlates[i].bValidCalibrationMatrix, forceSettings.vsForcePlates[i].bValidCalibrationMatrix);
            for (int j = 0; j < 12; j++)
            {
                for (int k = 0; k < 12; k++)
                {
                    CHECK_EQ(expectedPlates[i].afCalibrationMatrix[j][k], forceSettings.vsForcePlates[i].afCalibrationMatrix[j][k]);
                }
            }
            CHECK_EQ(expectedPlates[i].nCalibrationMatrixRows, forceSettings.vsForcePlates[i].nCalibrationMatrixRows);
            CHECK_EQ(expectedPlates[i].nCalibrationMatrixColumns, forceSettings.vsForcePlates[i].nCalibrationMatrixColumns);
        }

        return true;
    }
}

TEST_CASE("GetSettingsForceTest")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters Force", qualisys_cpp_sdk::xml_test_data::GetForceSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadForceSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CRTProtocol::SSettingsForce forceSettings;
    protocol->GetForceSettings(forceSettings);

    CHECK_EQ(true, VerifyForceSettings(forceSettings));
}
