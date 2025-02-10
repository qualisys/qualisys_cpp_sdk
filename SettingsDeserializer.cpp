#include "SettingsDeserializer.h"
#include "DeserializerApi.h"
#include "Settings.h"

#include <functional>
#include <map>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

SettingsDeserializer::SettingsDeserializer(const char* data, std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mDeserializer{nullptr}
{
    mDeserializer = new DeserializerApi(data);
}

SettingsDeserializer::~SettingsDeserializer()
{
    delete mDeserializer;
}

namespace
{
    void AddFlag(EProcessingActions flag, EProcessingActions& target)
    {
        target = static_cast<EProcessingActions>(target + flag);
    }
}

bool SettingsDeserializer::DeserializeGeneralSettings(SSettingsGeneral& generalSettings)
{
    generalSettings.vsCameras.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto generalElem = mDeserializer->FindChild("General");
    if (!generalElem)
    {
        return true;
    }

    if (auto frequencyElem = generalElem.FindChild("Frequency"))
    {
        generalSettings.nCaptureFrequency = frequencyElem.ReadUnsignedInt(0);
    }
    else
    {
        return false;
    }

    if (auto captureTimeElem = generalElem.FindChild("Capture_Time"))
    {
        generalSettings.fCaptureTime = captureTimeElem.ReadFloat(.0f);
    }
    else
    {
        return false;
    }

    if (!generalElem.TryReadElementBool("Start_On_External_Trigger", generalSettings.bStartOnExternalTrigger))
    {
        return false;
    }

    if (mMajorVersion > 1 || mMinorVersion > 14)
    {
        if (!generalElem.TryReadElementBool("Start_On_Trigger_NO", generalSettings.bStartOnTrigNO))
        {
            return false;
        }

        if (!generalElem.TryReadElementBool("Start_On_Trigger_NC", generalSettings.bStartOnTrigNC))
        {
            return false;
        }

        if (!generalElem.TryReadElementBool("Start_On_Trigger_Software", generalSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }

    if (auto extTimeBaseElem = generalElem.FindChild("External_Time_Base"))
    {
        if (!extTimeBaseElem.TryReadElementBool("Enabled", generalSettings.sExternalTimebase.bEnabled))
        {
            return false;
        }

        std::string signalSource;
        if (!extTimeBaseElem.TryReadElementString("Signal_Source", signalSource))
        {
            return false;
        }

        signalSource = ToLowerXmlString(signalSource);
        if (signalSource == "control port")
        {
            generalSettings.sExternalTimebase.eSignalSource = SourceControlPort;
        }
        else if (signalSource == "ir receiver")
        {
            generalSettings.sExternalTimebase.eSignalSource = SourceIRReceiver;
        }
        else if (signalSource == "smpte")
        {
            generalSettings.sExternalTimebase.eSignalSource = SourceSMPTE;
        }
        else if (signalSource == "irig")
        {
            generalSettings.sExternalTimebase.eSignalSource = SourceIRIG;
        }
        else if (signalSource == "video sync")
        {
            generalSettings.sExternalTimebase.eSignalSource = SourceVideoSync;
        }
        else
        {
            return false;
        }

        std::string signalMode;
        if (!extTimeBaseElem.TryReadElementString("Signal_Mode", signalMode))
        {
            return false;
        }

        signalMode = ToLowerXmlString(signalMode);
        if (signalMode == "periodic")
        {
            generalSettings.sExternalTimebase.bSignalModePeriodic = true;
        }
        else if (signalMode == "non-periodic")
        {
            generalSettings.sExternalTimebase.bSignalModePeriodic = false;
        }
        else
        {
            return false;
        }

        if (!extTimeBaseElem.TryReadElementUnsignedInt32("Frequency_Multiplier",
                                         generalSettings.sExternalTimebase.nFreqMultiplier))
        {
            return false;
        }

        if (!extTimeBaseElem.TryReadElementUnsignedInt32("Frequency_Divisor",
                                         generalSettings.sExternalTimebase.nFreqDivisor))
        {
            return false;
        }

        if (!extTimeBaseElem.TryReadElementUnsignedInt32("Frequency_Tolerance",
                                         generalSettings.sExternalTimebase.nFreqTolerance))
        {
            return false;
        }

        if (!extTimeBaseElem.TryReadElementFloat("Nominal_Frequency",
                                 generalSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (extTimeBaseElem.TryReadElementString("Nominal_Frequency", nominalFrequency))
            {
                if (ToLowerXmlString(nominalFrequency) == "none")
                {
                    generalSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
                }
            }
            else
            {
                return false;
            }
        }

        std::string signalEdge;
        if (extTimeBaseElem.TryReadElementString("Signal_Edge", signalEdge))
        {
            signalEdge = ToLowerXmlString(signalEdge);
            if (signalEdge == "negative")
            {
                generalSettings.sExternalTimebase.bNegativeEdge = true;
            }
            else if (signalEdge == "positive")
            {
                generalSettings.sExternalTimebase.bNegativeEdge = false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (!extTimeBaseElem.TryReadElementFloat("Nominal_Frequency",
                                 generalSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (extTimeBaseElem.TryReadElementString("Nominal_Frequency", nominalFrequency))
            {
                if (ToLowerXmlString(nominalFrequency) == "none")
                {
                    generalSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
                }
            }
            else
            {
                return false;
            }
        }

        if (!extTimeBaseElem.TryReadElementUnsignedInt32("Signal_Shutter_Delay",
                                         generalSettings.sExternalTimebase.nSignalShutterDelay))
        {
            return false;
        }

        if (!extTimeBaseElem.TryReadElementFloat("Non_Periodic_Timeout",
                                 generalSettings.sExternalTimebase.fNonPeriodicTimeout))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (auto externalTimestampElem = generalElem.FindChild("External_Timestamp"))
    {
        if (!externalTimestampElem.TryReadElementBool("Enabled", generalSettings.sTimestamp.bEnabled))
        {
            return false;
        }

        std::string type;
        if (externalTimestampElem.TryReadElementString("Type", type))
        {
            type = ToLowerXmlString(type);
            if (type == "smpte")
            {
                generalSettings.sTimestamp.nType = Timestamp_SMPTE;
            }
            else if (type == "irig")
            {
                generalSettings.sTimestamp.nType = Timestamp_IRIG;
            }
            else
            {
                generalSettings.sTimestamp.nType = Timestamp_CameraTime;
            }
        }

        externalTimestampElem.TryReadElementUnsignedInt32("Frequency", generalSettings.sTimestamp.nFrequency);
    }

    const char* processingActionTags[3] = {"Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions"};
    EProcessingActions* processingActions[3] =
    {
        &generalSettings.eProcessingActions,
        &generalSettings.eRtProcessingActions,
        &generalSettings.eReprocessingActions
    };

    auto AddFlagFromBoolElement = [this](DeserializerApi& parent, const char* elementName, EProcessingActions flag,
                                         EProcessingActions& target) -> bool
    {
        bool value;
        if (parent.TryReadElementBool(elementName, value))
        {
            if (value)
            {
                AddFlag(flag, target);
            }
            return true;
        }

        return false;
    };

    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================

        auto processingElem = generalElem.FindChild(processingActionTags[i]);
        if (!processingElem)
        {
            return false;
        }

        *processingActions[i] = ProcessingNone;

        if (mMajorVersion > 1 || mMinorVersion > 13)
        {
            if (!AddFlagFromBoolElement(processingElem, "PreProcessing2D", ProcessingPreProcess2D,
                                        *processingActions[i]))
            {
                return false;
            }
        }

        std::string trackingMode;
        if (!processingElem.TryReadElementString("Tracking", trackingMode))
        {
            return false;
        }

        trackingMode = ToLowerXmlString(trackingMode);
        if (trackingMode == "3d")
        {
            *processingActions[i] = static_cast<EProcessingActions>(*processingActions[i] + ProcessingTracking3D);
        }
        else if (trackingMode == "2d" && i != 1) // i != 1 => Not RtProcessingSettings
        {
            *processingActions[i] = static_cast<EProcessingActions>(*processingActions[i] + ProcessingTracking2D);
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!AddFlagFromBoolElement(processingElem, "TwinSystemMerge", ProcessingTwinSystemMerge,
                                        *processingActions[i]))
            {
                return false;
            }

            if (!AddFlagFromBoolElement(processingElem, "SplineFill", ProcessingSplineFill, *processingActions[i]))
            {
                return false;
            }
        }

        if (!AddFlagFromBoolElement(processingElem, "AIM", ProcessingAIM, *processingActions[i]))
        {
            return false;
        }

        if (!AddFlagFromBoolElement(processingElem, "Track6DOF", Processing6DOFTracking, *processingActions[i]))
        {
            return false;
        }

        if (!AddFlagFromBoolElement(processingElem, "ForceData", ProcessingForceData, *processingActions[i]))
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            if (!AddFlagFromBoolElement(processingElem, "GazeVector", ProcessingGazeVector, *processingActions[i]))
            {
                return false;
            }
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!AddFlagFromBoolElement(processingElem, "ExportTSV", ProcessingExportTSV, *processingActions[i]))
            {
                return false;
            }

            if (!AddFlagFromBoolElement(processingElem, "ExportC3D", ProcessingExportC3D, *processingActions[i]))
            {
                return false;
            }

            if (!AddFlagFromBoolElement(processingElem, "ExportMatlabFile", ProcessingExportMatlabFile,
                                        *processingActions[i]))
            {
                return false;
            }

            if (mMajorVersion > 1 || mMinorVersion > 11)
            {
                if (!AddFlagFromBoolElement(processingElem, "ExportAviFile", ProcessingExportAviFile,
                                            *processingActions[i]))
                {
                    return false;
                }
            }
        }
    }

    auto eulerElem = generalElem.FindChild("EulerAngles");
    if (eulerElem)
    {
        generalSettings.eulerRotations[0] = eulerElem.ReadAttributeString("First");
        generalSettings.eulerRotations[1] = eulerElem.ReadAttributeString("Second");
        generalSettings.eulerRotations[2] = eulerElem.ReadAttributeString("Third");
    }

    for (auto cameraElem : ChildElementRange{generalElem, "Camera"})
    {
        SSettingsGeneralCamera cameraSettings{};
        if (!cameraElem.TryReadElementUnsignedInt32("ID", cameraSettings.nID))
        {
            return false;
        }

        std::string model;
        if (!cameraElem.TryReadElementString("Model", model))
        {
            return false;
        }

        model = ToLowerXmlString(model);

        if (model == "macreflex")
        {
            cameraSettings.eModel = ModelMacReflex;
        }
        else if (model == "proreflex 120")
        {
            cameraSettings.eModel = ModelProReflex120;
        }
        else if (model == "proreflex 240")
        {
            cameraSettings.eModel = ModelProReflex240;
        }
        else if (model == "proreflex 500")
        {
            cameraSettings.eModel = ModelProReflex500;
        }
        else if (model == "proreflex 1000")
        {
            cameraSettings.eModel = ModelProReflex1000;
        }
        else if (model == "oqus 100")
        {
            cameraSettings.eModel = ModelOqus100;
        }
        else if (model == "oqus 200" || model == "oqus 200 c")
        {
            cameraSettings.eModel = ModelOqus200C;
        }
        else if (model == "oqus 300")
        {
            cameraSettings.eModel = ModelOqus300;
        }
        else if (model == "oqus 300 plus")
        {
            cameraSettings.eModel = ModelOqus300Plus;
        }
        else if (model == "oqus 400")
        {
            cameraSettings.eModel = ModelOqus400;
        }
        else if (model == "oqus 500")
        {
            cameraSettings.eModel = ModelOqus500;
        }
        else if (model == "oqus 500 plus")
        {
            cameraSettings.eModel = ModelOqus500Plus;
        }
        else if (model == "oqus 700")
        {
            cameraSettings.eModel = ModelOqus700;
        }
        else if (model == "oqus 700 plus")
        {
            cameraSettings.eModel = ModelOqus700Plus;
        }
        else if (model == "oqus 600 plus")
        {
            cameraSettings.eModel = ModelOqus600Plus;
        }
        else if (model == "miqus m1")
        {
            cameraSettings.eModel = ModelMiqusM1;
        }
        else if (model == "miqus m3")
        {
            cameraSettings.eModel = ModelMiqusM3;
        }
        else if (model == "miqus m5")
        {
            cameraSettings.eModel = ModelMiqusM5;
        }
        else if (model == "miqus sync unit")
        {
            cameraSettings.eModel = ModelMiqusSyncUnit;
        }
        else if (model == "miqus video")
        {
            cameraSettings.eModel = ModelMiqusVideo;
        }
        else if (model == "miqus video color")
        {
            cameraSettings.eModel = ModelMiqusVideoColor;
        }
        else if (model == "miqus hybrid")
        {
            cameraSettings.eModel = ModelMiqusHybrid;
        }
        else if (model == "miqus video color plus")
        {
            cameraSettings.eModel = ModelMiqusVideoColorPlus;
        }
        else if (model == "arqus a5")
        {
            cameraSettings.eModel = ModelArqusA5;
        }
        else if (model == "arqus a9")
        {
            cameraSettings.eModel = ModelArqusA9;
        }
        else if (model == "arqus a12")
        {
            cameraSettings.eModel = ModelArqusA12;
        }
        else if (model == "arqus a26")
        {
            cameraSettings.eModel = ModelArqusA26;
        }
        else
        {
            cameraSettings.eModel = ModelUnknown;
        }

        cameraElem.TryReadElementBool("Underwater", cameraSettings.bUnderwater);
        cameraElem.TryReadElementBool("Supports_HW_Sync", cameraSettings.bSupportsHwSync);

        if (!cameraElem.TryReadElementUnsignedInt32("Serial", cameraSettings.nSerial))
        {
            return false;
        }

        std::string mode;
        if (!cameraElem.TryReadElementString("Mode", mode))
        {
            return false;
        }

        mode = ToLowerXmlString(mode);
        if (mode == "marker")
        {
            cameraSettings.eMode = ModeMarker;
        }
        else if (mode == "marker intensity")
        {
            cameraSettings.eMode = ModeMarkerIntensity;
        }
        else if (mode == "video")
        {
            cameraSettings.eMode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            if (!cameraElem.TryReadElementUnsignedInt32("Video_Frequency", cameraSettings.nVideoFrequency))
            {
                return false;
            }
        }

        std::string videoResolution;
        if (cameraElem.TryReadElementString("Video_Resolution", videoResolution))
        {
            videoResolution = ToLowerXmlString(videoResolution);
            if (videoResolution == "1440p")
            {
                cameraSettings.eVideoResolution = VideoResolution1440p;
            }
            else if (videoResolution == "1080p")
            {
                cameraSettings.eVideoResolution = VideoResolution1080p;
            }
            else if (videoResolution == "720p")
            {
                cameraSettings.eVideoResolution = VideoResolution720p;
            }
            else if (videoResolution == "540p")
            {
                cameraSettings.eVideoResolution = VideoResolution540p;
            }
            else if (videoResolution == "480p")
            {
                cameraSettings.eVideoResolution = VideoResolution480p;
            }
            else
            {
                cameraSettings.eVideoResolution = VideoResolutionNone;
            }
        }
        else
        {
            cameraSettings.eVideoResolution = VideoResolutionNone;
        }

        std::string videoAspectRatio;
        if (cameraElem.TryReadElementString("Video_Aspect_Ratio", videoAspectRatio))
        {
            videoAspectRatio = ToLowerXmlString(videoAspectRatio);
            if (videoAspectRatio == "16x9")
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
            }
            else if (videoAspectRatio == "4x3")
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
            }
            else if (videoAspectRatio == "1x1")
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
            }
            else
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
            }
        }
        else
        {
            cameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
        }

        if (auto videoExposureElem = cameraElem.FindChild("Video_Exposure"))
        {
            if (!videoExposureElem.TryReadElementUnsignedInt32("Current", cameraSettings.nVideoExposure)
                || !videoExposureElem.TryReadElementUnsignedInt32("Min", cameraSettings.nVideoExposureMin)
                || !videoExposureElem.TryReadElementUnsignedInt32("Max", cameraSettings.nVideoExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto videoFlashTime = cameraElem.FindChild("Video_Flash_Time"))
        {
            if (!videoFlashTime.TryReadElementUnsignedInt32("Current", cameraSettings.nVideoFlashTime)
                || !videoFlashTime.TryReadElementUnsignedInt32("Min", cameraSettings.nVideoFlashTimeMin)
                || !videoFlashTime.TryReadElementUnsignedInt32("Max", cameraSettings.nVideoFlashTimeMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto markerExposureElem = cameraElem.FindChild("Marker_Exposure"))
        {
            if (!markerExposureElem.TryReadElementUnsignedInt32("Current", cameraSettings.nMarkerExposure)
                || !markerExposureElem.TryReadElementUnsignedInt32("Min", cameraSettings.nMarkerExposureMin)
                || !markerExposureElem.TryReadElementUnsignedInt32("Max", cameraSettings.nMarkerExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto markerThresholdElem = cameraElem.FindChild("Marker_Threshold"))
        {
            if (!markerThresholdElem.TryReadElementUnsignedInt32("Current", cameraSettings.nMarkerThreshold)
                || !markerThresholdElem.TryReadElementUnsignedInt32("Min", cameraSettings.nMarkerThresholdMin)
                || !markerThresholdElem.TryReadElementUnsignedInt32("Max", cameraSettings.nMarkerThresholdMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto positionElem = cameraElem.FindChild("Position"))
        {
            if (!positionElem.TryReadElementFloat("X", cameraSettings.fPositionX)
                || !positionElem.TryReadElementFloat("Y", cameraSettings.fPositionY)
                || !positionElem.TryReadElementFloat("Z", cameraSettings.fPositionZ)
                || !positionElem.TryReadElementFloat("Rot_1_1", cameraSettings.fPositionRotMatrix[0][0])
                || !positionElem.TryReadElementFloat("Rot_2_1", cameraSettings.fPositionRotMatrix[1][0])
                || !positionElem.TryReadElementFloat("Rot_3_1", cameraSettings.fPositionRotMatrix[2][0])
                || !positionElem.TryReadElementFloat("Rot_1_2", cameraSettings.fPositionRotMatrix[0][1])
                || !positionElem.TryReadElementFloat("Rot_2_2", cameraSettings.fPositionRotMatrix[1][1])
                || !positionElem.TryReadElementFloat("Rot_3_2", cameraSettings.fPositionRotMatrix[2][1])
                || !positionElem.TryReadElementFloat("Rot_1_3", cameraSettings.fPositionRotMatrix[0][2])
                || !positionElem.TryReadElementFloat("Rot_2_3", cameraSettings.fPositionRotMatrix[1][2])
                || !positionElem.TryReadElementFloat("Rot_3_3", cameraSettings.fPositionRotMatrix[2][2]))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (!cameraElem.TryReadElementUnsignedInt32("Orientation", cameraSettings.nOrientation))
        {
            return false;
        }

        if (auto markerResElem = cameraElem.FindChild("Marker_Res"))
        {
            if (!markerResElem.TryReadElementUnsignedInt32("Width", cameraSettings.nMarkerResolutionWidth)
                || !markerResElem.TryReadElementUnsignedInt32("Height", cameraSettings.nMarkerResolutionHeight))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto videoResElem = cameraElem.FindChild("Video_Res"))
        {
            if (!videoResElem.TryReadElementUnsignedInt32("Width", cameraSettings.nVideoResolutionWidth)
                || !videoResElem.TryReadElementUnsignedInt32("Height", cameraSettings.nVideoResolutionHeight))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto markerFovElem = cameraElem.FindChild("Marker_FOV"))
        {
            if (!markerFovElem.TryReadElementUnsignedInt32("Left", cameraSettings.nMarkerFOVLeft)
                || !markerFovElem.TryReadElementUnsignedInt32("Top", cameraSettings.nMarkerFOVTop)
                || !markerFovElem.TryReadElementUnsignedInt32("Right", cameraSettings.nMarkerFOVRight)
                || !markerFovElem.TryReadElementUnsignedInt32("Bottom", cameraSettings.nMarkerFOVBottom))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto videoFovElem = cameraElem.FindChild("Marker_FOV"))
        {
            if (!videoFovElem.TryReadElementUnsignedInt32("Left", cameraSettings.nVideoFOVLeft)
                || !videoFovElem.TryReadElementUnsignedInt32("Top", cameraSettings.nVideoFOVTop)
                || !videoFovElem.TryReadElementUnsignedInt32("Right", cameraSettings.nVideoFOVRight)
                || !videoFovElem.TryReadElementUnsignedInt32("Bottom", cameraSettings.nVideoFOVBottom))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        // Only available from protocol version 1.10 and later.
        for (std::size_t port = 0; port < 3; port++)
        {
            char syncOutStr[16];
            (void)sprintf_s(syncOutStr, 16, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));

            auto syncOutElem = cameraElem.FindChild(syncOutStr);
            if (syncOutElem)
            {
                if (port < 2)
                {
                    std::string mode;
                    if (!syncOutElem.TryReadElementString("Mode", mode))
                    {
                        return false;
                    }

                    mode = ToLowerXmlString(mode);
                    if (mode == "shutter out")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeShutterOut;
                    }
                    else if (mode == "multiplier")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeMultiplier;
                    }
                    else if (mode == "divisor")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeDivisor;
                    }
                    else if (mode == "camera independent")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (mode == "measurement time")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (mode == "continuous 100hz")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
                    }
                    else if (mode == "system live time")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeSystemLiveTime;
                    }
                    else
                    {
                        return false;
                    }

                    if (cameraSettings.eSyncOutMode[port] == ModeMultiplier ||
                        cameraSettings.eSyncOutMode[port] == ModeDivisor ||
                        cameraSettings.eSyncOutMode[port] == ModeIndependentFreq)
                    {
                        if (!syncOutElem.TryReadElementUnsignedInt32("Value", cameraSettings.nSyncOutValue[port])
                            || !syncOutElem.TryReadElementFloat("Duty_Cycle", cameraSettings.fSyncOutDutyCycle[port]))
                        {
                            return false;
                        }
                    }
                }

                if (port == 2 || (cameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    std::string signalPolarity;
                    if (syncOutElem.TryReadElementString("Signal_Polarity", signalPolarity))
                    {
                        cameraSettings.bSyncOutNegativePolarity[port] = ToLowerXmlString(signalPolarity) == "negative";
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            else
            {
                cameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                cameraSettings.nSyncOutValue[port] = 0;
                cameraSettings.fSyncOutDutyCycle[port] = 0;
                cameraSettings.bSyncOutNegativePolarity[port] = false;
            }
        }

        if (auto lensControlElem = cameraElem.FindChild("LensControl"))
        {
            auto focusElem = lensControlElem.FindChild("Focus");
            if (focusElem)
            {
                cameraSettings.fFocus = focusElem.ReadAttributeFloat("Value", std::numeric_limits<float>::quiet_NaN());
            }

            auto apertureElem = lensControlElem.FindChild("Aperture");
            if (apertureElem)
            {
                cameraSettings.fAperture = apertureElem.
                    ReadAttributeFloat("Value", std::numeric_limits<float>::quiet_NaN());
            }
        }
        else
        {
            cameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            cameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        if (auto autoExposureElem = cameraElem.FindChild("AutoExposure"))
        {
            cameraSettings.autoExposureEnabled = autoExposureElem.ReadAttributeBool("Enabled", false);
            cameraSettings.autoExposureCompensation = autoExposureElem.ReadAttributeFloat(
                "Compensation", std::numeric_limits<float>::quiet_NaN());
        }
        else
        {
            cameraSettings.autoExposureEnabled = false;
            cameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        bool autoWhiteBalance;
        if (cameraElem.TryReadElementBool("AutoWhiteBalance", autoWhiteBalance))
        {
            cameraSettings.autoWhiteBalance = autoWhiteBalance ? 1 : 0;
        }
        else
        {
            cameraSettings.autoWhiteBalance = -1;
        }

        generalSettings.vsCameras.push_back(cameraSettings);
    }

    return true;
}

bool SettingsDeserializer::Deserialize3DSettings(SSettings3D& settings3D, bool& dataAvailable)
{
    dataAvailable = false;

    settings3D.pCalibrationTime[0] = 0;

    if (!mDeserializer)
    {
        return true;
    }

    auto threeDElem = mDeserializer->FindChild("The_3D");
    if (!threeDElem)
    {
        return true;
    }

    if (auto axisUpwards = threeDElem.FindChild("AxisUpwards"))
    {
        auto str = ToLowerXmlString(axisUpwards.ReadString());
        if (str == "+x")
        {
            settings3D.eAxisUpwards = XPos;
        }
        else if (str == "-x")
        {
            settings3D.eAxisUpwards = XNeg;
        }
        else if (str == "+y")
        {
            settings3D.eAxisUpwards = YPos;
        }
        else if (str == "-y")
        {
            settings3D.eAxisUpwards = YNeg;
        }
        else if (str == "+z")
        {
            settings3D.eAxisUpwards = ZPos;
        }
        else if (str == "-z")
        {
            settings3D.eAxisUpwards = ZNeg;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (auto calibrationTimeElem = threeDElem.FindChild("CalibrationTime"))
    {
        auto str = calibrationTimeElem.ReadString();
        strcpy_s(settings3D.pCalibrationTime, 32, str.data());
    }
    else
    {
        return false;
    }

    std::size_t labelCount;
    if (auto labelsElem = threeDElem.FindChild("Labels"))
    {
        labelCount = labelsElem.ReadInt(0);
    }
    else
    {
        return false;
    }

    settings3D.s3DLabels.clear();
    settings3D.s3DLabels.reserve(labelCount);

    for (auto labelElem : ChildElementRange{threeDElem, "Label"})
    {
        SSettings3DLabel label{};
        if (auto nameElem = labelElem.FindChild("Name"))
        {
            label.oName = nameElem.ReadString();
        }

        if (auto colorElem = labelElem.FindChild("RGBColor"))
        {
            label.nRGBColor = colorElem.ReadInt(0);
        }

        if (auto typeElem = labelElem.FindChild("Trajectory_Type"))
        {
            label.type = typeElem.ReadString();
        }

        settings3D.s3DLabels.push_back(label);
    }

    if (settings3D.s3DLabels.size() != labelCount)
    {
        return false;
    }

    if (auto bonesElem = threeDElem.FindChild("Bones"))
    {
        for (auto boneElem : ChildElementRange{bonesElem, "Bone"})
        {
            SSettingsBone bone{};
            bone.fromName = boneElem.ReadAttributeString("From");
            bone.toName = boneElem.ReadAttributeString("To");
            bone.color = boneElem.ReadAttributeUnsignedInt("Color", bone.color);
            settings3D.sBones.push_back(bone);
        }
    }

    dataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryRead6DofElementEnabled(std::uint32_t majorVer, std::uint32_t minorVer, DeserializerApi& deserializer,
                                   bool& target)
    {
        if (majorVer > 1 || minorVer > 23)
        {
            if (auto enabledElem = deserializer.FindChild("Enabled"))
            {
                target = enabledElem.ReadString() == "true";
                return true;
            }
        }

        // Enabled is default true for 6dof bodies
        target = true;
        return false;
    }


    bool TryReadAttributesRGBColor(DeserializerApi& deserializer, std::uint32_t& target)
    {
        if (auto elem = deserializer.FindChild("Color"))
        {
            std::uint32_t colorR = elem.ReadAttributeUnsignedInt("R");
            std::uint32_t colorG = elem.ReadAttributeUnsignedInt("G");
            std::uint32_t colorB = elem.ReadAttributeUnsignedInt("B");
            target = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);
            return true;
        }

        target = 0;
        return false;
    }

    bool TryReadSetFilter(DeserializerApi& deserializer, std::string& target)
    {
        if (auto elem = deserializer.FindChild("Filter"))
        {
            target = elem.ReadAttributeString("Preset");
            return true;
        }

        return false;
    }

    bool TryReadSetPos(DeserializerApi& deserializer, float& targetX, float& targetY, float& targetZ)
    {
        if (auto elem = deserializer.FindChild("Position"))
        {
            targetX = elem.ReadAttributeFloat("X");
            targetY = elem.ReadAttributeFloat("Y");
            targetZ = elem.ReadAttributeFloat("Z");
            return true;
        }

        targetZ = targetY = targetX = .0f;
        return false;
    }

    bool TryReadSetRotation(DeserializerApi& deserializer, float& targetX, float& targetY, float& targetZ)
    {
        if (auto elem = deserializer.FindChild("Rotation"))
        {
            targetX = elem.ReadAttributeFloat("X");
            targetY = elem.ReadAttributeFloat("Y");
            targetZ = elem.ReadAttributeFloat("Z");
            return true;
        }

        targetZ = targetY = targetX = .0f;
        return false;
    }


    bool TryReadSetPoints(DeserializerApi& deserializer, std::vector<SBodyPoint>& target)
    {
        if (auto pointsElem = deserializer.FindChild("Points"))
        {
            for (auto pointElem : ChildElementRange{pointsElem, "Point"})
            {
                SBodyPoint bodyPoint;

                bodyPoint.fX = pointElem.ReadAttributeFloat("X");
                bodyPoint.fY = pointElem.ReadAttributeFloat("Y");
                bodyPoint.fZ = pointElem.ReadAttributeFloat("Z");

                bodyPoint.virtual_ = 0 != pointElem.ReadAttributeUnsignedInt("Virtual");
                bodyPoint.physicalId = pointElem.ReadAttributeUnsignedInt("PhysicalId");
                bodyPoint.name = pointElem.ReadAttributeString("Name");
                target.push_back(bodyPoint);
            }

            return true;
        }

        return false;
    }

    bool TryReadSetDataOrigin(DeserializerApi& deserializer, SOrigin& target)
    {
        if (auto elem = deserializer.FindChild("Data_origin"))
        {
            target.type = static_cast<EOriginType>(elem.ReadUnsignedInt());
            target.position.fX = elem.ReadAttributeFloat("X");
            target.position.fY = elem.ReadAttributeFloat("Y");
            target.position.fZ = elem.ReadAttributeFloat("Z");
            target.relativeBody = elem.ReadAttributeUnsignedInt("Relative_body");
        }
        else
        {
            target = {};
            return false;
        }

        if (auto elem = deserializer.FindChild("Data_orientation"))
        {
            char tmpStr[10];
            for (std::uint32_t i = 0; i < 9; i++)
            {
                (void)sprintf_s(tmpStr, 10, "R%u%u", (i / 3) + 1, (i % 3) + 1);
                target.rotation[i] = elem.ReadAttributeFloat(tmpStr);
            }


            auto type = static_cast<EOriginType>(elem.ReadUnsignedInt());
            auto body = static_cast<std::uint32_t>(elem.ReadAttributeUnsignedInt("Relative_body"));

            // Validation: type and relativeBody must be the same between orientation and origin
            return type == target.type && body == target.relativeBody;
        }

        target = {};
        return false;
    }

    bool TryReadElementRGBColor(DeserializerApi& deserializer, std::uint32_t& target)
    {
        if (auto elem = deserializer.FindChild("RGBColor"))
        {
            target = elem.ReadInt();
            return true;
        }

        target = 0;
        return false;
    }

    bool TryReadSetPointsOld(DeserializerApi& deserializer, std::vector<SBodyPoint>& target)
    {
        target.clear();
        for (auto pointElem : ChildElementRange{deserializer, "Bone"})
        {
            SBodyPoint point;

            if (!pointElem.TryReadElementFloat("X", point.fX))
            {
                return false;
            }

            if (!pointElem.TryReadElementFloat("Y", point.fY))
            {
                return false;
            }

            if (!pointElem.TryReadElementFloat("Z", point.fZ))
            {
                return false;
            }

            target.push_back(point);
        }
        return true;
    }

    bool TryReadSetEuler(DeserializerApi& deserializer, std::string& targetFirst, std::string& targetSecond,
                         std::string& targetThird)
    {
        if (auto elem = deserializer.FindChild("Euler"))
        {
            return elem.TryReadElementString("First", targetFirst)
                && elem.TryReadElementString("Second", targetSecond)
                && elem.TryReadElementString("Third", targetThird);
        }

        return false;
    }
}

bool SettingsDeserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& settings6Dof,
                                                    SSettingsGeneral& generalSettings, bool& dataAvailable)
{
    dataAvailable = false;

    settings6Dof.clear();

    if (!mDeserializer)
    {
        return true;
    }

    //
    // Read gaze vectors
    //
    DeserializerApi sixDofElem = mDeserializer->FindChild("The_6D");
    if (!sixDofElem)
    {
        return true; // NO eye tracker data available.
    }

    if (mMajorVersion > 1 || mMinorVersion > 20)
    {
        for (auto bodyElem : ChildElementRange{sixDofElem, "Body"})
        {
            SSettings6DOFBody bodySettings6Dof;

            if (!bodyElem.TryReadElementString("Name", bodySettings6Dof.name))
            {
                // Name --- REQUIRED
                return false;
            }

            TryRead6DofElementEnabled(mMajorVersion, mMinorVersion, bodyElem, bodySettings6Dof.enabled);
            if (!TryReadAttributesRGBColor(bodyElem, bodySettings6Dof.color)
                || !bodyElem.TryReadElementFloat("MaximumResidual", bodySettings6Dof.maxResidual)
                || !bodyElem.TryReadElementUnsignedInt32("MinimumMarkersInBody", bodySettings6Dof.minMarkersInBody)
                || !bodyElem.TryReadElementFloat("BoneLengthTolerance", bodySettings6Dof.boneLengthTolerance)
                || !TryReadSetFilter(bodyElem, bodySettings6Dof.filterPreset))
            {
                // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                return false;
            }


            if (auto meshElem = bodyElem.FindChild("Mesh"))
            {
                if (!meshElem.TryReadElementString("Name", bodySettings6Dof.mesh.name)
                    || !TryReadSetPos(meshElem, bodySettings6Dof.mesh.position.fX, bodySettings6Dof.mesh.position.fY,
                                      bodySettings6Dof.mesh.position.fZ)
                    || !TryReadSetRotation(meshElem, bodySettings6Dof.mesh.rotation.fX,
                                           bodySettings6Dof.mesh.rotation.fY, bodySettings6Dof.mesh.rotation.fZ)
                    || !meshElem.TryReadElementFloat("Scale", bodySettings6Dof.mesh.scale)
                    || !meshElem.TryReadElementFloat("Opacity", bodySettings6Dof.mesh.opacity))
                {
                    // Name, Position, Rotation, Scale, Opacity --- REQUIRED
                    return false;
                }
            }
            // Points --- REQUIRED
            if (!TryReadSetPoints(bodyElem, bodySettings6Dof.points))
            {
                return false;
            }

            // Data Orientation, Origin --- REQUIRED
            if (!TryReadSetDataOrigin(bodyElem, bodySettings6Dof.origin))
            {
                return false;
            }

            settings6Dof.push_back(bodySettings6Dof);
            dataAvailable = true;
        }
    }
    else
    {
        if (!mDeserializer->FindChild("Bodies"))
        {
            return false;
        }

        for (auto bodyElem : ChildElementRange{*mDeserializer, "Body"})
        {
            SSettings6DOFBody bodySettings6Dof{};

            // Name, RGBColor, Points(OLD) --- REQUIRED
            if (!bodyElem.TryReadElementString("Name", bodySettings6Dof.name)
                || !TryReadElementRGBColor(bodyElem, bodySettings6Dof.color)
                || !TryReadSetPointsOld(bodyElem, bodySettings6Dof.points))
            {
                return false;
            }

            if (mMajorVersion > 1 || mMinorVersion > 15)
            {
                // Euler --- REQUIRED
                if (!TryReadSetEuler(*mDeserializer, generalSettings.eulerRotations[0],
                                     generalSettings.eulerRotations[1], generalSettings.eulerRotations[2]))
                {
                    return false;
                }
            }

            settings6Dof.push_back(bodySettings6Dof);
            dataAvailable = true;
        }
    }

    return true;
} // Read6DOFSettings

bool SettingsDeserializer::DeserializeGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings,
                                                          bool& dataAvailable)
{
    dataAvailable = false;

    gazeVectorSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    //
    // Read gaze vectors
    //
    DeserializerApi gazeVectorElem = mDeserializer->FindChild("Gaze_Vector");
    if (!gazeVectorElem)
    {
        return true; // NO eye tracker data available.
    }

    for (auto vectorElem : ChildElementRange{gazeVectorElem, "Vector"})
    {
        std::string name;
        if (auto nameElem = vectorElem.FindChild("Name"))
        {
            name = nameElem.ReadString();
        }
        else
        {
            return false;
        }

        float frequency = 0;
        if (auto frequencyElem = vectorElem.FindChild("Frequency"))
        {
            frequency = frequencyElem.ReadFloat();
        }

        bool hwSync = false;
        vectorElem.TryReadElementBool("Hardware_Sync", hwSync);

        bool filter = false;
        vectorElem.TryReadElementBool("Filter", filter);

        gazeVectorSettings.push_back({name, frequency, hwSync, filter});
    }

    dataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool SettingsDeserializer::DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings,
                                                          bool& dataAvailable)
{
    dataAvailable = false;

    eyeTrackerSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    DeserializerApi eyeTrackerElem = mDeserializer->FindChild("Eye_Tracker");

    if (!eyeTrackerElem)
    {
        return true; // NO eye tracker data available.
    }

    for (auto deviceElem : ChildElementRange{eyeTrackerElem, "Device"})
    {
        std::string name;
        if (auto nameElem = deviceElem.FindChild("Name"))
        {
            name = nameElem.ReadString();
        }
        else
        {
            return false;
        }

        float frequency = 0;
        if (auto frequencyElem = deviceElem.FindChild("Frequency"))
        {
            frequency = frequencyElem.ReadFloat();
        }

        bool hwSync = false;
        deviceElem.TryReadElementBool("Hardware_Sync", hwSync);

        eyeTrackerSettings.push_back({name, frequency, hwSync});
    }

    dataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool SettingsDeserializer::DeserializeAnalogSettings(std::vector<SAnalogDevice>& analogDeviceSettings,
                                                      bool& dataAvailable)
{
    dataAvailable = false;
    analogDeviceSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto analogElem = mDeserializer->FindChild("Analog");
    if (!analogElem)
    {
        // No analog data available.
        return true;
    }

    if (mMajorVersion == 1 && mMinorVersion == 0)
    {
        SAnalogDevice analogDevice{};
        analogDevice.nDeviceID = 1; // Always channel 1
        analogDevice.oName = "AnalogDevice";
        if (!analogElem.TryReadElementUnsignedInt32("Channels", analogDevice.nChannels)
            || !analogElem.TryReadElementUnsignedInt32("Frequency", analogDevice.nFrequency)
            || !analogElem.TryReadElementString("Unit", analogDevice.oUnit))
        {
            return false;
        }

        auto rangeElem = analogElem.FindChild("Range");
        if (!rangeElem
            || !rangeElem.TryReadElementFloat("Min", analogDevice.fMinRange)
            || !rangeElem.TryReadElementFloat("Max", analogDevice.fMaxRange))
        {
            return false;
        }

        dataAvailable = true;
        analogDeviceSettings.push_back(analogDevice);
        return true;
    }

    for (auto deviceElem : ChildElementRange{analogElem, "Device"})
    {
        SAnalogDevice analogDevice{};
        if (!deviceElem.TryReadElementUnsignedInt32("Device_ID", analogDevice.nDeviceID)
            || !deviceElem.TryReadElementString("Device_Name", analogDevice.oName)
            || !deviceElem.TryReadElementUnsignedInt32("Channels", analogDevice.nChannels)
            || !deviceElem.TryReadElementUnsignedInt32("Frequency", analogDevice.nFrequency)
        )
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            if (!analogElem.TryReadElementString("Unit", analogDevice.oUnit))
            {
                continue;
            }
        }

        auto rangeElem = deviceElem.FindChild("Range");
        if (!rangeElem
            || !rangeElem.TryReadElementFloat("Min", analogDevice.fMinRange)
            || !rangeElem.TryReadElementFloat("Max", analogDevice.fMaxRange))
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            for (std::size_t i = 0; i < analogDevice.nChannels; i++)
            {
                std::string label;
                if (deviceElem.TryReadElementString("Label", label))
                {
                    analogDevice.voLabels.push_back(label);
                }
            }

            if (analogDevice.voLabels.size() != analogDevice.nChannels)
            {
                continue;
            }
        }
        else
        {
            for (auto channelElem : ChildElementRange{deviceElem, "Channel"})
            {
                std::string label;
                if (channelElem.TryReadElementString("Label", label))
                {
                    analogDevice.voLabels.push_back(label);
                }

                std::string unit;
                if (channelElem.TryReadElementString("Unit", unit))
                {
                    analogDevice.voUnits.push_back(unit);
                }
            }

            if (analogDevice.voLabels.size() != analogDevice.nChannels ||
                analogDevice.voUnits.size() != analogDevice.nChannels)
            {
                continue;
            }
        }

        dataAvailable = true;
        analogDeviceSettings.push_back(analogDevice);
    }

    return true;
} // ReadAnalogSettings

bool SettingsDeserializer::DeserializeForceSettings(SSettingsForce& forceSettings, bool& dataAvailable)
{
    dataAvailable = false;

    forceSettings.vsForcePlates.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto forceElem = mDeserializer->FindChild("Force");
    if (!forceElem)
    {
        // No analog data available.
        return true;
    }

    SForcePlate forcePlate{};
    forcePlate.bValidCalibrationMatrix = false;
    forcePlate.nCalibrationMatrixRows = 6;
    forcePlate.nCalibrationMatrixColumns = 6;

    if (!forceElem.TryReadElementString("Unit_Length", forceSettings.oUnitLength))
    {
        return false;
    }

    if (!forceElem.TryReadElementString("Unit_Force", forceSettings.oUnitForce))
    {
        return false;
    }

    std::size_t iPlate = 0;
    for (auto plateElem : ChildElementRange{forceElem, "Plate"})
    {
        iPlate++;

        if (!plateElem.TryReadElementUnsignedInt32("Plate_ID", forcePlate.nID))
        {
            if (!plateElem.TryReadElementUnsignedInt32("Force_Plate_Index", forcePlate.nID))
                // Version 1.7 and earlier.
            {
                return false;
            }
        }

        if (!plateElem.TryReadElementUnsignedInt32("Analog_Device_ID", forcePlate.nAnalogDeviceID))
        {
            forcePlate.nAnalogDeviceID = 0;
        }

        if (!plateElem.TryReadElementUnsignedInt32("Frequency", forcePlate.nFrequency))
        {
            return false;
        }

        if (!plateElem.TryReadElementString("Type", forcePlate.oType))
        {
            forcePlate.oType = "unknown";
        }

        if (!plateElem.TryReadElementString("Name", forcePlate.oName))
        {
            forcePlate.oName = "#" + std::to_string(iPlate);
        }

        plateElem.TryReadElementFloat("Length", forcePlate.fLength);
        plateElem.TryReadElementFloat("Width", forcePlate.fWidth);

        if (auto locationElem = plateElem.FindChild("Location"))
        {
            struct Corner
            {
                std::size_t index;
                const char* name;
            };

            constexpr Corner corners[]
            {
                {0, "Corner1"},
                {1, "Corner2"},
                {2, "Corner3"},
                {3, "Corner4"},
            };

            for (const auto& c : corners)
            {
                auto cornerElem = locationElem.FindChild(c.name);
                cornerElem.TryReadElementFloat("X", forcePlate.asCorner[c.index].fX);
                cornerElem.TryReadElementFloat("Y", forcePlate.asCorner[c.index].fY);
                cornerElem.TryReadElementFloat("Z", forcePlate.asCorner[c.index].fZ);
            }
        }

        if (auto originElem = plateElem.FindChild("Origin"))
        {
            originElem.TryReadElementFloat("X", forcePlate.sOrigin.fX);
            originElem.TryReadElementFloat("Y", forcePlate.sOrigin.fY);
            originElem.TryReadElementFloat("Z", forcePlate.sOrigin.fZ);
        }

        forcePlate.vChannels.clear();
        if (auto channelsElem = plateElem.FindChild("Channels"))
        {
            SForceChannel forceChannel{};
            for (auto channelElem : ChildElementRange{channelsElem, "Channel"})
            {
                channelElem.TryReadElementUnsignedInt32("Channel_No", forceChannel.nChannelNumber);
                channelElem.TryReadElementFloat("ConversionFactor", forceChannel.fConversionFactor);
                forcePlate.vChannels.push_back(forceChannel);
            }
        }

        if (auto calibrationMatrix = plateElem.FindChild("Calibration_Matrix"))
        {
            if (mMajorVersion == 1 && mMinorVersion < 12)
            {
                auto getRowStr = [](auto& buff, std::size_t buffSize, std::size_t index)-> const char* {
                    sprintf_s(buff, buffSize, "Row%zd", index + 1);
                    return buff;
                };

                auto getColStr = [](auto& buff, std::size_t buffSize, std::size_t index)-> const char* {
                    sprintf_s(buff, buffSize, "Col%zd", index + 1);
                    return buff;
                };

                unsigned int iRow = 0;
                for (auto row : ChildElementRange{calibrationMatrix, getRowStr})
                {
                    unsigned int iCol = 0;
                    for (auto col : ChildElementRange{row, getColStr})
                    {
                        forcePlate.afCalibrationMatrix[iRow][iCol++] = col.ReadFloat();
                    }
                    iRow++;
                    forcePlate.nCalibrationMatrixColumns = iCol;
                }
                forcePlate.nCalibrationMatrixRows = iRow;
                forcePlate.bValidCalibrationMatrix = true;
            }
            else
            {
                auto rows = calibrationMatrix.FindChild("Rows");
                if (rows)
                {
                    unsigned int iRow = 0;
                    for (auto rowElement : ChildElementRange{rows, "Row"})
                    {
                        auto columns = rowElement.FindChild("Columns");
                        if (columns)
                        {
                            unsigned int iCol = 0;
                            for (const auto col : ChildElementRange{columns, "Column"})
                            {
                                forcePlate.afCalibrationMatrix[iRow][iCol++] = col.ReadFloat();
                            }
                            forcePlate.nCalibrationMatrixColumns = iCol;
                        }
                        iRow++;
                    }
                    forcePlate.nCalibrationMatrixRows = iRow;
                    forcePlate.bValidCalibrationMatrix = true;
                }
            }
        }

        dataAvailable = true;
        forceSettings.vsForcePlates.push_back(forcePlate);
    }

    return true;
} // Read force settings

bool SettingsDeserializer::DeserializeImageSettings(std::vector<SImageCamera>& imageSettings, bool& dataAvailable)
{
    dataAvailable = false;

    imageSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto imageElem = mDeserializer->FindChild("Image");
    if (!imageElem)
    {
        return true;
    }

    for (auto cameraElem : ChildElementRange{imageElem, "Camera"})
    {
        SImageCamera imageCamera{};

        if (!cameraElem.TryReadElementUnsignedInt32("ID", imageCamera.nID))
        {
            return false;
        }

        if (!cameraElem.TryReadElementBool("Enabled", imageCamera.bEnabled))
        {
            return false;
        }

        std::string format;
        if (!cameraElem.TryReadElementString("Format", format))
        {
            return false;
        }

        format = ToLowerXmlString(format);
        if (format == "rawgrayscale")
        {
            imageCamera.eFormat = CRTPacket::FormatRawGrayscale;
        }
        else if (format == "rawbgr")
        {
            imageCamera.eFormat = CRTPacket::FormatRawBGR;
        }
        else if (format == "jpg")
        {
            imageCamera.eFormat = CRTPacket::FormatJPG;
        }
        else if (format == "png")
        {
            imageCamera.eFormat = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!cameraElem.TryReadElementUnsignedInt32("Width", imageCamera.nWidth)
            || !cameraElem.TryReadElementUnsignedInt32("Height", imageCamera.nHeight))
        {
            return false;
        }

        if (!cameraElem.TryReadElementFloat("Left_Crop", imageCamera.fCropLeft)
            || !cameraElem.TryReadElementFloat("Top_Crop", imageCamera.fCropTop)
            || !cameraElem.TryReadElementFloat("Right_Crop", imageCamera.fCropRight)
            || !cameraElem.TryReadElementFloat("Bottom_Crop", imageCamera.fCropBottom))
        {
            return false;
        }

        dataAvailable = true;
        imageSettings.push_back(imageCamera);
    }

    return true;
} // ReadImageSettings

namespace
{
    SPosition ReadSPosition(DeserializerApi& parentElem, const std::string& element)
    {
        auto positionElem = parentElem.FindChild(element.data());
        if (positionElem)
        {
            return {
                positionElem.ReadAttributeDouble("X"),
                positionElem.ReadAttributeDouble("Y"),
                positionElem.ReadAttributeDouble("Z"),
            };
        }

        return {};
    }

    SRotation ReadSRotation(DeserializerApi& parentElem, const std::string& element)
    {
        auto rotationElem = parentElem.FindChild(element.data());
        if (rotationElem)
        {
            return {
                rotationElem.ReadAttributeDouble("X"),
                rotationElem.ReadAttributeDouble("Y"),
                rotationElem.ReadAttributeDouble("Z"),
                rotationElem.ReadAttributeDouble("W")
            };
        }

        return {};
    }

    bool TryReadSDegreeOfFreedom(DeserializerApi& parentElement, const std::string& elementName,
                                 std::vector<SDegreeOfFreedom>& degreesOfFreedom)
    {
        SDegreeOfFreedom degreeOfFreedom;

        auto degreeOfFreedomElement = parentElement.FindChild(elementName.data());
        if (!degreeOfFreedomElement)
        {
            return false;
        }

        degreeOfFreedom.type = SkeletonStringToDofSettings(elementName);

        if (auto constraintElem = degreeOfFreedomElement.FindChild("Constraint"))
        {
            degreeOfFreedom.lowerBound = constraintElem.ReadAttributeDouble("LowerBound");
            degreeOfFreedom.upperBound = constraintElem.ReadAttributeDouble("UpperBound");
        }
        else
        {
            degreeOfFreedom.lowerBound = degreeOfFreedomElement.ReadAttributeDouble("LowerBound");
            degreeOfFreedom.upperBound = degreeOfFreedomElement.ReadAttributeDouble("UpperBound");
        }

        if (auto couplingsElem = degreeOfFreedomElement.FindChild("Couplings"))
        {
            for (auto couplingElem : ChildElementRange{couplingsElem, "Coupling"})
            {
                SCoupling coupling{};
                coupling.segment = couplingElem.ReadAttributeString("Segment");
                auto dof = couplingElem.ReadAttributeString("DegreeOfFreedom");
                coupling.degreeOfFreedom = SkeletonStringToDofSettings(dof);
                coupling.coefficient = couplingElem.ReadAttributeDouble("Coefficient");
                degreeOfFreedom.couplings.push_back(coupling);
            }
        }

        if (auto goalElem = degreeOfFreedomElement.FindChild("Goal"))
        {
            degreeOfFreedom.goalValue = goalElem.ReadAttributeDouble("Value");
            degreeOfFreedom.goalWeight = goalElem.ReadAttributeDouble("Weight");
        }

        degreesOfFreedom.push_back(degreeOfFreedom);

        return true;
    }
}

bool SettingsDeserializer::DeserializeSkeletonSettings(bool skeletonGlobalData,
                                                        std::vector<SSettingsSkeletonHierarchical>&
                                                        skeletonSettingsHierarchical,
                                                        std::vector<SSettingsSkeleton>& skeletonSettings,
                                                        bool& dataAvailable)
{
    dataAvailable = false;
    skeletonSettings.clear();
    skeletonSettingsHierarchical.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto skeletonsElem = mDeserializer->FindChild("Skeletons");
    if (!skeletonsElem)
    {
        return true;
    }

    int segmentIndex;
    std::map<int, int> segmentIdIndexMap;

    if (mMajorVersion > 1 || mMinorVersion > 20)
    {
        for (auto skeletonElem : ChildElementRange{skeletonsElem, "Skeleton"})
        {
            SSettingsSkeletonHierarchical skeletonHierarchical{};
            SSettingsSkeleton skeleton{};
            segmentIndex = 0;

            skeletonHierarchical.name = skeletonElem.ReadAttributeString("Name");
            skeleton.name = skeletonHierarchical.name;

            skeletonElem.TryReadElementString("Solver", skeletonHierarchical.rootSegment.solver);
            skeletonElem.TryReadElementDouble("Scale", skeletonHierarchical.scale);

            if (auto segmentsElem = skeletonElem.FindChild("Segments"))
            {
                std::function<void(DeserializerApi&, SSettingsSkeletonSegmentHierarchical&,
                                   std::vector<SSettingsSkeletonSegment>&, std::uint32_t)> recurseSegments
                    = [&recurseSegments, &segmentIdIndexMap, &segmentIndex, &skeleton](
                    DeserializerApi& segmentElem, SSettingsSkeletonSegmentHierarchical& segmentHierarchical,
                    std::vector<SSettingsSkeletonSegment>& segments, std::uint32_t parentId)
                {
                    segmentHierarchical.name = segmentElem.ReadAttributeString("Name");

                    segmentElem.TryReadElementUnsignedInt32("ID", segmentHierarchical.id);
                    segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                    segmentElem.TryReadElementString("Solver", segmentHierarchical.solver);

                    if (auto transformElem = segmentElem.FindChild("Transform"))
                    {
                        segmentHierarchical.position = ReadSPosition(transformElem, "Position");
                        segmentHierarchical.rotation = ReadSRotation(transformElem, "Rotation");
                    }

                    if (auto defaultTransformElem = segmentElem.FindChild("DefaultTransform"))
                    {
                        segmentHierarchical.defaultPosition = ReadSPosition(defaultTransformElem, "Position");
                        segmentHierarchical.defaultRotation = ReadSRotation(defaultTransformElem, "Rotation");
                    }

                    if (auto degreesOfFreedomElem = segmentElem.FindChild("DegreesOfFreedom"))
                    {
                        TryReadSDegreeOfFreedom(degreesOfFreedomElem, "RotationX",
                                                segmentHierarchical.degreesOfFreedom);
                        TryReadSDegreeOfFreedom(degreesOfFreedomElem, "RotationY",
                                                segmentHierarchical.degreesOfFreedom);
                        TryReadSDegreeOfFreedom(degreesOfFreedomElem, "RotationZ",
                                                segmentHierarchical.degreesOfFreedom);
                        TryReadSDegreeOfFreedom(degreesOfFreedomElem, "TranslationX",
                                                segmentHierarchical.degreesOfFreedom);
                        TryReadSDegreeOfFreedom(degreesOfFreedomElem, "TranslationY",
                                                segmentHierarchical.degreesOfFreedom);
                        TryReadSDegreeOfFreedom(degreesOfFreedomElem, "TranslationZ",
                                                segmentHierarchical.degreesOfFreedom);
                    }

                    segmentHierarchical.endpoint = ReadSPosition(segmentElem, "Endpoint");

                    if (auto markersElem = segmentElem.FindChild("Markers"))
                    {
                        for (auto markerElem : ChildElementRange{markersElem, "Marker"})
                        {
                            SMarker marker;

                            marker.name = markerElem.ReadAttributeString("Name");

                            marker.position = ReadSPosition(markerElem, "Position");

                            if (!markerElem.TryReadElementDouble("Weight", marker.weight))
                            {
                                marker.weight = 1.0;
                            }

                            segmentHierarchical.markers.push_back(marker);
                        }
                    }

                    if (auto rigidBodiesElem = segmentElem.FindChild("Markers"))
                    {
                        for (auto rigidBodyElem : ChildElementRange{rigidBodiesElem, "RigidBody"})
                        {
                            SBody body;

                            body.name = rigidBodyElem.ReadAttributeString("Name");

                            auto rbodyTransformElem = rigidBodyElem.FindChild("Transform");
                            if (rbodyTransformElem)
                            {
                                body.position = ReadSPosition(rbodyTransformElem, "Position");
                                body.rotation = ReadSRotation(rbodyTransformElem, "Rotation");
                            }

                            if (!rbodyTransformElem.TryReadElementDouble("Weight", body.weight))
                            {
                                body.weight = 1.0;
                            }

                            segmentHierarchical.bodies.push_back(body);
                        }
                    }

                    SSettingsSkeletonSegment segment;
                    segment.name = segmentHierarchical.name;
                    segment.id = segmentHierarchical.id;
                    segment.parentId = parentId;
                    segment.parentIndex = (parentId != -1) ? segmentIdIndexMap[parentId] : -1;
                    segment.positionX = static_cast<float>(segmentHierarchical.defaultPosition.x);
                    segment.positionY = static_cast<float>(segmentHierarchical.defaultPosition.y);
                    segment.positionZ = static_cast<float>(segmentHierarchical.defaultPosition.z);
                    segment.rotationX = static_cast<float>(segmentHierarchical.defaultRotation.x);
                    segment.rotationY = static_cast<float>(segmentHierarchical.defaultRotation.y);
                    segment.rotationZ = static_cast<float>(segmentHierarchical.defaultRotation.z);
                    segment.rotationW = static_cast<float>(segmentHierarchical.defaultRotation.w);

                    segments.push_back(segment);

                    for (auto childSegmentElem : ChildElementRange{segmentElem, "Segment"})
                    {
                        SSettingsSkeletonSegmentHierarchical childSegment;
                        recurseSegments(childSegmentElem, childSegment, skeleton.segments, segmentHierarchical.id);
                        segmentHierarchical.segments.push_back(childSegment);
                    }
                };

                if (auto rootSegmentElem = segmentsElem.FindChild("Segment"))
                {
                    recurseSegments(rootSegmentElem, skeletonHierarchical.rootSegment, skeleton.segments, -1);
                }
            }

            dataAvailable = true;
            skeletonSettings.push_back(skeleton);
            skeletonSettingsHierarchical.push_back(skeletonHierarchical);
        }

        return true;
    }

    for (auto skeletonElem : ChildElementRange{skeletonsElem, "Skeleton"})
    {
        SSettingsSkeleton skeleton{};
        segmentIndex = 0;
        skeleton.name = skeletonElem.ReadAttributeString("Name");
        for (auto segmentElem : ChildElementRange{skeletonElem, "Segment"})
        {
            SSettingsSkeletonSegment segment{};
            segment.name = segmentElem.ReadAttributeString("Name");
            segment.id = segmentElem.ReadAttributeUnsignedInt("ID");
            segmentIdIndexMap[segment.id] = segmentIndex++;
            segment.parentId = segmentElem.ReadAttributeInt("Parent_ID", -1);
            segment.parentIndex = -1;
            if (segmentIdIndexMap.count(segment.parentId) > 0)
            {
                segment.parentIndex = segmentIdIndexMap[segment.parentId];
            }

            if (auto positionElement = segmentElem.FindChild("Position"))
            {
                segment.positionX = positionElement.ReadAttributeFloat("X");
                segment.positionY = positionElement.ReadAttributeFloat("Y");
                segment.positionZ = positionElement.ReadAttributeFloat("Z");
            }

            if (auto rotationElement = segmentElem.FindChild("Rotation"))
            {
                segment.rotationX = rotationElement.ReadAttributeFloat("X");
                segment.rotationY = rotationElement.ReadAttributeFloat("Y");
                segment.rotationZ = rotationElement.ReadAttributeFloat("Z");
                segment.rotationW = rotationElement.ReadAttributeFloat("W");
            }

            skeleton.segments.push_back(segment);
        }

        dataAvailable = true;
        skeletonSettings.push_back(skeleton);
    }

    return true;
} // ReadSkeletonSettings

namespace
{
    bool TryReadXmlFov(std::string name, DeserializerApi& parentElement, SCalibrationFov& fov)
    {
        auto childElement = parentElement.FindChild(name.data());
        if (!childElement)
        {
            return false;
        }

        fov.left = childElement.ReadAttributeUnsignedInt("left");
        fov.top = childElement.ReadAttributeUnsignedInt("top");
        fov.right = childElement.ReadAttributeUnsignedInt("right");
        fov.bottom = childElement.ReadAttributeUnsignedInt("bottom");

        return true;
    }
}

bool SettingsDeserializer::DeserializeCalibrationSettings(SCalibration& calibrationSettings)
{
    SCalibration settings{};

    if (!mDeserializer)
    {
        return true;
    }

    auto calibrationElem = mDeserializer->FindChild("calibration");
    if (!calibrationElem)
    {
        return false;
    }

    try
    {
        settings.calibrated = calibrationElem.ReadAttributeBool("calibrated");
        settings.source = calibrationElem.ReadAttributeString("source");
        settings.created = calibrationElem.ReadAttributeString("created");
        settings.qtm_version = calibrationElem.ReadAttributeString("qtm-version");

        std::string typeStr = ToLowerXmlString(calibrationElem.ReadAttributeString("type"));
        if (typeStr == "regular")
        {
            settings.type = ECalibrationType::regular;
        }
        if (typeStr == "refine")
        {
            settings.type = ECalibrationType::refine;
        }
        if (typeStr == "fixed")
        {
            settings.type = ECalibrationType::fixed;
        }

        if (settings.type == ECalibrationType::refine)
        {
            settings.refit_residual = calibrationElem.ReadAttributeDouble("refit-residual");
        }

        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = calibrationElem.ReadAttributeDouble("wandLength");
            settings.max_frames = calibrationElem.ReadAttributeUnsignedInt("maximumFrames");
            settings.short_arm_end = calibrationElem.ReadAttributeDouble("shortArmEnd");
            settings.long_arm_end = calibrationElem.ReadAttributeDouble("longArmEnd");
            settings.long_arm_middle = calibrationElem.ReadAttributeDouble("longArmMiddle");

            auto resultsElem = calibrationElem.FindChild("results");
            if (!resultsElem)
            {
                return false;
            }

            settings.result_std_dev = resultsElem.ReadAttributeDouble("std-dev");
            settings.result_min_max_diff = resultsElem.ReadAttributeDouble("min-max-diff");

            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = resultsElem.ReadAttributeDouble("refit-residual");
                settings.result_consecutive = resultsElem.ReadAttributeUnsignedInt("consecutive");
            }
        }

        auto camerasElem = calibrationElem.FindChild("cameras");
        if (!camerasElem)
        {
            return false;
        }

        for (auto cameraElem : ChildElementRange{camerasElem, "camera"})
        {
            SCalibrationCamera camera{};
            camera.active = cameraElem.ReadAttributeUnsignedInt("active") != 0;
            camera.calibrated = cameraElem.ReadAttributeBool("calibrated");
            camera.message = cameraElem.ReadAttributeString("message");

            camera.point_count = cameraElem.ReadAttributeUnsignedInt("point-count");
            camera.avg_residual = cameraElem.ReadAttributeDouble("avg-residual");
            camera.serial = cameraElem.ReadAttributeUnsignedInt("serial");
            camera.model = cameraElem.ReadAttributeString("model");
            camera.view_rotation = cameraElem.ReadAttributeUnsignedInt("viewrotation");

            if (!TryReadXmlFov("fov_marker", cameraElem, camera.fov_marker))
            {
                return false;
            }

            if (!TryReadXmlFov("fov_marker_max", cameraElem, camera.fov_marker_max))
            {
                return false;
            }

            if (!TryReadXmlFov("fov_video", cameraElem, camera.fov_video))
            {
                return false;
            }

            if (!TryReadXmlFov("fov_video_max", cameraElem, camera.fov_video_max))
            {
                return false;
            }

            auto transformElem = cameraElem.FindChild("transform");
            if (!transformElem)
            {
                return false;
            }

            camera.transform.x = transformElem.ReadAttributeDouble("x");
            camera.transform.y = transformElem.ReadAttributeDouble("y");
            camera.transform.z = transformElem.ReadAttributeDouble("z");
            camera.transform.r11 = transformElem.ReadAttributeDouble("r11");
            camera.transform.r12 = transformElem.ReadAttributeDouble("r12");
            camera.transform.r13 = transformElem.ReadAttributeDouble("r13");
            camera.transform.r21 = transformElem.ReadAttributeDouble("r21");
            camera.transform.r22 = transformElem.ReadAttributeDouble("r22");
            camera.transform.r23 = transformElem.ReadAttributeDouble("r23");
            camera.transform.r31 = transformElem.ReadAttributeDouble("r31");
            camera.transform.r32 = transformElem.ReadAttributeDouble("r32");
            camera.transform.r33 = transformElem.ReadAttributeDouble("r33");

            auto intrinsicElem = cameraElem.FindChild("intrinsic");
            if (!intrinsicElem)
            {
                return false;
            }

            camera.intrinsic.focal_length = intrinsicElem.ReadAttributeDouble("focallength", 0);
            camera.intrinsic.sensor_min_u = intrinsicElem.ReadAttributeDouble("sensorMinU");
            camera.intrinsic.sensor_max_u = intrinsicElem.ReadAttributeDouble("sensorMaxU");
            camera.intrinsic.sensor_min_v = intrinsicElem.ReadAttributeDouble("sensorMinV");
            camera.intrinsic.sensor_max_v = intrinsicElem.ReadAttributeDouble("sensorMaxV");
            camera.intrinsic.focal_length_u = intrinsicElem.ReadAttributeDouble("focalLengthU");
            camera.intrinsic.focal_length_v = intrinsicElem.ReadAttributeDouble("focalLengthV");
            camera.intrinsic.center_point_u = intrinsicElem.ReadAttributeDouble("centerPointU");
            camera.intrinsic.center_point_v = intrinsicElem.ReadAttributeDouble("centerPointV");
            camera.intrinsic.skew = intrinsicElem.ReadAttributeDouble("skew");
            camera.intrinsic.radial_distortion_1 = intrinsicElem.ReadAttributeDouble("radialDistortion1");
            camera.intrinsic.radial_distortion_2 = intrinsicElem.ReadAttributeDouble("radialDistortion2");
            camera.intrinsic.radial_distortion_3 = intrinsicElem.ReadAttributeDouble("radialDistortion3");
            camera.intrinsic.tangental_distortion_1 = intrinsicElem.ReadAttributeDouble("tangentalDistortion1");
            camera.intrinsic.tangental_distortion_2 = intrinsicElem.ReadAttributeDouble("tangentalDistortion2");
            settings.cameras.push_back(camera);
        }
    }
    catch (...)
    {
        return false;
    }

    calibrationSettings = settings;
    return true;
}
