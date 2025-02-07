#include "Tinyxml2Deserializer.h"
#include "Deserializer.h"
#include "Settings.h"

#include <functional>
#include <map>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

CTinyxml2Deserializer::CTinyxml2Deserializer(const char* data, std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mDeserializer{nullptr}
{
    mDeserializer = {data};
}

namespace
{
    void AddFlag(EProcessingActions flag, EProcessingActions& target)
    {
        target = static_cast<EProcessingActions>(target + flag);
    }
}

bool CTinyxml2Deserializer::DeserializeGeneralSettings(SSettingsGeneral& generalSettings)
{
    generalSettings.vsCameras.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto generalElem = mDeserializer.FirstChildElement("General");
    if (!generalElem)
    {
        return true;
    }

    if (auto frequencyElem = generalElem.FirstChildElement("Frequency"))
    {
        generalSettings.nCaptureFrequency = frequencyElem.UnsignedText(0);
    }
    else
    {
        return false;
    }

    if (auto captureTimeElem = generalElem.FirstChildElement("Capture_Time"))
    {
        generalSettings.fCaptureTime = captureTimeElem.FloatText(.0f);
    }
    else
    {
        return false;
    }

    if (!ReadXmlBool(generalElem, "Start_On_External_Trigger", generalSettings.bStartOnExternalTrigger))
    {
        return false;
    }

    if (mMajorVersion > 1 || mMinorVersion > 14)
    {
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NO", generalSettings.bStartOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NC", generalSettings.bStartOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_Software", generalSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }

    if (auto extTimeBaseElem = generalElem.FirstChildElement("External_Time_Base"))
    {
        if (!ReadXmlBool(extTimeBaseElem, "Enabled", generalSettings.sExternalTimebase.bEnabled))
        {
            return false;
        }

        std::string signalSource;
        if (!TryReadElementString(extTimeBaseElem, "Signal_Source", signalSource))
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
        if (!TryReadElementString(extTimeBaseElem, "Signal_Mode", signalMode))
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
        if (!TryReadElementUnsignedInt32(extTimeBaseElem, "Frequency_Multiplier",
                                         generalSettings.sExternalTimebase.nFreqMultiplier))
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(extTimeBaseElem, "Frequency_Divisor",
                                         generalSettings.sExternalTimebase.nFreqDivisor))
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(extTimeBaseElem, "Frequency_Tolerance",
                                         generalSettings.sExternalTimebase.nFreqTolerance))
        {
            return false;
        }

        if (!TryReadElementFloat(extTimeBaseElem, "Nominal_Frequency",
                                 generalSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (TryReadElementString(extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
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
        if (TryReadElementString(extTimeBaseElem, "Signal_Edge", signalEdge))
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

        if (!TryReadElementFloat(extTimeBaseElem, "Nominal_Frequency",
                                 generalSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (TryReadElementString(extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
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

        if (!TryReadElementUnsignedInt32(extTimeBaseElem, "Signal_Shutter_Delay",
                                         generalSettings.sExternalTimebase.nSignalShutterDelay))
        {
            return false;
        }

        if (!TryReadElementFloat(extTimeBaseElem, "Non_Periodic_Timeout",
                                 generalSettings.sExternalTimebase.fNonPeriodicTimeout))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (auto externalTimestampElem = generalElem.FirstChildElement("External_Timestamp"))
    {
        if (!ReadXmlBool(externalTimestampElem, "Enabled", generalSettings.sTimestamp.bEnabled))
        {
            return false;
        }

        std::string type;
        if (TryReadElementString(externalTimestampElem, "Type", type))
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

        TryReadElementUnsignedInt32(externalTimestampElem, "Frequency", generalSettings.sTimestamp.nFrequency);
    }

    const char* processingActionTags[3] = {"Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions"};
    EProcessingActions* processingActions[3] =
    {
        &generalSettings.eProcessingActions,
        &generalSettings.eRtProcessingActions,
        &generalSettings.eReprocessingActions
    };

    auto AddFlagFromBoolElement = [this](Deserializer& parent, const char* elementName, EProcessingActions flag,
                                         EProcessingActions& target) -> bool
    {
        bool value;
        if (ReadXmlBool(parent, elementName, value))
        {
            if (value)
            {
                AddFlag(flag, target);
            }
            return true;
        }
        else
        {
            return false;
        }
    };

    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================

        auto processingElem = generalElem.FirstChildElement(processingActionTags[i]);
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
        if (!TryReadElementString(processingElem, "Tracking", trackingMode))
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

    auto eulerElem = generalElem.FirstChildElement("EulerAngles");
    if (eulerElem)
    {
        generalSettings.eulerRotations[0] = eulerElem.Attribute("First");
        generalSettings.eulerRotations[1] = eulerElem.Attribute("Second");
        generalSettings.eulerRotations[2] = eulerElem.Attribute("Third");
    }

    for (auto cameraElem : ChildElementRange{generalElem, "Camera"})
    {
        SSettingsGeneralCamera cameraSettings{};
        if (!TryReadElementUnsignedInt32(cameraElem, "ID", cameraSettings.nID))
        {
            return false;
        }
        std::string model;
        if (!TryReadElementString(cameraElem, "Model", model))
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

        ReadXmlBool(cameraElem, "Underwater", cameraSettings.bUnderwater);
        ReadXmlBool(cameraElem, "Supports_HW_Sync", cameraSettings.bSupportsHwSync);

        if (!TryReadElementUnsignedInt32(cameraElem, "Serial", cameraSettings.nSerial))
        {
            return false;
        }

        std::string mode;
        if (!TryReadElementString(cameraElem, "Mode", mode))
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
            if (!TryReadElementUnsignedInt32(cameraElem, "Video_Frequency", cameraSettings.nVideoFrequency))
            {
                return false;
            }
        }

        std::string videoResolution;
        if (TryReadElementString(cameraElem, "Video_Resolution", videoResolution))
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
        if (TryReadElementString(cameraElem, "Video_Aspect_Ratio", videoAspectRatio))
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

        if (auto videoExposureElem = cameraElem.FirstChildElement("Video_Exposure"))
        {
            if (!TryReadElementUnsignedInt32(videoExposureElem, "Current", cameraSettings.nVideoExposure)
                || !TryReadElementUnsignedInt32(videoExposureElem, "Min", cameraSettings.nVideoExposureMin)
                || !TryReadElementUnsignedInt32(videoExposureElem, "Max", cameraSettings.nVideoExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto videoFlashTime = cameraElem.FirstChildElement("Video_Flash_Time"))
        {
            if (!TryReadElementUnsignedInt32(videoFlashTime, "Current", cameraSettings.nVideoFlashTime)
                || !TryReadElementUnsignedInt32(videoFlashTime, "Min", cameraSettings.nVideoFlashTimeMin)
                || !TryReadElementUnsignedInt32(videoFlashTime, "Max", cameraSettings.nVideoFlashTimeMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto markerExposureElem = cameraElem.FirstChildElement("Marker_Exposure"))
        {
            if (!TryReadElementUnsignedInt32(markerExposureElem, "Current", cameraSettings.nMarkerExposure)
                || !TryReadElementUnsignedInt32(markerExposureElem, "Min", cameraSettings.nMarkerExposureMin)
                || !TryReadElementUnsignedInt32(markerExposureElem, "Max", cameraSettings.nMarkerExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto markerThresholdElem = cameraElem.FirstChildElement("Marker_Threshold"))
        {
            if (!TryReadElementUnsignedInt32(markerThresholdElem, "Current", cameraSettings.nMarkerThreshold)
                || !TryReadElementUnsignedInt32(markerThresholdElem, "Min", cameraSettings.nMarkerThresholdMin)
                || !TryReadElementUnsignedInt32(markerThresholdElem, "Max", cameraSettings.nMarkerThresholdMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto positionElem = cameraElem.FirstChildElement("Position"))
        {
            if (!TryReadElementFloat(positionElem, "X", cameraSettings.fPositionX)
                || !TryReadElementFloat(positionElem, "Y", cameraSettings.fPositionY)
                || !TryReadElementFloat(positionElem, "Z", cameraSettings.fPositionZ)
                || !TryReadElementFloat(positionElem, "Rot_1_1", cameraSettings.fPositionRotMatrix[0][0])
                || !TryReadElementFloat(positionElem, "Rot_2_1", cameraSettings.fPositionRotMatrix[1][0])
                || !TryReadElementFloat(positionElem, "Rot_3_1", cameraSettings.fPositionRotMatrix[2][0])
                || !TryReadElementFloat(positionElem, "Rot_1_2", cameraSettings.fPositionRotMatrix[0][1])
                || !TryReadElementFloat(positionElem, "Rot_2_2", cameraSettings.fPositionRotMatrix[1][1])
                || !TryReadElementFloat(positionElem, "Rot_3_2", cameraSettings.fPositionRotMatrix[2][1])
                || !TryReadElementFloat(positionElem, "Rot_1_3", cameraSettings.fPositionRotMatrix[0][2])
                || !TryReadElementFloat(positionElem, "Rot_2_3", cameraSettings.fPositionRotMatrix[1][2])
                || !TryReadElementFloat(positionElem, "Rot_3_3", cameraSettings.fPositionRotMatrix[2][2]))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(cameraElem, "Orientation", cameraSettings.nOrientation))
        {
            return false;
        }

        if (auto markerResElem = cameraElem.FirstChildElement("Marker_Res"))
        {
            if (!TryReadElementUnsignedInt32(markerResElem, "Width", cameraSettings.nMarkerResolutionWidth)
                || !TryReadElementUnsignedInt32(markerResElem, "Height", cameraSettings.nMarkerResolutionHeight))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto videoResElem = cameraElem.FirstChildElement("Video_Res"))
        {
            if (!TryReadElementUnsignedInt32(videoResElem, "Width", cameraSettings.nVideoResolutionWidth)
                || !TryReadElementUnsignedInt32(videoResElem, "Height", cameraSettings.nVideoResolutionHeight))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto markerFovElem = cameraElem.FirstChildElement("Marker_FOV"))
        {
            if (!TryReadElementUnsignedInt32(markerFovElem, "Left", cameraSettings.nMarkerFOVLeft)
                || !TryReadElementUnsignedInt32(markerFovElem, "Top", cameraSettings.nMarkerFOVTop)
                || !TryReadElementUnsignedInt32(markerFovElem, "Right", cameraSettings.nMarkerFOVRight)
                || !TryReadElementUnsignedInt32(markerFovElem, "Bottom", cameraSettings.nMarkerFOVBottom))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (auto videoFovElem = cameraElem.FirstChildElement("Marker_FOV"))
        {
            if (!TryReadElementUnsignedInt32(videoFovElem, "Left", cameraSettings.nVideoFOVLeft)
                || !TryReadElementUnsignedInt32(videoFovElem, "Top", cameraSettings.nVideoFOVTop)
                || !TryReadElementUnsignedInt32(videoFovElem, "Right", cameraSettings.nVideoFOVRight)
                || !TryReadElementUnsignedInt32(videoFovElem, "Bottom", cameraSettings.nVideoFOVBottom))
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

            auto syncOutElem = cameraElem.FirstChildElement(syncOutStr);
            if (syncOutElem)
            {
                if (port < 2)
                {
                    std::string mode;
                    if (!TryReadElementString(syncOutElem, "Mode", mode))
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
                        if (!TryReadElementUnsignedInt32(syncOutElem, "Value", cameraSettings.nSyncOutValue[port])
                            || !TryReadElementFloat(syncOutElem, "Duty_Cycle", cameraSettings.fSyncOutDutyCycle[port]))
                        {
                            return false;
                        }
                    }
                }

                if (port == 2 || (cameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    std::string signalPolarity;
                    if (TryReadElementString(syncOutElem, "Signal_Polarity", signalPolarity))
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

        if (auto lensControlElem = cameraElem.FirstChildElement("LensControl"))
        {
            auto focusElem = lensControlElem.FirstChildElement("Focus");
            if (focusElem)
            {
                cameraSettings.fFocus = focusElem.FloatAttribute("Value", std::numeric_limits<float>::quiet_NaN());
            }

            auto apertureElem = lensControlElem.FirstChildElement("Aperture");
            if (apertureElem)
            {
                cameraSettings.fAperture = apertureElem.
                    FloatAttribute("Value", std::numeric_limits<float>::quiet_NaN());
            }
        }
        else
        {
            cameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            cameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        if (auto autoExposureElem = cameraElem.FirstChildElement("AutoExposure"))
        {
            cameraSettings.autoExposureEnabled = autoExposureElem.BoolAttribute("Enabled", false);
            cameraSettings.autoExposureCompensation = autoExposureElem.FloatAttribute(
                "Compensation", std::numeric_limits<float>::quiet_NaN());
        }
        else
        {
            cameraSettings.autoExposureEnabled = false;
            cameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        bool autoWhiteBalance;
        if (ReadXmlBool(cameraElem, "AutoWhiteBalance", autoWhiteBalance))
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

bool CTinyxml2Deserializer::Deserialize3DSettings(SSettings3D& settings3D, bool& dataAvailable)
{
    dataAvailable = false;

    settings3D.pCalibrationTime[0] = 0;

    if (!mDeserializer)
    {
        return true;
    }

    auto threeDElem = mDeserializer.FirstChildElement("The_3D");
    if (!threeDElem)
    {
        return true;
    }

    if (auto axisUpwards = threeDElem.FirstChildElement("AxisUpwards"))
    {
        auto str = ToLowerXmlString(axisUpwards.GetText());
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

    if (auto calibrationTimeElem = threeDElem.FirstChildElement("CalibrationTime"))
    {
        auto str = calibrationTimeElem.GetText();
        strcpy_s(settings3D.pCalibrationTime, 32, str.data());
    }
    else
    {
        return false;
    }

    std::size_t labelCount;
    if (auto labelsElem = threeDElem.FirstChildElement("Labels"))
    {
        labelCount = labelsElem.IntText(0);
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
        if (auto nameElem = labelElem.FirstChildElement("Name"))
        {
            label.oName = nameElem.GetText();
        }

        if (auto colorElem = labelElem.FirstChildElement("RGBColor"))
        {
            label.nRGBColor = colorElem.IntText(0);
        }

        if (auto typeElem = labelElem.FirstChildElement("Trajectory_Type"))
        {
            label.type = typeElem.GetText();
        }

        settings3D.s3DLabels.push_back(label);
    }

    if (settings3D.s3DLabels.size() != labelCount)
    {
        return false;
    }

    if (auto bonesElem = threeDElem.FirstChildElement("Bones"))
    {
        for (auto boneElem : ChildElementRange{bonesElem, "Bone"})
        {
            SSettingsBone bone{};
            bone.fromName = boneElem.Attribute("From");
            bone.toName = boneElem.Attribute("To");
            bone.color = boneElem.UnsignedAttribute("Color", bone.color);
            settings3D.sBones.push_back(bone);
        }
    }

    dataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryRead6DofElementEnabled(std::uint32_t majorVer, std::uint32_t minorVer, Deserializer& deserializer,
                                   bool& target)
    {
        if (majorVer > 1 || minorVer > 23)
        {
            if (auto enabledElem = deserializer.FirstChildElement("Enabled"))
            {
                target = enabledElem.GetText() == "true";
                return true;
            }
        }

        // Enabled is default true for 6dof bodies
        target = true;
        return false;
    }


    bool TryReadAttributesRGBColor(Deserializer& deserializer, std::uint32_t& target)
    {
        if (auto elem = deserializer.FirstChildElement("Color"))
        {
            std::uint32_t colorR = elem.UnsignedAttribute("R");
            std::uint32_t colorG = elem.UnsignedAttribute("G");
            std::uint32_t colorB = elem.UnsignedAttribute("B");
            target = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);
            return true;
        }

        target = 0;
        return false;
    }

    bool TryReadSetFilter(Deserializer& deserializer, std::string& target)
    {
        if (auto elem = deserializer.FirstChildElement("Filter"))
        {
            target = elem.Attribute("Preset");
            return true;
        }

        return false;
    }

    bool TryReadSetPos(Deserializer& deserializer, float& targetX, float& targetY, float& targetZ)
    {
        if (auto elem = deserializer.FirstChildElement("Position"))
        {
            targetX = elem.FloatAttribute("X");
            targetY = elem.FloatAttribute("Y");
            targetZ = elem.FloatAttribute("Z");
            return true;
        }

        targetZ = targetY = targetX = .0f;
        return false;
    }

    bool TryReadSetRotation(Deserializer& deserializer, float& targetX, float& targetY, float& targetZ)
    {
        if (auto elem = deserializer.FirstChildElement("Rotation"))
        {
            targetX = elem.FloatAttribute("X");
            targetY = elem.FloatAttribute("Y");
            targetZ = elem.FloatAttribute("Z");
            return true;
        }

        targetZ = targetY = targetX = .0f;
        return false;
    }


    bool TryReadSetPoints(Deserializer& deserializer, std::vector<SBodyPoint>& target)
    {
        if (auto pointsElem = deserializer.FirstChildElement("Points"))
        {
            for (auto pointElem : ChildElementRange{pointsElem, "Point"})
            {
                SBodyPoint bodyPoint;

                bodyPoint.fX = pointElem.FloatAttribute("X");
                bodyPoint.fY = pointElem.FloatAttribute("Y");
                bodyPoint.fZ = pointElem.FloatAttribute("Z");

                bodyPoint.virtual_ = 0 != pointElem.UnsignedAttribute("Virtual");
                bodyPoint.physicalId = pointElem.UnsignedAttribute("PhysicalId");
                bodyPoint.name = pointElem.Attribute("Name");
                target.push_back(bodyPoint);
            }

            return true;
        }

        return false;
    }

    bool TryReadSetDataOrigin(Deserializer& deserializer, SOrigin& target)
    {
        if (auto elem = deserializer.FirstChildElement("Data_origin"))
        {
            target.type = static_cast<EOriginType>(elem.UnsignedText());
            target.position.fX = elem.FloatAttribute("X");
            target.position.fY = elem.FloatAttribute("Y");
            target.position.fZ = elem.FloatAttribute("Z");
            target.relativeBody = elem.UnsignedAttribute("Relative_body");
        }
        else
        {
            target = {};
            return false;
        }

        if (auto elem = deserializer.FirstChildElement("Data_orientation"))
        {
            char tmpStr[10];
            for (std::uint32_t i = 0; i < 9; i++)
            {
                (void)sprintf_s(tmpStr, 10, "R%u%u", (i / 3) + 1, (i % 3) + 1);
                target.rotation[i] = elem.FloatAttribute(tmpStr);
            }


            auto type = static_cast<EOriginType>(elem.UnsignedText());
            auto body = static_cast<std::uint32_t>(elem.UnsignedAttribute("Relative_body"));

            // Validation: type and relativeBody must be the same between orientation and origin
            return type == target.type && body == target.relativeBody;
        }

        target = {};
        return false;
    }

    bool TryReadElementRGBColor(Deserializer& deserializer, std::uint32_t& target)
    {
        if (auto elem = deserializer.FirstChildElement("RGBColor"))
        {
            target = elem.IntText();
            return true;
        }

        target = 0;
        return false;
    }

    bool TryReadSetPointsOld(Deserializer& deserializer, std::vector<SBodyPoint>& target)
    {
        target.clear();
        for (auto pointElem : ChildElementRange{deserializer, "Bone"})
        {
            SBodyPoint point;

            if (!TryReadElementFloat(pointElem, "X", point.fX))
            {
                return false;
            }

            if (!TryReadElementFloat(pointElem, "Y", point.fY))
            {
                return false;
            }

            if (!TryReadElementFloat(pointElem, "Z", point.fZ))
            {
                return false;
            }

            target.push_back(point);
        }
        return true;
    }

    bool TryReadSetEuler(Deserializer& deserializer, std::string& targetFirst, std::string& targetSecond,
                         std::string& targetThird)
    {
        if (auto elem = deserializer.FirstChildElement("Euler"))
        {
            return TryReadElementString(elem, "First", targetFirst)
                && TryReadElementString(elem, "Second", targetSecond)
                && TryReadElementString(elem, "Third", targetThird);
        }

        return false;
    }
}

bool CTinyxml2Deserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& settings6Dof,
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
    Deserializer sixDofElem = mDeserializer.FirstChildElement("The_6D");
    if (!sixDofElem)
    {
        return true; // NO eye tracker data available.
    }

    if (mMajorVersion > 1 || mMinorVersion > 20)
    {
        for (auto bodyElem : ChildElementRange{sixDofElem, "Body"})
        {
            SSettings6DOFBody bodySettings6Dof;

            if (!TryReadElementString(bodyElem, "Name", bodySettings6Dof.name))
            {
                // Name --- REQUIRED
                return false;
            }

            TryRead6DofElementEnabled(mMajorVersion, mMinorVersion, bodyElem, bodySettings6Dof.enabled);
            if (!TryReadAttributesRGBColor(bodyElem, bodySettings6Dof.color)
                || !TryReadElementFloat(bodyElem, "MaximumResidual", bodySettings6Dof.maxResidual)
                || !TryReadElementUnsignedInt32(bodyElem, "MinimumMarkersInBody", bodySettings6Dof.minMarkersInBody)
                || !TryReadElementFloat(bodyElem, "BoneLengthTolerance", bodySettings6Dof.boneLengthTolerance)
                || !TryReadSetFilter(bodyElem, bodySettings6Dof.filterPreset))
            {
                // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                return false;
            }


            if (auto meshElem = bodyElem.FirstChildElement("Mesh"))
            {
                if (!TryReadElementString(meshElem, "Name", bodySettings6Dof.mesh.name)
                    || !TryReadSetPos(meshElem, bodySettings6Dof.mesh.position.fX, bodySettings6Dof.mesh.position.fY,
                                      bodySettings6Dof.mesh.position.fZ)
                    || !TryReadSetRotation(meshElem, bodySettings6Dof.mesh.rotation.fX,
                                           bodySettings6Dof.mesh.rotation.fY, bodySettings6Dof.mesh.rotation.fZ)
                    || !TryReadElementFloat(meshElem, "Scale", bodySettings6Dof.mesh.scale)
                    || !TryReadElementFloat(meshElem, "Opacity", bodySettings6Dof.mesh.opacity))
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
        if (!mDeserializer.FirstChildElement("Bodies"))
        {
            return false;
        }

        for (auto bodyElem : ChildElementRange{mDeserializer, "Body"})
        {
            SSettings6DOFBody bodySettings6Dof{};

            // Name, RGBColor, Points(OLD) --- REQUIRED
            if (!TryReadElementString(bodyElem, "Name", bodySettings6Dof.name)
                || !TryReadElementRGBColor(bodyElem, bodySettings6Dof.color)
                || !TryReadSetPointsOld(bodyElem, bodySettings6Dof.points))
            {
                return false;
            }

            if (mMajorVersion > 1 || mMinorVersion > 15)
            {
                // Euler --- REQUIRED
                if (!TryReadSetEuler(mDeserializer, generalSettings.eulerRotations[0],
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

bool CTinyxml2Deserializer::DeserializeGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings,
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
    Deserializer gazeVectorElem = mDeserializer.FirstChildElement("Gaze_Vector");
    if (!gazeVectorElem)
    {
        return true; // NO eye tracker data available.
    }

    for (auto vectorElem : ChildElementRange{gazeVectorElem, "Vector"})
    {
        std::string name;
        if (auto nameElem = vectorElem.FirstChildElement("Name"))
        {
            name = nameElem.GetText();
        }
        else
        {
            return false;
        }

        float frequency = 0;
        if (auto frequencyElem = vectorElem.FirstChildElement("Frequency"))
        {
            frequency = frequencyElem.FloatText();
        }

        bool hwSync = false;
        ReadXmlBool(vectorElem, "Hardware_Sync", hwSync);

        bool filter = false;
        ReadXmlBool(vectorElem, "Filter", filter);

        gazeVectorSettings.push_back({name, frequency, hwSync, filter});
    }

    dataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CTinyxml2Deserializer::DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings,
                                                          bool& dataAvailable)
{
    dataAvailable = false;

    eyeTrackerSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    Deserializer eyeTrackerElem = mDeserializer.FirstChildElement("Eye_Tracker");

    if (!eyeTrackerElem)
    {
        return true; // NO eye tracker data available.
    }

    for (auto deviceElem : ChildElementRange{eyeTrackerElem, "Device"})
    {
        std::string name;
        if (auto nameElem = deviceElem.FirstChildElement("Name"))
        {
            name = nameElem.GetText();
        }
        else
        {
            return false;
        }

        float frequency = 0;
        if (auto frequencyElem = deviceElem.FirstChildElement("Frequency"))
        {
            frequency = frequencyElem.FloatText();
        }

        bool hwSync = false;
        ReadXmlBool(deviceElem, "Hardware_Sync", hwSync);

        eyeTrackerSettings.push_back({name, frequency, hwSync});
    }

    dataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CTinyxml2Deserializer::DeserializeAnalogSettings(std::vector<SAnalogDevice>& analogDeviceSettings,
                                                      bool& dataAvailable)
{
    dataAvailable = false;
    analogDeviceSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto analogElem = mDeserializer.FirstChildElement("Analog");
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
        if (!TryReadElementUnsignedInt32(analogElem, "Channels", analogDevice.nChannels)
            || !TryReadElementUnsignedInt32(analogElem, "Frequency", analogDevice.nFrequency)
            || !TryReadElementString(analogElem, "Unit", analogDevice.oUnit))
        {
            return false;
        }

        auto rangeElem = analogElem.FirstChildElement("Range");
        if (!rangeElem
            || !TryReadElementFloat(rangeElem, "Min", analogDevice.fMinRange)
            || !TryReadElementFloat(rangeElem, "Max", analogDevice.fMaxRange))
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
        if (!TryReadElementUnsignedInt32(deviceElem, "Device_ID", analogDevice.nDeviceID)
            || !TryReadElementString(deviceElem, "Device_Name", analogDevice.oName)
            || !TryReadElementUnsignedInt32(deviceElem, "Channels", analogDevice.nChannels)
            || !TryReadElementUnsignedInt32(deviceElem, "Frequency", analogDevice.nFrequency)
        )
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            if (!TryReadElementString(analogElem, "Unit", analogDevice.oUnit))
            {
                continue;
            }
        }

        auto rangeElem = deviceElem.FirstChildElement("Range");
        if (!rangeElem
            || !TryReadElementFloat(rangeElem, "Min", analogDevice.fMinRange)
            || !TryReadElementFloat(rangeElem, "Max", analogDevice.fMaxRange))
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            for (std::size_t i = 0; i < analogDevice.nChannels; i++)
            {
                std::string label;
                if (TryReadElementString(deviceElem, "Label", label))
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
                if (TryReadElementString(channelElem, "Label", label))
                {
                    analogDevice.voLabels.push_back(label);
                }

                std::string unit;
                if (TryReadElementString(channelElem, "Unit", unit))
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

bool CTinyxml2Deserializer::DeserializeForceSettings(SSettingsForce& forceSettings, bool& dataAvailable)
{
    dataAvailable = false;

    forceSettings.vsForcePlates.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto forceElem = mDeserializer.FirstChildElement("Force");
    if (!forceElem)
    {
        // No analog data available.
        return true;
    }

    SForcePlate forcePlate{};
    forcePlate.bValidCalibrationMatrix = false;
    forcePlate.nCalibrationMatrixRows = 6;
    forcePlate.nCalibrationMatrixColumns = 6;

    if (!TryReadElementString(forceElem, "Unit_Length", forceSettings.oUnitLength))
    {
        return false;
    }

    if (!TryReadElementString(forceElem, "Unit_Force", forceSettings.oUnitForce))
    {
        return false;
    }

    std::size_t iPlate = 0;
    for (auto plateElem : ChildElementRange{forceElem, "Plate"})
    {
        iPlate++;

        if (!TryReadElementUnsignedInt32(plateElem, "Plate_ID", forcePlate.nID))
        {
            if (!TryReadElementUnsignedInt32(plateElem, "Force_Plate_Index", forcePlate.nID))
            // Version 1.7 and earlier.
            {
                return false;
            }
        }

        if (!TryReadElementUnsignedInt32(plateElem, "Analog_Device_ID", forcePlate.nAnalogDeviceID))
        {
            forcePlate.nAnalogDeviceID = 0;
        }

        if (!TryReadElementUnsignedInt32(plateElem, "Frequency", forcePlate.nFrequency))
        {
            return false;
        }

        if (!TryReadElementString(plateElem, "Type", forcePlate.oType))
        {
            forcePlate.oType = "unknown";
        }

        if (!TryReadElementString(plateElem, "Name", forcePlate.oName))
        {
            forcePlate.oName = "#" + std::to_string(iPlate);
        }

        TryReadElementFloat(plateElem, "Length", forcePlate.fLength);
        TryReadElementFloat(plateElem, "Width", forcePlate.fWidth);

        if (auto locationElem = plateElem.FirstChildElement("Location"))
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
                auto cornerElem = locationElem.FirstChildElement(c.name);
                TryReadElementFloat(cornerElem, "X", forcePlate.asCorner[c.index].fX);
                TryReadElementFloat(cornerElem, "Y", forcePlate.asCorner[c.index].fY);
                TryReadElementFloat(cornerElem, "Z", forcePlate.asCorner[c.index].fZ);
            }
        }

        if (auto originElem = plateElem.FirstChildElement("Origin"))
        {
            TryReadElementFloat(originElem, "X", forcePlate.sOrigin.fX);
            TryReadElementFloat(originElem, "Y", forcePlate.sOrigin.fY);
            TryReadElementFloat(originElem, "Z", forcePlate.sOrigin.fZ);
        }

        forcePlate.vChannels.clear();
        if (auto channelsElem = plateElem.FirstChildElement("Channels"))
        {
            SForceChannel forceChannel{};
            for (auto channelElem : ChildElementRange{channelsElem, "Channel"})
            {
                TryReadElementUnsignedInt32(channelElem, "Channel_No", forceChannel.nChannelNumber);
                TryReadElementFloat(channelElem, "ConversionFactor", forceChannel.fConversionFactor);
                forcePlate.vChannels.push_back(forceChannel);
            }
        }

        if (auto calibrationMatrix = plateElem.FirstChildElement("Calibration_Matrix"))
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
                        forcePlate.afCalibrationMatrix[iRow][iCol++] = col.FloatText();
                    }
                    iRow++;
                    forcePlate.nCalibrationMatrixColumns = iCol;
                }
                forcePlate.nCalibrationMatrixRows = iRow;
                forcePlate.bValidCalibrationMatrix = true;
            }
            else
            {
                auto rows = calibrationMatrix.FirstChildElement("Rows");
                if (rows)
                {
                    unsigned int iRow = 0;
                    for (auto rowElement : ChildElementRange{rows, "Row"})
                    {
                        auto columns = rowElement.FirstChildElement("Columns");
                        if (columns)
                        {
                            unsigned int iCol = 0;
                            for (const auto col : ChildElementRange{columns, "Column"})
                            {
                                forcePlate.afCalibrationMatrix[iRow][iCol++] = col.FloatText();
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

bool CTinyxml2Deserializer::DeserializeImageSettings(std::vector<SImageCamera>& imageSettings, bool& dataAvailable)
{
    dataAvailable = false;

    imageSettings.clear();

    if (!mDeserializer)
    {
        return true;
    }

    auto imageElem = mDeserializer.FirstChildElement("Image");
    if (!imageElem)
    {
        return true;
    }

    for (auto camera : ChildElementRange{imageElem, "Camera"})
    {
        SImageCamera imageCamera{};

        if (!TryReadElementUnsignedInt32(camera, "ID", imageCamera.nID))
        {
            return false;
        }

        if (!ReadXmlBool(camera, "Enabled", imageCamera.bEnabled))
        {
            return false;
        }

        std::string format;
        if (!TryReadElementString(camera, "Format", format))
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

        if (!TryReadElementUnsignedInt32(camera, "Width", imageCamera.nWidth)
            || !TryReadElementUnsignedInt32(camera, "Height", imageCamera.nHeight))
        {
            return false;
        }

        if (!TryReadElementFloat(camera, "Left_Crop", imageCamera.fCropLeft)
            || !TryReadElementFloat(camera, "Top_Crop", imageCamera.fCropTop)
            || !TryReadElementFloat(camera, "Right_Crop", imageCamera.fCropRight)
            || !TryReadElementFloat(camera, "Bottom_Crop", imageCamera.fCropBottom))
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
    bool TryReadSDegreeOfFreedom(Deserializer& parentElement, const std::string& elementName,
                                 std::vector<SDegreeOfFreedom>& degreesOfFreedom)
    {
        SDegreeOfFreedom degreeOfFreedom;

        auto degreeOfFreedomElement = parentElement.FirstChildElement(elementName.data());
        if (!degreeOfFreedomElement)
        {
            return false;
        }

        degreeOfFreedom.type = SkeletonStringToDofSettings(elementName);

        if (auto constraintElem = degreeOfFreedomElement.FirstChildElement("Constraint"))
        {
            degreeOfFreedom.lowerBound = constraintElem.DoubleAttribute("LowerBound");
            degreeOfFreedom.upperBound = constraintElem.DoubleAttribute("UpperBound");
        }
        else
        {
            degreeOfFreedom.lowerBound = degreeOfFreedomElement.DoubleAttribute("LowerBound");
            degreeOfFreedom.upperBound = degreeOfFreedomElement.DoubleAttribute("UpperBound");
        }

        if (auto couplingsElem = degreeOfFreedomElement.FirstChildElement("Couplings"))
        {
            for (auto couplingElem : ChildElementRange{couplingsElem, "Coupling"})
            {
                SCoupling coupling{};
                coupling.segment = couplingElem.Attribute("Segment");
                auto dof = couplingElem.Attribute("DegreeOfFreedom");
                coupling.degreeOfFreedom = SkeletonStringToDofSettings(dof);
                coupling.coefficient = couplingElem.DoubleAttribute("Coefficient");
                degreeOfFreedom.couplings.push_back(coupling);
            }
        }

        if (auto goalElem = degreeOfFreedomElement.FirstChildElement("Goal"))
        {
            degreeOfFreedom.goalValue = goalElem.DoubleAttribute("Value");
            degreeOfFreedom.goalWeight = goalElem.DoubleAttribute("Weight");
        }

        degreesOfFreedom.push_back(degreeOfFreedom);

        return true;
    }
}

bool CTinyxml2Deserializer::DeserializeSkeletonSettings(bool skeletonGlobalData,
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

    auto skeletonsElem = mDeserializer.FirstChildElement("Skeletons");
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

            skeletonHierarchical.name = skeletonElem.Attribute("Name");
            skeleton.name = skeletonHierarchical.name;

            TryReadElementString(skeletonElem, "Solver", skeletonHierarchical.rootSegment.solver);
            TryReadElementDouble(skeletonElem, "Scale", skeletonHierarchical.scale);

            if (auto segmentsElem = skeletonElem.FirstChildElement("Segments"))
            {
                std::function<void(Deserializer&, SSettingsSkeletonSegmentHierarchical&,
                                   std::vector<SSettingsSkeletonSegment>&, std::uint32_t)> recurseSegments
                    = [&recurseSegments, &segmentIdIndexMap, &segmentIndex, &skeleton](
                    Deserializer& segmentElem, SSettingsSkeletonSegmentHierarchical& segmentHierarchical,
                    std::vector<SSettingsSkeletonSegment>& segments, std::uint32_t parentId)
                {
                    segmentHierarchical.name = segmentElem.Attribute("Name");

                    TryReadElementUnsignedInt32(segmentElem, "ID", segmentHierarchical.id);
                    segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                    TryReadElementString(segmentElem, "Solver", segmentHierarchical.solver);

                    if (auto transformElem = segmentElem.FirstChildElement("Transform"))
                    {
                        segmentHierarchical.position = ReadSPosition(transformElem, "Position");
                        segmentHierarchical.rotation = ReadSRotation(transformElem, "Rotation");
                    }

                    if (auto defaultTransformElem = segmentElem.FirstChildElement("DefaultTransform"))
                    {
                        segmentHierarchical.defaultPosition = ReadSPosition(defaultTransformElem, "Position");
                        segmentHierarchical.defaultRotation = ReadSRotation(defaultTransformElem, "Rotation");
                    }

                    if (auto degreesOfFreedomElem = segmentElem.FirstChildElement("DegreesOfFreedom"))
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

                    if (auto markersElem = segmentElem.FirstChildElement("Markers"))
                    {
                        for (auto markerElem : ChildElementRange{markersElem, "Marker"})
                        {
                            SMarker marker;

                            marker.name = markerElem.Attribute("Name");

                            marker.position = ReadSPosition(markerElem, "Position");

                            if (!TryReadElementDouble(markerElem, "Weight", marker.weight))
                            {
                                marker.weight = 1.0;
                            }

                            segmentHierarchical.markers.push_back(marker);
                        }
                    }

                    if (auto rigidBodiesElem = segmentElem.FirstChildElement("Markers"))
                    {
                        for (auto rigidBodyElem : ChildElementRange{rigidBodiesElem, "RigidBody"})
                        {
                            SBody body;

                            body.name = rigidBodyElem.Attribute("Name");

                            auto rbodyTransformElem = rigidBodyElem.FirstChildElement("Transform");
                            if (rbodyTransformElem)
                            {
                                body.position = ReadSPosition(rbodyTransformElem, "Position");
                                body.rotation = ReadSRotation(rbodyTransformElem, "Rotation");
                            }

                            if (!TryReadElementDouble(rbodyTransformElem, "Weight", body.weight))
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

                if (auto rootSegmentElem = segmentsElem.FirstChildElement("Segment"))
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
        skeleton.name = skeletonElem.Attribute("Name");
        for (auto segmentElem : ChildElementRange{skeletonElem, "Segment"})
        {
            SSettingsSkeletonSegment segment{};
            segment.name = segmentElem.Attribute("Name");
            segment.id = segmentElem.UnsignedAttribute("ID");
            segmentIdIndexMap[segment.id] = segmentIndex++;
            segment.parentId = segmentElem.IntAttribute("Parent_ID", -1);
            segment.parentIndex = -1;
            if (segmentIdIndexMap.count(segment.parentId) > 0)
            {
                segment.parentIndex = segmentIdIndexMap[segment.parentId];
            }


            if (auto positionElement = segmentElem.FirstChildElement("Position"))
            {
                segment.positionX = positionElement.FloatAttribute("X");
                segment.positionY = positionElement.FloatAttribute("Y");
                segment.positionZ = positionElement.FloatAttribute("Z");
            }

            if (auto rotationElement = segmentElem.FirstChildElement("Rotation"))
            {
                segment.rotationX = rotationElement.FloatAttribute("X");
                segment.rotationY = rotationElement.FloatAttribute("Y");
                segment.rotationZ = rotationElement.FloatAttribute("Z");
                segment.rotationW = rotationElement.FloatAttribute("W");
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
    bool TryReadXmlFov(std::string name, Deserializer& parentElement, SCalibrationFov& fov)
    {
        auto childElement = parentElement.FirstChildElement(name.data());
        if (!childElement)
        {
            return false;
        }

        fov.left = childElement.UnsignedAttribute("left");
        fov.top = childElement.UnsignedAttribute("top");
        fov.right = childElement.UnsignedAttribute("right");
        fov.bottom = childElement.UnsignedAttribute("bottom");

        return true;
    }
}

bool CTinyxml2Deserializer::DeserializeCalibrationSettings(SCalibration& calibrationSettings)
{
    SCalibration settings{};

    if (!mDeserializer)
    {
        return true;
    }

    auto calibrationElem = mDeserializer.FirstChildElement("calibration");
    if (!calibrationElem)
    {
        return false;
    }

    try
    {
        settings.calibrated = calibrationElem.BoolAttribute("calibrated");
        settings.source = calibrationElem.Attribute("source");
        settings.created = calibrationElem.Attribute("created");
        settings.qtm_version = calibrationElem.Attribute("qtm-version");

        std::string typeStr = ToLowerXmlString(calibrationElem.Attribute("type"));
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
            settings.refit_residual = calibrationElem.DoubleAttribute("refit-residual");
        }

        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = calibrationElem.DoubleAttribute("wandLength");
            settings.max_frames = calibrationElem.UnsignedAttribute("maximumFrames");
            settings.short_arm_end = calibrationElem.DoubleAttribute("shortArmEnd");
            settings.long_arm_end = calibrationElem.DoubleAttribute("longArmEnd");
            settings.long_arm_middle = calibrationElem.DoubleAttribute("longArmMiddle");

            auto resultsElem = calibrationElem.FirstChildElement("results");
            if (!resultsElem)
            {
                return false;
            }

            settings.result_std_dev = resultsElem.DoubleAttribute("std-dev");
            settings.result_min_max_diff = resultsElem.DoubleAttribute("min-max-diff");

            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = resultsElem.DoubleAttribute("refit-residual");
                settings.result_consecutive = resultsElem.UnsignedAttribute("consecutive");
            }
        }

        auto camerasElem = calibrationElem.FirstChildElement("cameras");
        if (!camerasElem)
        {
            return false;
        }

        for (auto cameraElem : ChildElementRange{camerasElem, "camera"})
        {
            SCalibrationCamera camera{};
            camera.active = cameraElem.UnsignedAttribute("active") != 0;
            camera.calibrated = cameraElem.BoolAttribute("calibrated");
            camera.message = cameraElem.Attribute("message");

            camera.point_count = cameraElem.UnsignedAttribute("point-count");
            camera.avg_residual = cameraElem.DoubleAttribute("avg-residual");
            camera.serial = cameraElem.UnsignedAttribute("serial");
            camera.model = cameraElem.Attribute("model");
            camera.view_rotation = cameraElem.UnsignedAttribute("viewrotation");

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

            auto transformElem = cameraElem.FirstChildElement("transform");
            if (!transformElem)
            {
                return false;
            }

            camera.transform.x = transformElem.DoubleAttribute("x");
            camera.transform.y = transformElem.DoubleAttribute("y");
            camera.transform.z = transformElem.DoubleAttribute("z");
            camera.transform.r11 = transformElem.DoubleAttribute("r11");
            camera.transform.r12 = transformElem.DoubleAttribute("r12");
            camera.transform.r13 = transformElem.DoubleAttribute("r13");
            camera.transform.r21 = transformElem.DoubleAttribute("r21");
            camera.transform.r22 = transformElem.DoubleAttribute("r22");
            camera.transform.r23 = transformElem.DoubleAttribute("r23");
            camera.transform.r31 = transformElem.DoubleAttribute("r31");
            camera.transform.r32 = transformElem.DoubleAttribute("r32");
            camera.transform.r33 = transformElem.DoubleAttribute("r33");

            auto intrinsicElem = cameraElem.FirstChildElement("intrinsic");
            if (!intrinsicElem)
            {
                return false;
            }

            camera.intrinsic.focal_length = intrinsicElem.DoubleAttribute("focallength", 0);
            camera.intrinsic.sensor_min_u = intrinsicElem.DoubleAttribute("sensorMinU");
            camera.intrinsic.sensor_max_u = intrinsicElem.DoubleAttribute("sensorMaxU");
            camera.intrinsic.sensor_min_v = intrinsicElem.DoubleAttribute("sensorMinV");
            camera.intrinsic.sensor_max_v = intrinsicElem.DoubleAttribute("sensorMaxV");
            camera.intrinsic.focal_length_u = intrinsicElem.DoubleAttribute("focalLengthU");
            camera.intrinsic.focal_length_v = intrinsicElem.DoubleAttribute("focalLengthV");
            camera.intrinsic.center_point_u = intrinsicElem.DoubleAttribute("centerPointU");
            camera.intrinsic.center_point_v = intrinsicElem.DoubleAttribute("centerPointV");
            camera.intrinsic.skew = intrinsicElem.DoubleAttribute("skew");
            camera.intrinsic.radial_distortion_1 = intrinsicElem.DoubleAttribute("radialDistortion1");
            camera.intrinsic.radial_distortion_2 = intrinsicElem.DoubleAttribute("radialDistortion2");
            camera.intrinsic.radial_distortion_3 = intrinsicElem.DoubleAttribute("radialDistortion3");
            camera.intrinsic.tangental_distortion_1 = intrinsicElem.DoubleAttribute("tangentalDistortion1");
            camera.intrinsic.tangental_distortion_2 = intrinsicElem.DoubleAttribute("tangentalDistortion2");
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
