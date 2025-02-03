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

void CTinyxml2Serializer::AddXMLElementInt(tinyxml2::XMLDocument* oXML, const char* tTag, const int* pnValue)
{
    //if (pnValue)
    //{
    //    std::string tVal;

    //    tVal = CTinyxml2::Format("%d", *pnValue);
    //    oXML->AddElem(tTag, tVal.c_str());
    //}
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

SPosition CTinyxml2Deserializer::ReadXMLPosition(tinyxml2::XMLDocument& xml, const std::string& element)
{
    SPosition position;

    //if (xml.FindElem(element.c_str()))
    //{
    //    ParseString(xml.GetAttrib("X"), position.x);
    //    ParseString(xml.GetAttrib("Y"), position.y);
    //    ParseString(xml.GetAttrib("Z"), position.z);
    //    xml.ResetMainPos();
    //}
    return position;
}

SRotation CTinyxml2Deserializer::ReadXMLRotation(tinyxml2::XMLDocument& xml, const std::string& element)
{
    SRotation rotation;

    //if (xml.FindElem(element.c_str()))
    //{
    //    ParseString(xml.GetAttrib("X"), rotation.x);
    //    ParseString(xml.GetAttrib("Y"), rotation.y);
    //    ParseString(xml.GetAttrib("Z"), rotation.z);
    //    ParseString(xml.GetAttrib("W"), rotation.w);
    //    xml.ResetMainPos();
    //}
    return rotation;
}

CTinyxml2Deserializer::CTinyxml2Deserializer(const char* pData, std::uint32_t pMajorVersion, std::uint32_t pMinorVersion)
    : mnMajorVersion(pMajorVersion), mnMinorVersion(pMinorVersion), maErrorStr{ 0 }
{
    oXML.Parse(pData);
}

bool CTinyxml2Deserializer::DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings)
{
    //std::string             tStr;

    //pGeneralSettings.vsCameras.clear();

    //// ==================== General ====================
    //if (!oXML.FindChildElem("General"))
    //{
    //    return false;
    //}
    //oXML.IntoElem();

    //if (!oXML.FindChildElem("Frequency"))
    //{
    //    return false;
    //}
    //pGeneralSettings.nCaptureFrequency = atoi(oXML.GetChildData().c_str());

    //if (!oXML.FindChildElem("Capture_Time"))
    //{
    //    return false;
    //}
    //pGeneralSettings.fCaptureTime = (float)atof(oXML.GetChildData().c_str());

    //// Refactored variant of all this copy/paste code. TODO: Refactor everything else.
    //if (!ReadXmlBool(&oXML, "Start_On_External_Trigger", pGeneralSettings.bStartOnExternalTrigger))
    //{
    //    return false;
    //}
    //if (mnMajorVersion > 1 || mnMinorVersion > 14)
    //{
    //    if (!ReadXmlBool(&oXML, "Start_On_Trigger_NO", pGeneralSettings.bStartOnTrigNO))
    //    {
    //        return false;
    //    }
    //    if (!ReadXmlBool(&oXML, "Start_On_Trigger_NC", pGeneralSettings.bStartOnTrigNC))
    //    {
    //        return false;
    //    }
    //    if (!ReadXmlBool(&oXML, "Start_On_Trigger_Software", pGeneralSettings.bStartOnTrigSoftware))
    //    {
    //        return false;
    //    }
    //}

    //// ==================== External time base ====================
    //if (!oXML.FindChildElem("External_Time_Base"))
    //{
    //    return false;
    //}
    //oXML.IntoElem();

    //if (!oXML.FindChildElem("Enabled"))
    //{
    //    return false;
    //}
    //tStr = ToLower(oXML.GetChildData());
    //pGeneralSettings.sExternalTimebase.bEnabled = (tStr == "true");

    //if (!oXML.FindChildElem("Signal_Source"))
    //{
    //    return false;
    //}
    //tStr = ToLower(oXML.GetChildData());
    //if (tStr == "control port")
    //{
    //    pGeneralSettings.sExternalTimebase.eSignalSource = SourceControlPort;
    //}
    //else if (tStr == "ir receiver")
    //{
    //    pGeneralSettings.sExternalTimebase.eSignalSource = SourceIRReceiver;
    //}
    //else if (tStr == "smpte")
    //{
    //    pGeneralSettings.sExternalTimebase.eSignalSource = SourceSMPTE;
    //}
    //else if (tStr == "irig")
    //{
    //    pGeneralSettings.sExternalTimebase.eSignalSource = SourceIRIG;
    //}
    //else if (tStr == "video sync")
    //{
    //    pGeneralSettings.sExternalTimebase.eSignalSource = SourceVideoSync;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Signal_Mode"))
    //{
    //    return false;
    //}
    //tStr = ToLower(oXML.GetChildData());
    //if (tStr == "periodic")
    //{
    //    pGeneralSettings.sExternalTimebase.bSignalModePeriodic = true;
    //}
    //else if (tStr == "non-periodic")
    //{
    //    pGeneralSettings.sExternalTimebase.bSignalModePeriodic = false;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Frequency_Multiplier"))
    //{
    //    return false;
    //}
    //unsigned int nMultiplier;
    //tStr = oXML.GetChildData();
    //if (sscanf(tStr.c_str(), "%u", &nMultiplier) == 1)
    //{
    //    pGeneralSettings.sExternalTimebase.nFreqMultiplier = nMultiplier;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Frequency_Divisor"))
    //{
    //    return false;
    //}
    //unsigned int nDivisor;
    //tStr = oXML.GetChildData();
    //if (sscanf(tStr.c_str(), "%u", &nDivisor) == 1)
    //{
    //    pGeneralSettings.sExternalTimebase.nFreqDivisor = nDivisor;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Frequency_Tolerance"))
    //{
    //    return false;
    //}
    //unsigned int nTolerance;
    //tStr = oXML.GetChildData();
    //if (sscanf(tStr.c_str(), "%u", &nTolerance) == 1)
    //{
    //    pGeneralSettings.sExternalTimebase.nFreqTolerance = nTolerance;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Nominal_Frequency"))
    //{
    //    return false;
    //}
    //tStr = ToLower(oXML.GetChildData());

    //if (tStr == "none")
    //{
    //    pGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
    //}
    //else
    //{
    //    float fFrequency;
    //    if (sscanf(tStr.c_str(), "%f", &fFrequency) == 1)
    //    {
    //        pGeneralSettings.sExternalTimebase.fNominalFrequency = fFrequency;
    //    }
    //    else
    //    {
    //        return false;
    //    }
    //}

    //if (!oXML.FindChildElem("Signal_Edge"))
    //{
    //    return false;
    //}
    //tStr = ToLower(oXML.GetChildData());
    //if (tStr == "negative")
    //{
    //    pGeneralSettings.sExternalTimebase.bNegativeEdge = true;
    //}
    //else if (tStr == "positive")
    //{
    //    pGeneralSettings.sExternalTimebase.bNegativeEdge = false;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Signal_Shutter_Delay"))
    //{
    //    return false;
    //}
    //unsigned int nDelay;
    //tStr = oXML.GetChildData();
    //if (sscanf(tStr.c_str(), "%u", &nDelay) == 1)
    //{
    //    pGeneralSettings.sExternalTimebase.nSignalShutterDelay = nDelay;
    //}
    //else
    //{
    //    return false;
    //}

    //if (!oXML.FindChildElem("Non_Periodic_Timeout"))
    //{
    //    return false;
    //}
    //float fTimeout;
    //tStr = oXML.GetChildData();
    //if (sscanf(tStr.c_str(), "%f", &fTimeout) == 1)
    //{
    //    pGeneralSettings.sExternalTimebase.fNonPeriodicTimeout = fTimeout;
    //}
    //else
    //{
    //    return false;
    //}

    //oXML.OutOfElem(); // External_Time_Base


    //// External_Timestamp
    //if (oXML.FindChildElem("External_Timestamp"))
    //{
    //    oXML.IntoElem();

    //    if (oXML.FindChildElem("Enabled"))
    //    {
    //        tStr = ToLower(oXML.GetChildData());
    //        pGeneralSettings.sTimestamp.bEnabled = (tStr == "true");
    //    }
    //    if (oXML.FindChildElem("Type"))
    //    {
    //        tStr = ToLower(oXML.GetChildData());
    //        if (tStr == "smpte")
    //        {
    //            pGeneralSettings.sTimestamp.nType = Timestamp_SMPTE;
    //        }
    //        else if (tStr == "irig")
    //        {
    //            pGeneralSettings.sTimestamp.nType = Timestamp_IRIG;
    //        }
    //        else
    //        {
    //            pGeneralSettings.sTimestamp.nType = Timestamp_CameraTime;
    //        }
    //    }
    //    if (oXML.FindChildElem("Frequency"))
    //    {
    //        unsigned int timestampFrequency;
    //        tStr = oXML.GetChildData();
    //        if (sscanf(tStr.c_str(), "%u", &timestampFrequency) == 1)
    //        {
    //            pGeneralSettings.sTimestamp.nFrequency = timestampFrequency;
    //        }
    //    }
    //    oXML.OutOfElem();
    //}
    //// External_Timestamp


    //const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    //EProcessingActions* processingActions[3] =
    //{
    //    &pGeneralSettings.eProcessingActions,
    //    &pGeneralSettings.eRtProcessingActions,
    //    &pGeneralSettings.eReprocessingActions
    //};
    //auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;
    //for (auto i = 0; i < actionsCount; i++)
    //{
    //    // ==================== Processing actions ====================
    //    if (!oXML.FindChildElem(processings[i]))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    *processingActions[i] = ProcessingNone;

    //    if (mnMajorVersion > 1 || mnMinorVersion > 13)
    //    {
    //        if (!oXML.FindChildElem("PreProcessing2D"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingPreProcess2D);
    //        }
    //    }

    //    if (!oXML.FindChildElem("Tracking"))
    //    {
    //        return false;
    //    }
    //    tStr = ToLower(oXML.GetChildData());
    //    if (tStr == "3d")
    //    {
    //        *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking3D);
    //    }
    //    else if (tStr == "2d" && i != 1) // i != 1 => Not RtProcessingSettings
    //    {
    //        *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking2D);
    //    }

    //    if (i != 1) //Not RtProcessingSettings
    //    {
    //        if (!oXML.FindChildElem("TwinSystemMerge"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTwinSystemMerge);
    //        }

    //        if (!oXML.FindChildElem("SplineFill"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingSplineFill);
    //        }
    //    }

    //    if (!oXML.FindChildElem("AIM"))
    //    {
    //        return false;
    //    }
    //    if (CompareNoCase(oXML.GetChildData(), "true"))
    //    {
    //        *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingAIM);
    //    }

    //    if (!oXML.FindChildElem("Track6DOF"))
    //    {
    //        return false;
    //    }
    //    if (CompareNoCase(oXML.GetChildData(), "true"))
    //    {
    //        *processingActions[i] = (EProcessingActions)(*processingActions[i] + Processing6DOFTracking);
    //    }

    //    if (!oXML.FindChildElem("ForceData"))
    //    {
    //        return false;
    //    }
    //    if (CompareNoCase(oXML.GetChildData(), "true"))
    //    {
    //        *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingForceData);
    //    }

    //    if (mnMajorVersion > 1 || mnMinorVersion > 11)
    //    {
    //        if (!oXML.FindChildElem("GazeVector"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingGazeVector);
    //        }
    //    }

    //    if (i != 1) //Not RtProcessingSettings
    //    {
    //        if (!oXML.FindChildElem("ExportTSV"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportTSV);
    //        }

    //        if (!oXML.FindChildElem("ExportC3D"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportC3D);
    //        }

    //        if (!oXML.FindChildElem("ExportMatlabFile"))
    //        {
    //            return false;
    //        }
    //        if (CompareNoCase(oXML.GetChildData(), "true"))
    //        {
    //            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportMatlabFile);
    //        }

    //        if (mnMajorVersion > 1 || mnMinorVersion > 11)
    //        {
    //            if (!oXML.FindChildElem("ExportAviFile"))
    //            {
    //                return false;
    //            }
    //            if (CompareNoCase(oXML.GetChildData(), "true"))
    //            {
    //                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportAviFile);
    //            }
    //        }
    //    }
    //    oXML.OutOfElem(); // Processing_Actions
    //}

    //if (oXML.FindChildElem("EulerAngles"))
    //{
    //    oXML.IntoElem();
    //    pGeneralSettings.eulerRotations[0] = oXML.GetAttrib("First");
    //    pGeneralSettings.eulerRotations[1] = oXML.GetAttrib("Second");
    //    pGeneralSettings.eulerRotations[2] = oXML.GetAttrib("Third");
    //    oXML.OutOfElem();
    //}

    //SSettingsGeneralCamera sCameraSettings;

    //while (oXML.FindChildElem("Camera"))
    //{
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("ID"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nID = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Model"))
    //    {
    //        return false;
    //    }
    //    tStr = ToLower(oXML.GetChildData());

    //    if (tStr == "macreflex")
    //    {
    //        sCameraSettings.eModel = ModelMacReflex;
    //    }
    //    else if (tStr == "proreflex 120")
    //    {
    //        sCameraSettings.eModel = ModelProReflex120;
    //    }
    //    else if (tStr == "proreflex 240")
    //    {
    //        sCameraSettings.eModel = ModelProReflex240;
    //    }
    //    else if (tStr == "proreflex 500")
    //    {
    //        sCameraSettings.eModel = ModelProReflex500;
    //    }
    //    else if (tStr == "proreflex 1000")
    //    {
    //        sCameraSettings.eModel = ModelProReflex1000;
    //    }
    //    else if (tStr == "oqus 100")
    //    {
    //        sCameraSettings.eModel = ModelOqus100;
    //    }
    //    else if (tStr == "oqus 200" || tStr == "oqus 200 c")
    //    {
    //        sCameraSettings.eModel = ModelOqus200C;
    //    }
    //    else if (tStr == "oqus 300")
    //    {
    //        sCameraSettings.eModel = ModelOqus300;
    //    }
    //    else if (tStr == "oqus 300 plus")
    //    {
    //        sCameraSettings.eModel = ModelOqus300Plus;
    //    }
    //    else if (tStr == "oqus 400")
    //    {
    //        sCameraSettings.eModel = ModelOqus400;
    //    }
    //    else if (tStr == "oqus 500")
    //    {
    //        sCameraSettings.eModel = ModelOqus500;
    //    }
    //    else if (tStr == "oqus 500 plus")
    //    {
    //        sCameraSettings.eModel = ModelOqus500Plus;
    //    }
    //    else if (tStr == "oqus 700")
    //    {
    //        sCameraSettings.eModel = ModelOqus700;
    //    }
    //    else if (tStr == "oqus 700 plus")
    //    {
    //        sCameraSettings.eModel = ModelOqus700Plus;
    //    }
    //    else if (tStr == "oqus 600 plus")
    //    {
    //        sCameraSettings.eModel = ModelOqus600Plus;
    //    }
    //    else if (tStr == "miqus m1")
    //    {
    //        sCameraSettings.eModel = ModelMiqusM1;
    //    }
    //    else if (tStr == "miqus m3")
    //    {
    //        sCameraSettings.eModel = ModelMiqusM3;
    //    }
    //    else if (tStr == "miqus m5")
    //    {
    //        sCameraSettings.eModel = ModelMiqusM5;
    //    }
    //    else if (tStr == "miqus sync unit")
    //    {
    //        sCameraSettings.eModel = ModelMiqusSyncUnit;
    //    }
    //    else if (tStr == "miqus video")
    //    {
    //        sCameraSettings.eModel = ModelMiqusVideo;
    //    }
    //    else if (tStr == "miqus video color")
    //    {
    //        sCameraSettings.eModel = ModelMiqusVideoColor;
    //    }
    //    else if (tStr == "miqus hybrid")
    //    {
    //        sCameraSettings.eModel = ModelMiqusHybrid;
    //    }
    //    else if (tStr == "miqus video color plus")
    //    {
    //        sCameraSettings.eModel = ModelMiqusVideoColorPlus;
    //    }
    //    else if (tStr == "arqus a5")
    //    {
    //        sCameraSettings.eModel = ModelArqusA5;
    //    }
    //    else if (tStr == "arqus a9")
    //    {
    //        sCameraSettings.eModel = ModelArqusA9;
    //    }
    //    else if (tStr == "arqus a12")
    //    {
    //        sCameraSettings.eModel = ModelArqusA12;
    //    }
    //    else if (tStr == "arqus a26")
    //    {
    //        sCameraSettings.eModel = ModelArqusA26;
    //    }
    //    else
    //    {
    //        sCameraSettings.eModel = ModelUnknown;
    //    }

    //    // Only available from protocol version 1.10 and later.
    //    if (oXML.FindChildElem("Underwater"))
    //    {
    //        tStr = ToLower(oXML.GetChildData());
    //        sCameraSettings.bUnderwater = (tStr == "true");
    //    }

    //    if (oXML.FindChildElem("Supports_HW_Sync"))
    //    {
    //        tStr = ToLower(oXML.GetChildData());
    //        sCameraSettings.bSupportsHwSync = (tStr == "true");
    //    }

    //    if (!oXML.FindChildElem("Serial"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nSerial = atoi(oXML.GetChildData().c_str());

    //    // ==================== Camera Mode ====================
    //    if (!oXML.FindChildElem("Mode"))
    //    {
    //        return false;
    //    }
    //    tStr = ToLower(oXML.GetChildData());
    //    if (tStr == "marker")
    //    {
    //        sCameraSettings.eMode = ModeMarker;
    //    }
    //    else if (tStr == "marker intensity")
    //    {
    //        sCameraSettings.eMode = ModeMarkerIntensity;
    //    }
    //    else if (tStr == "video")
    //    {
    //        sCameraSettings.eMode = ModeVideo;
    //    }
    //    else
    //    {
    //        return false;
    //    }

    //    if (mnMajorVersion > 1 || mnMinorVersion > 11)
    //    {
    //        // ==================== Video frequency ====================
    //        if (!oXML.FindChildElem("Video_Frequency"))
    //        {
    //            return false;
    //        }
    //        sCameraSettings.nVideoFrequency = atoi(oXML.GetChildData().c_str());
    //    }

    //    // ==================== Video Resolution ====================
    //    if (oXML.FindChildElem("Video_Resolution"))
    //    {
    //        tStr = ToLower(oXML.GetChildData());
    //        if (tStr == "1440p")
    //        {
    //            sCameraSettings.eVideoResolution = VideoResolution1440p;
    //        }
    //        else if (tStr == "1080p")
    //        {
    //            sCameraSettings.eVideoResolution = VideoResolution1080p;
    //        }
    //        else if (tStr == "720p")
    //        {
    //            sCameraSettings.eVideoResolution = VideoResolution720p;
    //        }
    //        else if (tStr == "540p")
    //        {
    //            sCameraSettings.eVideoResolution = VideoResolution540p;
    //        }
    //        else if (tStr == "480p")
    //        {
    //            sCameraSettings.eVideoResolution = VideoResolution480p;
    //        }
    //        else
    //        {
    //            return false;
    //        }
    //    }
    //    else
    //    {
    //        sCameraSettings.eVideoResolution = VideoResolutionNone;
    //    }

    //    // ==================== Video AspectRatio ====================
    //    if (oXML.FindChildElem("Video_Aspect_Ratio"))
    //    {
    //        tStr = ToLower(oXML.GetChildData());
    //        if (tStr == "16x9")
    //        {
    //            sCameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
    //        }
    //        else if (tStr == "4x3")
    //        {
    //            sCameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
    //        }
    //        else if (tStr == "1x1")
    //        {
    //            sCameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
    //        }
    //    }
    //    else
    //    {
    //        sCameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
    //    }

    //    // ==================== Video exposure ====================
    //    if (!oXML.FindChildElem("Video_Exposure"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Current"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoExposure = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Min"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoExposureMin = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Max"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoExposureMax = atoi(oXML.GetChildData().c_str());
    //    oXML.OutOfElem(); // Video_Exposure

    //    // ==================== Video flash time ====================
    //    if (!oXML.FindChildElem("Video_Flash_Time"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Current"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFlashTime = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Min"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFlashTimeMin = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Max"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFlashTimeMax = atoi(oXML.GetChildData().c_str());
    //    oXML.OutOfElem(); // Video_Flash_Time

    //    // ==================== Marker exposure ====================
    //    if (!oXML.FindChildElem("Marker_Exposure"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Current"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerExposure = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Min"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerExposureMin = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Max"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerExposureMax = atoi(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Marker_Exposure

    //    // ==================== Marker threshold ====================
    //    if (!oXML.FindChildElem("Marker_Threshold"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Current"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerThreshold = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Min"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerThresholdMin = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Max"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerThresholdMax = atoi(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Marker_Threshold

    //    // ==================== Position ====================
    //    if (!oXML.FindChildElem("Position"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("X"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionX = (float)atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Y"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionY = (float)atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Z"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionZ = (float)atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_1_1"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[0][0] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_2_1"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[1][0] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_3_1"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[2][0] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_1_2"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[0][1] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_2_2"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[1][1] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_3_2"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[2][1] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_1_3"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[0][2] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_2_3"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[1][2] = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Rot_3_3"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.fPositionRotMatrix[2][2] = (float)atof(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Position


    //    if (!oXML.FindChildElem("Orientation"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nOrientation = atoi(oXML.GetChildData().c_str());

    //    // ==================== Marker resolution ====================
    //    if (!oXML.FindChildElem("Marker_Res"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Width"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerResolutionWidth = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Height"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerResolutionHeight = atoi(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Marker_Res

    //    // ==================== Video resolution ====================
    //    if (!oXML.FindChildElem("Video_Res"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Width"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoResolutionWidth = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Height"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoResolutionHeight = atoi(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Video_Res

    //    // ==================== Marker FOV ====================
    //    if (!oXML.FindChildElem("Marker_FOV"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Left"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerFOVLeft = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Top"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerFOVTop = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Right"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerFOVRight = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Bottom"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nMarkerFOVBottom = atoi(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Marker_FOV

    //    // ==================== Video FOV ====================
    //    if (!oXML.FindChildElem("Video_FOV"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    if (!oXML.FindChildElem("Left"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFOVLeft = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Top"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFOVTop = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Right"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFOVRight = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Bottom"))
    //    {
    //        return false;
    //    }
    //    sCameraSettings.nVideoFOVBottom = atoi(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // Video_FOV

    //    // ==================== Sync out ====================
    //    // Only available from protocol version 1.10 and later.
    //    for (int port = 0; port < 3; port++)
    //    {
    //        char syncOutStr[16];
    //        sprintf(syncOutStr, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));
    //        if (oXML.FindChildElem(syncOutStr))
    //        {
    //            oXML.IntoElem();

    //            if (port < 2)
    //            {
    //                if (!oXML.FindChildElem("Mode"))
    //                {
    //                    return false;
    //                }
    //                tStr = ToLower(oXML.GetChildData());
    //                if (tStr == "shutter out")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeShutterOut;
    //                }
    //                else if (tStr == "multiplier")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeMultiplier;
    //                }
    //                else if (tStr == "divisor")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeDivisor;
    //                }
    //                else if (tStr == "camera independent")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
    //                }
    //                else if (tStr == "measurement time")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
    //                }
    //                else if (tStr == "continuous 100hz")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
    //                }
    //                else if (tStr == "system live time")
    //                {
    //                    sCameraSettings.eSyncOutMode[port] = ModeSystemLiveTime;
    //                }
    //                else
    //                {
    //                    return false;
    //                }

    //                if (sCameraSettings.eSyncOutMode[port] == ModeMultiplier ||
    //                    sCameraSettings.eSyncOutMode[port] == ModeDivisor ||
    //                    sCameraSettings.eSyncOutMode[port] == ModeIndependentFreq)
    //                {
    //                    if (!oXML.FindChildElem("Value"))
    //                    {
    //                        return false;
    //                    }
    //                    sCameraSettings.nSyncOutValue[port] = atoi(oXML.GetChildData().c_str());

    //                    if (!oXML.FindChildElem("Duty_Cycle"))
    //                    {
    //                        return false;
    //                    }
    //                    sCameraSettings.fSyncOutDutyCycle[port] = (float)atof(oXML.GetChildData().c_str());
    //                }
    //            }
    //            if (port == 2 ||
    //                (sCameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
    //            {
    //                if (!oXML.FindChildElem("Signal_Polarity"))
    //                {
    //                    return false;
    //                }
    //                if (CompareNoCase(oXML.GetChildData(), "negative"))
    //                {
    //                    sCameraSettings.bSyncOutNegativePolarity[port] = true;
    //                }
    //                else
    //                {
    //                    sCameraSettings.bSyncOutNegativePolarity[port] = false;
    //                }
    //            }
    //            oXML.OutOfElem(); // Sync_Out
    //        }
    //        else
    //        {
    //            sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
    //            sCameraSettings.nSyncOutValue[port] = 0;
    //            sCameraSettings.fSyncOutDutyCycle[port] = 0;
    //            sCameraSettings.bSyncOutNegativePolarity[port] = false;
    //        }
    //    }

    //    if (oXML.FindChildElem("LensControl"))
    //    {
    //        oXML.IntoElem();
    //        if (oXML.FindChildElem("Focus"))
    //        {
    //            oXML.IntoElem();
    //            float focus;
    //            if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &focus) == 1)
    //            {
    //                sCameraSettings.fFocus = focus;
    //            }
    //            oXML.OutOfElem();
    //        }
    //        if (oXML.FindChildElem("Aperture"))
    //        {
    //            oXML.IntoElem();
    //            float aperture;
    //            if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &aperture) == 1)
    //            {
    //                sCameraSettings.fAperture = aperture;
    //            }
    //            oXML.OutOfElem();
    //        }
    //        oXML.OutOfElem();
    //    }
    //    else
    //    {
    //        sCameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
    //        sCameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
    //    }

    //    if (oXML.FindChildElem("AutoExposure"))
    //    {
    //        oXML.IntoElem();
    //        if (CompareNoCase(oXML.GetAttrib("Enabled"), "true"))
    //        {
    //            sCameraSettings.autoExposureEnabled = true;
    //        }
    //        float autoExposureCompensation;
    //        if (sscanf(oXML.GetAttrib("Compensation").c_str(), "%f", &autoExposureCompensation) == 1)
    //        {
    //            sCameraSettings.autoExposureCompensation = autoExposureCompensation;
    //        }
    //        oXML.OutOfElem();
    //    }
    //    else
    //    {
    //        sCameraSettings.autoExposureEnabled = false;
    //        sCameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
    //    }

    //    if (oXML.FindChildElem("AutoWhiteBalance"))
    //    {
    //        sCameraSettings.autoWhiteBalance = CompareNoCase(oXML.GetChildData().c_str(), "true") ? 1 : 0;
    //    }
    //    else
    //    {
    //        sCameraSettings.autoWhiteBalance = -1;
    //    }

    //    oXML.OutOfElem(); // Camera

    //    pGeneralSettings.vsCameras.push_back(sCameraSettings);
    //}

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

bool CTinyxml2Deserializer::DeserializeForceSettings(SSettingsForce& pForceSettings, bool& pDataAvailable)
{
    //pDataAvailable = false;

    //pForceSettings.vsForcePlates.clear();

    ////
    //// Read some force plate parameters
    ////
    //if (!oXML.FindChildElem("Force"))
    //{
    //    return true;
    //}

    //oXML.IntoElem();

    //SForcePlate sForcePlate;
    //sForcePlate.bValidCalibrationMatrix = false;
    //for (int i = 0; i < 12; i++)
    //{
    //    for (int j = 0; j < 12; j++)
    //    {
    //        sForcePlate.afCalibrationMatrix[i][j] = 0.0f;
    //    }
    //}
    //sForcePlate.nCalibrationMatrixRows = 6;
    //sForcePlate.nCalibrationMatrixColumns = 6;

    //if (!oXML.FindChildElem("Unit_Length"))
    //{
    //    return false;
    //}
    //pForceSettings.oUnitLength = oXML.GetChildData();

    //if (!oXML.FindChildElem("Unit_Force"))
    //{
    //    return false;
    //}
    //pForceSettings.oUnitForce = oXML.GetChildData();

    //int  iPlate = 1;
    //while (oXML.FindChildElem("Plate"))
    //{
    //    //
    //    // Get name and type of the plates
    //    //
    //    oXML.IntoElem(); // "Plate"
    //    if (oXML.FindChildElem("Force_Plate_Index")) // Version 1.7 and earlier.
    //    {
    //        sForcePlate.nID = atoi(oXML.GetChildData().c_str());
    //    }
    //    else if (oXML.FindChildElem("Plate_ID")) // Version 1.8 and later.
    //    {
    //        sForcePlate.nID = atoi(oXML.GetChildData().c_str());
    //    }
    //    else
    //    {
    //        return false;
    //    }

    //    if (oXML.FindChildElem("Analog_Device_ID"))
    //    {
    //        sForcePlate.nAnalogDeviceID = atoi(oXML.GetChildData().c_str());
    //    }
    //    else
    //    {
    //        sForcePlate.nAnalogDeviceID = 0;
    //    }

    //    if (!oXML.FindChildElem("Frequency"))
    //    {
    //        return false;
    //    }
    //    sForcePlate.nFrequency = atoi(oXML.GetChildData().c_str());

    //    if (oXML.FindChildElem("Type"))
    //    {
    //        sForcePlate.oType = oXML.GetChildData();
    //    }
    //    else
    //    {
    //        sForcePlate.oType = "unknown";
    //    }

    //    if (oXML.FindChildElem("Name"))
    //    {
    //        sForcePlate.oName = oXML.GetChildData();
    //    }
    //    else
    //    {
    //        sForcePlate.oName = CTinyxml2::Format("#%d", iPlate);
    //    }

    //    if (oXML.FindChildElem("Length"))
    //    {
    //        sForcePlate.fLength = (float)atof(oXML.GetChildData().c_str());
    //    }
    //    if (oXML.FindChildElem("Width"))
    //    {
    //        sForcePlate.fWidth = (float)atof(oXML.GetChildData().c_str());
    //    }

    //    if (oXML.FindChildElem("Location"))
    //    {
    //        oXML.IntoElem();
    //        if (oXML.FindChildElem("Corner1"))
    //        {
    //            oXML.IntoElem();
    //            if (oXML.FindChildElem("X"))
    //            {
    //                sForcePlate.asCorner[0].fX = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Y"))
    //            {
    //                sForcePlate.asCorner[0].fY = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Z"))
    //            {
    //                sForcePlate.asCorner[0].fZ = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            oXML.OutOfElem();
    //        }
    //        if (oXML.FindChildElem("Corner2"))
    //        {
    //            oXML.IntoElem();
    //            if (oXML.FindChildElem("X"))
    //            {
    //                sForcePlate.asCorner[1].fX = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Y"))
    //            {
    //                sForcePlate.asCorner[1].fY = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Z"))
    //            {
    //                sForcePlate.asCorner[1].fZ = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            oXML.OutOfElem();
    //        }
    //        if (oXML.FindChildElem("Corner3"))
    //        {
    //            oXML.IntoElem();
    //            if (oXML.FindChildElem("X"))
    //            {
    //                sForcePlate.asCorner[2].fX = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Y"))
    //            {
    //                sForcePlate.asCorner[2].fY = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Z"))
    //            {
    //                sForcePlate.asCorner[2].fZ = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            oXML.OutOfElem();
    //        }
    //        if (oXML.FindChildElem("Corner4"))
    //        {
    //            oXML.IntoElem();
    //            if (oXML.FindChildElem("X"))
    //            {
    //                sForcePlate.asCorner[3].fX = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Y"))
    //            {
    //                sForcePlate.asCorner[3].fY = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("Z"))
    //            {
    //                sForcePlate.asCorner[3].fZ = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            oXML.OutOfElem();
    //        }
    //        oXML.OutOfElem();
    //    }

    //    if (oXML.FindChildElem("Origin"))
    //    {
    //        oXML.IntoElem();
    //        if (oXML.FindChildElem("X"))
    //        {
    //            sForcePlate.sOrigin.fX = (float)atof(oXML.GetChildData().c_str());
    //        }
    //        if (oXML.FindChildElem("Y"))
    //        {
    //            sForcePlate.sOrigin.fY = (float)atof(oXML.GetChildData().c_str());
    //        }
    //        if (oXML.FindChildElem("Z"))
    //        {
    //            sForcePlate.sOrigin.fZ = (float)atof(oXML.GetChildData().c_str());
    //        }
    //        oXML.OutOfElem();
    //    }

    //    sForcePlate.vChannels.clear();
    //    if (oXML.FindChildElem("Channels"))
    //    {
    //        oXML.IntoElem();
    //        SForceChannel sForceChannel;
    //        while (oXML.FindChildElem("Channel"))
    //        {
    //            oXML.IntoElem();
    //            if (oXML.FindChildElem("Channel_No"))
    //            {
    //                sForceChannel.nChannelNumber = atoi(oXML.GetChildData().c_str());
    //            }
    //            if (oXML.FindChildElem("ConversionFactor"))
    //            {
    //                sForceChannel.fConversionFactor = (float)atof(oXML.GetChildData().c_str());
    //            }
    //            sForcePlate.vChannels.push_back(sForceChannel);
    //            oXML.OutOfElem();
    //        }
    //        oXML.OutOfElem();
    //    }

    //    if (oXML.FindChildElem("Calibration_Matrix"))
    //    {
    //        oXML.IntoElem();
    //        int nRow = 0;

    //        if (mnMajorVersion == 1 && mnMinorVersion < 12)
    //        {
    //            char strRow[16];
    //            char strCol[16];
    //            sprintf(strRow, "Row%d", nRow + 1);
    //            while (oXML.FindChildElem(strRow))
    //            {
    //                oXML.IntoElem();
    //                int nCol = 0;
    //                sprintf(strCol, "Col%d", nCol + 1);
    //                while (oXML.FindChildElem(strCol))
    //                {
    //                    sForcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
    //                    nCol++;
    //                    sprintf(strCol, "Col%d", nCol + 1);
    //                }
    //                sForcePlate.nCalibrationMatrixColumns = nCol;

    //                nRow++;
    //                sprintf(strRow, "Row%d", nRow + 1);
    //                oXML.OutOfElem(); // RowX
    //            }
    //        }
    //        else
    //        {
    //            //<Rows>
    //            if (oXML.FindChildElem("Rows"))
    //            {
    //                oXML.IntoElem();

    //                while (oXML.FindChildElem("Row"))
    //                {
    //                    oXML.IntoElem();

    //                    //<Columns>
    //                    if (oXML.FindChildElem("Columns"))
    //                    {
    //                        oXML.IntoElem();

    //                        int nCol = 0;
    //                        while (oXML.FindChildElem("Column"))
    //                        {
    //                            sForcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
    //                            nCol++;
    //                        }
    //                        sForcePlate.nCalibrationMatrixColumns = nCol;

    //                        nRow++;
    //                        oXML.OutOfElem(); // Columns
    //                    }
    //                    oXML.OutOfElem(); // Row
    //                }
    //                oXML.OutOfElem(); // Rows
    //            }
    //        }
    //        sForcePlate.nCalibrationMatrixRows = nRow;
    //        sForcePlate.bValidCalibrationMatrix = true;

    //        oXML.OutOfElem(); // "Calibration_Matrix"
    //    }
    //    oXML.OutOfElem(); // "Plate"

    //    pDataAvailable = true;
    //    pForceSettings.vsForcePlates.push_back(sForcePlate);
    //}

    return true;
} // Read force settings

bool CTinyxml2Deserializer::DeserializeImageSettings(std::vector<SImageCamera>& pImageSettings, bool& pDataAvailable)
{
    //pDataAvailable = false;

    //pImageSettings.clear();

    ////
    //// Read some Image parameters
    ////
    //if (!oXML.FindChildElem("Image"))
    //{
    //    return true;
    //}
    //oXML.IntoElem();

    //while (oXML.FindChildElem("Camera"))
    //{
    //    oXML.IntoElem();

    //    SImageCamera sImageCamera;

    //    if (!oXML.FindChildElem("ID"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.nID = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Enabled"))
    //    {
    //        return false;
    //    }
    //    std::string tStr;
    //    tStr = ToLower(oXML.GetChildData());

    //    if (tStr == "true")
    //    {
    //        sImageCamera.bEnabled = true;
    //    }
    //    else
    //    {
    //        sImageCamera.bEnabled = false;
    //    }

    //    if (!oXML.FindChildElem("Format"))
    //    {
    //        return false;
    //    }
    //    tStr = ToLower(oXML.GetChildData());

    //    if (tStr == "rawgrayscale")
    //    {
    //        sImageCamera.eFormat = CRTPacket::FormatRawGrayscale;
    //    }
    //    else if (tStr == "rawbgr")
    //    {
    //        sImageCamera.eFormat = CRTPacket::FormatRawBGR;
    //    }
    //    else if (tStr == "jpg")
    //    {
    //        sImageCamera.eFormat = CRTPacket::FormatJPG;
    //    }
    //    else if (tStr == "png")
    //    {
    //        sImageCamera.eFormat = CRTPacket::FormatPNG;
    //    }
    //    else
    //    {
    //        return false;
    //    }

    //    if (!oXML.FindChildElem("Width"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.nWidth = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Height"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.nHeight = atoi(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Left_Crop"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.fCropLeft = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Top_Crop"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.fCropTop = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Right_Crop"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.fCropRight = (float)atof(oXML.GetChildData().c_str());

    //    if (!oXML.FindChildElem("Bottom_Crop"))
    //    {
    //        return false;
    //    }
    //    sImageCamera.fCropBottom = (float)atof(oXML.GetChildData().c_str());

    //    oXML.OutOfElem(); // "Camera"

    //    pImageSettings.push_back(sImageCamera);
    //    pDataAvailable = true;
    //}

    return true;
} // ReadImageSettings

bool CTinyxml2Deserializer::DeserializeSkeletonSettings(bool pSkeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>& mSkeletonSettingsHierarchical, std::vector<SSettingsSkeleton>& mSkeletonSettings, bool& pDataAvailable)
{
    //CTinyxml2 xml = oXML;

    //pDataAvailable = false;

    //mSkeletonSettings.clear();
    //mSkeletonSettingsHierarchical.clear();

    //int segmentIndex;
    //std::map<int, int> segmentIdIndexMap;
    //xml.ResetPos();

    //xml.FindElem();
    //xml.IntoElem();

    //if (xml.FindElem("Skeletons"))
    //{
    //    xml.IntoElem();

    //    if (mnMajorVersion > 1 || mnMinorVersion > 20)
    //    {
    //        while (xml.FindElem("Skeleton"))
    //        {
    //            SSettingsSkeletonHierarchical skeletonHierarchical;
    //            SSettingsSkeleton skeleton;
    //            segmentIndex = 0;

    //            skeletonHierarchical.name = xml.GetAttrib("Name");
    //            skeleton.name = skeletonHierarchical.name;

    //            xml.IntoElem();

    //            if (xml.FindElem("Solver"))
    //            {
    //                skeletonHierarchical.rootSegment.solver = xml.GetData();
    //            }

    //            if (xml.FindElem("Scale"))
    //            {
    //                if (!ParseString(xml.GetData(), skeletonHierarchical.scale))
    //                {
    //                    sprintf(maErrorStr, "Scale element parse error");
    //                    return false;
    //                }
    //            }

    //            if (xml.FindElem("Segments"))
    //            {
    //                xml.IntoElem();

    //                std::function<void(SSettingsSkeletonSegmentHierarchical&, std::vector<SSettingsSkeletonSegment>&, std::uint32_t)> recurseSegments =
    //                    [&](SSettingsSkeletonSegmentHierarchical& segmentHierarchical, std::vector<SSettingsSkeletonSegment>& segments, std::uint32_t parentId)
    //                    {
    //                        segmentHierarchical.name = xml.GetAttrib("Name");
    //                        ParseString(xml.GetAttrib("ID"), segmentHierarchical.id);

    //                        segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

    //                        xml.IntoElem();

    //                        if (xml.FindElem("Solver"))
    //                        {
    //                            segmentHierarchical.solver = xml.GetData();
    //                        }

    //                        if (xml.FindElem("Transform"))
    //                        {
    //                            xml.IntoElem();
    //                            segmentHierarchical.position = ReadXMLPosition(xml, "Position");
    //                            segmentHierarchical.rotation = ReadXMLRotation(xml, "Rotation");
    //                            xml.OutOfElem(); // Transform
    //                        }

    //                        if (xml.FindElem("DefaultTransform"))
    //                        {
    //                            xml.IntoElem();
    //                            segmentHierarchical.defaultPosition = ReadXMLPosition(xml, "Position");
    //                            segmentHierarchical.defaultRotation = ReadXMLRotation(xml, "Rotation");
    //                            xml.OutOfElem(); // DefaultTransform
    //                        }

    //                        if (xml.FindElem("DegreesOfFreedom"))
    //                        {
    //                            xml.IntoElem();
    //                            ReadXMLDegreesOfFreedom(xml, "RotationX", segmentHierarchical.degreesOfFreedom);
    //                            ReadXMLDegreesOfFreedom(xml, "RotationY", segmentHierarchical.degreesOfFreedom);
    //                            ReadXMLDegreesOfFreedom(xml, "RotationZ", segmentHierarchical.degreesOfFreedom);
    //                            ReadXMLDegreesOfFreedom(xml, "TranslationX", segmentHierarchical.degreesOfFreedom);
    //                            ReadXMLDegreesOfFreedom(xml, "TranslationY", segmentHierarchical.degreesOfFreedom);
    //                            ReadXMLDegreesOfFreedom(xml, "TranslationZ", segmentHierarchical.degreesOfFreedom);
    //                            xml.OutOfElem(); // DegreesOfFreedom
    //                        }

    //                        segmentHierarchical.endpoint = ReadXMLPosition(xml, "Endpoint");

    //                        if (xml.FindElem("Markers"))
    //                        {
    //                            xml.IntoElem();

    //                            while (xml.FindElem("Marker"))
    //                            {
    //                                SMarker marker;

    //                                marker.name = xml.GetAttrib("Name");
    //                                marker.weight = 1.0;

    //                                xml.IntoElem();
    //                                marker.position = ReadXMLPosition(xml, "Position");
    //                                if (xml.FindElem("Weight"))
    //                                {
    //                                    ParseString(xml.GetData(), marker.weight);
    //                                }

    //                                xml.OutOfElem(); // Marker

    //                                segmentHierarchical.markers.push_back(marker);
    //                            }

    //                            xml.OutOfElem(); // Markers
    //                        }

    //                        if (xml.FindElem("RigidBodies"))
    //                        {
    //                            xml.IntoElem();

    //                            while (xml.FindElem("RigidBody"))
    //                            {
    //                                SBody body;

    //                                body.name = xml.GetAttrib("Name");
    //                                body.weight = 1.0;

    //                                xml.IntoElem();

    //                                if (xml.FindElem("Transform"))
    //                                {
    //                                    xml.IntoElem();
    //                                    body.position = ReadXMLPosition(xml, "Position");
    //                                    body.rotation = ReadXMLRotation(xml, "Rotation");
    //                                    xml.OutOfElem(); // Transform
    //                                }
    //                                if (xml.FindElem("Weight"))
    //                                {
    //                                    ParseString(xml.GetData(), body.weight);
    //                                }

    //                                xml.OutOfElem(); // RigidBody

    //                                segmentHierarchical.bodies.push_back(body);
    //                            }

    //                            xml.OutOfElem(); // RigidBodies
    //                        }
    //                        SSettingsSkeletonSegment segment;
    //                        segment.name = segmentHierarchical.name;
    //                        segment.id = segmentHierarchical.id;
    //                        segment.parentId = parentId;
    //                        segment.parentIndex = (parentId != -1) ? segmentIdIndexMap[parentId] : -1;
    //                        segment.positionX = (float)segmentHierarchical.defaultPosition.x;
    //                        segment.positionY = (float)segmentHierarchical.defaultPosition.y;
    //                        segment.positionZ = (float)segmentHierarchical.defaultPosition.z;
    //                        segment.rotationX = (float)segmentHierarchical.defaultRotation.x;
    //                        segment.rotationY = (float)segmentHierarchical.defaultRotation.y;
    //                        segment.rotationZ = (float)segmentHierarchical.defaultRotation.z;
    //                        segment.rotationW = (float)segmentHierarchical.defaultRotation.w;

    //                        segments.push_back(segment);

    //                        while (xml.FindElem("Segment"))
    //                        {
    //                            SSettingsSkeletonSegmentHierarchical childSegment;
    //                            recurseSegments(childSegment, skeleton.segments, segmentHierarchical.id);
    //                            segmentHierarchical.segments.push_back(childSegment);
    //                        }
    //                        xml.OutOfElem();
    //                    };

    //                if (xml.FindElem("Segment"))
    //                {
    //                    recurseSegments(skeletonHierarchical.rootSegment, skeleton.segments, -1);
    //                }
    //                xml.OutOfElem(); // Segments
    //            }
    //            xml.OutOfElem(); // Skeleton
    //            mSkeletonSettingsHierarchical.push_back(skeletonHierarchical);
    //            mSkeletonSettings.push_back(skeleton);
    //        }
    //        pDataAvailable = true;
    //    }
    //    else
    //    {
    //        while (xml.FindElem("Skeleton"))
    //        {
    //            SSettingsSkeleton skeleton;
    //            segmentIndex = 0;

    //            skeleton.name = xml.GetAttrib("Name");
    //            xml.IntoElem();

    //            while (xml.FindElem("Segment"))
    //            {
    //                SSettingsSkeletonSegment segment;

    //                segment.name = xml.GetAttrib("Name");
    //                if (segment.name.size() == 0 || sscanf(xml.GetAttrib("ID").c_str(), "%u", &segment.id) != 1)
    //                {
    //                    return false;
    //                }

    //                segmentIdIndexMap[segment.id] = segmentIndex++;

    //                int parentId;
    //                if (sscanf(xml.GetAttrib("Parent_ID").c_str(), "%d", &parentId) != 1)
    //                {
    //                    segment.parentId = -1;
    //                    segment.parentIndex = -1;
    //                }
    //                else if (segmentIdIndexMap.count(parentId) > 0)
    //                {
    //                    segment.parentId = parentId;
    //                    segment.parentIndex = segmentIdIndexMap[parentId];
    //                }

    //                xml.IntoElem();

    //                if (xml.FindElem("Position"))
    //                {
    //                    ParseString(xml.GetAttrib("X"), segment.positionX);
    //                    ParseString(xml.GetAttrib("Y"), segment.positionY);
    //                    ParseString(xml.GetAttrib("Z"), segment.positionZ);
    //                }

    //                if (xml.FindElem("Rotation"))
    //                {
    //                    ParseString(xml.GetAttrib("X"), segment.rotationX);
    //                    ParseString(xml.GetAttrib("Y"), segment.rotationY);
    //                    ParseString(xml.GetAttrib("Z"), segment.rotationZ);
    //                    ParseString(xml.GetAttrib("W"), segment.rotationW);
    //                }

    //                skeleton.segments.push_back(segment);

    //                xml.OutOfElem(); // Segment
    //            }

    //            mSkeletonSettings.push_back(skeleton);

    //            xml.OutOfElem(); // Skeleton
    //        }
    //    }
    //    xml.OutOfElem(); // Skeletons
    //    pDataAvailable = true;
    //}
    return true;
} // ReadSkeletonSettings

namespace
{
    bool ReadXmlFov(std::string name, tinyxml2::XMLDocument& oXML, SCalibrationFov& fov)
    {
        //if (!oXML.FindChildElem(name.c_str()))
        //{
        //    return false;
        //}
        //fov.left = std::stoul(oXML.GetChildAttrib("left"));
        //fov.top = std::stoul(oXML.GetChildAttrib("top"));
        //fov.right = std::stoul(oXML.GetChildAttrib("right"));
        //fov.bottom = std::stoul(oXML.GetChildAttrib("bottom"));

        return true;
    }
}

bool CTinyxml2Deserializer::DeserializeCalibrationSettings(SCalibration& pCalibrationSettings)
{
    //SCalibration settings;

    //if (!oXML.FindChildElem("calibration"))
    //{
    //    sprintf(maErrorStr, "Missing calibration element");
    //    return false;
    //}
    //oXML.IntoElem();

    //try
    //{
    //    std::string resultStr = ToLower(oXML.GetAttrib("calibrated"));

    //    settings.calibrated = (resultStr == "true");
    //    settings.source = oXML.GetAttrib("source");
    //    settings.created = oXML.GetAttrib("created");
    //    settings.qtm_version = oXML.GetAttrib("qtm-version");
    //    std::string typeStr = oXML.GetAttrib("type");
    //    if (typeStr == "regular")
    //    {
    //        settings.type = ECalibrationType::regular;
    //    }
    //    if (typeStr == "refine")
    //    {
    //        settings.type = ECalibrationType::refine;
    //    }
    //    if (typeStr == "fixed")
    //    {
    //        settings.type = ECalibrationType::fixed;
    //    }

    //    if (settings.type == ECalibrationType::refine)
    //    {
    //        settings.refit_residual = std::stod(oXML.GetAttrib("refit-residual"));
    //    }
    //    if (settings.type != ECalibrationType::fixed)
    //    {
    //        settings.wand_length = std::stod(oXML.GetAttrib("wandLength"));
    //        settings.max_frames = std::stoul(oXML.GetAttrib("maximumFrames"));
    //        settings.short_arm_end = std::stod(oXML.GetAttrib("shortArmEnd"));
    //        settings.long_arm_end = std::stod(oXML.GetAttrib("longArmEnd"));
    //        settings.long_arm_middle = std::stod(oXML.GetAttrib("longArmMiddle"));

    //        if (!oXML.FindChildElem("results"))
    //        {
    //            return false;
    //        }

    //        settings.result_std_dev = std::stod(oXML.GetChildAttrib("std-dev"));
    //        settings.result_min_max_diff = std::stod(oXML.GetChildAttrib("min-max-diff"));
    //        if (settings.type == ECalibrationType::refine)
    //        {
    //            settings.result_refit_residual = std::stod(oXML.GetChildAttrib("refit-residual"));
    //            settings.result_consecutive = std::stoul(oXML.GetChildAttrib("consecutive"));
    //        }
    //    }

    //    if (!oXML.FindChildElem("cameras"))
    //    {
    //        return false;
    //    }
    //    oXML.IntoElem();

    //    while (oXML.FindChildElem("camera"))
    //    {
    //        oXML.IntoElem();
    //        SCalibrationCamera camera;
    //        camera.active = std::stod(oXML.GetAttrib("active")) != 0;

    //        std::string calibratedStr = ToLower(oXML.GetAttrib("calibrated"));

    //        camera.calibrated = (calibratedStr == "true");
    //        camera.message = oXML.GetAttrib("message");

    //        camera.point_count = std::stoul(oXML.GetAttrib("point-count"));
    //        camera.avg_residual = std::stod(oXML.GetAttrib("avg-residual"));
    //        camera.serial = std::stoul(oXML.GetAttrib("serial"));
    //        camera.model = oXML.GetAttrib("model");
    //        camera.view_rotation = std::stoul(oXML.GetAttrib("viewrotation"));
    //        if (!ReadXmlFov("fov_marker", oXML, camera.fov_marker))
    //        {
    //            return false;
    //        }
    //        if (!ReadXmlFov("fov_marker_max", oXML, camera.fov_marker_max))
    //        {
    //            return false;
    //        }
    //        if (!ReadXmlFov("fov_video", oXML, camera.fov_video))
    //        {
    //            return false;
    //        }
    //        if (!ReadXmlFov("fov_video_max", oXML, camera.fov_video_max))
    //        {
    //            return false;
    //        }
    //        if (!oXML.FindChildElem("transform"))
    //        {
    //            return false;
    //        }
    //        camera.transform.x = std::stod(oXML.GetChildAttrib("x"));
    //        camera.transform.y = std::stod(oXML.GetChildAttrib("y"));
    //        camera.transform.z = std::stod(oXML.GetChildAttrib("z"));
    //        camera.transform.r11 = std::stod(oXML.GetChildAttrib("r11"));
    //        camera.transform.r12 = std::stod(oXML.GetChildAttrib("r12"));
    //        camera.transform.r13 = std::stod(oXML.GetChildAttrib("r13"));
    //        camera.transform.r21 = std::stod(oXML.GetChildAttrib("r21"));
    //        camera.transform.r22 = std::stod(oXML.GetChildAttrib("r22"));
    //        camera.transform.r23 = std::stod(oXML.GetChildAttrib("r23"));
    //        camera.transform.r31 = std::stod(oXML.GetChildAttrib("r31"));
    //        camera.transform.r32 = std::stod(oXML.GetChildAttrib("r32"));
    //        camera.transform.r33 = std::stod(oXML.GetChildAttrib("r33"));

    //        if (!oXML.FindChildElem("intrinsic"))
    //        {
    //            return false;
    //        }

    //        auto focalLength = oXML.GetChildAttrib("focallength");
    //        try
    //        {
    //            camera.intrinsic.focal_length = std::stod(focalLength);
    //        }
    //        catch (const std::invalid_argument&)
    //        {
    //            camera.intrinsic.focal_length = 0;
    //        }

    //        camera.intrinsic.sensor_min_u = std::stod(oXML.GetChildAttrib("sensorMinU"));
    //        camera.intrinsic.sensor_max_u = std::stod(oXML.GetChildAttrib("sensorMaxU"));
    //        camera.intrinsic.sensor_min_v = std::stod(oXML.GetChildAttrib("sensorMinV"));
    //        camera.intrinsic.sensor_max_v = std::stod(oXML.GetChildAttrib("sensorMaxV"));
    //        camera.intrinsic.focal_length_u = std::stod(oXML.GetChildAttrib("focalLengthU"));
    //        camera.intrinsic.focal_length_v = std::stod(oXML.GetChildAttrib("focalLengthV"));
    //        camera.intrinsic.center_point_u = std::stod(oXML.GetChildAttrib("centerPointU"));
    //        camera.intrinsic.center_point_v = std::stod(oXML.GetChildAttrib("centerPointV"));
    //        camera.intrinsic.skew = std::stod(oXML.GetChildAttrib("skew"));
    //        camera.intrinsic.radial_distortion_1 = std::stod(oXML.GetChildAttrib("radialDistortion1"));
    //        camera.intrinsic.radial_distortion_2 = std::stod(oXML.GetChildAttrib("radialDistortion2"));
    //        camera.intrinsic.radial_distortion_3 = std::stod(oXML.GetChildAttrib("radialDistortion3"));
    //        camera.intrinsic.tangental_distortion_1 = std::stod(oXML.GetChildAttrib("tangentalDistortion1"));
    //        camera.intrinsic.tangental_distortion_2 = std::stod(oXML.GetChildAttrib("tangentalDistortion2"));
    //        oXML.OutOfElem(); // camera
    //        settings.cameras.push_back(camera);
    //    }
    //    oXML.OutOfElem(); // cameras
    //}
    //catch (...)
    //{
    //    return false;
    //}

    //oXML.OutOfElem(); // calibration

    //pCalibrationSettings = settings;
    return true;
}

SPosition CTinyxml2Deserializer::DeserializeXMLPosition(tinyxml2::XMLDocument& xml, const std::string& element)
{
    SPosition position;

    //if (xml.FindElem(element.c_str()))
    //{
    //    ParseString(xml.GetAttrib("X"), position.x);
    //    ParseString(xml.GetAttrib("Y"), position.y);
    //    ParseString(xml.GetAttrib("Z"), position.z);
    //    xml.ResetMainPos();
    //}
    return position;
}

SRotation CTinyxml2Deserializer::DeserializeXMLRotation(tinyxml2::XMLDocument& xml, const std::string& element)
{
    SRotation rotation;

    //if (xml.FindElem(element.c_str()))
    //{
    //    ParseString(xml.GetAttrib("X"), rotation.x);
    //    ParseString(xml.GetAttrib("Y"), rotation.y);
    //    ParseString(xml.GetAttrib("Z"), rotation.z);
    //    ParseString(xml.GetAttrib("W"), rotation.w);
    //    xml.ResetMainPos();
    //}
    return rotation;
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
    //CTinyxml2 oXML;

    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();
    //oXML.AddElem("External_Time_Base");
    //oXML.IntoElem();

    //AddXMLElementBool(&oXML, "Enabled", pbEnabled);

    //if (peSignalSource)
    //{
    //    switch (*peSignalSource)
    //    {
    //    case SourceControlPort:
    //        oXML.AddElem("Signal_Source", "Control port");
    //        break;
    //    case SourceIRReceiver:
    //        oXML.AddElem("Signal_Source", "IR receiver");
    //        break;
    //    case SourceSMPTE:
    //        oXML.AddElem("Signal_Source", "SMPTE");
    //        break;
    //    case SourceVideoSync:
    //        oXML.AddElem("Signal_Source", "Video sync");
    //        break;
    //    case SourceIRIG:
    //        oXML.AddElem("Signal_Source", "IRIG");
    //        break;
    //    }
    //}

    //AddXMLElementBool(&oXML, "Signal_Mode", pbSignalModePeriodic, "Periodic", "Non-periodic");
    //AddXMLElementUnsignedInt(&oXML, "Frequency_Multiplier", pnFreqMultiplier);
    //AddXMLElementUnsignedInt(&oXML, "Frequency_Divisor", pnFreqDivisor);
    //AddXMLElementUnsignedInt(&oXML, "Frequency_Tolerance", pnFreqTolerance);

    //if (pfNominalFrequency)
    //{
    //    if (*pfNominalFrequency < 0)
    //    {
    //        oXML.AddElem("Nominal_Frequency", "None");
    //    }
    //    else
    //    {
    //        AddXMLElementFloat(&oXML, "Nominal_Frequency", pfNominalFrequency, 3);
    //    }
    //}

    //AddXMLElementBool(&oXML, "Signal_Edge", pbNegativeEdge, "Negative", "Positive");
    //AddXMLElementUnsignedInt(&oXML, "Signal_Shutter_Delay", pnSignalShutterDelay);
    //AddXMLElementFloat(&oXML, "Non_Periodic_Timeout", pfNonPeriodicTimeout, 3);

    //oXML.OutOfElem(); // External_Time_Base            
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();
    return "";
}

std::string CTinyxml2Serializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    //CTinyxml2 oXML;

    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();
    //oXML.AddElem("External_Timestamp");
    //oXML.IntoElem();

    //AddXMLElementBool(&oXML, "Enabled", timestampSettings.bEnabled);

    //switch (timestampSettings.nType)
    //{
    //default:
    //case ETimestampType::Timestamp_SMPTE:
    //    oXML.AddElem("Type", "SMPTE");
    //    break;
    //case ETimestampType::Timestamp_IRIG:
    //    oXML.AddElem("Type", "IRIG");
    //    break;
    //case ETimestampType::Timestamp_CameraTime:
    //    oXML.AddElem("Type", "CameraTime");
    //    break;
    //}
    //AddXMLElementUnsignedInt(&oXML, "Frequency", timestampSettings.nFrequency);

    //oXML.OutOfElem(); // Timestamp
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();
    return "";
}

std::string CTinyxml2Serializer::SetCameraSettings(
    const unsigned int pCameraId, const ECameraMode* peMode,
    const float* pfMarkerExposure, const float* pfMarkerThreshold,
    const int* pnOrientation)
{
    //CTinyxml2 oXML;

    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();

    //oXML.AddElem("Camera");
    //oXML.IntoElem();

    //AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    //if (peMode)
    //{
    //    switch (*peMode)
    //    {
    //    case ModeMarker:
    //        oXML.AddElem("Mode", "Marker");
    //        break;
    //    case ModeMarkerIntensity:
    //        oXML.AddElem("Mode", "Marker Intensity");
    //        break;
    //    case ModeVideo:
    //        oXML.AddElem("Mode", "Video");
    //        break;
    //    }
    //}
    //AddXMLElementFloat(&oXML, "Marker_Exposure", pfMarkerExposure);
    //AddXMLElementFloat(&oXML, "Marker_Threshold", pfMarkerThreshold);
    //AddXMLElementInt(&oXML, "Orientation", pnOrientation);

    //oXML.OutOfElem(); // Camera
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();

    return "";
}

std::string CTinyxml2Serializer::SetCameraVideoSettings(const unsigned int pCameraId,
    const EVideoResolution* eVideoResolution, const EVideoAspectRatio* eVideoAspectRatio,
    const unsigned int* pnVideoFrequency, const float* pfVideoExposure, const float* pfVideoFlashTime)
{
    return "";
}

std::string CTinyxml2Serializer::SetCameraSyncOutSettings(const unsigned int pCameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* peSyncOutMode, const unsigned int* pnSyncOutValue, const float* pfSyncOutDutyCycle,
    const bool* pbSyncOutNegativePolarity)
{
    //CTinyxml2 oXML;
    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();

    //oXML.AddElem("Camera");
    //oXML.IntoElem();

    //AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    //int port = portNumber - 1;
    //if (((port == 0 || port == 1) && peSyncOutMode) || (port == 2))
    //{
    //    oXML.AddElem(port == 0 ? "Sync_Out" : (port == 1 ? "Sync_Out2" : "Sync_Out_MT"));
    //    oXML.IntoElem();

    //    if (port == 0 || port == 1)
    //    {
    //        switch (*peSyncOutMode)
    //        {
    //        case ModeShutterOut:
    //            oXML.AddElem("Mode", "Shutter out");
    //            break;
    //        case ModeMultiplier:
    //            oXML.AddElem("Mode", "Multiplier");
    //            break;
    //        case ModeDivisor:
    //            oXML.AddElem("Mode", "Divisor");
    //            break;
    //        case ModeIndependentFreq:
    //            oXML.AddElem("Mode", "Camera independent");
    //            break;
    //        case ModeMeasurementTime:
    //            oXML.AddElem("Mode", "Measurement time");
    //            break;
    //        case ModeFixed100Hz:
    //            oXML.AddElem("Mode", "Continuous 100Hz");
    //            break;
    //        case ModeSystemLiveTime:
    //            oXML.AddElem("Mode", "System live time");
    //            break;
    //        default:
    //            return false; // Should never happen
    //        }

    //        if (*peSyncOutMode == ModeMultiplier ||
    //            *peSyncOutMode == ModeDivisor ||
    //            *peSyncOutMode == ModeIndependentFreq)
    //        {
    //            if (pnSyncOutValue)
    //            {
    //                AddXMLElementUnsignedInt(&oXML, "Value", pnSyncOutValue);
    //            }
    //            if (pfSyncOutDutyCycle)
    //            {
    //                AddXMLElementFloat(&oXML, "Duty_Cycle", pfSyncOutDutyCycle, 3);
    //            }
    //        }
    //    }
    //    if (pbSyncOutNegativePolarity && (port == 2 ||
    //        (peSyncOutMode && *peSyncOutMode != ModeFixed100Hz)))
    //    {
    //        AddXMLElementBool(&oXML, "Signal_Polarity", pbSyncOutNegativePolarity, "Negative", "Positive");
    //    }
    //    oXML.OutOfElem(); // Sync_Out
    //}
    //oXML.OutOfElem(); // Camera
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();

    return "";
}

std::string CTinyxml2Serializer::SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus,
    const float pAperture)
{
    //CTinyxml2 oXML;

    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();

    //oXML.AddElem("Camera");
    //oXML.IntoElem();

    //AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    //oXML.AddElem("LensControl");
    //oXML.IntoElem();

    //oXML.AddElem("Focus");
    //oXML.AddAttrib("Value", CTinyxml2::Format("%f", pFocus).c_str());
    //oXML.AddElem("Aperture");
    //oXML.AddAttrib("Value", CTinyxml2::Format("%f", pAperture).c_str());

    //oXML.OutOfElem(); // LensControl
    //oXML.OutOfElem(); // Camera
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();

    return "";
}

