#include "Tinyxml2Deserializer.h"
#include <tinyxml2.h>

#include <algorithm>
#include <map>

#include "Settings.h"

#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;


namespace
{
    std::string ToLower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
        return str;
    }

    void RemoveInvalidChars(std::string& str)
    {
        auto isInvalidChar = [](int c) -> int
            {
                // iscntrl: control codes(NUL, etc.), '\t', '\n', '\v', '\f', '\r', backspace (DEL)
                // isspace: some common checks but also 0x20 (SPACE)
                // return != 0 --> invalid char
                return std::iscntrl(c) + std::isspace(c);
            };
        str.erase(std::remove_if(str.begin(), str.end(), isInvalidChar), str.end());
    }

    bool ReadXmlBool(tinyxml2::XMLElement* xml, const std::string& element, bool& value)
    {
        auto xmlElem = xml->FirstChildElement(element.c_str());
        if (!xmlElem)
        {
            return false;
        }

        auto str = std::string(xmlElem->GetText());
        RemoveInvalidChars(str);
        str = ToLower(str);

        if (str == "true")
        {
            value = true;
        }
        else if (str == "false")
        {
            value = false;
        }
        else
        {
            // Don't change value, just report error.
            return false;
        }

        return true;
    }

    SPosition ReadSPosition(tinyxml2::XMLElement& parentElem, const std::string& element)
    {
        auto positionElem = parentElem.FirstChildElement(element.data());
        if (positionElem)
        {
            return {
                positionElem->DoubleAttribute("X"),
                positionElem->DoubleAttribute("Y"),
                positionElem->DoubleAttribute("Z"),
            };
        }

        return {};
    }

    SRotation ReadSRotation(tinyxml2::XMLElement& parentElem, const std::string& element)
    {
        auto rotationElem = parentElem.FirstChildElement(element.data());
        if (rotationElem)
        {
            return {
                rotationElem->DoubleAttribute("X"),
                rotationElem->DoubleAttribute("Y"),
                rotationElem->DoubleAttribute("Z"),
                rotationElem->DoubleAttribute("W")
            };
        }

        return {};
    }
}

CTinyxml2Deserializer::CTinyxml2Deserializer(const char* pData, std::uint32_t pMajorVersion, std::uint32_t pMinorVersion)
    : mMajorVersion(pMajorVersion), mMinorVersion(pMinorVersion)
{
    mXmlDocument.Parse(pData);
}

namespace
{
    bool TryReadElementDouble(tinyxml2::XMLElement& element, const char* elementName, double& output)
    {
        if (auto childElem = element.FirstChildElement(elementName))
        {
            return childElem->QueryDoubleText(&output) == tinyxml2::XML_SUCCESS;
        }

        return false;
    }


    bool TryReadElementFloat(tinyxml2::XMLElement& element, const char* elementName, float& output)
    {
        if (auto childElem = element.FirstChildElement(elementName))
        {
            return childElem->QueryFloatText(&output) == tinyxml2::XML_SUCCESS;
        }

        return false;
    }

    bool TryReadElementUnsignedInt32(tinyxml2::XMLElement& element, const char* elementName, std::uint32_t& output)
    {
        if (auto childElem = element.FirstChildElement(elementName))
        {
            return childElem->QueryUnsignedText(&output) == tinyxml2::XML_SUCCESS;
        }

        return false;
    }

    bool TryReadElementString(tinyxml2::XMLElement& element, const char* elementName, std::string& output)
    {
        output.clear();

        if (auto childElem = element.FirstChildElement(elementName))
        {
            if (auto charPtr = childElem->GetText())
            {
                output = charPtr;
            }
            return true;
        }
    }

    void AddFlag(EProcessingActions flag, EProcessingActions& target)
    {
        target = static_cast<EProcessingActions>(target + flag);
    }
}

