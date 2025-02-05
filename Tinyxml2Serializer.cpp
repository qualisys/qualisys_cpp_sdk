#define _CRT_SECURE_NO_WARNINGS

#include "Tinyxml2Serializer.h"
#include <tinyxml2.h>

#include <algorithm>
#include <map>

#include "Settings.h"

#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

void CTinyxml2Serializer::AddXMLElementBool(tinyxml2::XMLElement& parent, const char* tTag, const bool* pbValue, tinyxml2::XMLDocument& oXML, const char* tTrue, const char* tFalse)
{
    if (pbValue)
    {
        tinyxml2::XMLElement* pElement = oXML.NewElement(tTag);
        pElement->SetText(*pbValue ? tTrue : tFalse);
        parent.InsertEndChild(pElement);
    }
}

void CTinyxml2Serializer::AddXMLElementBool(tinyxml2::XMLElement& parent, const char* tTag, const bool pbValue, tinyxml2::XMLDocument& oXML, const char* tTrue, const char* tFalse)
{
    tinyxml2::XMLElement* pElement = oXML.NewElement(tTag);
    pElement->SetText(pbValue ? tTrue : tFalse);
    parent.InsertEndChild(pElement);
}

void CTinyxml2Serializer::AddXMLElementInt(tinyxml2::XMLElement& parent, const char* tTag, const int* pnValue, tinyxml2::XMLDocument& oXML)
{
    if (pnValue)
    {
        tinyxml2::XMLElement* elem = oXML.NewElement(tTag);
        elem->SetText(*pnValue);
        parent.InsertEndChild(elem);
    }
}


void CTinyxml2Serializer::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parent, const char* tTag, const unsigned int nValue, tinyxml2::XMLDocument& oXML)
{
    tinyxml2::XMLElement* elem = oXML.NewElement(tTag);
    elem->SetText(nValue);
    parent.InsertEndChild(elem);
}

void CTinyxml2Serializer::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parent, const char* tTag, const unsigned int* pnValue, tinyxml2::XMLDocument& oXML)
{
    if (pnValue)
    {
        AddXMLElementUnsignedInt(parent, tTag, *pnValue, oXML);
    }
}

void CTinyxml2Serializer::AddXMLElementFloat(tinyxml2::XMLElement& parent, const char* tTag, const float* pfValue, unsigned int pnDecimals, tinyxml2::XMLDocument& oXML)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", pnDecimals, *pfValue);

    tinyxml2::XMLElement* elem = oXML.NewElement(tTag);
    elem->SetText(formattedValue);
    parent.InsertEndChild(elem);
}

void CTinyxml2Serializer::AddXMLElementFloatWithTextAttribute(tinyxml2::XMLDocument& oXML, tinyxml2::XMLElement& parent, const char* elementName, const char* attributeName, const float& value, unsigned int decimals)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);

    tinyxml2::XMLElement* elem = oXML.NewElement(elementName);
    elem->SetAttribute(attributeName, formattedValue);
    parent.InsertEndChild(elem);
}


void CTinyxml2Serializer::AddXMLElementTransform(tinyxml2::XMLDocument& oXML, tinyxml2::XMLElement& parentElem, const std::string& name, const SPosition& position, const SRotation& rotation)
{
    tinyxml2::XMLElement* transformElem = oXML.NewElement(name.c_str());
    parentElem.InsertEndChild(transformElem);

    tinyxml2::XMLElement* positionElem = oXML.NewElement("Position");
    positionElem->SetAttribute("X", std::to_string(position.x).c_str());
    positionElem->SetAttribute("Y", std::to_string(position.y).c_str());
    positionElem->SetAttribute("Z", std::to_string(position.z).c_str());
    transformElem->InsertEndChild(positionElem);

    tinyxml2::XMLElement* rotationElem = oXML.NewElement("Rotation");
    rotationElem->SetAttribute("X", std::to_string(rotation.x).c_str());
    rotationElem->SetAttribute("Y", std::to_string(rotation.y).c_str());
    rotationElem->SetAttribute("Z", std::to_string(rotation.z).c_str());
    rotationElem->SetAttribute("W", std::to_string(rotation.w).c_str());
    transformElem->InsertEndChild(rotationElem);
}

void CTinyxml2Serializer::AddXMLElementDOF(tinyxml2::XMLDocument& oXML, tinyxml2::XMLElement& parentElem, const std::string& name, const SDegreeOfFreedom& degreeOfFreedoms)
{
    tinyxml2::XMLElement* dofElem = oXML.NewElement(name.c_str());
    parentElem.InsertEndChild(dofElem);

    if (!std::isnan(degreeOfFreedoms.lowerBound) && !std::isnan(degreeOfFreedoms.upperBound))
    {
        if (mnMajorVersion > 1 || mnMinorVersion > 21)
        {
            tinyxml2::XMLElement* constraintElem = oXML.NewElement("Constraint");
            constraintElem->SetAttribute("LowerBound", std::to_string(degreeOfFreedoms.lowerBound).c_str());
            constraintElem->SetAttribute("UpperBound", std::to_string(degreeOfFreedoms.upperBound).c_str());
            dofElem->InsertEndChild(constraintElem);
        }
        else
        {
            // If not in a 'Constraint' block, add 'LowerBound' & 'UpperBound' directly to dofElem
            dofElem->SetAttribute("LowerBound", std::to_string(degreeOfFreedoms.lowerBound).c_str());
            dofElem->SetAttribute("UpperBound", std::to_string(degreeOfFreedoms.upperBound).c_str());
        }
    }

    if (!degreeOfFreedoms.couplings.empty())
    {
        tinyxml2::XMLElement* couplingsElem = oXML.NewElement("Couplings");
        dofElem->InsertEndChild(couplingsElem);

        for (const auto& coupling : degreeOfFreedoms.couplings)
        {
            tinyxml2::XMLElement* couplingElem = oXML.NewElement("Coupling");
            couplingElem->SetAttribute("Segment", coupling.segment.c_str());
            couplingElem->SetAttribute("DegreeOfFreedom", SkeletonDofToStringSettings(coupling.degreeOfFreedom));
            couplingElem->SetAttribute("Coefficient", std::to_string(coupling.coefficient).c_str());
            couplingsElem->InsertEndChild(couplingElem);
        }
    }

    if (!std::isnan(degreeOfFreedoms.goalValue) && !std::isnan(degreeOfFreedoms.goalWeight))
    {
        tinyxml2::XMLElement* goalElem = oXML.NewElement("Goal");
        goalElem->SetAttribute("Value", std::to_string(degreeOfFreedoms.goalValue).c_str());
        goalElem->SetAttribute("Weight", std::to_string(degreeOfFreedoms.goalWeight).c_str());
        dofElem->InsertEndChild(goalElem);
    }
}