std::string CTinyxml2Serializer::SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure,
    const float pCompensation)
{
    //CTinyxml2 oXML;

    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();

    //oXML.AddElem("Camera");
    //oXML.IntoElem();

    //AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    //oXML.AddElem("LensControl");
    //oXML.IntoElem();

    //oXML.AddElem("AutoExposure");
    //oXML.AddAttrib("Enabled", pAutoExposure ? "true" : "false");
    //oXML.AddAttrib("Compensation", CTinyxml2::Format("%f", pCompensation).c_str());

    //oXML.OutOfElem(); // AutoExposure
    //oXML.OutOfElem(); // Camera
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();

    return "";
}

std::string CTinyxml2Serializer::SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable)
{
    //CTinyxml2 oXML;

    //oXML.AddElem("QTM_Settings");
    //oXML.IntoElem();
    //oXML.AddElem("General");
    //oXML.IntoElem();

    //oXML.AddElem("Camera");
    //oXML.IntoElem();

    //AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    //oXML.AddElem("AutoWhiteBalance", pEnable ? "true" : "false");

    //oXML.OutOfElem(); // Camera
    //oXML.OutOfElem(); // General
    //oXML.OutOfElem(); // QTM_Settings

    //return oXML.GetDoc();

    return "";
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

bool CTinyxml2Deserializer::ReadXMLDegreesOfFreedom(tinyxml2::XMLDocument& xml, const std::string& element, std::vector<SDegreeOfFreedom>& degreesOfFreedom)
{
    //SDegreeOfFreedom degreeOfFreedom;

    //if (xml.FindElem(element.c_str()))
    //{
    //    degreeOfFreedom.type = SkeletonStringToDofSettings(element);
    //    ParseString(xml.GetAttrib("LowerBound"), degreeOfFreedom.lowerBound);
    //    ParseString(xml.GetAttrib("UpperBound"), degreeOfFreedom.upperBound);
    //    xml.IntoElem();
    //    if (xml.FindElem("Constraint"))
    //    {
    //        ParseString(xml.GetAttrib("LowerBound"), degreeOfFreedom.lowerBound);
    //        ParseString(xml.GetAttrib("UpperBound"), degreeOfFreedom.upperBound);
    //    }
    //    if (xml.FindElem("Couplings"))
    //    {
    //        xml.IntoElem();
    //        while (xml.FindElem("Coupling"))
    //        {
    //            SCoupling coupling;
    //            coupling.segment = xml.GetAttrib("Segment");
    //            auto dof = xml.GetAttrib("DegreeOfFreedom");
    //            coupling.degreeOfFreedom = SkeletonStringToDofSettings(dof);
    //            ParseString(xml.GetAttrib("Coefficient"), coupling.coefficient);
    //            degreeOfFreedom.couplings.push_back(coupling);
    //        }
    //        xml.OutOfElem();
    //    }
    //    if (xml.FindElem("Goal"))
    //    {
    //        ParseString(xml.GetAttrib("Value"), degreeOfFreedom.goalValue);
    //        ParseString(xml.GetAttrib("Weight"), degreeOfFreedom.goalWeight);
    //    }
    //    xml.OutOfElem();
    //    xml.ResetMainPos();

    //    degreesOfFreedom.push_back(degreeOfFreedom);
    //    return true;
    //}

    return false;
}