bool CTinyxml2Deserializer::DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings)
{
    pGeneralSettings.vsCameras.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto generalElem = rootElem->FirstChildElement("General");
    if (!generalElem)
    {
        return true;
    }

    if (auto frequencyElem = generalElem->FirstChildElement("Frequency"))
    {
        pGeneralSettings.nCaptureFrequency = frequencyElem->UnsignedText(0);
    }
    else
    {
        return false;
    }

    if (auto captureTimeElem = generalElem->FirstChildElement("Capture_Time"))
    {
        pGeneralSettings.fCaptureTime = captureTimeElem->FloatText(.0f);
    }
    else
    {
        return false;
    }

    if (!ReadXmlBool(generalElem, "Start_On_External_Trigger", pGeneralSettings.bStartOnExternalTrigger))
    {
        return false;
    }
    if (mMajorVersion > 1 || mMinorVersion > 14)
    {
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NO", pGeneralSettings.bStartOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NC", pGeneralSettings.bStartOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_Software", pGeneralSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }


    if (auto extTimeBaseElem = generalElem->FirstChildElement("External_Time_Base"))
    {
        if (!ReadXmlBool(extTimeBaseElem, "Enabled", pGeneralSettings.sExternalTimebase.bEnabled))
        {
            return false;
        }

        std::string signalSource;
        if (!TryReadElementString(*extTimeBaseElem, "Signal_Source", signalSource))
        {
            return false;
        }

        signalSource = ToLower(signalSource);
        if (signalSource == "control port")
        {
            pGeneralSettings.sExternalTimebase.eSignalSource = SourceControlPort;
        }
        else if (signalSource == "ir receiver")
        {
            pGeneralSettings.sExternalTimebase.eSignalSource = SourceIRReceiver;
        }
        else if (signalSource == "smpte")
        {
            pGeneralSettings.sExternalTimebase.eSignalSource = SourceSMPTE;
        }
        else if (signalSource == "irig")
        {
            pGeneralSettings.sExternalTimebase.eSignalSource = SourceIRIG;
        }
        else if (signalSource == "video sync")
        {
            pGeneralSettings.sExternalTimebase.eSignalSource = SourceVideoSync;
        }
        else
        {
            return false;
        }

        std::string signalMode;
        if (!TryReadElementString(*extTimeBaseElem, "Signal_Mode", signalMode))
        {
            return false;
        }

        signalMode = ToLower(signalMode);
        if (signalMode == "periodic")
        {
            pGeneralSettings.sExternalTimebase.bSignalModePeriodic = true;
        }
        else if (signalMode == "non-periodic")
        {
            pGeneralSettings.sExternalTimebase.bSignalModePeriodic = false;
        }
        else
        {
            return false;
        }
        if (!TryReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Multiplier", pGeneralSettings.sExternalTimebase.nFreqMultiplier))
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Divisor", pGeneralSettings.sExternalTimebase.nFreqDivisor))
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Tolerance", pGeneralSettings.sExternalTimebase.nFreqTolerance))
        {
            return false;
        }

        if (!TryReadElementFloat(*extTimeBaseElem, "Nominal_Frequency", pGeneralSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (TryReadElementString(*extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
            {
                if (ToLower(nominalFrequency) == "none")
                {
                    pGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
                }
            }
            else
            {
                return false;
            }
        }

        std::string signalEdge;
        if (TryReadElementString(*extTimeBaseElem, "Signal_Edge", signalEdge))
        {
            signalEdge = ToLower(signalEdge);
            if (signalEdge == "negative")
            {
                pGeneralSettings.sExternalTimebase.bNegativeEdge = true;
            }
            else if (signalEdge == "positive")
            {
                pGeneralSettings.sExternalTimebase.bNegativeEdge = false;
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

        if (!TryReadElementFloat(*extTimeBaseElem, "Nominal_Frequency", pGeneralSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (TryReadElementString(*extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
            {
                if (ToLower(nominalFrequency) == "none")
                {
                    pGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
                }
            }
            else
            {
                return false;
            }
        }

        if (!TryReadElementUnsignedInt32(*extTimeBaseElem, "Signal_Shutter_Delay", pGeneralSettings.sExternalTimebase.nSignalShutterDelay))
        {
            return false;
        }

        if (!TryReadElementFloat(*extTimeBaseElem, "Non_Periodic_Timeout", pGeneralSettings.sExternalTimebase.fNonPeriodicTimeout))
        {
            return false;
        }

    }
    else
    {
        return false;
    }

    if (auto externalTimestampElem = generalElem->FirstChildElement("External_Timestamp"))
    {
        if (!ReadXmlBool(externalTimestampElem, "Enabled", pGeneralSettings.sTimestamp.bEnabled))
        {
            return false;
        }

        std::string type;
        if (TryReadElementString(*externalTimestampElem, "Type", type))
        {
            type = ToLower(type);
            if (type == "smpte")
            {
                pGeneralSettings.sTimestamp.nType = Timestamp_SMPTE;
            }
            else if (type == "irig")
            {
                pGeneralSettings.sTimestamp.nType = Timestamp_IRIG;
            }
            else
            {
                pGeneralSettings.sTimestamp.nType = Timestamp_CameraTime;
            }
        }

        TryReadElementUnsignedInt32(*externalTimestampElem, "Frequency", pGeneralSettings.sTimestamp.nFrequency);
    }

    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    EProcessingActions* processingActions[3] =
    {
        &pGeneralSettings.eProcessingActions,
        &pGeneralSettings.eRtProcessingActions,
        &pGeneralSettings.eReprocessingActions
    };

    auto AddFlagFromBoolElement = [this](tinyxml2::XMLElement& parent, const char* elementName, EProcessingActions flag, EProcessingActions& target) -> bool
        {
            bool value;
            if (ReadXmlBool(&parent, elementName, value))
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

        auto processingElem = generalElem->FirstChildElement(processings[i]);
        if (!processingElem)
        {
            return false;
        }

        *processingActions[i] = ProcessingNone;

        if (mMajorVersion > 1 || mMinorVersion > 13)
        {
            if (!AddFlagFromBoolElement(*processingElem, "PreProcessing2D", ProcessingPreProcess2D, *processingActions[i]))
            {
                return false;
            }
        }

        std::string trackingMode;
        if (!TryReadElementString(*processingElem, "Tracking", trackingMode))
        {
            return false;
        }
        trackingMode = ToLower(trackingMode);
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

            if (!AddFlagFromBoolElement(*processingElem, "TwinSystemMerge", ProcessingTwinSystemMerge, *processingActions[i]))
            {
                return false;
            }

            if (!AddFlagFromBoolElement(*processingElem, "SplineFill", ProcessingSplineFill, *processingActions[i]))
            {
                return false;
            }
        }

        if (!AddFlagFromBoolElement(*processingElem, "AIM", ProcessingAIM, *processingActions[i]))
        {
            return false;
        }

        if (!AddFlagFromBoolElement(*processingElem, "Track6DOF", Processing6DOFTracking, *processingActions[i]))
        {
            return false;
        }

        if (!AddFlagFromBoolElement(*processingElem, "ForceData", ProcessingForceData, *processingActions[i]))
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            if (!AddFlagFromBoolElement(*processingElem, "GazeVector", ProcessingGazeVector, *processingActions[i]))
            {
                return false;
            }
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!AddFlagFromBoolElement(*processingElem, "ExportTSV", ProcessingExportTSV, *processingActions[i]))
            {
                return false;
            }

            if (!AddFlagFromBoolElement(*processingElem, "ExportC3D", ProcessingExportC3D, *processingActions[i]))
            {
                return false;
            }

            if (!AddFlagFromBoolElement(*processingElem, "ExportMatlabFile", ProcessingExportMatlabFile, *processingActions[i]))
            {
                return false;
            }

            if (mMajorVersion > 1 || mMinorVersion > 11)
            {
                if (!AddFlagFromBoolElement(*processingElem, "ExportAviFile", ProcessingExportAviFile, *processingActions[i]))
                {
                    return false;
                }
            }
        }
    }

    auto eulerElem = generalElem->FirstChildElement("EulerAngles");
    if (eulerElem)
    {
        pGeneralSettings.eulerRotations[0] = eulerElem->Attribute("First");
        pGeneralSettings.eulerRotations[1] = eulerElem->Attribute("Second");
        pGeneralSettings.eulerRotations[2] = eulerElem->Attribute("Third");
    }

    for (auto cameraElem = generalElem->FirstChildElement("Camera"); cameraElem != nullptr; cameraElem = cameraElem->NextSiblingElement("Camera"))
    {
        SSettingsGeneralCamera sCameraSettings{};
        if (!TryReadElementUnsignedInt32(*cameraElem, "ID", sCameraSettings.nID))
        {
            return false;
        }
        std::string model;
        if (!TryReadElementString(*cameraElem, "Model", model))
        {
            return false;
        }

        model = ToLower(model);

        if (model == "macreflex")
        {
            sCameraSettings.eModel = ModelMacReflex;
        }
        else if (model == "proreflex 120")
        {
            sCameraSettings.eModel = ModelProReflex120;
        }
        else if (model == "proreflex 240")
        {
            sCameraSettings.eModel = ModelProReflex240;
        }
        else if (model == "proreflex 500")
        {
            sCameraSettings.eModel = ModelProReflex500;
        }
        else if (model == "proreflex 1000")
        {
            sCameraSettings.eModel = ModelProReflex1000;
        }
        else if (model == "oqus 100")
        {
            sCameraSettings.eModel = ModelOqus100;
        }
        else if (model == "oqus 200" || model == "oqus 200 c")
        {
            sCameraSettings.eModel = ModelOqus200C;
        }
        else if (model == "oqus 300")
        {
            sCameraSettings.eModel = ModelOqus300;
        }
        else if (model == "oqus 300 plus")
        {
            sCameraSettings.eModel = ModelOqus300Plus;
        }
        else if (model == "oqus 400")
        {
            sCameraSettings.eModel = ModelOqus400;
        }
        else if (model == "oqus 500")
        {
            sCameraSettings.eModel = ModelOqus500;
        }
        else if (model == "oqus 500 plus")
        {
            sCameraSettings.eModel = ModelOqus500Plus;
        }
        else if (model == "oqus 700")
        {
            sCameraSettings.eModel = ModelOqus700;
        }
        else if (model == "oqus 700 plus")
        {
            sCameraSettings.eModel = ModelOqus700Plus;
        }
        else if (model == "oqus 600 plus")
        {
            sCameraSettings.eModel = ModelOqus600Plus;
        }
        else if (model == "miqus m1")
        {
            sCameraSettings.eModel = ModelMiqusM1;
        }
        else if (model == "miqus m3")
        {
            sCameraSettings.eModel = ModelMiqusM3;
        }
        else if (model == "miqus m5")
        {
            sCameraSettings.eModel = ModelMiqusM5;
        }
        else if (model == "miqus sync unit")
        {
            sCameraSettings.eModel = ModelMiqusSyncUnit;
        }
        else if (model == "miqus video")
        {
            sCameraSettings.eModel = ModelMiqusVideo;
        }
        else if (model == "miqus video color")
        {
            sCameraSettings.eModel = ModelMiqusVideoColor;
        }
        else if (model == "miqus hybrid")
        {
            sCameraSettings.eModel = ModelMiqusHybrid;
        }
        else if (model == "miqus video color plus")
        {
            sCameraSettings.eModel = ModelMiqusVideoColorPlus;
        }
        else if (model == "arqus a5")
        {
            sCameraSettings.eModel = ModelArqusA5;
        }
        else if (model == "arqus a9")
        {
            sCameraSettings.eModel = ModelArqusA9;
        }
        else if (model == "arqus a12")
        {
            sCameraSettings.eModel = ModelArqusA12;
        }
        else if (model == "arqus a26")
        {
            sCameraSettings.eModel = ModelArqusA26;
        }
        else
        {
            sCameraSettings.eModel = ModelUnknown;
        }

        ReadXmlBool(cameraElem, "Underwater", sCameraSettings.bUnderwater);
        ReadXmlBool(cameraElem, "Supports_HW_Sync", sCameraSettings.bSupportsHwSync);

        if (!TryReadElementUnsignedInt32(*cameraElem, "Serial", sCameraSettings.nSerial))
        {
            return false;
        }

        std::string mode;
        if (!TryReadElementString(*cameraElem, "Mode", mode))
        {
            return false;
        }

        mode = ToLower(mode);
        if (mode == "marker")
        {
            sCameraSettings.eMode = ModeMarker;
        }
        else if (mode == "marker intensity")
        {
            sCameraSettings.eMode = ModeMarkerIntensity;
        }
        else if (mode == "video")
        {
            sCameraSettings.eMode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            if (!TryReadElementUnsignedInt32(*cameraElem, "Video_Frequency", sCameraSettings.nVideoFrequency))
            {
                return false;
            }
        }

        std::string videoResolution;
        if (TryReadElementString(*cameraElem, "Video_Resolution", videoResolution))
        {
            videoResolution = ToLower(videoResolution);
            if (videoResolution == "1440p")
            {
                sCameraSettings.eVideoResolution = VideoResolution1440p;
            }
            else if (videoResolution == "1080p")
            {
                sCameraSettings.eVideoResolution = VideoResolution1080p;
            }
            else if (videoResolution == "720p")
            {
                sCameraSettings.eVideoResolution = VideoResolution720p;
            }
            else if (videoResolution == "540p")
            {
                sCameraSettings.eVideoResolution = VideoResolution540p;
            }
            else if (videoResolution == "480p")
            {
                sCameraSettings.eVideoResolution = VideoResolution480p;
            }
            else
            {
                sCameraSettings.eVideoResolution = VideoResolutionNone;
            }
        }
        else
        {
            sCameraSettings.eVideoResolution = VideoResolutionNone;
        }

        std::string videoAspectRatio;
        if (TryReadElementString(*cameraElem, "Video_Aspect_Ratio", videoAspectRatio))
        {
            videoAspectRatio = ToLower(videoAspectRatio);
            if (videoAspectRatio == "16x9")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
            }
            else if (videoAspectRatio == "4x3")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
            }
            else if (videoAspectRatio == "1x1")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
            }
            else
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
            }
        }
        else
        {
            sCameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
        }

        auto videoExposureElem = cameraElem->FirstChildElement("Video_Exposure");
        if (videoExposureElem)
        {
            if (!TryReadElementUnsignedInt32(*videoExposureElem, "Current", sCameraSettings.nVideoExposure)
                || !TryReadElementUnsignedInt32(*videoExposureElem, "Min", sCameraSettings.nVideoExposureMin)
                || !TryReadElementUnsignedInt32(*videoExposureElem, "Max", sCameraSettings.nVideoExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto videoFlashTime = cameraElem->FirstChildElement("Video_Flash_Time");
        if (videoFlashTime)
        {
            if (!TryReadElementUnsignedInt32(*videoFlashTime, "Current", sCameraSettings.nVideoFlashTime)
                || !TryReadElementUnsignedInt32(*videoFlashTime, "Min", sCameraSettings.nVideoFlashTimeMin)
                || !TryReadElementUnsignedInt32(*videoFlashTime, "Max", sCameraSettings.nVideoFlashTimeMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto markerExposureElem = cameraElem->FirstChildElement("Marker_Exposure");
        if (markerExposureElem)
        {
            if (!TryReadElementUnsignedInt32(*markerExposureElem, "Current", sCameraSettings.nMarkerExposure)
                || !TryReadElementUnsignedInt32(*markerExposureElem, "Min", sCameraSettings.nMarkerExposureMin)
                || !TryReadElementUnsignedInt32(*markerExposureElem, "Max", sCameraSettings.nMarkerExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto markerThresholdElem = cameraElem->FirstChildElement("Marker_Threshold");
        if (markerThresholdElem)
        {
            if (!TryReadElementUnsignedInt32(*markerThresholdElem, "Current", sCameraSettings.nMarkerThreshold)
                || !TryReadElementUnsignedInt32(*markerThresholdElem, "Min", sCameraSettings.nMarkerThresholdMin)
                || !TryReadElementUnsignedInt32(*markerThresholdElem, "Max", sCameraSettings.nMarkerThresholdMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto positionElem = cameraElem->FirstChildElement("Position");
        if (positionElem)
        {
            if (!TryReadElementFloat(*positionElem, "X", sCameraSettings.fPositionX)
                || !TryReadElementFloat(*positionElem, "Y", sCameraSettings.fPositionY)
                || !TryReadElementFloat(*positionElem, "Z", sCameraSettings.fPositionZ)
                || !TryReadElementFloat(*positionElem, "Rot_1_1", sCameraSettings.fPositionRotMatrix[0][0])
                || !TryReadElementFloat(*positionElem, "Rot_2_1", sCameraSettings.fPositionRotMatrix[1][0])
                || !TryReadElementFloat(*positionElem, "Rot_3_1", sCameraSettings.fPositionRotMatrix[2][0])
                || !TryReadElementFloat(*positionElem, "Rot_1_2", sCameraSettings.fPositionRotMatrix[0][1])
                || !TryReadElementFloat(*positionElem, "Rot_2_2", sCameraSettings.fPositionRotMatrix[1][1])
                || !TryReadElementFloat(*positionElem, "Rot_3_2", sCameraSettings.fPositionRotMatrix[2][1])
                || !TryReadElementFloat(*positionElem, "Rot_1_3", sCameraSettings.fPositionRotMatrix[0][2])
                || !TryReadElementFloat(*positionElem, "Rot_2_3", sCameraSettings.fPositionRotMatrix[1][2])
                || !TryReadElementFloat(*positionElem, "Rot_3_3", sCameraSettings.fPositionRotMatrix[2][2])
                )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(*cameraElem, "Orientation", sCameraSettings.nOrientation))
        {
            return false;
        }

        auto markerResElem = cameraElem->FirstChildElement("Marker_Res");
        if (markerResElem)
        {
            if (!TryReadElementUnsignedInt32(*markerResElem, "Width", sCameraSettings.nMarkerResolutionWidth)
                || !TryReadElementUnsignedInt32(*markerResElem, "Height", sCameraSettings.nMarkerResolutionHeight)
                )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto videoResElem = cameraElem->FirstChildElement("Video_Res");
        if (videoResElem)
        {
            if (!TryReadElementUnsignedInt32(*videoResElem, "Width", sCameraSettings.nVideoResolutionWidth)
                || !TryReadElementUnsignedInt32(*videoResElem, "Height", sCameraSettings.nVideoResolutionHeight)
                )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto markerFovElem = cameraElem->FirstChildElement("Marker_FOV");
        if (markerFovElem)
        {
            if (!TryReadElementUnsignedInt32(*markerFovElem, "Left", sCameraSettings.nMarkerFOVLeft)
                || !TryReadElementUnsignedInt32(*markerFovElem, "Top", sCameraSettings.nMarkerFOVTop)
                || !TryReadElementUnsignedInt32(*markerFovElem, "Right", sCameraSettings.nMarkerFOVRight)
                || !TryReadElementUnsignedInt32(*markerFovElem, "Bottom", sCameraSettings.nMarkerFOVBottom)
                )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto videoFovElem = cameraElem->FirstChildElement("Marker_FOV");
        if (videoFovElem)
        {
            if (!TryReadElementUnsignedInt32(*videoFovElem, "Left", sCameraSettings.nVideoFOVLeft)
                || !TryReadElementUnsignedInt32(*videoFovElem, "Top", sCameraSettings.nVideoFOVTop)
                || !TryReadElementUnsignedInt32(*videoFovElem, "Right", sCameraSettings.nVideoFOVRight)
                || !TryReadElementUnsignedInt32(*videoFovElem, "Bottom", sCameraSettings.nVideoFOVBottom)
                )
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
            (void)sprintf_s(syncOutStr,16, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));

            auto syncOutElem = cameraElem->FirstChildElement(syncOutStr);
            if (syncOutElem)
            {
                if (port < 2)
                {
                    std::string mode;
                    if (!TryReadElementString(*syncOutElem, "Mode", mode))
                    {
                        return false;
                    }

                    mode = ToLower(mode);
                    if (mode == "shutter out")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeShutterOut;
                    }
                    else if (mode == "multiplier")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeMultiplier;
                    }
                    else if (mode == "divisor")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeDivisor;
                    }
                    else if (mode == "camera independent")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (mode == "measurement time")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (mode == "continuous 100hz")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
                    }
                    else if (mode == "system live time")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeSystemLiveTime;
                    }
                    else
                    {
                        return false;
                    }

                    if (sCameraSettings.eSyncOutMode[port] == ModeMultiplier ||
                        sCameraSettings.eSyncOutMode[port] == ModeDivisor ||
                        sCameraSettings.eSyncOutMode[port] == ModeIndependentFreq)
                    {
                        if (!TryReadElementUnsignedInt32(*syncOutElem, "Value", sCameraSettings.nSyncOutValue[port])
                            || !TryReadElementFloat(*syncOutElem, "Duty_Cycle", sCameraSettings.fSyncOutDutyCycle[port]))
                        {
                            return false;
                        }
                    }
                }

                if (port == 2 || (sCameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    std::string signalPolarity;
                    if (TryReadElementString(*syncOutElem, "Signal_Polarity", signalPolarity))
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = ToLower(signalPolarity) == "negative";
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            else
            {
                sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                sCameraSettings.nSyncOutValue[port] = 0;
                sCameraSettings.fSyncOutDutyCycle[port] = 0;
                sCameraSettings.bSyncOutNegativePolarity[port] = false;
            }
        }

        auto lensControlElem = cameraElem->FirstChildElement("LensControl");
        if (lensControlElem)
        {
            auto focusElem = lensControlElem->FirstChildElement("Focus");
            if (focusElem)
            {
                sCameraSettings.fFocus = focusElem->FloatAttribute("Value", std::numeric_limits<float>::quiet_NaN());
            }

            auto apertureElem = lensControlElem->FirstChildElement("Aperture");
            if (apertureElem)
            {
                sCameraSettings.fAperture = apertureElem->FloatAttribute("Value", std::numeric_limits<float>::quiet_NaN());
            }
        }
        else
        {
            sCameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            sCameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        auto autoExposureElem = cameraElem->FirstChildElement("AutoExposure");
        if (autoExposureElem)
        {
            sCameraSettings.autoExposureEnabled = autoExposureElem->BoolAttribute("Enabled", false);
            sCameraSettings.autoExposureCompensation = autoExposureElem->FloatAttribute("Compensation", std::numeric_limits<float>::quiet_NaN());
        }
        else
        {
            sCameraSettings.autoExposureEnabled = false;
            sCameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        bool autoWhiteBalance;
        if (ReadXmlBool(cameraElem, "AutoWhiteBalance", autoWhiteBalance))
        {
            sCameraSettings.autoWhiteBalance = autoWhiteBalance ? 1 : 0;
        }
        else
        {
            sCameraSettings.autoWhiteBalance = -1;
        }

        pGeneralSettings.vsCameras.push_back(sCameraSettings);
    }

    return true;
}

bool CTinyxml2Deserializer::Deserialize3DSettings(SSettings3D& p3dSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    p3dSettings.pCalibrationTime[0] = 0;

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto t3dElem = rootElem->FirstChildElement("The_3D");
    if (!t3dElem)
    {
        return true;
    }

    if (auto axisUpwards = t3dElem->FirstChildElement("AxisUpwards"))
    {
        if (const char* charPtr = axisUpwards->GetText())
        {
            auto tStr = ToLower(charPtr);
            if (tStr == "+x")
            {
                p3dSettings.eAxisUpwards = XPos;
            }
            else if (tStr == "-x")
            {
                p3dSettings.eAxisUpwards = XNeg;
            }
            else if (tStr == "+y")
            {
                p3dSettings.eAxisUpwards = YPos;
            }
            else if (tStr == "-y")
            {
                p3dSettings.eAxisUpwards = YNeg;
            }
            else if (tStr == "+z")
            {
                p3dSettings.eAxisUpwards = ZPos;
            }
            else if (tStr == "-z")
            {
                p3dSettings.eAxisUpwards = ZNeg;
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
    }
    else
    {
        return false;
    }

    if (auto calibrationTimeElem = t3dElem->FirstChildElement("CalibrationTime"))
    {
        if (const char* charPtr = calibrationTimeElem->GetText())
        {
            strcpy_s(p3dSettings.pCalibrationTime, 32, charPtr);
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

    std::size_t labelCount;
    if (auto labelsElem = t3dElem->FirstChildElement("Labels"))
    {
        labelCount = labelsElem->IntText(0);
    }
    else
    {
        return false;
    }

    p3dSettings.s3DLabels.clear();
    p3dSettings.s3DLabels.reserve(labelCount);
    for (auto labelElem = t3dElem->FirstChildElement("Label"); labelElem != nullptr; labelElem = labelElem->NextSiblingElement("Label"))
    {
        SSettings3DLabel label{};
        if (auto nameElem = labelElem->FirstChildElement("Name"))
        {
            if (auto namePtr = nameElem->GetText())
            {
                label.oName = namePtr;
            }
        }

        if (auto colorElem = labelElem->FirstChildElement("RGBColor"))
        {
            label.nRGBColor = colorElem->IntText(0);
        }

        if (auto typeElem = labelElem->FirstChildElement("Trajectory_Type"))
        {
            if (auto charPtr = typeElem->GetText())
            {
                label.type = charPtr;
            }
        }

        p3dSettings.s3DLabels.push_back(label);
    }

    if (p3dSettings.s3DLabels.size() != labelCount)
    {
        return false;
    }

    if (auto bonesElem = t3dElem->FirstChildElement("Bones"))
    {
        for (auto boneElem = bonesElem->FirstChildElement("Bone"); boneElem != nullptr; boneElem = boneElem->NextSiblingElement("Bone"))
        {
            SSettingsBone bone{};
            if (auto attribute = boneElem->Attribute("From"))
            {
                bone.fromName = attribute;
            }

            if (auto attribute = boneElem->Attribute("To"))
            {
                bone.toName = attribute;
            }

            bone.color = boneElem->UnsignedAttribute("Color", bone.color);

            p3dSettings.sBones.push_back(bone);
        }
    }

    pDataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryRead6DofElementEnabled(std::uint32_t nMajorVer, std::uint32_t nMinorVer, tinyxml2::XMLElement& oXML, bool& bTarget)
    {
        if (nMajorVer > 1 || nMinorVer > 23)
        {
            if (auto enabledElem = oXML.FirstChildElement("Enabled"))
            {
                bTarget = std::string(enabledElem->GetText()) == "true";
                return true;
            }
        }

        // Enabled is default true for 6dof bodies
        bTarget = true;
        return false;
    }


    bool TryReadAttributesRGBColor(tinyxml2::XMLElement& oXML, std::uint32_t& nTarget)
    {
        if (auto elem = oXML.FirstChildElement("Color"))
        {
            std::uint32_t colorR = atoi(elem->Attribute("R"));
            std::uint32_t colorG = atoi(elem->Attribute("G"));
            std::uint32_t colorB = atoi(elem->Attribute("B"));
            nTarget = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);
            return true;
        }

        nTarget = 0;
        return false;
    }

    bool TryReadSetFilter(tinyxml2::XMLElement& oXML, std::string& sTarget)
    {
        if (auto elem = oXML.FirstChildElement("Filter"))
        {
            sTarget = elem->Attribute("Preset");
            return true;
        }

        return false;
    }

    bool TryReadSetPos(tinyxml2::XMLElement& oXML, float& fTargetX, float& fTargetY, float& fTargetZ)
    {
        if (auto elem = oXML.FirstChildElement("Position"))
        {
            fTargetX = static_cast<float>(atof(elem->Attribute("X")));
            fTargetY = static_cast<float>(atof(elem->Attribute("Y")));
            fTargetZ = static_cast<float>(atof(elem->Attribute("Z")));
            return true;
        }

        fTargetZ = fTargetY = fTargetX = .0f;
        return false;
    }

    bool TryReadSetRotation(tinyxml2::XMLElement& oXML, float& fTargetX, float& fTargetY, float& fTargetZ)
    {
        if (auto elem = oXML.FirstChildElement("Rotation"))
        {
            fTargetX = static_cast<float>(atof(elem->Attribute("X")));
            fTargetY = static_cast<float>(atof(elem->Attribute("Y")));
            fTargetZ = static_cast<float>(atof(elem->Attribute("Z")));
            return true;
        }

        fTargetZ = fTargetY = fTargetX = .0f;
        return false;
    }


    bool TryReadSetPoints(tinyxml2::XMLElement& oXML, std::vector<SBodyPoint>& vTarget)
    {
        if (auto pointsElem = oXML.FirstChildElement("Points"))
        {
            for (auto pointElem = pointsElem->FirstChildElement("Point"); pointElem != nullptr; pointElem = pointElem->NextSiblingElement("Point"))
            {
                SBodyPoint sBodyPoint;

                sBodyPoint.fX = static_cast<float>(atof(pointElem->Attribute("X")));
                sBodyPoint.fY = static_cast<float>(atof(pointElem->Attribute("Y")));
                sBodyPoint.fZ = static_cast<float>(atof(pointElem->Attribute("Z")));

                sBodyPoint.virtual_ = (0 != atoi(pointElem->Attribute("Virtual")));
                sBodyPoint.physicalId = atoi(pointElem->Attribute("PhysicalId"));
                sBodyPoint.name = pointElem->Attribute("Name");
                vTarget.push_back(sBodyPoint);
            }

            return true;
        }

        return false;
    }

    bool TryReadSetDataOrigin(tinyxml2::XMLElement& oXML, SOrigin& oTarget)
    {
        if (auto elem = oXML.FirstChildElement("Data_origin"))
        {
            oTarget.type = static_cast<EOriginType>(atoi(elem->GetText()));
            oTarget.position.fX = static_cast<float>(atof(elem->Attribute("X")));
            oTarget.position.fY = static_cast<float>(atof(elem->Attribute("Y")));
            oTarget.position.fZ = static_cast<float>(atof(elem->Attribute("Z")));
            oTarget.relativeBody = atoi(elem->Attribute("Relative_body"));
        }
        else
        {
            oTarget = {};
            return false;
        }

        if (auto elem = oXML.FirstChildElement("Data_orientation"))
        {
            char tmpStr[10];
            for (std::uint32_t i = 0; i < 9; i++)
            {
                (void)sprintf_s(tmpStr, 10, "R%u%u", (i / 3) + 1, (i % 3) + 1);
                oTarget.rotation[i] = static_cast<float>(atof(elem->Attribute(tmpStr)));
            }


            auto type = static_cast<EOriginType>(atoi(elem->GetText()));
            auto body = static_cast<std::uint32_t>(atoi(elem->Attribute("Relative_body")));

            // Validation: type and relativeBody must be the same between orientation and origin
            return type == oTarget.type && body == oTarget.relativeBody;
        }

        oTarget = {};
        return false;
    }

    bool TryReadElementRGBColor(tinyxml2::XMLElement& oXML, std::uint32_t& oTarget)
    {
        if (auto elem = oXML.FirstChildElement("RGBColor"))
        {
            oTarget = atoi(elem->GetText());
            return true;
        }

        oTarget = 0;
        return false;
    }

    bool TryReadSetPointsOld(tinyxml2::XMLElement& oXML, std::vector<SBodyPoint>& vTarget)
    {
        vTarget.clear();
        for (auto pointElem = oXML.FirstChildElement("Point"); pointElem != nullptr; pointElem = pointElem->NextSiblingElement("Point"))
        {
            SBodyPoint sPoint;

            if (!TryReadElementFloat(*pointElem, "X", sPoint.fX))
            {
                return false;
            }

            if (!TryReadElementFloat(*pointElem, "Y", sPoint.fY))
            {
                return false;
            }

            if (!TryReadElementFloat(*pointElem, "Z", sPoint.fZ))
            {
                return false;
            }

            vTarget.push_back(sPoint);
        }
        return true;
    }

    bool TryReadSetEuler(tinyxml2::XMLDocument& oXML, std::string& sTargetFirst, std::string& sTargetSecond, std::string& sTargetThird)
    {
        if (auto elem = oXML.FirstChildElement("Euler"))
        {
            return TryReadElementString(*elem, "First", sTargetFirst)
                && TryReadElementString(*elem, "Second", sTargetSecond)
                && TryReadElementString(*elem, "Third", sTargetThird);
        }

        return false;
    }
}

bool CTinyxml2Deserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& p6DOFSettings, SSettingsGeneral& pGeneralSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    p6DOFSettings.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    //
    // Read gaze vectors
    //
    tinyxml2::XMLElement* sixdofElem = rootElem->FirstChildElement("The_6D");
    if (!sixdofElem)
    {
        return true; // NO eye tracker data available.
    }

    if (mMajorVersion > 1 || mMinorVersion > 20)
    {
        for (auto bodyElem = sixdofElem->FirstChildElement("Body"); bodyElem != nullptr; bodyElem = bodyElem->NextSiblingElement("Body"))
        {
            SSettings6DOFBody s6DOFBodySettings;

            if (!TryReadElementString(*bodyElem, "Name", s6DOFBodySettings.name))
            { // Name --- REQUIRED
                return false;
            }

            TryRead6DofElementEnabled(mMajorVersion, mMinorVersion, *bodyElem, s6DOFBodySettings.enabled);
            if (!TryReadAttributesRGBColor(*bodyElem, s6DOFBodySettings.color)
                || !TryReadElementFloat(*bodyElem, "MaximumResidual", s6DOFBodySettings.maxResidual)
                || !TryReadElementUnsignedInt32(*bodyElem, "MinimumMarkersInBody", s6DOFBodySettings.minMarkersInBody)
                || !TryReadElementFloat(*bodyElem, "BoneLengthTolerance", s6DOFBodySettings.boneLengthTolerance)
                || !TryReadSetFilter(*bodyElem, s6DOFBodySettings.filterPreset))
            { // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                return false;
            }


            if (auto meshElem = bodyElem->FirstChildElement("Mesh"))
            {
                if (!TryReadElementString(*meshElem, "Name",s6DOFBodySettings.mesh.name)
                    || !TryReadSetPos(*meshElem, s6DOFBodySettings.mesh.position.fX, s6DOFBodySettings.mesh.position.fY, s6DOFBodySettings.mesh.position.fZ)
                    || !TryReadSetRotation(*meshElem, s6DOFBodySettings.mesh.rotation.fX, s6DOFBodySettings.mesh.rotation.fY, s6DOFBodySettings.mesh.rotation.fZ)
                    || !TryReadElementFloat(*meshElem, "Scale", s6DOFBodySettings.mesh.scale)
                    || !TryReadElementFloat(*meshElem, "Opacity", s6DOFBodySettings.mesh.opacity))
                { // Name, Position, Rotation, Scale, Opacity --- REQUIRED
                    return false;
                }
            }
            // Points --- REQUIRED
            if (!TryReadSetPoints(*bodyElem, s6DOFBodySettings.points))
            {
                return false;
            }

            // Data Orientation, Origin --- REQUIRED
            if (!TryReadSetDataOrigin(*bodyElem, s6DOFBodySettings.origin))
            {
                return false;
            }

            p6DOFSettings.push_back(s6DOFBodySettings);
            pDataAvailable = true;
        }
    }
    else
    {
        if (!mXmlDocument.FirstChildElement("Bodies"))
        {
            return false;
        }

        for (auto bodyElem = mXmlDocument.FirstChildElement("Body"); bodyElem != nullptr; bodyElem = bodyElem->NextSiblingElement("Body"))
        {
            SSettings6DOFBody s6DOFBodySettings{};

            // Name, RGBColor, Points(OLD) --- REQUIRED
            if (!TryReadElementString(*bodyElem, "Name", s6DOFBodySettings.name)
                || !TryReadElementRGBColor(*bodyElem, s6DOFBodySettings.color)
                || !TryReadSetPointsOld(*bodyElem, s6DOFBodySettings.points))
            {
                return false;
            }

            if (mMajorVersion > 1 || mMinorVersion > 15)
            {
                // Euler --- REQUIRED
                if (!TryReadSetEuler(mXmlDocument, pGeneralSettings.eulerRotations[0], pGeneralSettings.eulerRotations[1], pGeneralSettings.eulerRotations[2]))
                {
                    return false;
                }
            }

            p6DOFSettings.push_back(s6DOFBodySettings);
            pDataAvailable = true;
        }
    }

    return true;
} // Read6DOFSettings

bool CTinyxml2Deserializer::DeserializeGazeVectorSettings(std::vector<SGazeVector>& pGazeVectorSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pGazeVectorSettings.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    //
    // Read gaze vectors
    //
    tinyxml2::XMLElement* gazeVectorElem = rootElem->FirstChildElement("Gaze_Vector");
    if (!gazeVectorElem)
    {
        return true; // NO eye tracker data available.
    }

    for (auto vectorElem = gazeVectorElem->FirstChildElement("Vector"); vectorElem != nullptr; vectorElem = vectorElem->NextSiblingElement("Vector"))
    {
        std::string name;
        if (auto nameElem = vectorElem->FirstChildElement("Name"))
        {
            name = nameElem->GetText();
        }
        else
        {
            return false;
        }

        float frequency = 0;
        if (auto frequencyElem = vectorElem->FirstChildElement("Frequency"))
        {
            frequency = static_cast<float>(atof(frequencyElem->GetText()));
        }

        bool hwSync = false;
        ReadXmlBool(vectorElem, "Hardware_Sync", hwSync);

        bool filter = false;
        ReadXmlBool(vectorElem, "Filter", filter);

        pGazeVectorSettings.push_back({ name, frequency, hwSync, filter });
    }

    pDataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CTinyxml2Deserializer::DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& pEyeTrackerSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pEyeTrackerSettings.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    tinyxml2::XMLElement* eyeTrackerElem = rootElem->FirstChildElement("Eye_Tracker");

    if (!eyeTrackerElem)
    {
        return true; // NO eye tracker data available.
    }

    for (auto deviceElem = eyeTrackerElem->FirstChildElement("Device"); deviceElem != nullptr; deviceElem = deviceElem->NextSiblingElement("Device"))
    {
        std::string name;
        if (auto nameElem = deviceElem->FirstChildElement("Name"))
        {
            name = nameElem->GetText();
        }
        else
        {
            return false;
        }

        float frequency = 0;
        if (auto frequencyElem = deviceElem->FirstChildElement("Frequency"))
        {
            frequency = static_cast<float>(atof(frequencyElem->GetText()));
        }

        bool hwSync = false;
        ReadXmlBool(deviceElem, "Hardware_Sync", hwSync);

        pEyeTrackerSettings.push_back({ name, frequency, hwSync });
    }

    pDataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CTinyxml2Deserializer::DeserializeAnalogSettings(std::vector<SAnalogDevice>& pAnalogDeviceSettings, bool& pDataAvailable)
{
    pDataAvailable = false;
    pAnalogDeviceSettings.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto analogElem = rootElem->FirstChildElement("Analog");
    if (!analogElem)
    {
        // No analog data available.
        return true;
    }

    if (mMajorVersion == 1 && mMinorVersion == 0)
    {
        SAnalogDevice analogDevice{};
        analogDevice.nDeviceID = 1;   // Always channel 1
        analogDevice.oName = "AnalogDevice";
        if (!TryReadElementUnsignedInt32(*analogElem, "Channels", analogDevice.nChannels)
            || !TryReadElementUnsignedInt32(*analogElem, "Frequency", analogDevice.nFrequency)
            || !TryReadElementString(*analogElem, "Unit", analogDevice.oUnit))
        {
            return false;
        }

        auto rangeElem = analogElem->FirstChildElement("Range");
        if (!rangeElem
            || !TryReadElementFloat(*rangeElem, "Min", analogDevice.fMinRange)
            || !TryReadElementFloat(*rangeElem, "Max", analogDevice.fMaxRange))
        {
            return false;
        }

        pDataAvailable = true;
        pAnalogDeviceSettings.push_back(analogDevice);
        return true;
    }

    for (auto deviceElem = analogElem->FirstChildElement("Device"); deviceElem != nullptr; deviceElem = deviceElem->NextSiblingElement("Device"))
    {
        SAnalogDevice analogDevice{};
        if (!TryReadElementUnsignedInt32(*deviceElem, "Device_ID", analogDevice.nDeviceID)
            || !TryReadElementString(*deviceElem, "Device_Name", analogDevice.oName)
            || !TryReadElementUnsignedInt32(*deviceElem, "Channels", analogDevice.nChannels)
            || !TryReadElementUnsignedInt32(*deviceElem, "Frequency", analogDevice.nFrequency)
            )
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            if (!TryReadElementString(*analogElem, "Unit", analogDevice.oUnit))
            {
                continue;
            }
        }

        auto rangeElem = deviceElem->FirstChildElement("Range");
        if (!rangeElem
            || !TryReadElementFloat(*rangeElem, "Min", analogDevice.fMinRange)
            || !TryReadElementFloat(*rangeElem, "Max", analogDevice.fMaxRange))
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            for (std::size_t i = 0; i < analogDevice.nChannels; i++)
            {
                std::string label;
                if (TryReadElementString(*deviceElem, "Label", label))
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
            for (auto channelElem = deviceElem->FirstChildElement("Channel"); channelElem != nullptr; channelElem = channelElem->NextSiblingElement("Channel"))
            {
                std::string label;
                if (TryReadElementString(*channelElem, "Label", label))
                {
                    analogDevice.voLabels.push_back(label);
                }

                std::string unit;
                if (TryReadElementString(*channelElem, "Unit", unit))
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

        pDataAvailable = true;
        pAnalogDeviceSettings.push_back(analogDevice);
    }

    return true;
} // ReadAnalogSettings

namespace
{
    struct ChildElementRange
    {
        static constexpr std::size_t buffSize = 128;
        using TElementNameGenerator = std::function<const char* (char(&buff)[buffSize], std::size_t bufferSize, std::size_t index)>;

    private:
        tinyxml2::XMLElement& parent;
        TElementNameGenerator elementNameGenerator;

    public:
        ChildElementRange() = delete;

        ChildElementRange(tinyxml2::XMLElement& parent, const char* elementName)
            : parent(parent), elementNameGenerator([elementName](auto& buff, std::size_t, std::size_t) { return elementName; })
        {
        }

        ChildElementRange(tinyxml2::XMLElement& parent,
            TElementNameGenerator generator)
            : parent(parent), elementNameGenerator(std::move(generator))
        {
        }

        struct Iterator
        {
            char buffer[buffSize];
            tinyxml2::XMLElement* current;
            const ChildElementRange& range;
            std::size_t index;

            explicit Iterator(const ChildElementRange& range)
                : buffer{}, current(nullptr), range(range), index(std::numeric_limits<std::size_t>::max())
            {
            }

            Iterator(const ChildElementRange& range, std::size_t index)
                : buffer{}, current(nullptr), range(range), index(index)
            {
                current = range.parent.FirstChildElement(range.elementNameGenerator(buffer, buffSize, index++));
            }

            tinyxml2::XMLElement* operator*() const
            {
                return current;
            }

            Iterator& operator++()
            {
                current = current->NextSiblingElement(range.elementNameGenerator(buffer, buffSize, index++));
                return *this;
            }

            bool operator!=(const Iterator& other) const
            {
                return current != other.current;
            }
        };

        Iterator begin() const
        {
            return Iterator(*this, 0);
        }

        Iterator end() const
        {
            return Iterator(*this);
        }
    };
}

bool CTinyxml2Deserializer::DeserializeForceSettings(SSettingsForce& pForceSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pForceSettings.vsForcePlates.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto forceElem = rootElem->FirstChildElement("Force");
    if (!forceElem)
    {
        // No analog data available.
        return true;
    }

    SForcePlate sForcePlate{};
    sForcePlate.bValidCalibrationMatrix = false;
    sForcePlate.nCalibrationMatrixRows = 6;
    sForcePlate.nCalibrationMatrixColumns = 6;

    if (!TryReadElementString(*forceElem, "Unit_Length", pForceSettings.oUnitLength))
    {
        return false;
    }

    if (!TryReadElementString(*forceElem, "Unit_Force", pForceSettings.oUnitForce))
    {
        return false;
    }

    std::size_t iPlate = 0;
    for (auto plateElem : ChildElementRange{ *forceElem, "Plate" })
    {
        iPlate++;

        if (!TryReadElementUnsignedInt32(*plateElem, "Plate_ID", sForcePlate.nID))
        {
            if (!TryReadElementUnsignedInt32(*plateElem, "Force_Plate_Index", sForcePlate.nID)) // Version 1.7 and earlier.
            {
                return false;
            }
        }

        if (!TryReadElementUnsignedInt32(*plateElem, "Analog_Device_ID", sForcePlate.nAnalogDeviceID))
        {
            sForcePlate.nAnalogDeviceID = 0;
        }

        if (!TryReadElementUnsignedInt32(*plateElem, "Frequency", sForcePlate.nFrequency))
        {
            return false;
        }

        if (!TryReadElementString(*plateElem, "Type", sForcePlate.oType))
        {
            sForcePlate.oType = "unknown";
        }

        if (!TryReadElementString(*plateElem, "Name", sForcePlate.oName))
        {
            sForcePlate.oName = "#" + std::to_string(iPlate);
        }

        TryReadElementFloat(*plateElem, "Length", sForcePlate.fLength);
        TryReadElementFloat(*plateElem, "Width", sForcePlate.fWidth);

        auto locationElem = plateElem->FirstChildElement("Location");
        if (locationElem)
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
                auto cornerElem = locationElem->FirstChildElement(c.name);
                TryReadElementFloat(*cornerElem, "X", sForcePlate.asCorner[c.index].fX);
                TryReadElementFloat(*cornerElem, "Y", sForcePlate.asCorner[c.index].fY);
                TryReadElementFloat(*cornerElem, "Z", sForcePlate.asCorner[c.index].fZ);
            }
        }

        auto originElem = plateElem->FirstChildElement("Origin");
        if (originElem)
        {
            TryReadElementFloat(*originElem, "X", sForcePlate.sOrigin.fX);
            TryReadElementFloat(*originElem, "Y", sForcePlate.sOrigin.fY);
            TryReadElementFloat(*originElem, "Z", sForcePlate.sOrigin.fZ);
        }

        sForcePlate.vChannels.clear();
        auto channelsElem = plateElem->FirstChildElement("Channels");
        if (channelsElem)
        {
            SForceChannel sForceChannel{};
            for (auto channelElem : ChildElementRange{ *channelsElem, "Channel" })
            {
                TryReadElementUnsignedInt32(*channelElem, "Channel_No", sForceChannel.nChannelNumber);
                TryReadElementFloat(*channelElem, "ConversionFactor", sForceChannel.fConversionFactor);
                sForcePlate.vChannels.push_back(sForceChannel);
            }
        }

        auto calibrationMatrix = plateElem->FirstChildElement("Calibration_Matrix");
        if (calibrationMatrix)
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

                unsigned int nRow = 0;
                for (const auto row : ChildElementRange{ *calibrationMatrix, getRowStr })
                {
                    unsigned int nCol = 0;
                    for (const auto col : ChildElementRange{ *row, getColStr })
                    {
                        sForcePlate.afCalibrationMatrix[nRow][nCol++] = col->FloatText();
                    }
                    nRow++;
                    sForcePlate.nCalibrationMatrixColumns = nCol;
                }
                sForcePlate.nCalibrationMatrixRows = nRow;
                sForcePlate.bValidCalibrationMatrix = true;
            }
            else
            {
                auto rows = calibrationMatrix->FirstChildElement("Rows");
                if (rows)
                {
                    unsigned int nRow = 0;
                    for (auto rowElement : ChildElementRange{ *rows, "Row" })
                    {
                        auto columns = rowElement->FirstChildElement("Columns");
                        if (columns)
                        {
                            unsigned int nCol = 0;
                            for (const auto col : ChildElementRange{ *columns, "Column" })
                            {
                                sForcePlate.afCalibrationMatrix[nRow][nCol++] = col->FloatText();
                            }
                            sForcePlate.nCalibrationMatrixColumns = nCol;
                        }
                        nRow++;
                    }
                    sForcePlate.nCalibrationMatrixRows = nRow;
                    sForcePlate.bValidCalibrationMatrix = true;
                }
            }
        }

        pDataAvailable = true;
        pForceSettings.vsForcePlates.push_back(sForcePlate);
    }

    return true;
} // Read force settings

bool CTinyxml2Deserializer::DeserializeImageSettings(std::vector<SImageCamera>& pImageSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pImageSettings.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto imageElem = rootElem->FirstChildElement("Image");
    if (!imageElem)
    {
        return true;
    }

    for (auto camera : ChildElementRange{ *imageElem, "Camera" })
    {
        SImageCamera sImageCamera{};

        if (!TryReadElementUnsignedInt32(*camera, "ID", sImageCamera.nID))
        {
            return false;
        }

        if (!ReadXmlBool(camera, "Enabled", sImageCamera.bEnabled))
        {
            return false;
        }

        std::string format;
        if (!TryReadElementString(*camera, "Format", format))
        {
            return false;
        }

        format = ToLower(format);
        if (format == "rawgrayscale")
        {
            sImageCamera.eFormat = CRTPacket::FormatRawGrayscale;
        }
        else if (format == "rawbgr")
        {
            sImageCamera.eFormat = CRTPacket::FormatRawBGR;
        }
        else if (format == "jpg")
        {
            sImageCamera.eFormat = CRTPacket::FormatJPG;
        }
        else if (format == "png")
        {
            sImageCamera.eFormat = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!TryReadElementUnsignedInt32(*camera, "Width", sImageCamera.nWidth)
            || !TryReadElementUnsignedInt32(*camera, "Height", sImageCamera.nHeight))
        {
            return false;
        }

        if (!TryReadElementFloat(*camera, "Left_Crop", sImageCamera.fCropLeft)
            || !TryReadElementFloat(*camera, "Top_Crop", sImageCamera.fCropTop)
            || !TryReadElementFloat(*camera, "Right_Crop", sImageCamera.fCropRight)
            || !TryReadElementFloat(*camera, "Bottom_Crop", sImageCamera.fCropBottom))
        {
            return false;
        }

        pDataAvailable = true;
        pImageSettings.push_back(sImageCamera);
    }

    return true;
} // ReadImageSettings

namespace
{
    bool TryReadSDegreeOfFreedom(tinyxml2::XMLElement& parentElement, const std::string& elementName, std::vector<SDegreeOfFreedom>& degreesOfFreedom)
    {
        SDegreeOfFreedom degreeOfFreedom;

        auto degreeOfFreedomElement = parentElement.FirstChildElement(elementName.data());
        if (!degreeOfFreedomElement)
        {
            return false;
        }

        degreeOfFreedom.type = SkeletonStringToDofSettings(elementName);

        auto constraintElem = degreeOfFreedomElement->FirstChildElement("Constraint");
        if (constraintElem)
        {
            degreeOfFreedom.lowerBound = constraintElem->DoubleAttribute("LowerBound");
            degreeOfFreedom.upperBound = constraintElem->DoubleAttribute("UpperBound");
        }
        else
        {
            degreeOfFreedom.lowerBound = degreeOfFreedomElement->DoubleAttribute("LowerBound");
            degreeOfFreedom.upperBound = degreeOfFreedomElement->DoubleAttribute("UpperBound");
        }

        auto couplingsElem = degreeOfFreedomElement->FirstChildElement("Couplings");
        if (couplingsElem)
        {
            for (auto couplingElem : ChildElementRange{ *couplingsElem, "Coupling" })
            {
                SCoupling coupling{};
                coupling.segment = couplingElem->Attribute("Segment");
                auto dof = couplingElem->Attribute("DegreeOfFreedom");
                coupling.degreeOfFreedom = SkeletonStringToDofSettings(dof);
                coupling.coefficient = couplingElem->DoubleAttribute("Coefficient");
                degreeOfFreedom.couplings.push_back(coupling);
            }
        }

        auto goalElem = degreeOfFreedomElement->FirstChildElement("Goal");
        if (goalElem)
        {
            degreeOfFreedom.goalValue = goalElem->DoubleAttribute("Value");
            degreeOfFreedom.goalWeight = goalElem->DoubleAttribute("Weight");
        }

        degreesOfFreedom.push_back(degreeOfFreedom);

        return true;
    }
}

bool CTinyxml2Deserializer::DeserializeSkeletonSettings(bool pSkeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>& pSkeletonSettingsHierarchical, std::vector<SSettingsSkeleton>& pSkeletonSettings, bool& pDataAvailable)
{

    pDataAvailable = false;
    pSkeletonSettings.clear();
    pSkeletonSettingsHierarchical.clear();

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto skeletonsElem = rootElem->FirstChildElement("Skeletons");
    if (!skeletonsElem)
    {
        return true;
    }

    int segmentIndex;
    std::map<int, int> segmentIdIndexMap;

    if (mMajorVersion > 1 || mMinorVersion > 20)
    {
        for (auto skeletonElem : ChildElementRange{ *skeletonsElem, "Skeleton" })
        {
            SSettingsSkeletonHierarchical skeletonHierarchical{};
            SSettingsSkeleton skeleton{};
            segmentIndex = 0;

            skeletonHierarchical.name = skeletonElem->Attribute("Name");
            skeleton.name = skeletonHierarchical.name;

            TryReadElementString(*skeletonElem, "Solver", skeletonHierarchical.rootSegment.solver);
            TryReadElementDouble(*skeletonElem, "Scale", skeletonHierarchical.scale);

            auto segmentsElem = skeletonElem->FirstChildElement("Segments");
            if (segmentsElem)
            {
                std::function<void(tinyxml2::XMLElement&, SSettingsSkeletonSegmentHierarchical&,
                    std::vector<SSettingsSkeletonSegment>&, std::uint32_t)> recurseSegments
                    = [&recurseSegments, &segmentIdIndexMap, &segmentIndex, &skeleton](
                        tinyxml2::XMLElement& segmentElem, SSettingsSkeletonSegmentHierarchical& segmentHierarchical,
                        std::vector<SSettingsSkeletonSegment>& segments, std::uint32_t parentId)
                    {
                        segmentHierarchical.name = segmentElem.Attribute("Name");

                        TryReadElementUnsignedInt32(segmentElem, "ID", segmentHierarchical.id);
                        segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                        TryReadElementString(segmentElem, "Solver", segmentHierarchical.solver);

                        auto transformElem = segmentElem.FirstChildElement("Transform");
                        if (transformElem)
                        {
                            segmentHierarchical.position = ReadSPosition(*transformElem, "Position");
                            segmentHierarchical.rotation = ReadSRotation(*transformElem, "Rotation");
                        }

                        auto defaultTransformElem = segmentElem.FirstChildElement("DefaultTransform");
                        if (defaultTransformElem)
                        {
                            segmentHierarchical.defaultPosition = ReadSPosition(*defaultTransformElem, "Position");
                            segmentHierarchical.defaultRotation = ReadSRotation(*defaultTransformElem, "Rotation");
                        }

                        auto degreesOfFreedomElem = segmentElem.FirstChildElement("DegreesOfFreedom");
                        if (degreesOfFreedomElem)
                        {
                            TryReadSDegreeOfFreedom(*degreesOfFreedomElem, "RotationX",
                                segmentHierarchical.degreesOfFreedom);
                            TryReadSDegreeOfFreedom(*degreesOfFreedomElem, "RotationY",
                                segmentHierarchical.degreesOfFreedom);
                            TryReadSDegreeOfFreedom(*degreesOfFreedomElem, "RotationZ",
                                segmentHierarchical.degreesOfFreedom);
                            TryReadSDegreeOfFreedom(*degreesOfFreedomElem, "TranslationX",
                                segmentHierarchical.degreesOfFreedom);
                            TryReadSDegreeOfFreedom(*degreesOfFreedomElem, "TranslationY",
                                segmentHierarchical.degreesOfFreedom);
                            TryReadSDegreeOfFreedom(*degreesOfFreedomElem, "TranslationZ",
                                segmentHierarchical.degreesOfFreedom);
                        }

                        segmentHierarchical.endpoint = ReadSPosition(segmentElem, "Endpoint");

                        auto markersElem = segmentElem.FirstChildElement("Markers");
                        if (markersElem)
                        {
                            for (auto markerElem : ChildElementRange{ *markersElem, "Marker" })
                            {
                                SMarker marker;

                                marker.name = markerElem->Attribute("Name");

                                marker.position = ReadSPosition(*markerElem, "Position");

                                if (!TryReadElementDouble(*markerElem, "Weight", marker.weight))
                                {
                                    marker.weight = 1.0;
                                }

                                segmentHierarchical.markers.push_back(marker);
                            }
                        }
                        auto rigidBodiesElem = segmentElem.FirstChildElement("Markers");
                        if (rigidBodiesElem)
                        {
                            for (auto rigidBodyElem : ChildElementRange{ *rigidBodiesElem, "RigidBody" })
                            {
                                SBody body;

                                body.name = rigidBodyElem->Attribute("Name");

                                auto rbodyTransformElem = rigidBodyElem->FirstChildElement("Transform");
                                if (rbodyTransformElem)
                                {
                                    body.position = ReadSPosition(*rbodyTransformElem, "Position");
                                    body.rotation = ReadSRotation(*rbodyTransformElem, "Rotation");
                                }

                                if (!TryReadElementDouble(*rbodyTransformElem, "Weight", body.weight))
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

                        for (auto childSegmentElem : ChildElementRange{ segmentElem, "Segment" })
                        {
                            SSettingsSkeletonSegmentHierarchical childSegment;
                            recurseSegments(*childSegmentElem, childSegment, skeleton.segments, segmentHierarchical.id);
                            segmentHierarchical.segments.push_back(childSegment);
                        }
                    };

                auto rootSegmentElem = segmentsElem->FirstChildElement("Segment");
                if (rootSegmentElem)
                {
                    recurseSegments(*rootSegmentElem, skeletonHierarchical.rootSegment, skeleton.segments, -1);
                }
            }

            pDataAvailable = true;
            pSkeletonSettings.push_back(skeleton);
            pSkeletonSettingsHierarchical.push_back(skeletonHierarchical);
        }

        return true;
    }

    for (auto skeletonElem : ChildElementRange{ *skeletonsElem, "Skeleton" })
    {
        SSettingsSkeleton skeleton{};
        segmentIndex = 0;
        skeleton.name = skeletonElem->Attribute("Name");
        for (auto segmentElem : ChildElementRange{ *skeletonElem, "Segment" })
        {
            SSettingsSkeletonSegment segment{};

            const char* name;
            if (segmentElem->QueryAttribute("Name", &name) == tinyxml2::XML_SUCCESS)
            {
                segment.name = name;
            }
            else
            {
                return false;
            }

            if (segmentElem->QueryUnsignedAttribute("ID", &segment.id) != tinyxml2::XML_SUCCESS)
            {
                return false;
            }

            segmentIdIndexMap[segment.id] = segmentIndex++;

            if (segmentElem->QueryAttribute("Parent_ID", &segment.parentId) != tinyxml2::XML_SUCCESS)
            {
                segment.parentId = -1;
                segment.parentIndex = -1;
            }
            else if (segmentIdIndexMap.count(segment.parentId) > 0)
            {
                segment.parentIndex = segmentIdIndexMap[segment.parentId];
            }

            auto positionElement = segmentElem->FirstChildElement("Position");
            if (positionElement)
            {
                segment.positionX = positionElement->FloatAttribute("X");
                segment.positionY = positionElement->FloatAttribute("Y");
                segment.positionZ = positionElement->FloatAttribute("Z");
            }

            auto rotationElement = segmentElem->FirstChildElement("Rotation");
            if (rotationElement)
            {
                segment.rotationX = rotationElement->FloatAttribute("X");
                segment.rotationY = rotationElement->FloatAttribute("Y");
                segment.rotationZ = rotationElement->FloatAttribute("Z");
                segment.rotationW = rotationElement->FloatAttribute("W");
            }

            skeleton.segments.push_back(segment);
        }

        pDataAvailable = true;
        pSkeletonSettings.push_back(skeleton);
    }

    return true;
} // ReadSkeletonSettings

namespace
{
    bool TryReadXmlFov(std::string name, tinyxml2::XMLElement& parentElement, SCalibrationFov& fov)
    {
        auto childElement = parentElement.FirstChildElement(name.data());
        if (!childElement)
        {
            return false;
        }

        fov.left = childElement->UnsignedAttribute("left");
        fov.top = childElement->UnsignedAttribute("top");
        fov.right = childElement->UnsignedAttribute("right");
        fov.bottom = childElement->UnsignedAttribute("bottom");

        return true;
    }
}

bool CTinyxml2Deserializer::DeserializeCalibrationSettings(SCalibration& pCalibrationSettings)
{
    SCalibration settings{};

    auto rootElem = mXmlDocument.RootElement();
    if (!rootElem)
    {
        return true;
    }

    auto calibrationElem = rootElem->FirstChildElement("calibration");
    if (!calibrationElem)
    {
        return false;
    }

    try
    {
        settings.calibrated = calibrationElem->BoolAttribute("calibrated");
        settings.source = calibrationElem->Attribute("source");
        settings.created = calibrationElem->Attribute("created");
        settings.qtm_version = calibrationElem->Attribute("qtm-version");

        std::string typeStr = ToLower(calibrationElem->Attribute("type"));
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
            settings.refit_residual = calibrationElem->DoubleAttribute("refit-residual");
        }

        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = calibrationElem->DoubleAttribute("wandLength");
            settings.max_frames = calibrationElem->UnsignedAttribute("maximumFrames");
            settings.short_arm_end = calibrationElem->DoubleAttribute("shortArmEnd");
            settings.long_arm_end = calibrationElem->DoubleAttribute("longArmEnd");
            settings.long_arm_middle = calibrationElem->DoubleAttribute("longArmMiddle");

            auto resultsElem = calibrationElem->FirstChildElement("results");
            if (!resultsElem)
            {
                return false;
            }

            settings.result_std_dev = resultsElem->DoubleAttribute("std-dev");
            settings.result_min_max_diff = resultsElem->DoubleAttribute("min-max-diff");

            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = resultsElem->DoubleAttribute("refit-residual");
                settings.result_consecutive = resultsElem->UnsignedAttribute("consecutive");
            }
        }

        auto camerasElem = calibrationElem->FirstChildElement("cameras");
        if (!camerasElem)
        {
            return false;
        }

        for (auto cameraElem : ChildElementRange{ *camerasElem, "camera" })
        {
            SCalibrationCamera camera{};
            camera.active = cameraElem->UnsignedAttribute("active") != 0;
            camera.calibrated = cameraElem->BoolAttribute("calibrated");
            camera.message = cameraElem->Attribute("message");

            camera.point_count = cameraElem->UnsignedAttribute("point-count");
            camera.avg_residual = cameraElem->DoubleAttribute("avg-residual");
            camera.serial = cameraElem->UnsignedAttribute("serial");
            camera.model = cameraElem->Attribute("model");
            camera.view_rotation = cameraElem->UnsignedAttribute("viewrotation");

            if (!TryReadXmlFov("fov_marker", *cameraElem, camera.fov_marker))
            {
                return false;
            }

            if (!TryReadXmlFov("fov_marker_max", *cameraElem, camera.fov_marker_max))
            {
                return false;
            }

            if (!TryReadXmlFov("fov_video", *cameraElem, camera.fov_video))
            {
                return false;
            }

            if (!TryReadXmlFov("fov_video_max", *cameraElem, camera.fov_video_max))
            {
                return false;
            }

            auto transformElem = cameraElem->FirstChildElement("transform");
            if (!transformElem)
            {
                return false;
            }

            camera.transform.x = transformElem->DoubleAttribute("x");
            camera.transform.y = transformElem->DoubleAttribute("y");
            camera.transform.z = transformElem->DoubleAttribute("z");
            camera.transform.r11 = transformElem->DoubleAttribute("r11");
            camera.transform.r12 = transformElem->DoubleAttribute("r12");
            camera.transform.r13 = transformElem->DoubleAttribute("r13");
            camera.transform.r21 = transformElem->DoubleAttribute("r21");
            camera.transform.r22 = transformElem->DoubleAttribute("r22");
            camera.transform.r23 = transformElem->DoubleAttribute("r23");
            camera.transform.r31 = transformElem->DoubleAttribute("r31");
            camera.transform.r32 = transformElem->DoubleAttribute("r32");
            camera.transform.r33 = transformElem->DoubleAttribute("r33");

            auto intrinsicElem = cameraElem->FirstChildElement("intrinsic");
            if (!intrinsicElem)
            {
                return false;
            }

            if (intrinsicElem->QueryDoubleAttribute("focallength", &camera.intrinsic.focal_length) !=
                tinyxml2::XML_SUCCESS)
            {
                camera.intrinsic.focal_length = 0;
            }

            camera.intrinsic.sensor_min_u = intrinsicElem->DoubleAttribute("sensorMinU");
            camera.intrinsic.sensor_max_u = intrinsicElem->DoubleAttribute("sensorMaxU");
            camera.intrinsic.sensor_min_v = intrinsicElem->DoubleAttribute("sensorMinV");
            camera.intrinsic.sensor_max_v = intrinsicElem->DoubleAttribute("sensorMaxV");
            camera.intrinsic.focal_length_u = intrinsicElem->DoubleAttribute("focalLengthU");
            camera.intrinsic.focal_length_v = intrinsicElem->DoubleAttribute("focalLengthV");
            camera.intrinsic.center_point_u = intrinsicElem->DoubleAttribute("centerPointU");
            camera.intrinsic.center_point_v = intrinsicElem->DoubleAttribute("centerPointV");
            camera.intrinsic.skew = intrinsicElem->DoubleAttribute("skew");
            camera.intrinsic.radial_distortion_1 = intrinsicElem->DoubleAttribute("radialDistortion1");
            camera.intrinsic.radial_distortion_2 = intrinsicElem->DoubleAttribute("radialDistortion2");
            camera.intrinsic.radial_distortion_3 = intrinsicElem->DoubleAttribute("radialDistortion3");
            camera.intrinsic.tangental_distortion_1 = intrinsicElem->DoubleAttribute("tangentalDistortion1");
            camera.intrinsic.tangental_distortion_2 = intrinsicElem->DoubleAttribute("tangentalDistortion2");
            settings.cameras.push_back(camera);
        }
    }
    catch (...)
    {
        return false;
    }

    pCalibrationSettings = settings;
    return true;
}
