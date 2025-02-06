#include "Tinyxml2Deserializer.h"
#include <tinyxml2.h>

#include <algorithm>
#include <map>

#include "Settings.h"

#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

std::string CTinyxml2Deserializer::ToLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
    return str;
}

bool CTinyxml2Deserializer::ParseString(const std::string& str, std::uint32_t& value)
{
    try
    {
        value = std::stoul(str);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool CTinyxml2Deserializer::ParseString(const std::string& str, std::int32_t& value)
{
    try
    {
        value = std::stol(str);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool CTinyxml2Deserializer::ParseString(const std::string& str, float& value)
{
    try
    {
        value = std::stof(str);
    }
    catch (...)
    {
        value = std::numeric_limits<float>::quiet_NaN();
        return false;
    }
    return true;
}

bool CTinyxml2Deserializer::ParseString(const std::string& str, double& value)
{
    try
    {
        value = std::stod(str);
    }
    catch (...)
    {
        value = std::numeric_limits<double>::quiet_NaN();
        return false;
    }
    return true;
}

bool CTinyxml2Deserializer::ParseString(const std::string& str, bool& value)
{
    std::string strLower = ToLower(str);

    if (strLower == "true" || strLower == "1")
    {
        value = true;
        return true;
    }
    else if (strLower == "false" || strLower == "0")
    {
        value = false;
        return true;
    }
    return false;
}

namespace
{
    inline void RemoveInvalidChars(std::string& str)
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
}

bool CTinyxml2Deserializer::ReadXmlBool(tinyxml2::XMLElement* xml, const std::string& element, bool& value) const
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

namespace
{
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
    oXML.Parse(pData);
}

namespace
{
    bool ReadElementDouble(tinyxml2::XMLElement& element, const char* elementName, double& output)
    {
        if (auto childElem = element.FirstChildElement(elementName))
        {
            return childElem->QueryDoubleText(&output) == tinyxml2::XML_SUCCESS;
        }

        return false;
    }


    bool ReadElementFloat(tinyxml2::XMLElement& element, const char* elementName, float& output)
    {
        if (auto childElem = element.FirstChildElement(elementName))
        {
            return childElem->QueryFloatText(&output) == tinyxml2::XML_SUCCESS;
        }

        return false;
    }

    bool ReadElementUnsignedInt32(tinyxml2::XMLElement& element, const char* elementName, std::uint32_t& output)
    {
        if (auto childElem = element.FirstChildElement(elementName))
        {
            return childElem->QueryUnsignedText(&output) == tinyxml2::XML_SUCCESS;
        }

        return false;
    }

    bool ReadElementStringAllowEmpty(tinyxml2::XMLElement& element, const char* elementName, std::string& output)
    {
        output.clear();

        if (auto childElem = element.FirstChildElement(elementName))
        {
            if (auto text = childElem->GetText())
            {
                output = text;
            }
            return true;

        }
        else
        {
            return false;
        }
    }

    void AddFlag(EProcessingActions flag, EProcessingActions& target)
    {
        target = static_cast<EProcessingActions>(target + flag);
    }
}

bool CTinyxml2Deserializer::DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings)
{
    pGeneralSettings.cameras.clear();

    auto rootElem = oXML.RootElement();
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
        pGeneralSettings.captureFrequency = frequencyElem->UnsignedText(0);
    }
    else
    {
        return false;
    }

    if (auto captureTimeElem = generalElem->FirstChildElement("Capture_Time"))
    {
        pGeneralSettings.captureTime = captureTimeElem->FloatText(.0f);
    }
    else
    {
        return false;
    }

    if (!ReadXmlBool(generalElem, "Start_On_External_Trigger", pGeneralSettings.startOnExternalTrigger))
    {
        return false;
    }
    if (mMajorVersion > 1 || mMinorVersion > 14)
    {
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NO", pGeneralSettings.startOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NC", pGeneralSettings.startOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_Software", pGeneralSettings.startOnTrigSoftware))
        {
            return false;
        }
    }


    if (auto extTimeBaseElem = generalElem->FirstChildElement("External_Time_Base"))
    {
        if (!ReadXmlBool(extTimeBaseElem, "Enabled", pGeneralSettings.externalTimebase.enabled))
        {
            return false;
        }

        std::string signalSource;
        if (!ReadElementStringAllowEmpty(*extTimeBaseElem, "Signal_Source", signalSource))
        {
            return false;
        }

        signalSource = ToLower(signalSource);
        if (signalSource == "control port")
        {
            pGeneralSettings.externalTimebase.signalSource = SourceControlPort;
        }
        else if (signalSource == "ir receiver")
        {
            pGeneralSettings.externalTimebase.signalSource = SourceIRReceiver;
        }
        else if (signalSource == "smpte")
        {
            pGeneralSettings.externalTimebase.signalSource = SourceSMPTE;
        }
        else if (signalSource == "irig")
        {
            pGeneralSettings.externalTimebase.signalSource = SourceIRIG;
        }
        else if (signalSource == "video sync")
        {
            pGeneralSettings.externalTimebase.signalSource = SourceVideoSync;
        }
        else
        {
            return false;
        }

        std::string signalMode;
        if (!ReadElementStringAllowEmpty(*extTimeBaseElem, "Signal_Mode", signalMode))
        {
            return false;
        }

        signalMode = ToLower(signalMode);
        if (signalMode == "periodic")
        {
            pGeneralSettings.externalTimebase.signalModePeriodic = true;
        }
        else if (signalMode == "non-periodic")
        {
            pGeneralSettings.externalTimebase.signalModePeriodic = false;
        }
        else
        {
            return false;
        }
        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Multiplier", pGeneralSettings.externalTimebase.freqMultiplier))
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Divisor", pGeneralSettings.externalTimebase.freqDivisor))
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Tolerance", pGeneralSettings.externalTimebase.freqTolerance))
        {
            return false;
        }

        if (!ReadElementFloat(*extTimeBaseElem, "Nominal_Frequency", pGeneralSettings.externalTimebase.nominalFrequency))
        {
            std::string nominalFrequency;
            if (ReadElementStringAllowEmpty(*extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
            {
                if (ToLower(nominalFrequency) == "none")
                {
                    pGeneralSettings.externalTimebase.nominalFrequency = -1; // -1 = disabled
                }
            }
            else
            {
                return false;
            }
        }

        std::string signalEdge;
        if (ReadElementStringAllowEmpty(*extTimeBaseElem, "Signal_Edge", signalEdge))
        {
            signalEdge = ToLower(signalEdge);
            if (signalEdge == "negative")
            {
                pGeneralSettings.externalTimebase.negativeEdge = true;
            }
            else if (signalEdge == "positive")
            {
                pGeneralSettings.externalTimebase.negativeEdge = false;
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

        if (!ReadElementFloat(*extTimeBaseElem, "Nominal_Frequency", pGeneralSettings.externalTimebase.nominalFrequency))
        {
            std::string nominalFrequency;
            if (ReadElementStringAllowEmpty(*extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
            {
                if (ToLower(nominalFrequency) == "none")
                {
                    pGeneralSettings.externalTimebase.nominalFrequency = -1; // -1 = disabled
                }
            }
            else
            {
                return false;
            }
        }

        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Signal_Shutter_Delay", pGeneralSettings.externalTimebase.signalShutterDelay))
        {
            return false;
        }

        if (!ReadElementFloat(*extTimeBaseElem, "Non_Periodic_Timeout", pGeneralSettings.externalTimebase.nonPeriodicTimeout))
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
        if (!ReadXmlBool(externalTimestampElem, "Enabled", pGeneralSettings.timestamp.enabled))
        {
            return false;
        }

        std::string type;
        if (ReadElementStringAllowEmpty(*externalTimestampElem, "Type", type))
        {
            type = ToLower(type);
            if (type == "smpte")
            {
                pGeneralSettings.timestamp.type = Timestamp_SMPTE;
            }
            else if (type == "irig")
            {
                pGeneralSettings.timestamp.type = Timestamp_IRIG;
            }
            else
            {
                pGeneralSettings.timestamp.type = Timestamp_CameraTime;
            }
        }

        ReadElementUnsignedInt32(*externalTimestampElem, "Frequency", pGeneralSettings.timestamp.frequency);
    }

    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    EProcessingActions* processingActions[3] =
    {
        &pGeneralSettings.processingActions,
        &pGeneralSettings.rtProcessingActions,
        &pGeneralSettings.reprocessingActions
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
        if (!ReadElementStringAllowEmpty(*processingElem, "Tracking", trackingMode))
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
        if (!ReadElementUnsignedInt32(*cameraElem, "ID", sCameraSettings.id))
        {
            return false;
        }
        std::string model;
        if (!ReadElementStringAllowEmpty(*cameraElem, "Model", model))
        {
            return false;
        }

        model = ToLower(model);

        if (model == "macreflex")
        {
            sCameraSettings.model = ModelMacReflex;
        }
        else if (model == "proreflex 120")
        {
            sCameraSettings.model = ModelProReflex120;
        }
        else if (model == "proreflex 240")
        {
            sCameraSettings.model = ModelProReflex240;
        }
        else if (model == "proreflex 500")
        {
            sCameraSettings.model = ModelProReflex500;
        }
        else if (model == "proreflex 1000")
        {
            sCameraSettings.model = ModelProReflex1000;
        }
        else if (model == "oqus 100")
        {
            sCameraSettings.model = ModelOqus100;
        }
        else if (model == "oqus 200" || model == "oqus 200 c")
        {
            sCameraSettings.model = ModelOqus200C;
        }
        else if (model == "oqus 300")
        {
            sCameraSettings.model = ModelOqus300;
        }
        else if (model == "oqus 300 plus")
        {
            sCameraSettings.model = ModelOqus300Plus;
        }
        else if (model == "oqus 400")
        {
            sCameraSettings.model = ModelOqus400;
        }
        else if (model == "oqus 500")
        {
            sCameraSettings.model = ModelOqus500;
        }
        else if (model == "oqus 500 plus")
        {
            sCameraSettings.model = ModelOqus500Plus;
        }
        else if (model == "oqus 700")
        {
            sCameraSettings.model = ModelOqus700;
        }
        else if (model == "oqus 700 plus")
        {
            sCameraSettings.model = ModelOqus700Plus;
        }
        else if (model == "oqus 600 plus")
        {
            sCameraSettings.model = ModelOqus600Plus;
        }
        else if (model == "miqus m1")
        {
            sCameraSettings.model = ModelMiqusM1;
        }
        else if (model == "miqus m3")
        {
            sCameraSettings.model = ModelMiqusM3;
        }
        else if (model == "miqus m5")
        {
            sCameraSettings.model = ModelMiqusM5;
        }
        else if (model == "miqus sync unit")
        {
            sCameraSettings.model = ModelMiqusSyncUnit;
        }
        else if (model == "miqus video")
        {
            sCameraSettings.model = ModelMiqusVideo;
        }
        else if (model == "miqus video color")
        {
            sCameraSettings.model = ModelMiqusVideoColor;
        }
        else if (model == "miqus hybrid")
        {
            sCameraSettings.model = ModelMiqusHybrid;
        }
        else if (model == "miqus video color plus")
        {
            sCameraSettings.model = ModelMiqusVideoColorPlus;
        }
        else if (model == "arqus a5")
        {
            sCameraSettings.model = ModelArqusA5;
        }
        else if (model == "arqus a9")
        {
            sCameraSettings.model = ModelArqusA9;
        }
        else if (model == "arqus a12")
        {
            sCameraSettings.model = ModelArqusA12;
        }
        else if (model == "arqus a26")
        {
            sCameraSettings.model = ModelArqusA26;
        }
        else
        {
            sCameraSettings.model = ModelUnknown;
        }

        ReadXmlBool(cameraElem, "Underwater", sCameraSettings.underwater);
        ReadXmlBool(cameraElem, "Supports_HW_Sync", sCameraSettings.supportsHwSync);

        if (!ReadElementUnsignedInt32(*cameraElem, "Serial", sCameraSettings.serial))
        {
            return false;
        }

        std::string mode;
        if (!ReadElementStringAllowEmpty(*cameraElem, "Mode", mode))
        {
            return false;
        }

        mode = ToLower(mode);
        if (mode == "marker")
        {
            sCameraSettings.mode = ModeMarker;
        }
        else if (mode == "marker intensity")
        {
            sCameraSettings.mode = ModeMarkerIntensity;
        }
        else if (mode == "video")
        {
            sCameraSettings.mode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            if (!ReadElementUnsignedInt32(*cameraElem, "Video_Frequency", sCameraSettings.videoFrequency))
            {
                return false;
            }
        }

        std::string videoResolution;
        if (ReadElementStringAllowEmpty(*cameraElem, "Video_Resolution", videoResolution))
        {
            videoResolution = ToLower(videoResolution);
            if (videoResolution == "1440p")
            {
                sCameraSettings.videoResolution = VideoResolution1440p;
            }
            else if (videoResolution == "1080p")
            {
                sCameraSettings.videoResolution = VideoResolution1080p;
            }
            else if (videoResolution == "720p")
            {
                sCameraSettings.videoResolution = VideoResolution720p;
            }
            else if (videoResolution == "540p")
            {
                sCameraSettings.videoResolution = VideoResolution540p;
            }
            else if (videoResolution == "480p")
            {
                sCameraSettings.videoResolution = VideoResolution480p;
            }
            else
            {
                sCameraSettings.videoResolution = VideoResolutionNone;
            }
        }
        else
        {
            sCameraSettings.videoResolution = VideoResolutionNone;
        }

        std::string videoAspectRatio;
        if (ReadElementStringAllowEmpty(*cameraElem, "Video_Aspect_Ratio", videoAspectRatio))
        {
            videoAspectRatio = ToLower(videoAspectRatio);
            if (videoAspectRatio == "16x9")
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatio16x9;
            }
            else if (videoAspectRatio == "4x3")
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatio4x3;
            }
            else if (videoAspectRatio == "1x1")
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatio1x1;
            }
            else
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatioNone;
            }
        }
        else
        {
            sCameraSettings.videoAspectRatio = VideoAspectRatioNone;
        }

        auto videoExposureElem = cameraElem->FirstChildElement("Video_Exposure");
        if (videoExposureElem)
        {
            if (!ReadElementUnsignedInt32(*videoExposureElem, "Current", sCameraSettings.videoExposure)
                || !ReadElementUnsignedInt32(*videoExposureElem, "Min", sCameraSettings.videoExposureMin)
                || !ReadElementUnsignedInt32(*videoExposureElem, "Max", sCameraSettings.videoExposureMax))
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
            if (!ReadElementUnsignedInt32(*videoFlashTime, "Current", sCameraSettings.videoFlashTime)
                || !ReadElementUnsignedInt32(*videoFlashTime, "Min", sCameraSettings.videoFlashTimeMin)
                || !ReadElementUnsignedInt32(*videoFlashTime, "Max", sCameraSettings.videoFlashTimeMax))
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
            if (!ReadElementUnsignedInt32(*markerExposureElem, "Current", sCameraSettings.markerExposure)
                || !ReadElementUnsignedInt32(*markerExposureElem, "Min", sCameraSettings.markerExposureMin)
                || !ReadElementUnsignedInt32(*markerExposureElem, "Max", sCameraSettings.markerExposureMax))
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
            if (!ReadElementUnsignedInt32(*markerThresholdElem, "Current", sCameraSettings.markerThreshold)
                || !ReadElementUnsignedInt32(*markerThresholdElem, "Min", sCameraSettings.markerThresholdMin)
                || !ReadElementUnsignedInt32(*markerThresholdElem, "Max", sCameraSettings.markerThresholdMax))
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
            if (!ReadElementFloat(*positionElem, "X", sCameraSettings.positionX)
                || !ReadElementFloat(*positionElem, "Y", sCameraSettings.positionY)
                || !ReadElementFloat(*positionElem, "Z", sCameraSettings.positionZ)
                || !ReadElementFloat(*positionElem, "Rot_1_1", sCameraSettings.positionRotMatrix[0][0])
                || !ReadElementFloat(*positionElem, "Rot_2_1", sCameraSettings.positionRotMatrix[1][0])
                || !ReadElementFloat(*positionElem, "Rot_3_1", sCameraSettings.positionRotMatrix[2][0])
                || !ReadElementFloat(*positionElem, "Rot_1_2", sCameraSettings.positionRotMatrix[0][1])
                || !ReadElementFloat(*positionElem, "Rot_2_2", sCameraSettings.positionRotMatrix[1][1])
                || !ReadElementFloat(*positionElem, "Rot_3_2", sCameraSettings.positionRotMatrix[2][1])
                || !ReadElementFloat(*positionElem, "Rot_1_3", sCameraSettings.positionRotMatrix[0][2])
                || !ReadElementFloat(*positionElem, "Rot_2_3", sCameraSettings.positionRotMatrix[1][2])
                || !ReadElementFloat(*positionElem, "Rot_3_3", sCameraSettings.positionRotMatrix[2][2])
                )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*cameraElem, "Orientation", sCameraSettings.orientation))
        {
            return false;
        }

        auto markerResElem = cameraElem->FirstChildElement("Marker_Res");
        if (markerResElem)
        {
            if (!ReadElementUnsignedInt32(*markerResElem, "Width", sCameraSettings.markerResolutionWidth)
                || !ReadElementUnsignedInt32(*markerResElem, "Height", sCameraSettings.markerResolutionHeight)
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
            if (!ReadElementUnsignedInt32(*videoResElem, "Width", sCameraSettings.videoResolutionWidth)
                || !ReadElementUnsignedInt32(*videoResElem, "Height", sCameraSettings.videoResolutionHeight)
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
            if (!ReadElementUnsignedInt32(*markerFovElem, "Left", sCameraSettings.markerFOVLeft)
                || !ReadElementUnsignedInt32(*markerFovElem, "Top", sCameraSettings.markerFOVTop)
                || !ReadElementUnsignedInt32(*markerFovElem, "Right", sCameraSettings.markerFOVRight)
                || !ReadElementUnsignedInt32(*markerFovElem, "Bottom", sCameraSettings.markerFOVBottom)
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
            if (!ReadElementUnsignedInt32(*videoFovElem, "Left", sCameraSettings.videoFOVLeft)
                || !ReadElementUnsignedInt32(*videoFovElem, "Top", sCameraSettings.videoFOVTop)
                || !ReadElementUnsignedInt32(*videoFovElem, "Right", sCameraSettings.videoFOVRight)
                || !ReadElementUnsignedInt32(*videoFovElem, "Bottom", sCameraSettings.videoFOVBottom)
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
                    if (!ReadElementStringAllowEmpty(*syncOutElem, "Mode", mode))
                    {
                        return false;
                    }

                    mode = ToLower(mode);
                    if (mode == "shutter out")
                    {
                        sCameraSettings.syncOutMode[port] = ModeShutterOut;
                    }
                    else if (mode == "multiplier")
                    {
                        sCameraSettings.syncOutMode[port] = ModeMultiplier;
                    }
                    else if (mode == "divisor")
                    {
                        sCameraSettings.syncOutMode[port] = ModeDivisor;
                    }
                    else if (mode == "camera independent")
                    {
                        sCameraSettings.syncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (mode == "measurement time")
                    {
                        sCameraSettings.syncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (mode == "continuous 100hz")
                    {
                        sCameraSettings.syncOutMode[port] = ModeFixed100Hz;
                    }
                    else if (mode == "system live time")
                    {
                        sCameraSettings.syncOutMode[port] = ModeSystemLiveTime;
                    }
                    else
                    {
                        return false;
                    }

                    if (sCameraSettings.syncOutMode[port] == ModeMultiplier ||
                        sCameraSettings.syncOutMode[port] == ModeDivisor ||
                        sCameraSettings.syncOutMode[port] == ModeIndependentFreq)
                    {
                        if (!ReadElementUnsignedInt32(*syncOutElem, "Value", sCameraSettings.syncOutValue[port])
                            || !ReadElementFloat(*syncOutElem, "Duty_Cycle", sCameraSettings.syncOutDutyCycle[port]))
                        {
                            return false;
                        }
                    }
                }

                if (port == 2 || (sCameraSettings.syncOutMode[port] != ModeFixed100Hz))
                {
                    std::string signalPolarity;
                    if (ReadElementStringAllowEmpty(*syncOutElem, "Signal_Polarity", signalPolarity))
                    {
                        sCameraSettings.syncOutNegativePolarity[port] = ToLower(signalPolarity) == "negative";
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            else
            {
                sCameraSettings.syncOutMode[port] = ModeIndependentFreq;
                sCameraSettings.syncOutValue[port] = 0;
                sCameraSettings.syncOutDutyCycle[port] = 0;
                sCameraSettings.syncOutNegativePolarity[port] = false;
            }
        }

        auto lensControlElem = cameraElem->FirstChildElement("LensControl");
        if (lensControlElem)
        {
            auto focusElem = lensControlElem->FirstChildElement("Focus");
            if (focusElem)
            {
                sCameraSettings.focus = focusElem->FloatAttribute("Value", std::numeric_limits<float>::quiet_NaN());
            }

            auto apertureElem = lensControlElem->FirstChildElement("Aperture");
            if (apertureElem)
            {
                sCameraSettings.aperture = apertureElem->FloatAttribute("Value", std::numeric_limits<float>::quiet_NaN());
            }
        }
        else
        {
            sCameraSettings.focus = std::numeric_limits<float>::quiet_NaN();
            sCameraSettings.aperture = std::numeric_limits<float>::quiet_NaN();
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

        pGeneralSettings.cameras.push_back(sCameraSettings);
    }

    return true;
}

bool CTinyxml2Deserializer::Deserialize3DSettings(SSettings3D& p3dSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    p3dSettings.calibrationTime[0] = 0;

    auto rootElem = oXML.RootElement();
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
        if (const char* charptr = axisUpwards->GetText())
        {
            auto tStr = ToLower(charptr);
            if (tStr == "+x")
            {
                p3dSettings.axisUpwards = XPos;
            }
            else if (tStr == "-x")
            {
                p3dSettings.axisUpwards = XNeg;
            }
            else if (tStr == "+y")
            {
                p3dSettings.axisUpwards = YPos;
            }
            else if (tStr == "-y")
            {
                p3dSettings.axisUpwards = YNeg;
            }
            else if (tStr == "+z")
            {
                p3dSettings.axisUpwards = ZPos;
            }
            else if (tStr == "-z")
            {
                p3dSettings.axisUpwards = ZNeg;
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
            strcpy_s(p3dSettings.calibrationTime, 32, charPtr);
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

    p3dSettings.labels3D.clear();
    p3dSettings.labels3D.reserve(labelCount);
    for (auto labelElem = t3dElem->FirstChildElement("Label"); labelElem != nullptr; labelElem = labelElem->NextSiblingElement("Label"))
    {
        SSettings3DLabel label{};
        if (auto nameElem = labelElem->FirstChildElement("Name"))
        {
            if (auto namePtr = nameElem->GetText())
            {
                label.name = namePtr;
            }
        }

        if (auto colorElem = labelElem->FirstChildElement("RGBColor"))
        {
            label.rgbColor = colorElem->IntText(0);
        }

        if (auto typeElem = labelElem->FirstChildElement("Trajectory_Type"))
        {
            if (auto text = typeElem->GetText())
            {
                label.type = text;
            }
        }

        p3dSettings.labels3D.push_back(label);
    }

    if (p3dSettings.labels3D.size() != labelCount)
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

            p3dSettings.bones.push_back(bone);
        }
    }

    pDataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryReadSetEnabled(std::uint32_t nMajorVer, std::uint32_t nMinorVer, tinyxml2::XMLElement& oXML, bool& bTarget)
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

    bool TryReadSetName(tinyxml2::XMLElement& oXML, std::string& sTarget)
    {
        if (auto elem = oXML.FirstChildElement("Name"))
        {
            if (auto text = elem->GetText())
            {
                sTarget = text;
            }
            else
            {
                sTarget = {};
            }
            return true;
        }

        return false;
    }

    bool TryReadSetColor(tinyxml2::XMLElement& oXML, std::uint32_t& nTarget)
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

    bool TryReadSetMaxResidual(tinyxml2::XMLElement& oXML, float& fTarget)
    {
        if (auto elem = oXML.FirstChildElement("MaximumResidual"))
        {
            fTarget = static_cast<float>(atof(elem->GetText()));
            return true;
        }

        fTarget = .0f;
        return false;
    }

    bool TryReadSetMinMarkersInBody(tinyxml2::XMLElement& oXML, std::uint32_t& nTarget)
    {
        if (auto elem = oXML.FirstChildElement("MinimumMarkersInBody"))
        {
            nTarget = elem->IntText(0);
            return true;
        }

        nTarget = 0;
        return false;
    }

    bool TryReadSetBoneLenTolerance(tinyxml2::XMLElement& oXML, float& fTarget)
    {
        if (auto elem = oXML.FirstChildElement("BoneLengthTolerance"))
        {
            fTarget = static_cast<float>(atof(elem->GetText()));
            return true;
        }

        fTarget = .0f;
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

    bool TryReadSetScale(tinyxml2::XMLElement& oXML, float& fTarget)
    {
        if (auto elem = oXML.FirstChildElement("Scale"))
        {
            fTarget = static_cast<float>(atof(elem->GetText()));
            return true;
        }

        fTarget = .0f;
        return false;
    }

    bool TryReadSetOpacity(tinyxml2::XMLElement& oXML, float& fTarget)
    {
        if (auto elem = oXML.FirstChildElement("Opacity"))
        {
            fTarget = static_cast<float>(atof(elem->GetText()));
            return true;
        }

        fTarget = .0f;
        return false;
    }

    bool TryReadSetPoints(tinyxml2::XMLElement& oXML, std::vector<SBodyPoint>& vTarget)
    {
        if (auto pointsElem = oXML.FirstChildElement("Points"))
        {
            for (auto pointElem = pointsElem->FirstChildElement("Point"); pointElem != nullptr; pointElem = pointElem->NextSiblingElement("Point"))
            {
                SBodyPoint sBodyPoint;

                sBodyPoint.x = static_cast<float>(atof(pointElem->Attribute("X")));
                sBodyPoint.y = static_cast<float>(atof(pointElem->Attribute("Y")));
                sBodyPoint.z = static_cast<float>(atof(pointElem->Attribute("Z")));

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
            oTarget.position.x = static_cast<float>(atof(elem->Attribute("X")));
            oTarget.position.y = static_cast<float>(atof(elem->Attribute("Y")));
            oTarget.position.z = static_cast<float>(atof(elem->Attribute("Z")));
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


    bool TryReadSetRGBColor(tinyxml2::XMLElement& oXML, std::uint32_t& oTarget)
    {
        if (auto elem = oXML.FirstChildElement("RGBColor"))
        {
            oTarget = atoi(elem->GetText());
            return true;
        }

        oTarget = 0;
        return false;
    }

    bool TryReadFloatElement(tinyxml2::XMLElement& parent, const char* childElementName, float& output)
    {
        if (auto elem = parent.FirstChildElement(childElementName))
        {
            output = elem->FloatText(.0f);
            return true;
        }
        output = .0f;
        return false;
    }

    bool TryReadTextElement(tinyxml2::XMLElement& parent, const char* childElementName, std::string& output)
    {
        if (auto elem = parent.FirstChildElement(childElementName))
        {
            output = elem->GetText();
            return true;
        }

        output = {};
        return false;
    }

    bool TryReadSetPointsOld(tinyxml2::XMLElement& oXML, std::vector<SBodyPoint>& vTarget)
    {
        vTarget.clear();
        for (auto pointElem = oXML.FirstChildElement("Point"); pointElem != nullptr; pointElem = pointElem->NextSiblingElement("Point"))
        {
            SBodyPoint sPoint;

            if (!TryReadFloatElement(*pointElem, "X", sPoint.x))
            {
                return false;
            }

            if (!TryReadFloatElement(*pointElem, "Y", sPoint.y))
            {
                return false;
            }

            if (!TryReadFloatElement(*pointElem, "Z", sPoint.z))
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
            return TryReadTextElement(*elem, "First", sTargetFirst)
                && TryReadTextElement(*elem, "Second", sTargetSecond)
                && TryReadTextElement(*elem, "Third", sTargetThird);
        }

        return false;
    }
}

bool CTinyxml2Deserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& p6DOFSettings, SSettingsGeneral& pGeneralSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    p6DOFSettings.clear();

    auto rootElem = oXML.RootElement();
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

            if (!TryReadSetName(*bodyElem, s6DOFBodySettings.name))
            { // Name --- REQUIRED
                return false;
            }

            TryReadSetEnabled(mMajorVersion, mMinorVersion, *bodyElem, s6DOFBodySettings.enabled);
            if (!TryReadSetColor(*bodyElem, s6DOFBodySettings.color)
                || !TryReadSetMaxResidual(*bodyElem, s6DOFBodySettings.maxResidual)
                || !TryReadSetMinMarkersInBody(*bodyElem, s6DOFBodySettings.minMarkersInBody)
                || !TryReadSetBoneLenTolerance(*bodyElem, s6DOFBodySettings.boneLengthTolerance)
                || !TryReadSetFilter(*bodyElem, s6DOFBodySettings.filterPreset))
            { // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                return false;
            }


            if (auto meshElem = bodyElem->FirstChildElement("Mesh"))
            {
                if (!TryReadSetName(*meshElem, s6DOFBodySettings.mesh.name)
                    || !TryReadSetPos(*meshElem, s6DOFBodySettings.mesh.position.x, s6DOFBodySettings.mesh.position.y, s6DOFBodySettings.mesh.position.z)
                    || !TryReadSetRotation(*meshElem, s6DOFBodySettings.mesh.rotation.x, s6DOFBodySettings.mesh.rotation.y, s6DOFBodySettings.mesh.rotation.z)
                    || !TryReadSetScale(*meshElem, s6DOFBodySettings.mesh.scale)
                    || !TryReadSetOpacity(*meshElem, s6DOFBodySettings.mesh.opacity))
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
        if (!oXML.FirstChildElement("Bodies"))
        {
            return false;
        }

        for (auto bodyElem = oXML.FirstChildElement("Body"); bodyElem != nullptr; bodyElem = bodyElem->NextSiblingElement("Body"))
        {
            SSettings6DOFBody s6DOFBodySettings{};

            // Name, RGBColor, Points(OLD) --- REQUIRED
            if (!TryReadSetName(*bodyElem, s6DOFBodySettings.name)
                || !TryReadSetRGBColor(*bodyElem, s6DOFBodySettings.color)
                || !TryReadSetPointsOld(*bodyElem, s6DOFBodySettings.points))
            {
                return false;
            }

            if (mMajorVersion > 1 || mMinorVersion > 15)
            {
                // Euler --- REQUIRED
                if (!TryReadSetEuler(oXML, pGeneralSettings.eulerRotations[0], pGeneralSettings.eulerRotations[1], pGeneralSettings.eulerRotations[2]))
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

    auto rootElem = oXML.RootElement();
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

    auto rootElem = oXML.RootElement();
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

    auto rootElem = oXML.RootElement();
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
        analogDevice.deviceID = 1;   // Always channel 1
        analogDevice.name = "AnalogDevice";
        if (!ReadElementUnsignedInt32(*analogElem, "Channels", analogDevice.channels)
            || !ReadElementUnsignedInt32(*analogElem, "Frequency", analogDevice.frequency)
            || !ReadElementStringAllowEmpty(*analogElem, "Unit", analogDevice.unit))
        {
            return false;
        }

        auto rangeElem = analogElem->FirstChildElement("Range");
        if (!rangeElem
            || !ReadElementFloat(*rangeElem, "Min", analogDevice.minRange)
            || !ReadElementFloat(*rangeElem, "Max", analogDevice.maxRange))
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
        if (!ReadElementUnsignedInt32(*deviceElem, "Device_ID", analogDevice.deviceID)
            || !ReadElementStringAllowEmpty(*deviceElem, "Device_Name", analogDevice.name)
            || !ReadElementUnsignedInt32(*deviceElem, "Channels", analogDevice.channels)
            || !ReadElementUnsignedInt32(*deviceElem, "Frequency", analogDevice.frequency)
            )
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            if (!ReadElementStringAllowEmpty(*analogElem, "Unit", analogDevice.unit))
            {
                continue;
            }
        }

        auto rangeElem = deviceElem->FirstChildElement("Range");
        if (!rangeElem
            || !ReadElementFloat(*rangeElem, "Min", analogDevice.minRange)
            || !ReadElementFloat(*rangeElem, "Max", analogDevice.maxRange))
        {
            continue;
        }

        if (mMajorVersion == 1 && mMinorVersion < 11)
        {
            for (std::size_t i = 0; i < analogDevice.channels; i++)
            {
                std::string label;
                if (ReadElementStringAllowEmpty(*deviceElem, "Label", label))
                {
                    analogDevice.labels.push_back(label);
                }
            }

            if (analogDevice.labels.size() != analogDevice.channels)
            {
                continue;
            }
        }
        else
        {
            for (auto channelElem = deviceElem->FirstChildElement("Channel"); channelElem != nullptr; channelElem = channelElem->NextSiblingElement("Channel"))
            {
                std::string label;
                if (ReadElementStringAllowEmpty(*channelElem, "Label", label))
                {
                    analogDevice.labels.push_back(label);
                }

                std::string unit;
                if (ReadElementStringAllowEmpty(*channelElem, "Unit", unit))
                {
                    analogDevice.units.push_back(unit);
                }
            }

            if (analogDevice.labels.size() != analogDevice.channels ||
                analogDevice.units.size() != analogDevice.channels)
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

    pForceSettings.forcePlates.clear();

    auto rootElem = oXML.RootElement();
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
    sForcePlate.validCalibrationMatrix = false;
    sForcePlate.calibrationMatrixRows = 6;
    sForcePlate.calibrationMatrixColumns = 6;

    if (!TryReadTextElement(*forceElem, "Unit_Length", pForceSettings.unitLength))
    {
        return false;
    }

    if (!TryReadTextElement(*forceElem, "Unit_Force", pForceSettings.unitForce))
    {
        return false;
    }

    std::size_t iPlate = 0;
    for (auto plateElem : ChildElementRange{ *forceElem, "Plate" })
    {
        iPlate++;

        if (!ReadElementUnsignedInt32(*plateElem, "Plate_ID", sForcePlate.id))
        {
            if (!ReadElementUnsignedInt32(*plateElem, "Force_Plate_Index", sForcePlate.id)) // Version 1.7 and earlier.
            {
                return false;
            }
        }

        if (!ReadElementUnsignedInt32(*plateElem, "Analog_Device_ID", sForcePlate.analogDeviceID))
        {
            sForcePlate.analogDeviceID = 0;
        }

        if (!ReadElementUnsignedInt32(*plateElem, "Frequency", sForcePlate.frequency))
        {
            return false;
        }

        if (!TryReadTextElement(*plateElem, "Type", sForcePlate.type))
        {
            sForcePlate.type = "unknown";
        }

        if (!TryReadTextElement(*plateElem, "Name", sForcePlate.name))
        {
            sForcePlate.name = "#" + std::to_string(iPlate);
        }

        TryReadFloatElement(*plateElem, "Length", sForcePlate.length);
        TryReadFloatElement(*plateElem, "Width", sForcePlate.width);

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
                TryReadFloatElement(*cornerElem, "X", sForcePlate.corner[c.index].x);
                TryReadFloatElement(*cornerElem, "Y", sForcePlate.corner[c.index].y);
                TryReadFloatElement(*cornerElem, "Z", sForcePlate.corner[c.index].z);
            }
        }

        auto originElem = plateElem->FirstChildElement("Origin");
        if (originElem)
        {
            TryReadFloatElement(*originElem, "X", sForcePlate.origin.x);
            TryReadFloatElement(*originElem, "Y", sForcePlate.origin.y);
            TryReadFloatElement(*originElem, "Z", sForcePlate.origin.z);
        }

        sForcePlate.channels.clear();
        auto channelsElem = plateElem->FirstChildElement("Channels");
        if (channelsElem)
        {
            SForceChannel sForceChannel{};
            for (auto channelElem : ChildElementRange{ *channelsElem, "Channel" })
            {
                ReadElementUnsignedInt32(*channelElem, "Channel_No", sForceChannel.channelNumber);
                ReadElementFloat(*channelElem, "ConversionFactor", sForceChannel.conversionFactor);
                sForcePlate.channels.push_back(sForceChannel);
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
                        sForcePlate.calibrationMatrix[nRow][nCol++] = col->FloatText();
                    }
                    nRow++;
                    sForcePlate.calibrationMatrixColumns = nCol;
                }
                sForcePlate.calibrationMatrixRows = nRow;
                sForcePlate.validCalibrationMatrix = true;
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
                                sForcePlate.calibrationMatrix[nRow][nCol++] = col->FloatText();
                            }
                            sForcePlate.calibrationMatrixColumns = nCol;
                        }
                        nRow++;
                    }
                    sForcePlate.calibrationMatrixRows = nRow;
                    sForcePlate.validCalibrationMatrix = true;
                }
            }
        }

        pDataAvailable = true;
        pForceSettings.forcePlates.push_back(sForcePlate);
    }

    return true;
} // Read force settings

bool CTinyxml2Deserializer::DeserializeImageSettings(std::vector<SImageCamera>& pImageSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pImageSettings.clear();

    auto rootElem = oXML.RootElement();
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

        if (!ReadElementUnsignedInt32(*camera, "ID", sImageCamera.id))
        {
            return false;
        }

        if (!ReadXmlBool(camera, "Enabled", sImageCamera.enabled))
        {
            return false;
        }

        std::string format;
        if (!ReadElementStringAllowEmpty(*camera, "Format", format))
        {
            return false;
        }

        format = ToLower(format);
        if (format == "rawgrayscale")
        {
            sImageCamera.format = CRTPacket::FormatRawGrayscale;
        }
        else if (format == "rawbgr")
        {
            sImageCamera.format = CRTPacket::FormatRawBGR;
        }
        else if (format == "jpg")
        {
            sImageCamera.format = CRTPacket::FormatJPG;
        }
        else if (format == "png")
        {
            sImageCamera.format = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*camera, "Width", sImageCamera.width)
            || !ReadElementUnsignedInt32(*camera, "Height", sImageCamera.height))
        {
            return false;
        }

        if (!ReadElementFloat(*camera, "Left_Crop", sImageCamera.cropLeft)
            || !ReadElementFloat(*camera, "Top_Crop", sImageCamera.cropTop)
            || !ReadElementFloat(*camera, "Right_Crop", sImageCamera.cropRight)
            || !ReadElementFloat(*camera, "Bottom_Crop", sImageCamera.cropBottom))
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

    auto rootElem = oXML.RootElement();
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

            ReadElementStringAllowEmpty(*skeletonElem, "Solver", skeletonHierarchical.rootSegment.solver);
            ReadElementDouble(*skeletonElem, "Scale", skeletonHierarchical.scale);

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

                        ReadElementUnsignedInt32(segmentElem, "ID", segmentHierarchical.id);
                        segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                        ReadElementStringAllowEmpty(segmentElem, "Solver", segmentHierarchical.solver);

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

                                if (!ReadElementDouble(*markerElem, "Weight", marker.weight))
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

                                if (!ReadElementDouble(*rbodyTransformElem, "Weight", body.weight))
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
    bool ReadXmlFov(std::string name, tinyxml2::XMLElement& parentElement, SCalibrationFov& fov)
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

    auto rootElem = oXML.RootElement();
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

            if (!ReadXmlFov("fov_marker", *cameraElem, camera.fov_marker))
            {
                return false;
            }

            if (!ReadXmlFov("fov_marker_max", *cameraElem, camera.fov_marker_max))
            {
                return false;
            }

            if (!ReadXmlFov("fov_video", *cameraElem, camera.fov_video))
            {
                return false;
            }

            if (!ReadXmlFov("fov_video_max", *cameraElem, camera.fov_video_max))
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