bool CTinyxml2Deserializer::CompareNoCase(std::string tStr1, const char* tStr2) const
{
    tStr1 = ToLower(tStr1);
    return tStr1.compare(tStr2) == 0;
}

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
    : mnMajorVersion(pMajorVersion), mnMinorVersion(pMinorVersion), maErrorStr{ 0 }
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

    bool ReadElementStringAllowEmpty(tinyxml2::XMLElement& element, const char *elementName,std::string& output )
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
    pGeneralSettings.vsCameras.clear();

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
    if (mnMajorVersion > 1 || mnMinorVersion > 14)
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
        if(!ReadXmlBool(extTimeBaseElem, "Enabled", pGeneralSettings.sExternalTimebase.bEnabled))
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
        if (!ReadElementStringAllowEmpty(*extTimeBaseElem, "Signal_Mode", signalMode))
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
        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Multiplier", pGeneralSettings.sExternalTimebase.nFreqMultiplier))
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Divisor", pGeneralSettings.sExternalTimebase.nFreqDivisor))
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Frequency_Tolerance", pGeneralSettings.sExternalTimebase.nFreqTolerance))
        {
            return false;
        }

        if (!ReadElementFloat(*extTimeBaseElem, "Nominal_Frequency", pGeneralSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (ReadElementStringAllowEmpty(*extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
            {
                if (ToLower(nominalFrequency) == "none")
                {
                    pGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
                }
            }else
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

        if (!ReadElementFloat(*extTimeBaseElem, "Nominal_Frequency", pGeneralSettings.sExternalTimebase.fNominalFrequency))
        {
            std::string nominalFrequency;
            if (ReadElementStringAllowEmpty(*extTimeBaseElem, "Nominal_Frequency", nominalFrequency))
            {
                if (ToLower(nominalFrequency) == "none")
                {
                    pGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
                }
            }else
            {
                return false;
            }
        }

        if (!ReadElementUnsignedInt32(*extTimeBaseElem, "Signal_Shutter_Delay", pGeneralSettings.sExternalTimebase.nSignalShutterDelay))
        {
            return false;
        }

        if (!ReadElementFloat(*extTimeBaseElem, "Non_Periodic_Timeout", pGeneralSettings.sExternalTimebase.fNonPeriodicTimeout))
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
        if (ReadElementStringAllowEmpty(*externalTimestampElem, "Type", type))
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

        ReadElementUnsignedInt32(*externalTimestampElem, "Frequency", pGeneralSettings.sTimestamp.nFrequency);
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

    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================

        auto processingElem = generalElem->FirstChildElement(processings[i]);
        if (!processingElem)
        {
            return false;
        }

        *processingActions[i] = ProcessingNone;

        if (mnMajorVersion > 1 || mnMinorVersion > 13)
        {
            if(!AddFlagFromBoolElement(*processingElem, "PreProcessing2D", ProcessingPreProcess2D, *processingActions[i]))
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

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
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

            if (mnMajorVersion > 1 || mnMinorVersion > 11)
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
        if (!ReadElementUnsignedInt32(*cameraElem, "ID", sCameraSettings.nID))
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

        if (!ReadElementUnsignedInt32(*cameraElem, "Serial", sCameraSettings.nSerial))
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

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            if (!ReadElementUnsignedInt32(*cameraElem, "Video_Frequency", sCameraSettings.nVideoFrequency))
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
        if (ReadElementStringAllowEmpty(*cameraElem, "Video_Aspect_Ratio", videoAspectRatio))
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
            if (!ReadElementUnsignedInt32(*videoExposureElem, "Current", sCameraSettings.nVideoExposure)
            || !ReadElementUnsignedInt32(*videoExposureElem, "Min", sCameraSettings.nVideoExposureMin)
            || !ReadElementUnsignedInt32(*videoExposureElem, "Max", sCameraSettings.nVideoExposureMax))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        auto videoFlashTime = cameraElem->FirstChildElement("Video_Flash_Time");
        if(videoFlashTime)
        {
            if (!ReadElementUnsignedInt32(*videoFlashTime, "Current", sCameraSettings.nVideoFlashTime)
                || !ReadElementUnsignedInt32(*videoFlashTime, "Min", sCameraSettings.nVideoFlashTimeMin)
                || !ReadElementUnsignedInt32(*videoFlashTime, "Max", sCameraSettings.nVideoFlashTimeMax))
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
            if (!ReadElementUnsignedInt32(*markerExposureElem, "Current", sCameraSettings.nMarkerExposure)
                || !ReadElementUnsignedInt32(*markerExposureElem, "Min", sCameraSettings.nMarkerExposureMin)
                || !ReadElementUnsignedInt32(*markerExposureElem, "Max", sCameraSettings.nMarkerExposureMax))
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
            if (!ReadElementUnsignedInt32(*markerThresholdElem, "Current", sCameraSettings.nMarkerThreshold)
                || !ReadElementUnsignedInt32(*markerThresholdElem, "Min", sCameraSettings.nMarkerThresholdMin)
                || !ReadElementUnsignedInt32(*markerThresholdElem, "Max", sCameraSettings.nMarkerThresholdMax))
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
            if (!ReadElementFloat(*positionElem, "X", sCameraSettings.fPositionX)
            || !ReadElementFloat(*positionElem, "Y", sCameraSettings.fPositionY)
            || !ReadElementFloat(*positionElem, "Z", sCameraSettings.fPositionZ)
            || !ReadElementFloat(*positionElem, "Rot_1_1", sCameraSettings.fPositionRotMatrix[0][0])
            || !ReadElementFloat(*positionElem, "Rot_2_1", sCameraSettings.fPositionRotMatrix[1][0])
            || !ReadElementFloat(*positionElem, "Rot_3_1", sCameraSettings.fPositionRotMatrix[2][0])
            || !ReadElementFloat(*positionElem, "Rot_1_2", sCameraSettings.fPositionRotMatrix[0][1])
            || !ReadElementFloat(*positionElem, "Rot_2_2", sCameraSettings.fPositionRotMatrix[1][1])
            || !ReadElementFloat(*positionElem, "Rot_3_2", sCameraSettings.fPositionRotMatrix[2][1])
            || !ReadElementFloat(*positionElem, "Rot_1_3", sCameraSettings.fPositionRotMatrix[0][2])
            || !ReadElementFloat(*positionElem, "Rot_2_3", sCameraSettings.fPositionRotMatrix[1][2])
            || !ReadElementFloat(*positionElem, "Rot_3_3", sCameraSettings.fPositionRotMatrix[2][2])
            )
            {
                return false;
            }
        }else
        {
            return false;
        }

        if (!ReadElementUnsignedInt32(*cameraElem, "Orientation", sCameraSettings.nOrientation))
        {
            return false;
        }

        auto markerResElem = cameraElem->FirstChildElement("Marker_Res");
        if (markerResElem)
        {
            if (!ReadElementUnsignedInt32(*markerResElem, "Width", sCameraSettings.nMarkerResolutionWidth)
            || !ReadElementUnsignedInt32(*markerResElem, "Height", sCameraSettings.nMarkerResolutionHeight)
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
            if (!ReadElementUnsignedInt32(*videoResElem, "Width", sCameraSettings.nVideoResolutionWidth)
                || !ReadElementUnsignedInt32(*videoResElem, "Height", sCameraSettings.nVideoResolutionHeight)
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
            if (!ReadElementUnsignedInt32(*markerFovElem, "Left", sCameraSettings.nMarkerFOVLeft)
                || !ReadElementUnsignedInt32(*markerFovElem, "Top", sCameraSettings.nMarkerFOVTop)
                || !ReadElementUnsignedInt32(*markerFovElem, "Right", sCameraSettings.nMarkerFOVRight)
                || !ReadElementUnsignedInt32(*markerFovElem, "Bottom", sCameraSettings.nMarkerFOVBottom)
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
            if (!ReadElementUnsignedInt32(*videoFovElem, "Left", sCameraSettings.nVideoFOVLeft)
                || !ReadElementUnsignedInt32(*videoFovElem, "Top", sCameraSettings.nVideoFOVTop)
                || !ReadElementUnsignedInt32(*videoFovElem, "Right", sCameraSettings.nVideoFOVRight)
                || !ReadElementUnsignedInt32(*videoFovElem, "Bottom", sCameraSettings.nVideoFOVBottom)
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
        for (std:: size_t port = 0; port < 3; port++)
        {
            char syncOutStr[16];
            sprintf(syncOutStr, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));
            auto syncOutElem = cameraElem->FirstChildElement(syncOutStr);
            if(syncOutElem)
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
                        if (!ReadElementUnsignedInt32(*syncOutElem, "Value", sCameraSettings.nSyncOutValue[port])
                            || !ReadElementFloat(*syncOutElem, "Duty_Cycle", sCameraSettings.fSyncOutDutyCycle[port]))
                        {
                            return false;
                        }
                    }
                }

                if (port == 2 || (sCameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    std::string signalPolarity;
                    if (ReadElementStringAllowEmpty(*syncOutElem, "Signal_Polarity", signalPolarity))
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
            sCameraSettings.autoExposureEnabled  = autoExposureElem->BoolAttribute("Enabled",false);
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
            strcpy_s(p3dSettings.pCalibrationTime,32, charPtr);
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
            if (auto text = typeElem->GetText())
            {
                label.type = text;
            }
        }

        p3dSettings.s3DLabels.push_back(label);
    }

    if (p3dSettings.s3DLabels.size() != labelCount)
    {
        return false;
    }

    if(auto bonesElem = t3dElem->FirstChildElement("Bones"))
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
        if(auto elem = oXML.FirstChildElement("Name"))
        {
            if(auto text = elem->GetText())
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
                sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
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

            if (!TryReadFloatElement(*pointElem, "X", sPoint.fX))
            {
                return false;
            }

            if (!TryReadFloatElement(*pointElem, "Y", sPoint.fY))
            {
                return false;
            }

            if (!TryReadFloatElement(*pointElem, "Z", sPoint.fZ))
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

    if (mnMajorVersion > 1 || mnMinorVersion > 20)
    {
        for (auto bodyElem = sixdofElem->FirstChildElement("Body"); bodyElem != nullptr; bodyElem = bodyElem->NextSiblingElement("Body"))
        {
            SSettings6DOFBody s6DOFBodySettings;
            
            if (!TryReadSetName(*bodyElem, s6DOFBodySettings.name))
            { // Name --- REQUIRED
                return false;
            }

            TryReadSetEnabled(mnMajorVersion, mnMinorVersion, *bodyElem, s6DOFBodySettings.enabled);
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
                    || !TryReadSetPos(*meshElem, s6DOFBodySettings.mesh.position.fX, s6DOFBodySettings.mesh.position.fY, s6DOFBodySettings.mesh.position.fZ)
                    || !TryReadSetRotation(*meshElem, s6DOFBodySettings.mesh.rotation.fX, s6DOFBodySettings.mesh.rotation.fY, s6DOFBodySettings.mesh.rotation.fZ)
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
            SSettings6DOFBody s6DOFBodySettings {};

            // Name, RGBColor, Points(OLD) --- REQUIRED
            if (!TryReadSetName(*bodyElem, s6DOFBodySettings.name)
                || !TryReadSetRGBColor(*bodyElem, s6DOFBodySettings.color)
                || !TryReadSetPointsOld(*bodyElem, s6DOFBodySettings.points))
            { 
                return false;
            }

            if (mnMajorVersion > 1 || mnMinorVersion > 15)
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

    if (mnMajorVersion == 1 && mnMinorVersion == 0)
    {
        SAnalogDevice analogDevice{};
        analogDevice.nDeviceID = 1;   // Always channel 1
        analogDevice.oName = "AnalogDevice";
        if (!ReadElementUnsignedInt32(*analogElem, "Channels", analogDevice.nChannels) 
         || !ReadElementUnsignedInt32(*analogElem, "Frequency", analogDevice.nFrequency)
         || !ReadElementStringAllowEmpty(*analogElem, "Unit", analogDevice.oUnit))
        {
            return false;
        }

        auto rangeElem = analogElem->FirstChildElement("Range");
        if (!rangeElem
        || !ReadElementFloat(*rangeElem, "Min", analogDevice.fMinRange)
        || !ReadElementFloat(*rangeElem, "Max", analogDevice.fMaxRange))
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
        if (!ReadElementUnsignedInt32(*deviceElem, "Device_ID", analogDevice.nDeviceID)
            || !ReadElementStringAllowEmpty(*deviceElem, "Device_Name", analogDevice.oName)
            || !ReadElementUnsignedInt32(*deviceElem, "Channels", analogDevice.nChannels)
            || !ReadElementUnsignedInt32(*deviceElem, "Frequency", analogDevice.nFrequency)
        )
        {
            continue;
        }

        if (mnMajorVersion == 1 && mnMinorVersion < 11)
        {
            if (!ReadElementStringAllowEmpty(*analogElem, "Unit", analogDevice.oUnit))
            {
                continue;
            }
        }

        auto rangeElem = deviceElem->FirstChildElement("Range");
        if (!rangeElem
            || !ReadElementFloat(*rangeElem, "Min", analogDevice.fMinRange)
            || !ReadElementFloat(*rangeElem, "Max", analogDevice.fMaxRange))
        {
            continue;
        }

        if (mnMajorVersion == 1 && mnMinorVersion < 11)
        {
            for (std::size_t i = 0; i < analogDevice.nChannels; i++)
            {
                std::string label;
                if (ReadElementStringAllowEmpty(*deviceElem, "Label", label))
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
                if (ReadElementStringAllowEmpty(*channelElem, "Label", label))
                {
                    analogDevice.voLabels.push_back(label);
                }

                std::string unit;
                if (ReadElementStringAllowEmpty(*channelElem, "Unit", unit))
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
    private:
        static constexpr std::size_t buffSize = 128;

        tinyxml2::XMLElement& parent;
        std::function<const char*(char (&buff)[buffSize], std::size_t index)> elementNameGenerator;

    public:
        ChildElementRange() = delete;

        ChildElementRange(tinyxml2::XMLElement& parent, const char* elementName)
            : parent(parent), elementNameGenerator([elementName](auto& buff, std::size_t) { return elementName; })
        {
        }

        ChildElementRange(tinyxml2::XMLElement& parent,
                          std::function<const char*(char (&buff)[buffSize], std::size_t index)> generator)
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
                current = range.parent.FirstChildElement(range.elementNameGenerator(buffer, index++));
            }

            tinyxml2::XMLElement* operator*() const
            {
                return current;
            }

            Iterator& operator++()
            {
                current = current->NextSiblingElement(range.elementNameGenerator(buffer, index++));
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
    sForcePlate.bValidCalibrationMatrix = false;
    sForcePlate.nCalibrationMatrixRows = 6;
    sForcePlate.nCalibrationMatrixColumns = 6;

    if (!TryReadTextElement(*forceElem, "Unit_Length", pForceSettings.oUnitLength))
    {
        return false;
    }

    if (!TryReadTextElement(*forceElem, "Unit_Force", pForceSettings.oUnitForce))
    {
        return false;
    }

    std::size_t iPlate = 0;
    for (auto plateElem : ChildElementRange{*forceElem, "Plate"})
    {
        iPlate++;

        if (!ReadElementUnsignedInt32(*plateElem, "Plate_ID", sForcePlate.nID))
        {
            if (!ReadElementUnsignedInt32(*plateElem, "Force_Plate_Index", sForcePlate.nID)) // Version 1.7 and earlier.
            {
                return false;
            }
        }

        if (!ReadElementUnsignedInt32(*plateElem, "Analog_Device_ID", sForcePlate.nAnalogDeviceID))
        {
            sForcePlate.nAnalogDeviceID = 0;
        }

        if (!ReadElementUnsignedInt32(*plateElem, "Frequency", sForcePlate.nFrequency))
        {
            return false;
        }

        if (!TryReadTextElement(*plateElem, "Type", sForcePlate.oType))
        {
            sForcePlate.oType = "unknown";
        }

        if (!TryReadTextElement(*plateElem, "Name", sForcePlate.oName))
        {
            sForcePlate.oName = "#" + std::to_string(iPlate);
        }

        TryReadFloatElement(*plateElem, "Length", sForcePlate.fLength);
        TryReadFloatElement(*plateElem, "Width", sForcePlate.fWidth);

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
                TryReadFloatElement(*cornerElem, "X", sForcePlate.asCorner[c.index].fX);
                TryReadFloatElement(*cornerElem, "Y", sForcePlate.asCorner[c.index].fY);
                TryReadFloatElement(*cornerElem, "Z", sForcePlate.asCorner[c.index].fZ);
            }
        }

        auto originElem = plateElem->FirstChildElement("Origin");
        if (originElem)
        {
            TryReadFloatElement(*originElem, "X", sForcePlate.sOrigin.fX);
            TryReadFloatElement(*originElem, "Y", sForcePlate.sOrigin.fY);
            TryReadFloatElement(*originElem, "Z", sForcePlate.sOrigin.fZ);
        }

        sForcePlate.vChannels.clear();
        auto channelsElem = plateElem->FirstChildElement("Channels");
        if (channelsElem)
        {
            SForceChannel sForceChannel{};
            for (auto channelElem : ChildElementRange{*channelsElem, "Channel"})
            {
                ReadElementUnsignedInt32(*channelElem, "Channel_No", sForceChannel.nChannelNumber);
                ReadElementFloat(*channelElem, "ConversionFactor", sForceChannel.fConversionFactor);
                sForcePlate.vChannels.push_back(sForceChannel);
            }
        }

        auto calibrationMatrix = plateElem->FirstChildElement("Calibration_Matrix");
        if (calibrationMatrix)
        {
            if (mnMajorVersion == 1 && mnMinorVersion < 12)
            {
                auto getRowStr = [](auto& buff, std::size_t index)-> const char* {
                    sprintf(buff, "Row%zd", index + 1);
                    return buff;
                };

                auto getColStr = [](auto& buff, std::size_t index)-> const char* {
                    sprintf(buff, "Col%zd", index + 1);
                    return buff;
                };

                unsigned int nRow = 0;
                for (const auto row : ChildElementRange{*calibrationMatrix, getRowStr})
                {
                    unsigned int nCol = 0;
                    for (const auto col : ChildElementRange{*row, getColStr})
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
                    for (auto rowElement : ChildElementRange{*rows, "Row"})
                    {
                        auto columns = rowElement->FirstChildElement("Columns");
                        if (columns)
                        {
                            unsigned int nCol = 0;
                            for (const auto col : ChildElementRange{*columns, "Column"})
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

    for (auto camera : ChildElementRange{*imageElem, "Camera"})
    {
        SImageCamera sImageCamera{};

        if (!ReadElementUnsignedInt32(*camera, "ID", sImageCamera.nID))
        {
            return false;
        }

        if (!ReadXmlBool(camera, "Enabled", sImageCamera.bEnabled))
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

        if (!ReadElementUnsignedInt32(*camera, "Width", sImageCamera.nWidth)
            || !ReadElementUnsignedInt32(*camera, "Height", sImageCamera.nHeight))
        {
            return false;
        }

        if (!ReadElementFloat(*camera, "Left_Crop", sImageCamera.fCropLeft)
            || !ReadElementFloat(*camera, "Top_Crop", sImageCamera.fCropTop)
            || !ReadElementFloat(*camera, "Right_Crop", sImageCamera.fCropRight)
            || !ReadElementFloat(*camera, "Bottom_Crop", sImageCamera.fCropBottom))
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
            for (auto couplingElem : ChildElementRange{*couplingsElem, "Coupling"})
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

    if (mnMajorVersion > 1 || mnMinorVersion > 20)
    {
        for (auto skeletonElem : ChildElementRange{*skeletonsElem, "Skeleton"})
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
                        for (auto markerElem : ChildElementRange{*markersElem, "Marker"})
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
                        for (auto rigidBodyElem : ChildElementRange{*rigidBodiesElem, "RigidBody"})
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

                    for (auto childSegmentElem : ChildElementRange{segmentElem, "Segment"})
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

    for (auto skeletonElem : ChildElementRange{*skeletonsElem, "Skeleton"})
    {
        SSettingsSkeleton skeleton{};
        segmentIndex = 0;
        skeleton.name = skeletonElem->Attribute("Name");
        for (auto segmentElem : ChildElementRange{*skeletonElem, "Segment"})
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
        sprintf(maErrorStr, "Missing calibration element");
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

        for (auto cameraElem : ChildElementRange{*camerasElem, "camera"})
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

CTinyxml2Serializer::CTinyxml2Serializer(std::uint32_t pMajorVersion, std::uint32_t pMinorVersion)
    : mnMajorVersion(pMajorVersion), mnMinorVersion(pMinorVersion)
{
}

std::string CTinyxml2Serializer::SetGeneralSettings(const unsigned int* pnCaptureFrequency,
    const float* pfCaptureTime, const bool* pbStartOnExtTrig,
    const bool* pStartOnTrigNO, const bool* pStartOnTrigNC,
    const bool* pStartOnTrigSoftware, const EProcessingActions* peProcessingActions,
    const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Capture Frequency
    if (pnCaptureFrequency)
    {
        AddXMLElementUnsignedInt(*pGeneral, "Frequency", pnCaptureFrequency, oXML);
    }

    // Capture Time
    if (pfCaptureTime)
    {
        AddXMLElementFloat(*pGeneral, "Capture_Time", pfCaptureTime, 3, oXML);
    }

    // External Trigger and additional triggers
    if (pbStartOnExtTrig)
    {
        AddXMLElementBool(*pGeneral, "Start_On_External_Trigger", pbStartOnExtTrig, oXML);

        if (mnMajorVersion > 1 || mnMinorVersion > 14)
        {
            AddXMLElementBool(*pGeneral, "Start_On_Trigger_NO", pStartOnTrigNO, oXML);
            AddXMLElementBool(*pGeneral, "Start_On_Trigger_NC", pStartOnTrigNC, oXML);
            AddXMLElementBool(*pGeneral, "Start_On_Trigger_Software", pStartOnTrigSoftware, oXML);
        }
    }

    // Processing Actions
    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActions[3] = { peProcessingActions, peRtProcessingActions, peReprocessingActions };

    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActions[i])
        {
            tinyxml2::XMLElement* pProcessing = oXML.NewElement(processings[i]);
            pGeneral->InsertEndChild(pProcessing);

            if (mnMajorVersion > 1 || mnMinorVersion > 13)
            {
                AddXMLElementBool(*pProcessing, "PreProcessing2D", (*processingActions[i] & ProcessingPreProcess2D) != 0, oXML);
            }

            if (*processingActions[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                tinyxml2::XMLElement* pTracking = oXML.NewElement("Tracking");
                pTracking->SetText("2D");
                pProcessing->InsertEndChild(pTracking);
            }
            else if (*processingActions[i] & ProcessingTracking3D)
            {
                tinyxml2::XMLElement* pTracking = oXML.NewElement("Tracking");
                pTracking->SetText("3D");
                pProcessing->InsertEndChild(pTracking);
            }
            else
            {
                tinyxml2::XMLElement* pTracking = oXML.NewElement("Tracking");
                pTracking->SetText("False");
                pProcessing->InsertEndChild(pTracking);
            }

            if (i != 1) // Not RtProcessingSettings
            {
                AddXMLElementBool(*pProcessing, "TwinSystemMerge", (*processingActions[i] & ProcessingTwinSystemMerge) != 0, oXML);
                AddXMLElementBool(*pProcessing, "SplineFill", (*processingActions[i] & ProcessingSplineFill) != 0, oXML);
            }

            AddXMLElementBool(*pProcessing, "AIM", (*processingActions[i] & ProcessingAIM) != 0, oXML);
            AddXMLElementBool(*pProcessing, "Track6DOF", (*processingActions[i] & Processing6DOFTracking) != 0, oXML);
            AddXMLElementBool(*pProcessing, "ForceData", (*processingActions[i] & ProcessingForceData) != 0, oXML);
            AddXMLElementBool(*pProcessing, "GazeVector", (*processingActions[i] & ProcessingGazeVector) != 0, oXML);

            if (i != 1) // Not RtProcessingSettings
            {
                AddXMLElementBool(*pProcessing, "ExportTSV", (*processingActions[i] & ProcessingExportTSV) != 0, oXML);
                AddXMLElementBool(*pProcessing, "ExportC3D", (*processingActions[i] & ProcessingExportC3D) != 0, oXML);
                AddXMLElementBool(*pProcessing, "ExportMatlabFile", (*processingActions[i] & ProcessingExportMatlabFile) != 0, oXML);
                AddXMLElementBool(*pProcessing, "ExportAviFile", (*processingActions[i] & ProcessingExportAviFile) != 0, oXML);
            }
        }
    }

    // Convert to string
    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetExtTimeBaseSettings(const bool* pbEnabled, const ESignalSource* peSignalSource,
    const bool* pbSignalModePeriodic, const unsigned int* pnFreqMultiplier, const unsigned int* pnFreqDivisor,
    const unsigned int* pnFreqTolerance, const float* pfNominalFrequency, const bool* pbNegativeEdge,
    const unsigned int* pnSignalShutterDelay, const float* pfNonPeriodicTimeout)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // External Time Base element
    tinyxml2::XMLElement* pTimeBase = oXML.NewElement("External_Time_Base");
    pGeneral->InsertEndChild(pTimeBase);

    // Add Enabled element
    AddXMLElementBool(*pTimeBase, "Enabled", pbEnabled, oXML);

    // Add Signal Source if available
    if (peSignalSource)
    {
        tinyxml2::XMLElement* pSignalSource = oXML.NewElement("Signal_Source");
        switch (*peSignalSource)
        {
        case SourceControlPort:
            pSignalSource->SetText("Control port");
            break;
        case SourceIRReceiver:
            pSignalSource->SetText("IR receiver");
            break;
        case SourceSMPTE:
            pSignalSource->SetText("SMPTE");
            break;
        case SourceVideoSync:
            pSignalSource->SetText("Video sync");
            break;
        case SourceIRIG:
            pSignalSource->SetText("IRIG");
            break;
        }
        pTimeBase->InsertEndChild(pSignalSource);
    }

    // Add remaining elements
    AddXMLElementBool(*pTimeBase, "Signal_Mode", pbSignalModePeriodic, oXML, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(*pTimeBase, "Frequency_Multiplier", pnFreqMultiplier, oXML);
    AddXMLElementUnsignedInt(*pTimeBase, "Frequency_Divisor", pnFreqDivisor, oXML);
    AddXMLElementUnsignedInt(*pTimeBase, "Frequency_Tolerance", pnFreqTolerance, oXML);

    // Add Nominal Frequency element
    if (pfNominalFrequency)
    {
        if (*pfNominalFrequency < 0)
        {
            tinyxml2::XMLElement* pNominalFreq = oXML.NewElement("Nominal_Frequency");
            pNominalFreq->SetText("None");
            pTimeBase->InsertEndChild(pNominalFreq);
        }
        else
        {
            AddXMLElementFloat(*pTimeBase, "Nominal_Frequency", pfNominalFrequency, 3, oXML);
        }
    }

    AddXMLElementBool(*pTimeBase, "Signal_Edge", pbNegativeEdge, oXML, "Negative", "Positive");
    AddXMLElementUnsignedInt(*pTimeBase, "Signal_Shutter_Delay", pnSignalShutterDelay, oXML);
    AddXMLElementFloat(*pTimeBase, "Non_Periodic_Timeout", pfNonPeriodicTimeout, 3, oXML);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // External Timestamp element
    tinyxml2::XMLElement* pTimestamp = oXML.NewElement("External_Timestamp");
    pGeneral->InsertEndChild(pTimestamp);

    // Add Enabled element
    AddXMLElementBool(*pTimestamp, "Enabled", timestampSettings.bEnabled, oXML);

    // Add Type element
    tinyxml2::XMLElement* pType = oXML.NewElement("Type");
    switch (timestampSettings.nType)
    {
    case ETimestampType::Timestamp_SMPTE:
        pType->SetText("SMPTE");
        break;
    case ETimestampType::Timestamp_IRIG:
        pType->SetText("IRIG");
        break;
    case ETimestampType::Timestamp_CameraTime:
        pType->SetText("CameraTime");
        break;
    default:
        break;
    }
    pTimestamp->InsertEndChild(pType);

    // Add Frequency element
    AddXMLElementUnsignedInt(*pTimestamp, "Frequency", timestampSettings.nFrequency, oXML);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetCameraSettings(
    const unsigned int pCameraId, const ECameraMode* peMode,
    const float* pfMarkerExposure, const float* pfMarkerThreshold,
    const int* pnOrientation)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pGeneral->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, oXML);

    // Add Mode
    if (peMode)
    {
        tinyxml2::XMLElement* pMode = oXML.NewElement("Mode");
        switch (*peMode)
        {
        case ModeMarker:
            pMode->SetText("Marker");
            break;
        case ModeMarkerIntensity:
            pMode->SetText("Marker Intensity");
            break;
        case ModeVideo:
            pMode->SetText("Video");
            break;
        }
        pCamera->InsertEndChild(pMode);
    }

    // Add remaining elements
    AddXMLElementFloat(*pCamera, "Marker_Exposure", pfMarkerExposure, 6, oXML);
    AddXMLElementFloat(*pCamera, "Marker_Threshold", pfMarkerThreshold, 6, oXML);
    AddXMLElementInt(*pCamera, "Orientation", pnOrientation, oXML);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraVideoSettings(const unsigned int pCameraId,
    const EVideoResolution* eVideoResolution, const EVideoAspectRatio* eVideoAspectRatio,
    const unsigned int* pnVideoFrequency, const float* pfVideoExposure, const float* pfVideoFlashTime)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pGeneral->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, oXML);

    // Add Video Resolution
    if (eVideoResolution)
    {
        tinyxml2::XMLElement* pResolution = oXML.NewElement("Video_Resolution");
        switch (*eVideoResolution)
        {
        case VideoResolution1440p:
            pResolution->SetText("1440p");
            break;
        case VideoResolution1080p:
            pResolution->SetText("1080p");
            break;
        case VideoResolution720p:
            pResolution->SetText("720p");
            break;
        case VideoResolution540p:
            pResolution->SetText("540p");
            break;
        case VideoResolution480p:
            pResolution->SetText("480p");
            break;
        case VideoResolutionNone:
            break;
        }
        pCamera->InsertEndChild(pResolution);
    }

    // Add Video Aspect Ratio
    if (eVideoAspectRatio)
    {
        tinyxml2::XMLElement* pAspectRatio = oXML.NewElement("Video_Aspect_Ratio");
        switch (*eVideoAspectRatio)
        {
        case VideoAspectRatio16x9:
            pAspectRatio->SetText("16x9");
            break;
        case VideoAspectRatio4x3:
            pAspectRatio->SetText("4x3");
            break;
        case VideoAspectRatio1x1:
            pAspectRatio->SetText("1x1");
            break;
        case VideoAspectRatioNone:
            break;
        }
        pCamera->InsertEndChild(pAspectRatio);
    }

    // Add remaining elements
    AddXMLElementUnsignedInt(*pCamera, "Video_Frequency", pnVideoFrequency, oXML);
    AddXMLElementFloat(*pCamera, "Video_Exposure", pfVideoExposure, 6, oXML);
    AddXMLElementFloat(*pCamera, "Video_Flash_Time", pfVideoFlashTime, 6, oXML);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraSyncOutSettings(const unsigned int pCameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* peSyncOutMode, const unsigned int* pnSyncOutValue, const float* pfSyncOutDutyCycle,
    const bool* pbSyncOutNegativePolarity)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pGeneral->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, oXML);

    // Determine port name
    int port = portNumber - 1;
    if (((port == 0 || port == 1) && peSyncOutMode) || (port == 2))
    {
        tinyxml2::XMLElement* pSyncOut = nullptr;
        if (port == 0)
            pSyncOut = oXML.NewElement("Sync_Out");
        else if (port == 1)
            pSyncOut = oXML.NewElement("Sync_Out2");
        else
            pSyncOut = oXML.NewElement("Sync_Out_MT");

        pCamera->InsertEndChild(pSyncOut);

        // Add Sync Out Mode
        if (port == 0 || port == 1)
        {
            tinyxml2::XMLElement* pMode = oXML.NewElement("Mode");
            switch (*peSyncOutMode)
            {
            case ModeShutterOut:
                pMode->SetText("Shutter out");
                break;
            case ModeMultiplier:
                pMode->SetText("Multiplier");
                break;
            case ModeDivisor:
                pMode->SetText("Divisor");
                break;
            case ModeIndependentFreq:
                pMode->SetText("Camera independent");
                break;
            case ModeMeasurementTime:
                pMode->SetText("Measurement time");
                break;
            case ModeFixed100Hz:
                pMode->SetText("Continuous 100Hz");
                break;
            case ModeSystemLiveTime:
                pMode->SetText("System live time");
                break;
            default:
                return ""; // Should never happen
            }
            pSyncOut->InsertEndChild(pMode);

            // Add Value and Duty Cycle if applicable
            if (*peSyncOutMode == ModeMultiplier ||
                *peSyncOutMode == ModeDivisor ||
                *peSyncOutMode == ModeIndependentFreq)
            {
                AddXMLElementUnsignedInt(*pSyncOut, "Value", pnSyncOutValue, oXML);
                AddXMLElementFloat(*pSyncOut, "Duty_Cycle", pfSyncOutDutyCycle, 3, oXML);
            }
        }

        // Add Signal Polarity
        if (pbSyncOutNegativePolarity && (port == 2 ||
            (peSyncOutMode && *peSyncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(*pSyncOut, "Signal_Polarity", pbSyncOutNegativePolarity, oXML, "Negative", "Positive");
        }
    }

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus,
    const float pAperture)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pGeneral->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, oXML);

    // LensControl element
    tinyxml2::XMLElement* pLensControl = oXML.NewElement("LensControl");
    pCamera->InsertEndChild(pLensControl);

    // Add Focus and Aperture as float attributes
    AddXMLElementFloatWithTextAttribute(oXML, *pLensControl, "Focus", "Value", pFocus, 6);
    AddXMLElementFloatWithTextAttribute(oXML, *pLensControl, "Aperture", "Value", pAperture, 6);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure,
    const float pCompensation)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pGeneral->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, oXML);

    // LensControl element
    tinyxml2::XMLElement* pLensControl = oXML.NewElement("LensControl");
    pCamera->InsertEndChild(pLensControl);

    // AutoExposure element with attributes
    tinyxml2::XMLElement* pAutoExposureElem = oXML.NewElement("AutoExposure");
    pAutoExposureElem->SetAttribute("Enabled", pAutoExposure ? "true" : "false");

    // Format Compensation float value
    char compensationStr[32];
    snprintf(compensationStr, sizeof(compensationStr), "%.6f", pCompensation);
    pAutoExposureElem->SetAttribute("Compensation", compensationStr);

    pLensControl->InsertEndChild(pAutoExposureElem);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // General element
    tinyxml2::XMLElement* pGeneral = oXML.NewElement("General");
    pRoot->InsertEndChild(pGeneral);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pGeneral->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, oXML);

    // AutoWhiteBalance element
    tinyxml2::XMLElement* pAWB = oXML.NewElement("AutoWhiteBalance");
    pAWB->SetText(pEnable ? "true" : "false");
    pCamera->InsertEndChild(pAWB);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetImageSettings(const unsigned int pCameraId, const bool* pbEnable,
    const CRTPacket::EImageFormat* peFormat, const unsigned int* pnWidth, const unsigned int* pnHeight,
    const float* pfLeftCrop, const float* pfTopCrop, const float* pfRightCrop, const float* pfBottomCrop)
{
    tinyxml2::XMLDocument oXML;

    // Root element
    tinyxml2::XMLElement* pRoot = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(pRoot);

    // Image element
    tinyxml2::XMLElement* pImage = oXML.NewElement("Image");
    pRoot->InsertEndChild(pImage);

    // Camera element
    tinyxml2::XMLElement* pCamera = oXML.NewElement("Camera");
    pImage->InsertEndChild(pCamera);

    // ID
    AddXMLElementUnsignedInt(*pCamera, "ID", pCameraId, oXML);

    // Enabled
    AddXMLElementBool(*pCamera, "Enabled", pbEnable, oXML);

    // Format
    if (peFormat)
    {
        const char* formatStr = nullptr;
        switch (*peFormat)
        {
        case CRTPacket::FormatRawGrayscale:
            formatStr = "RAWGrayscale";
            break;
        case CRTPacket::FormatRawBGR:
            formatStr = "RAWBGR";
            break;
        case CRTPacket::FormatJPG:
            formatStr = "JPG";
            break;
        case CRTPacket::FormatPNG:
            formatStr = "PNG";
            break;
        }

        if (formatStr)
        {
            tinyxml2::XMLElement* pFormat = oXML.NewElement("Format");
            pFormat->SetText(formatStr);
            pCamera->InsertEndChild(pFormat);
        }
    }

    // Other settings
    AddXMLElementUnsignedInt(*pCamera, "Width", pnWidth, oXML);
    AddXMLElementUnsignedInt(*pCamera, "Height", pnHeight, oXML);
    AddXMLElementFloat(*pCamera, "Left_Crop", pfLeftCrop, 6, oXML);
    AddXMLElementFloat(*pCamera, "Top_Crop", pfTopCrop, 6, oXML);
    AddXMLElementFloat(*pCamera, "Right_Crop", pfRightCrop, 6, oXML);
    AddXMLElementFloat(*pCamera, "Bottom_Crop", pfBottomCrop, 6, oXML);

    // Convert to string
    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);

    return printer.CStr();
}

std::string CTinyxml2Serializer::SetForceSettings(const unsigned int pPlateId, const SPoint* pCorner1,
    const SPoint* pCorner2, const SPoint* pCorner3, const SPoint* pCorner4)
{
    tinyxml2::XMLDocument oXML;
    tinyxml2::XMLElement* root = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(root);

    tinyxml2::XMLElement* forceElem = oXML.NewElement("Force");
    root->InsertEndChild(forceElem);

    tinyxml2::XMLElement* plateElem = oXML.NewElement("Plate");
    forceElem->InsertEndChild(plateElem);

    if (mnMajorVersion > 1 || mnMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(*plateElem, "Plate_ID", &pPlateId, oXML);
    }
    else
    {
        AddXMLElementUnsignedInt(*plateElem, "Force_Plate_Index", &pPlateId, oXML);
    }

    auto addCorner = [&](const char* name, const SPoint* pCorner)
        {
            if (pCorner)
            {
                tinyxml2::XMLElement* cornerElem = oXML.NewElement(name);
                plateElem->InsertEndChild(cornerElem);

                AddXMLElementFloat(*cornerElem, "X", &(pCorner->fX), 6, oXML);
                AddXMLElementFloat(*cornerElem, "Y", &(pCorner->fY), 6, oXML);
                AddXMLElementFloat(*cornerElem, "Z", &(pCorner->fZ), 6, oXML);
            }
        };

    addCorner("Corner1", pCorner1);
    addCorner("Corner2", pCorner2);
    addCorner("Corner3", pCorner3);
    addCorner("Corner4", pCorner4);

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);

    return printer.CStr();
}

std::string CTinyxml2Serializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs)
{
    tinyxml2::XMLDocument oXML;
    auto* root = oXML.NewElement("QTM_Settings");
    oXML.InsertFirstChild(root);

    auto* the6D = oXML.NewElement("The_6D");
    root->InsertEndChild(the6D);

    for (const auto& body : pSettings6Dofs)
    {
        auto* bodyElem = oXML.NewElement("Body");
        the6D->InsertEndChild(bodyElem);

        auto* nameElem = oXML.NewElement("Name");
        nameElem->SetText(body.name.c_str());
        bodyElem->InsertEndChild(nameElem);

        auto* enabledElem = oXML.NewElement("Enabled");
        enabledElem->SetText(body.enabled ? "true" : "false");
        bodyElem->InsertEndChild(enabledElem);

        auto* colorElem = oXML.NewElement("Color");
        colorElem->SetAttribute("R", body.color & 0xff);
        colorElem->SetAttribute("G", (body.color >> 8) & 0xff);
        colorElem->SetAttribute("B", (body.color >> 16) & 0xff);
        bodyElem->InsertEndChild(colorElem);

        auto* maxResidualElem = oXML.NewElement("MaximumResidual");
        maxResidualElem->SetText(std::to_string(body.maxResidual).c_str());
        bodyElem->InsertEndChild(maxResidualElem);

        auto* minMarkersElem = oXML.NewElement("MinimumMarkersInBody");
        minMarkersElem->SetText(std::to_string(body.minMarkersInBody).c_str());
        bodyElem->InsertEndChild(minMarkersElem);

        auto* boneToleranceElem = oXML.NewElement("BoneLengthTolerance");
        boneToleranceElem->SetText(std::to_string(body.boneLengthTolerance).c_str());
        bodyElem->InsertEndChild(boneToleranceElem);

        auto* filterElem = oXML.NewElement("Filter");
        filterElem->SetAttribute("Preset", body.filterPreset.c_str());
        bodyElem->InsertEndChild(filterElem);

        if (!body.mesh.name.empty())
        {
            auto* meshElem = oXML.NewElement("Mesh");
            bodyElem->InsertEndChild(meshElem);

            auto* meshNameElem = oXML.NewElement("Name");
            meshNameElem->SetText(body.mesh.name.c_str());
            meshElem->InsertEndChild(meshNameElem);

            auto* positionElem = oXML.NewElement("Position");
            positionElem->SetAttribute("X", std::to_string(body.mesh.position.fX).c_str());
            positionElem->SetAttribute("Y", std::to_string(body.mesh.position.fY).c_str());
            positionElem->SetAttribute("Z", std::to_string(body.mesh.position.fZ).c_str());
            meshElem->InsertEndChild(positionElem);

            auto* rotationElem = oXML.NewElement("Rotation");
            rotationElem->SetAttribute("X", std::to_string(body.mesh.rotation.fX).c_str());
            rotationElem->SetAttribute("Y", std::to_string(body.mesh.rotation.fY).c_str());
            rotationElem->SetAttribute("Z", std::to_string(body.mesh.rotation.fZ).c_str());
            meshElem->InsertEndChild(rotationElem);

            auto* scaleElem = oXML.NewElement("Scale");
            scaleElem->SetText(std::to_string(body.mesh.scale).c_str());
            meshElem->InsertEndChild(scaleElem);

            auto* opacityElem = oXML.NewElement("Opacity");
            opacityElem->SetText(std::to_string(body.mesh.opacity).c_str());
            meshElem->InsertEndChild(opacityElem);
        }

        if (!body.points.empty())
        {
            auto* pointsElem = oXML.NewElement("Points");
            bodyElem->InsertEndChild(pointsElem);

            for (const auto& point : body.points)
            {
                auto* pointElem = oXML.NewElement("Point");
                pointElem->SetAttribute("X", std::to_string(point.fX).c_str());
                pointElem->SetAttribute("Y", std::to_string(point.fY).c_str());
                pointElem->SetAttribute("Z", std::to_string(point.fZ).c_str());
                pointElem->SetAttribute("Virtual", point.virtual_ ? "1" : "0");
                pointElem->SetAttribute("PhysicalId", point.physicalId);
                pointElem->SetAttribute("Name", point.name.c_str());
                pointsElem->InsertEndChild(pointElem);
            }
        }

        auto* dataOriginElem = oXML.NewElement("Data_origin");
        dataOriginElem->SetText(std::to_string(body.origin.type).c_str());
        dataOriginElem->SetAttribute("X", std::to_string(body.origin.position.fX).c_str());
        dataOriginElem->SetAttribute("Y", std::to_string(body.origin.position.fY).c_str());
        dataOriginElem->SetAttribute("Z", std::to_string(body.origin.position.fZ).c_str());
        dataOriginElem->SetAttribute("Relative_body", body.origin.relativeBody);
        bodyElem->InsertEndChild(dataOriginElem);

        auto* dataOrientationElem = oXML.NewElement("Data_orientation");
        dataOrientationElem->SetText(std::to_string(body.origin.type).c_str());
        for (std::uint32_t i = 0; i < 9; i++)
        {
            char tmpStr[16];
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            dataOrientationElem->SetAttribute(tmpStr, std::to_string(body.origin.rotation[i]).c_str());
        }
        dataOrientationElem->SetAttribute("Relative_body", body.origin.relativeBody);
        bodyElem->InsertEndChild(dataOrientationElem);
    }

    tinyxml2::XMLPrinter printer;
    oXML.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& pSettingsSkeletons)
{
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLElement* root = xmlDoc.NewElement("QTM_Settings");
    xmlDoc.InsertFirstChild(root);

    tinyxml2::XMLElement* skeletonsElem = xmlDoc.NewElement("Skeletons");
    root->InsertEndChild(skeletonsElem);

    for (const auto& skeleton : pSettingsSkeletons)
    {
        tinyxml2::XMLElement* skeletonElem = xmlDoc.NewElement("Skeleton");
        skeletonElem->SetAttribute("Name", skeleton.name.c_str());
        skeletonsElem->InsertEndChild(skeletonElem);

        if (mnMajorVersion == 1 && mnMinorVersion < 22)
        {
            tinyxml2::XMLElement* solverElem = xmlDoc.NewElement("Solver");
            solverElem->SetText(skeleton.rootSegment.solver.c_str());
            skeletonElem->InsertEndChild(solverElem);
        }

        tinyxml2::XMLElement* scaleElem = xmlDoc.NewElement("Scale");
        scaleElem->SetText(std::to_string(skeleton.scale).c_str());
        skeletonElem->InsertEndChild(scaleElem);

        tinyxml2::XMLElement* segmentsElem = xmlDoc.NewElement("Segments");
        skeletonElem->InsertEndChild(segmentsElem);

        std::function<void(const SSettingsSkeletonSegmentHierarchical&, tinyxml2::XMLElement*)> recurseSegments;
        recurseSegments = [&](const SSettingsSkeletonSegmentHierarchical& segment, tinyxml2::XMLElement* parentElem)
            {
                tinyxml2::XMLElement* segmentElem = xmlDoc.NewElement("Segment");
                segmentElem->SetAttribute("Name", segment.name.c_str());
                parentElem->InsertEndChild(segmentElem);

                if (mnMajorVersion > 1 || mnMinorVersion > 21)
                {
                    tinyxml2::XMLElement* solverElem = xmlDoc.NewElement("Solver");
                    solverElem->SetText(segment.solver.c_str());
                    segmentElem->InsertEndChild(solverElem);
                }

                if (!std::isnan(segment.position.x))
                {
                    AddXMLElementTransform(xmlDoc, *segmentElem, "Transform", segment.position, segment.rotation);
                }

                if (!std::isnan(segment.defaultPosition.x))
                {
                    AddXMLElementTransform(xmlDoc, *segmentElem, "DefaultTransform", segment.defaultPosition, segment.defaultRotation);
                }

                tinyxml2::XMLElement* dofElem = xmlDoc.NewElement("DegreesOfFreedom");
                segmentElem->InsertEndChild(dofElem);
                for (const auto& dof : segment.degreesOfFreedom)
                {
                    AddXMLElementDOF(xmlDoc, *dofElem, SkeletonDofToStringSettings(dof.type), dof);
                }

                tinyxml2::XMLElement* endpointElem = xmlDoc.NewElement("Endpoint");
                if (!std::isnan(segment.endpoint.x) && !std::isnan(segment.endpoint.y) && !std::isnan(segment.endpoint.z))
                {
                    endpointElem->SetAttribute("X", std::to_string(segment.endpoint.x).c_str());
                    endpointElem->SetAttribute("Y", std::to_string(segment.endpoint.y).c_str());
                    endpointElem->SetAttribute("Z", std::to_string(segment.endpoint.z).c_str());
                }
                segmentElem->InsertEndChild(endpointElem);

                tinyxml2::XMLElement* markersElem = xmlDoc.NewElement("Markers");
                segmentElem->InsertEndChild(markersElem);
                for (const auto& marker : segment.markers)
                {
                    tinyxml2::XMLElement* markerElem = xmlDoc.NewElement("Marker");
                    markerElem->SetAttribute("Name", marker.name.c_str());
                    markersElem->InsertEndChild(markerElem);

                    tinyxml2::XMLElement* positionElem = xmlDoc.NewElement("Position");
                    positionElem->SetAttribute("X", std::to_string(marker.position.x).c_str());
                    positionElem->SetAttribute("Y", std::to_string(marker.position.y).c_str());
                    positionElem->SetAttribute("Z", std::to_string(marker.position.z).c_str());
                    markerElem->InsertEndChild(positionElem);

                    tinyxml2::XMLElement* weightElem = xmlDoc.NewElement("Weight");
                    weightElem->SetText(std::to_string(marker.weight).c_str());
                    markerElem->InsertEndChild(weightElem);
                }

                tinyxml2::XMLElement* rigidBodiesElem = xmlDoc.NewElement("RigidBodies");
                segmentElem->InsertEndChild(rigidBodiesElem);
                for (const auto& rigidBody : segment.bodies)
                {
                    tinyxml2::XMLElement* rigidBodyElem = xmlDoc.NewElement("RigidBody");
                    rigidBodyElem->SetAttribute("Name", rigidBody.name.c_str());
                    rigidBodiesElem->InsertEndChild(rigidBodyElem);

                    tinyxml2::XMLElement* transformElem = xmlDoc.NewElement("Transform");
                    rigidBodyElem->InsertEndChild(transformElem);

                    tinyxml2::XMLElement* positionElem = xmlDoc.NewElement("Position");
                    positionElem->SetAttribute("X", std::to_string(rigidBody.position.x).c_str());
                    positionElem->SetAttribute("Y", std::to_string(rigidBody.position.y).c_str());
                    positionElem->SetAttribute("Z", std::to_string(rigidBody.position.z).c_str());
                    transformElem->InsertEndChild(positionElem);

                    tinyxml2::XMLElement* rotationElem = xmlDoc.NewElement("Rotation");
                    rotationElem->SetAttribute("X", std::to_string(rigidBody.rotation.x).c_str());
                    rotationElem->SetAttribute("Y", std::to_string(rigidBody.rotation.y).c_str());
                    rotationElem->SetAttribute("Z", std::to_string(rigidBody.rotation.z).c_str());
                    rotationElem->SetAttribute("W", std::to_string(rigidBody.rotation.w).c_str());
                    transformElem->InsertEndChild(rotationElem);

                    tinyxml2::XMLElement* weightElem = xmlDoc.NewElement("Weight");
                    weightElem->SetText(std::to_string(rigidBody.weight).c_str());
                    rigidBodyElem->InsertEndChild(weightElem);
                }

                for (const auto& childSegment : segment.segments)
                {
                    recurseSegments(childSegment, segmentElem);
                }
            };

        recurseSegments(skeleton.rootSegment, segmentsElem);
    }

    tinyxml2::XMLPrinter printer;
    xmlDoc.Print(&printer);
    return printer.CStr();
}


