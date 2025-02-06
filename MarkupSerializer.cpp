#define _CRT_SECURE_NO_WARNINGS

#include "MarkupSerializer.h"

#include <algorithm>
#include <map>

#include "Settings.h"
#include "Markup.h"
#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

void CMarkupSerializer::AddXMLElementBool(CMarkup* xmlDocument, const char* tTag, const bool* pbValue, const char* tTrue, const char* tFalse)
{
    if (pbValue)
    {
        xmlDocument->AddElem(tTag, *pbValue ? tTrue : tFalse);
    }
}


void CMarkupSerializer::AddXMLElementBool(CMarkup* xmlDocument, const char* tTag, const bool pbValue, const char* tTrue, const char* tFalse)
{
    xmlDocument->AddElem(tTag, pbValue ? tTrue : tFalse);
}


void CMarkupSerializer::AddXMLElementInt(CMarkup* xmlDocument, const char* tTag, const int* pnValue)
{
    if (pnValue)
    {
        std::string tVal;

        tVal = CMarkup::Format("%d", *pnValue);
        xmlDocument->AddElem(tTag, tVal.c_str());
    }
}


void CMarkupSerializer::AddXMLElementUnsignedInt(CMarkup* xmlDocument, const char* tTag, const unsigned int value)
{
    std::string tVal = CMarkup::Format("%u", value);
    xmlDocument->AddElem(tTag, tVal.c_str());
}

void CMarkupSerializer::AddXMLElementUnsignedInt(CMarkup* xmlDocument, const char* tTag, const unsigned int* pnValue)
{
    if (pnValue)
    {
        AddXMLElementUnsignedInt(xmlDocument, tTag, *pnValue);
    }
}

void CMarkupSerializer::AddXMLElementFloat(CMarkup* xmlDocument, const char* tTag, const float* pfValue, unsigned int pnDecimals)
{
    if (pfValue)
    {
        std::string tVal;
        char fFormat[10];

        sprintf(fFormat, "%%.%df", pnDecimals);
        tVal = CMarkup::Format(fFormat, *pfValue);
        xmlDocument->AddElem(tTag, tVal.c_str());
    }
}

void CMarkupSerializer::AddXMLElementTransform(CMarkup& xml, const std::string& name, const SPosition& position, const SRotation& rotation)
{
    xml.AddElem(name.c_str());
    xml.IntoElem();

    xml.AddElem("Position");
    xml.AddAttrib("X", std::to_string(position.x).c_str());
    xml.AddAttrib("Y", std::to_string(position.y).c_str());
    xml.AddAttrib("Z", std::to_string(position.z).c_str());

    xml.AddElem("Rotation");
    xml.AddAttrib("X", std::to_string(rotation.x).c_str());
    xml.AddAttrib("Y", std::to_string(rotation.y).c_str());
    xml.AddAttrib("Z", std::to_string(rotation.z).c_str());
    xml.AddAttrib("W", std::to_string(rotation.w).c_str());

    xml.OutOfElem();
}

void CMarkupSerializer::AddXMLElementDOF(CMarkup& xml, const std::string& name, SDegreeOfFreedom degreeOfFreedoms)
{
    xml.AddElem(name.c_str());
    if (!std::isnan(degreeOfFreedoms.lowerBound) && !std::isnan(degreeOfFreedoms.upperBound))
    {
        if (mMajorVersion > 1 || mMinorVersion > 21)
        {
            xml.IntoElem();
            xml.AddElem("Constraint");
        }
        xml.AddAttrib("LowerBound", std::to_string(degreeOfFreedoms.lowerBound).c_str());
        xml.AddAttrib("UpperBound", std::to_string(degreeOfFreedoms.upperBound).c_str());
    }

    if (std::isnan(degreeOfFreedoms.lowerBound) || std::isnan(degreeOfFreedoms.upperBound) || (mMajorVersion == 1 && mMinorVersion < 22))
    {
        xml.IntoElem();
    }

    if (!degreeOfFreedoms.couplings.empty())
    {
        xml.AddElem("Couplings");
        xml.IntoElem();
        {
            for (const auto& coupling : degreeOfFreedoms.couplings)
            {
                xml.AddElem("Coupling");
                xml.AddAttrib("Segment", coupling.segment.c_str());
                xml.AddAttrib("DegreeOfFreedom", SkeletonDofToStringSettings(coupling.degreeOfFreedom));
                xml.AddAttrib("Coefficient", std::to_string(coupling.coefficient).c_str());
            }
        }
        xml.OutOfElem(); // Couplings
    }

    if (!std::isnan(degreeOfFreedoms.goalValue) && !std::isnan(degreeOfFreedoms.goalWeight))
    {
        xml.AddElem("Goal");
        xml.AddAttrib("Value", std::to_string(degreeOfFreedoms.goalValue).c_str());
        xml.AddAttrib("Weight", std::to_string(degreeOfFreedoms.goalWeight).c_str());
    }
    xml.OutOfElem();
}

bool CMarkupDeserializer::CompareNoCase(std::string tStr1, const char* tStr2) const
{
    tStr1 = ToLower(tStr1);
    return tStr1.compare(tStr2) == 0;
}

std::string CMarkupDeserializer::ToLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
    return str;
}

bool CMarkupDeserializer::ParseString(const std::string& str, std::uint32_t& value)
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

bool CMarkupDeserializer::ParseString(const std::string& str, std::int32_t& value)
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

bool CMarkupDeserializer::ParseString(const std::string& str, float& value)
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

bool CMarkupDeserializer::ParseString(const std::string& str, double& value)
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

bool CMarkupDeserializer::ParseString(const std::string& str, bool& value)
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
bool CMarkupDeserializer::ReadXmlBool(CMarkup* xml, const std::string& element, bool& value) const
{
    if (!xml->FindChildElem(element.c_str()))
    {
        return false;
    }

    auto str = xml->GetChildData();
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

SPosition CMarkupDeserializer::ReadXMLPosition(CMarkup& xml, const std::string& element)
{
    SPosition position;

    if (xml.FindElem(element.c_str()))
    {
        ParseString(xml.GetAttrib("X"), position.x);
        ParseString(xml.GetAttrib("Y"), position.y);
        ParseString(xml.GetAttrib("Z"), position.z);
        xml.ResetMainPos();
    }
    return position;
}

SRotation CMarkupDeserializer::ReadXMLRotation(CMarkup& xml, const std::string& element)
{
    SRotation rotation;

    if (xml.FindElem(element.c_str()))
    {
        ParseString(xml.GetAttrib("X"), rotation.x);
        ParseString(xml.GetAttrib("Y"), rotation.y);
        ParseString(xml.GetAttrib("Z"), rotation.z);
        ParseString(xml.GetAttrib("W"), rotation.w);
        xml.ResetMainPos();
    }
    return rotation;
}


CMarkupDeserializer::CMarkupDeserializer(const char* pData, std::uint32_t pMajorVersion, std::uint32_t pMinorVersion)
    : mMajorVersion(pMajorVersion), mMinorVersion(pMinorVersion), mErrorStr{0}, xmlDocument(pData)
{
}

bool CMarkupDeserializer::DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings)
{
    std::string             str;

    pGeneralSettings.vsCameras.clear();

    // ==================== General ====================
    if (!xmlDocument.FindChildElem("General"))
    {
        return false;
    }
    xmlDocument.IntoElem();

    if (!xmlDocument.FindChildElem("Frequency"))
    {
        return false;
    }
    pGeneralSettings.nCaptureFrequency = atoi(xmlDocument.GetChildData().c_str());

    if (!xmlDocument.FindChildElem("Capture_Time"))
    {
        return false;
    }
    pGeneralSettings.fCaptureTime = (float)atof(xmlDocument.GetChildData().c_str());

    // Refactored variant of all this copy/paste code. TODO: Refactor everything else.
    if (!ReadXmlBool(&xmlDocument, "Start_On_External_Trigger", pGeneralSettings.bStartOnExternalTrigger))
    {
        return false;
    }
    if (mMajorVersion > 1 || mMinorVersion > 14)
    {
        if (!ReadXmlBool(&xmlDocument, "Start_On_Trigger_NO", pGeneralSettings.bStartOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(&xmlDocument, "Start_On_Trigger_NC", pGeneralSettings.bStartOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(&xmlDocument, "Start_On_Trigger_Software", pGeneralSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }

    // ==================== External time base ====================
    if (!xmlDocument.FindChildElem("External_Time_Base"))
    {
        return false;
    }
    xmlDocument.IntoElem();

    if (!xmlDocument.FindChildElem("Enabled"))
    {
        return false;
    }
    str = ToLower(xmlDocument.GetChildData());
    pGeneralSettings.sExternalTimebase.bEnabled = (str == "true");

    if (!xmlDocument.FindChildElem("Signal_Source"))
    {
        return false;
    }
    str = ToLower(xmlDocument.GetChildData());
    if (str == "control port")
    {
        pGeneralSettings.sExternalTimebase.eSignalSource = SourceControlPort;
    }
    else if (str == "ir receiver")
    {
        pGeneralSettings.sExternalTimebase.eSignalSource = SourceIRReceiver;
    }
    else if (str == "smpte")
    {
        pGeneralSettings.sExternalTimebase.eSignalSource = SourceSMPTE;
    }
    else if (str == "irig")
    {
        pGeneralSettings.sExternalTimebase.eSignalSource = SourceIRIG;
    }
    else if (str == "video sync")
    {
        pGeneralSettings.sExternalTimebase.eSignalSource = SourceVideoSync;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Signal_Mode"))
    {
        return false;
    }
    str = ToLower(xmlDocument.GetChildData());
    if (str == "periodic")
    {
        pGeneralSettings.sExternalTimebase.bSignalModePeriodic = true;
    }
    else if (str == "non-periodic")
    {
        pGeneralSettings.sExternalTimebase.bSignalModePeriodic = false;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Frequency_Multiplier"))
    {
        return false;
    }
    unsigned int nMultiplier;
    str = xmlDocument.GetChildData();
    if (sscanf(str.c_str(), "%u", &nMultiplier) == 1)
    {
        pGeneralSettings.sExternalTimebase.nFreqMultiplier = nMultiplier;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Frequency_Divisor"))
    {
        return false;
    }
    unsigned int nDivisor;
    str = xmlDocument.GetChildData();
    if (sscanf(str.c_str(), "%u", &nDivisor) == 1)
    {
        pGeneralSettings.sExternalTimebase.nFreqDivisor = nDivisor;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Frequency_Tolerance"))
    {
        return false;
    }
    unsigned int nTolerance;
    str = xmlDocument.GetChildData();
    if (sscanf(str.c_str(), "%u", &nTolerance) == 1)
    {
        pGeneralSettings.sExternalTimebase.nFreqTolerance = nTolerance;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Nominal_Frequency"))
    {
        return false;
    }
    str = ToLower(xmlDocument.GetChildData());

    if (str == "none")
    {
        pGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
    }
    else
    {
        float fFrequency;
        if (sscanf(str.c_str(), "%f", &fFrequency) == 1)
        {
            pGeneralSettings.sExternalTimebase.fNominalFrequency = fFrequency;
        }
        else
        {
            return false;
        }
    }

    if (!xmlDocument.FindChildElem("Signal_Edge"))
    {
        return false;
    }
    str = ToLower(xmlDocument.GetChildData());
    if (str == "negative")
    {
        pGeneralSettings.sExternalTimebase.bNegativeEdge = true;
    }
    else if (str == "positive")
    {
        pGeneralSettings.sExternalTimebase.bNegativeEdge = false;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Signal_Shutter_Delay"))
    {
        return false;
    }
    unsigned int nDelay;
    str = xmlDocument.GetChildData();
    if (sscanf(str.c_str(), "%u", &nDelay) == 1)
    {
        pGeneralSettings.sExternalTimebase.nSignalShutterDelay = nDelay;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("Non_Periodic_Timeout"))
    {
        return false;
    }
    float fTimeout;
    str = xmlDocument.GetChildData();
    if (sscanf(str.c_str(), "%f", &fTimeout) == 1)
    {
        pGeneralSettings.sExternalTimebase.fNonPeriodicTimeout = fTimeout;
    }
    else
    {
        return false;
    }

    xmlDocument.OutOfElem(); // External_Time_Base


    // External_Timestamp
    if (xmlDocument.FindChildElem("External_Timestamp"))
    {
        xmlDocument.IntoElem();

        if (xmlDocument.FindChildElem("Enabled"))
        {
            str = ToLower(xmlDocument.GetChildData());
            pGeneralSettings.sTimestamp.bEnabled = (str == "true");
        }
        if (xmlDocument.FindChildElem("Type"))
        {
            str = ToLower(xmlDocument.GetChildData());
            if (str == "smpte")
            {
                pGeneralSettings.sTimestamp.nType = Timestamp_SMPTE;
            }
            else if (str == "irig")
            {
                pGeneralSettings.sTimestamp.nType = Timestamp_IRIG;
            }
            else
            {
                pGeneralSettings.sTimestamp.nType = Timestamp_CameraTime;
            }
        }
        if (xmlDocument.FindChildElem("Frequency"))
        {
            unsigned int timestampFrequency;
            str = xmlDocument.GetChildData();
            if (sscanf(str.c_str(), "%u", &timestampFrequency) == 1)
            {
                pGeneralSettings.sTimestamp.nFrequency = timestampFrequency;
            }
        }
        xmlDocument.OutOfElem();
    }
    // External_Timestamp


    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    EProcessingActions* processingActions[3] =
    {
        &pGeneralSettings.eProcessingActions,
        &pGeneralSettings.eRtProcessingActions,
        &pGeneralSettings.eReprocessingActions
    };
    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================
        if (!xmlDocument.FindChildElem(processings[i]))
        {
            return false;
        }
        xmlDocument.IntoElem();

        *processingActions[i] = ProcessingNone;

        if (mMajorVersion > 1 || mMinorVersion > 13)
        {
            if (!xmlDocument.FindChildElem("PreProcessing2D"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingPreProcess2D);
            }
        }

        if (!xmlDocument.FindChildElem("Tracking"))
        {
            return false;
        }
        str = ToLower(xmlDocument.GetChildData());
        if (str == "3d")
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking3D);
        }
        else if (str == "2d" && i != 1) // i != 1 => Not RtProcessingSettings
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking2D);
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!xmlDocument.FindChildElem("TwinSystemMerge"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTwinSystemMerge);
            }

            if (!xmlDocument.FindChildElem("SplineFill"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingSplineFill);
            }
        }

        if (!xmlDocument.FindChildElem("AIM"))
        {
            return false;
        }
        if (CompareNoCase(xmlDocument.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingAIM);
        }

        if (!xmlDocument.FindChildElem("Track6DOF"))
        {
            return false;
        }
        if (CompareNoCase(xmlDocument.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + Processing6DOFTracking);
        }

        if (!xmlDocument.FindChildElem("ForceData"))
        {
            return false;
        }
        if (CompareNoCase(xmlDocument.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingForceData);
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            if (!xmlDocument.FindChildElem("GazeVector"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingGazeVector);
            }
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!xmlDocument.FindChildElem("ExportTSV"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportTSV);
            }

            if (!xmlDocument.FindChildElem("ExportC3D"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportC3D);
            }

            if (!xmlDocument.FindChildElem("ExportMatlabFile"))
            {
                return false;
            }
            if (CompareNoCase(xmlDocument.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportMatlabFile);
            }

            if (mMajorVersion > 1 || mMinorVersion > 11)
            {
                if (!xmlDocument.FindChildElem("ExportAviFile"))
                {
                    return false;
                }
                if (CompareNoCase(xmlDocument.GetChildData(), "true"))
                {
                    *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportAviFile);
                }
            }
        }
        xmlDocument.OutOfElem(); // Processing_Actions
    }

    if (xmlDocument.FindChildElem("EulerAngles"))
    {
        xmlDocument.IntoElem();
        pGeneralSettings.eulerRotations[0] = xmlDocument.GetAttrib("First");
        pGeneralSettings.eulerRotations[1] = xmlDocument.GetAttrib("Second");
        pGeneralSettings.eulerRotations[2] = xmlDocument.GetAttrib("Third");
        xmlDocument.OutOfElem();
    }

    SSettingsGeneralCamera cameraSettings;

    while (xmlDocument.FindChildElem("Camera"))
    {
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("ID"))
        {
            return false;
        }
        cameraSettings.nID = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Model"))
        {
            return false;
        }
        str = ToLower(xmlDocument.GetChildData());

        if (str == "macreflex")
        {
            cameraSettings.eModel = ModelMacReflex;
        }
        else if (str == "proreflex 120")
        {
            cameraSettings.eModel = ModelProReflex120;
        }
        else if (str == "proreflex 240")
        {
            cameraSettings.eModel = ModelProReflex240;
        }
        else if (str == "proreflex 500")
        {
            cameraSettings.eModel = ModelProReflex500;
        }
        else if (str == "proreflex 1000")
        {
            cameraSettings.eModel = ModelProReflex1000;
        }
        else if (str == "oqus 100")
        {
            cameraSettings.eModel = ModelOqus100;
        }
        else if (str == "oqus 200" || str == "oqus 200 c")
        {
            cameraSettings.eModel = ModelOqus200C;
        }
        else if (str == "oqus 300")
        {
            cameraSettings.eModel = ModelOqus300;
        }
        else if (str == "oqus 300 plus")
        {
            cameraSettings.eModel = ModelOqus300Plus;
        }
        else if (str == "oqus 400")
        {
            cameraSettings.eModel = ModelOqus400;
        }
        else if (str == "oqus 500")
        {
            cameraSettings.eModel = ModelOqus500;
        }
        else if (str == "oqus 500 plus")
        {
            cameraSettings.eModel = ModelOqus500Plus;
        }
        else if (str == "oqus 700")
        {
            cameraSettings.eModel = ModelOqus700;
        }
        else if (str == "oqus 700 plus")
        {
            cameraSettings.eModel = ModelOqus700Plus;
        }
        else if (str == "oqus 600 plus")
        {
            cameraSettings.eModel = ModelOqus600Plus;
        }
        else if (str == "miqus m1")
        {
            cameraSettings.eModel = ModelMiqusM1;
        }
        else if (str == "miqus m3")
        {
            cameraSettings.eModel = ModelMiqusM3;
        }
        else if (str == "miqus m5")
        {
            cameraSettings.eModel = ModelMiqusM5;
        }
        else if (str == "miqus sync unit")
        {
            cameraSettings.eModel = ModelMiqusSyncUnit;
        }
        else if (str == "miqus video")
        {
            cameraSettings.eModel = ModelMiqusVideo;
        }
        else if (str == "miqus video color")
        {
            cameraSettings.eModel = ModelMiqusVideoColor;
        }
        else if (str == "miqus hybrid")
        {
            cameraSettings.eModel = ModelMiqusHybrid;
        }
        else if (str == "miqus video color plus")
        {
            cameraSettings.eModel = ModelMiqusVideoColorPlus;
        }
        else if (str == "arqus a5")
        {
            cameraSettings.eModel = ModelArqusA5;
        }
        else if (str == "arqus a9")
        {
            cameraSettings.eModel = ModelArqusA9;
        }
        else if (str == "arqus a12")
        {
            cameraSettings.eModel = ModelArqusA12;
        }
        else if (str == "arqus a26")
        {
            cameraSettings.eModel = ModelArqusA26;
        }
        else
        {
            cameraSettings.eModel = ModelUnknown;
        }

        // Only available from protocol version 1.10 and later.
        if (xmlDocument.FindChildElem("Underwater"))
        {
            str = ToLower(xmlDocument.GetChildData());
            cameraSettings.bUnderwater = (str == "true");
        }

        if (xmlDocument.FindChildElem("Supports_HW_Sync"))
        {
            str = ToLower(xmlDocument.GetChildData());
            cameraSettings.bSupportsHwSync = (str == "true");
        }

        if (!xmlDocument.FindChildElem("Serial"))
        {
            return false;
        }
        cameraSettings.nSerial = atoi(xmlDocument.GetChildData().c_str());

        // ==================== Camera Mode ====================
        if (!xmlDocument.FindChildElem("Mode"))
        {
            return false;
        }
        str = ToLower(xmlDocument.GetChildData());
        if (str == "marker")
        {
            cameraSettings.eMode = ModeMarker;
        }
        else if (str == "marker intensity")
        {
            cameraSettings.eMode = ModeMarkerIntensity;
        }
        else if (str == "video")
        {
            cameraSettings.eMode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            // ==================== Video frequency ====================
            if (!xmlDocument.FindChildElem("Video_Frequency"))
            {
                return false;
            }
            cameraSettings.nVideoFrequency = atoi(xmlDocument.GetChildData().c_str());
        }

        // ==================== Video Resolution ====================
        if (xmlDocument.FindChildElem("Video_Resolution"))
        {
            str = ToLower(xmlDocument.GetChildData());
            if (str == "1440p")
            {
                cameraSettings.eVideoResolution = VideoResolution1440p;
            }
            else if (str == "1080p")
            {
                cameraSettings.eVideoResolution = VideoResolution1080p;
            }
            else if (str == "720p")
            {
                cameraSettings.eVideoResolution = VideoResolution720p;
            }
            else if (str == "540p")
            {
                cameraSettings.eVideoResolution = VideoResolution540p;
            }
            else if (str == "480p")
            {
                cameraSettings.eVideoResolution = VideoResolution480p;
            }
            else
            {
                return false;
            }
        }
        else
        {
            cameraSettings.eVideoResolution = VideoResolutionNone;
        }

        // ==================== Video AspectRatio ====================
        if (xmlDocument.FindChildElem("Video_Aspect_Ratio"))
        {
            str = ToLower(xmlDocument.GetChildData());
            if (str == "16x9")
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
            }
            else if (str == "4x3")
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
            }
            else if (str == "1x1")
            {
                cameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
            }
        }
        else
        {
            cameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
        }

        // ==================== Video exposure ====================
        if (!xmlDocument.FindChildElem("Video_Exposure"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Current"))
        {
            return false;
        }
        cameraSettings.nVideoExposure = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Min"))
        {
            return false;
        }
        cameraSettings.nVideoExposureMin = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Max"))
        {
            return false;
        }
        cameraSettings.nVideoExposureMax = atoi(xmlDocument.GetChildData().c_str());
        xmlDocument.OutOfElem(); // Video_Exposure

        // ==================== Video flash time ====================
        if (!xmlDocument.FindChildElem("Video_Flash_Time"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Current"))
        {
            return false;
        }
        cameraSettings.nVideoFlashTime = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Min"))
        {
            return false;
        }
        cameraSettings.nVideoFlashTimeMin = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Max"))
        {
            return false;
        }
        cameraSettings.nVideoFlashTimeMax = atoi(xmlDocument.GetChildData().c_str());
        xmlDocument.OutOfElem(); // Video_Flash_Time

        // ==================== Marker exposure ====================
        if (!xmlDocument.FindChildElem("Marker_Exposure"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Current"))
        {
            return false;
        }
        cameraSettings.nMarkerExposure = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Min"))
        {
            return false;
        }
        cameraSettings.nMarkerExposureMin = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Max"))
        {
            return false;
        }
        cameraSettings.nMarkerExposureMax = atoi(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Marker_Exposure

        // ==================== Marker threshold ====================
        if (!xmlDocument.FindChildElem("Marker_Threshold"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Current"))
        {
            return false;
        }
        cameraSettings.nMarkerThreshold = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Min"))
        {
            return false;
        }
        cameraSettings.nMarkerThresholdMin = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Max"))
        {
            return false;
        }
        cameraSettings.nMarkerThresholdMax = atoi(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Marker_Threshold

        // ==================== Position ====================
        if (!xmlDocument.FindChildElem("Position"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("X"))
        {
            return false;
        }
        cameraSettings.fPositionX = (float)atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Y"))
        {
            return false;
        }
        cameraSettings.fPositionY = (float)atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Z"))
        {
            return false;
        }
        cameraSettings.fPositionZ = (float)atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_1_1"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[0][0] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_2_1"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[1][0] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_3_1"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[2][0] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_1_2"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[0][1] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_2_2"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[1][1] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_3_2"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[2][1] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_1_3"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[0][2] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_2_3"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[1][2] = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Rot_3_3"))
        {
            return false;
        }
        cameraSettings.fPositionRotMatrix[2][2] = (float)atof(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Position


        if (!xmlDocument.FindChildElem("Orientation"))
        {
            return false;
        }
        cameraSettings.nOrientation = atoi(xmlDocument.GetChildData().c_str());

        // ==================== Marker resolution ====================
        if (!xmlDocument.FindChildElem("Marker_Res"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Width"))
        {
            return false;
        }
        cameraSettings.nMarkerResolutionWidth = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Height"))
        {
            return false;
        }
        cameraSettings.nMarkerResolutionHeight = atoi(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Marker_Res

        // ==================== Video resolution ====================
        if (!xmlDocument.FindChildElem("Video_Res"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Width"))
        {
            return false;
        }
        cameraSettings.nVideoResolutionWidth = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Height"))
        {
            return false;
        }
        cameraSettings.nVideoResolutionHeight = atoi(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Video_Res

        // ==================== Marker FOV ====================
        if (!xmlDocument.FindChildElem("Marker_FOV"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Left"))
        {
            return false;
        }
        cameraSettings.nMarkerFOVLeft = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Top"))
        {
            return false;
        }
        cameraSettings.nMarkerFOVTop = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Right"))
        {
            return false;
        }
        cameraSettings.nMarkerFOVRight = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Bottom"))
        {
            return false;
        }
        cameraSettings.nMarkerFOVBottom = atoi(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Marker_FOV

        // ==================== Video FOV ====================
        if (!xmlDocument.FindChildElem("Video_FOV"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Left"))
        {
            return false;
        }
        cameraSettings.nVideoFOVLeft = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Top"))
        {
            return false;
        }
        cameraSettings.nVideoFOVTop = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Right"))
        {
            return false;
        }
        cameraSettings.nVideoFOVRight = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Bottom"))
        {
            return false;
        }
        cameraSettings.nVideoFOVBottom = atoi(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // Video_FOV

        // ==================== Sync out ====================
        // Only available from protocol version 1.10 and later.
        for (int port = 0; port < 3; port++)
        {
            char syncOutStr[16];
            sprintf(syncOutStr, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));
            if (xmlDocument.FindChildElem(syncOutStr))
            {
                xmlDocument.IntoElem();

                if (port < 2)
                {
                    if (!xmlDocument.FindChildElem("Mode"))
                    {
                        return false;
                    }
                    str = ToLower(xmlDocument.GetChildData());
                    if (str == "shutter out")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeShutterOut;
                    }
                    else if (str == "multiplier")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeMultiplier;
                    }
                    else if (str == "divisor")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeDivisor;
                    }
                    else if (str == "camera independent")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (str == "measurement time")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (str == "continuous 100hz")
                    {
                        cameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
                    }
                    else if (str == "system live time")
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
                        if (!xmlDocument.FindChildElem("Value"))
                        {
                            return false;
                        }
                        cameraSettings.nSyncOutValue[port] = atoi(xmlDocument.GetChildData().c_str());

                        if (!xmlDocument.FindChildElem("Duty_Cycle"))
                        {
                            return false;
                        }
                        cameraSettings.fSyncOutDutyCycle[port] = (float)atof(xmlDocument.GetChildData().c_str());
                    }
                }
                if (port == 2 ||
                    (cameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    if (!xmlDocument.FindChildElem("Signal_Polarity"))
                    {
                        return false;
                    }
                    if (CompareNoCase(xmlDocument.GetChildData(), "negative"))
                    {
                        cameraSettings.bSyncOutNegativePolarity[port] = true;
                    }
                    else
                    {
                        cameraSettings.bSyncOutNegativePolarity[port] = false;
                    }
                }
                xmlDocument.OutOfElem(); // Sync_Out
            }
            else
            {
                cameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                cameraSettings.nSyncOutValue[port] = 0;
                cameraSettings.fSyncOutDutyCycle[port] = 0;
                cameraSettings.bSyncOutNegativePolarity[port] = false;
            }
        }

        if (xmlDocument.FindChildElem("LensControl"))
        {
            xmlDocument.IntoElem();
            if (xmlDocument.FindChildElem("Focus"))
            {
                xmlDocument.IntoElem();
                float focus;
                if (sscanf(xmlDocument.GetAttrib("Value").c_str(), "%f", &focus) == 1)
                {
                    cameraSettings.fFocus = focus;
                }
                xmlDocument.OutOfElem();
            }
            if (xmlDocument.FindChildElem("Aperture"))
            {
                xmlDocument.IntoElem();
                float aperture;
                if (sscanf(xmlDocument.GetAttrib("Value").c_str(), "%f", &aperture) == 1)
                {
                    cameraSettings.fAperture = aperture;
                }
                xmlDocument.OutOfElem();
            }
            xmlDocument.OutOfElem();
        }
        else
        {
            cameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            cameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        if (xmlDocument.FindChildElem("AutoExposure"))
        {
            xmlDocument.IntoElem();
            if (CompareNoCase(xmlDocument.GetAttrib("Enabled"), "true"))
            {
                cameraSettings.autoExposureEnabled = true;
            }
            float autoExposureCompensation;
            if (sscanf(xmlDocument.GetAttrib("Compensation").c_str(), "%f", &autoExposureCompensation) == 1)
            {
                cameraSettings.autoExposureCompensation = autoExposureCompensation;
            }
            xmlDocument.OutOfElem();
        }
        else
        {
            cameraSettings.autoExposureEnabled = false;
            cameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        if (xmlDocument.FindChildElem("AutoWhiteBalance"))
        {
            cameraSettings.autoWhiteBalance = CompareNoCase(xmlDocument.GetChildData().c_str(), "true") ? 1 : 0;
        }
        else
        {
            cameraSettings.autoWhiteBalance = -1;
        }

        xmlDocument.OutOfElem(); // Camera

        pGeneralSettings.vsCameras.push_back(cameraSettings);
    }

    return true;

}

bool CMarkupDeserializer::Deserialize3DSettings(SSettings3D& p3dSettings, bool& pDataAvailable)
{
    std::string str;

    pDataAvailable = false;

    p3dSettings.s3DLabels.clear();
    p3dSettings.pCalibrationTime[0] = 0;

    if (!xmlDocument.FindChildElem("The_3D"))
    {
        // No 3D data available.
        return true;
    }
    xmlDocument.IntoElem();

    if (!xmlDocument.FindChildElem("AxisUpwards"))
    {
        return false;
    }
    str = ToLower(xmlDocument.GetChildData());

    if (str == "+x")
    {
        p3dSettings.eAxisUpwards = XPos;
    }
    else if (str == "-x")
    {
        p3dSettings.eAxisUpwards = XNeg;
    }
    else if (str == "+y")
    {
        p3dSettings.eAxisUpwards = YPos;
    }
    else if (str == "-y")
    {
        p3dSettings.eAxisUpwards = YNeg;
    }
    else if (str == "+z")
    {
        p3dSettings.eAxisUpwards = ZPos;
    }
    else if (str == "-z")
    {
        p3dSettings.eAxisUpwards = ZNeg;
    }
    else
    {
        return false;
    }

    if (!xmlDocument.FindChildElem("CalibrationTime"))
    {
        return false;
    }
    str = xmlDocument.GetChildData();
    strcpy(p3dSettings.pCalibrationTime, str.c_str());

    if (!xmlDocument.FindChildElem("Labels"))
    {
        return false;
    }
    unsigned int nNumberOfLabels = atoi(xmlDocument.GetChildData().c_str());

    p3dSettings.s3DLabels.resize(nNumberOfLabels);
    SSettings3DLabel sLabel;

    for (unsigned int iLabel = 0; iLabel < nNumberOfLabels; iLabel++)
    {
        if (xmlDocument.FindChildElem("Label"))
        {
            xmlDocument.IntoElem();
            if (xmlDocument.FindChildElem("Name"))
            {
                sLabel.oName = xmlDocument.GetChildData();
                if (xmlDocument.FindChildElem("RGBColor"))
                {
                    sLabel.nRGBColor = atoi(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Trajectory_Type"))
                {
                    sLabel.type = xmlDocument.GetChildData();
                }
                p3dSettings.s3DLabels[iLabel] = sLabel;
            }
            xmlDocument.OutOfElem();
        }
        else
        {
            return false;
        }
    }

    p3dSettings.sBones.clear();
    if (xmlDocument.FindChildElem("Bones"))
    {
        xmlDocument.IntoElem();
        while (xmlDocument.FindChildElem("Bone"))
        {
            xmlDocument.IntoElem();
            SSettingsBone bone = { };
            bone.fromName = xmlDocument.GetAttrib("From").c_str();
            bone.toName = xmlDocument.GetAttrib("To").c_str();

            auto colorString = xmlDocument.GetAttrib("Color");
            if (!colorString.empty())
            {
                bone.color = atoi(colorString.c_str());
            }
            p3dSettings.sBones.push_back(bone);
            xmlDocument.OutOfElem();
        }
        xmlDocument.OutOfElem();
    }

    pDataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryReadSetEnabled(const int majorVer, const int minorVer, CMarkup& xmlDocument, bool& target)
    {
        if (majorVer > 1 || minorVer > 23)
        {
            if (!xmlDocument.FindChildElem("Enabled"))
            {
                target = true;
                return true;
            }

            target = xmlDocument.GetChildData() == "true" ? true : false;
            return false;
        }

        return false;
    }

    bool TryReadSetName(CMarkup& xmlDocument, std::string& target)
    {
        if (!xmlDocument.FindChildElem("Name"))
        {
            return false;
        }
        target = xmlDocument.GetChildData();
        return true;
    }

    bool TryReadSetColor(CMarkup& xmlDocument, std::uint32_t& target)
    {
        if (!xmlDocument.FindChildElem("Color"))
        {
            return false;
        }
        std::uint32_t colorR = atoi(xmlDocument.GetChildAttrib("R").c_str());
        std::uint32_t colorG = atoi(xmlDocument.GetChildAttrib("G").c_str());
        std::uint32_t colorB = atoi(xmlDocument.GetChildAttrib("B").c_str());
        target = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);

        return true;
    }

    bool TryReadSetMaxResidual(CMarkup& xmlDocument, float& fTarget)
    {
        if (!xmlDocument.FindChildElem("MaximumResidual"))
        {
            return false;
        }
        fTarget = (float)atof(xmlDocument.GetChildData().c_str());

        return true;
    }

    bool TryReadSetMinMarkersInBody(CMarkup& xmlDocument, std::uint32_t& target)
    {
        if (!xmlDocument.FindChildElem("MinimumMarkersInBody"))
        {
            return false;
        }
        target = atoi(xmlDocument.GetChildData().c_str());

        return true;
    }

    bool TryReadSetBoneLenTolerance(CMarkup& xmlDocument, float& fTarget)
    {
        if (!xmlDocument.FindChildElem("BoneLengthTolerance"))
        {
            return false;
        }
        fTarget = (float)atof(xmlDocument.GetChildData().c_str());

        return true;
    }

    bool TryReadSetFilter(CMarkup& xmlDocument, std::string& target)
    {
        if (!xmlDocument.FindChildElem("Filter"))
        {
            return false;
        }
        target = xmlDocument.GetChildAttrib("Preset");

        return true;
    }

    bool TryReadSetPos(CMarkup& xmlDocument, float& targetX, float& targetY, float& targetZ)
    {
        if (!xmlDocument.FindChildElem("Position"))
        {
            return false;
        }
        targetX = (float)atof(xmlDocument.GetChildAttrib("X").c_str());
        targetY = (float)atof(xmlDocument.GetChildAttrib("Y").c_str());
        targetZ = (float)atof(xmlDocument.GetChildAttrib("Z").c_str());

        return true;
    }

    bool TryReadSetRotation(CMarkup& xmlDocument, float& targetX, float& targetY, float& targetZ)
    {
        if (!xmlDocument.FindChildElem("Rotation"))
        {
            return false;
        }
        targetX = (float)atof(xmlDocument.GetChildAttrib("X").c_str());
        targetY = (float)atof(xmlDocument.GetChildAttrib("Y").c_str());
        targetZ = (float)atof(xmlDocument.GetChildAttrib("Z").c_str());

        return true;
    }

    bool TryReadSetScale(CMarkup& xmlDocument, float& fTarget)
    {
        if (!xmlDocument.FindChildElem("Scale"))
        {
            return false;
        }
        fTarget = (float)atof(xmlDocument.GetChildData().c_str());

        return true;
    }

    bool TryReadSetOpacity(CMarkup& xmlDocument, float& fTarget)
    {
        if (!xmlDocument.FindChildElem("Opacity"))
        {
            return false;
        }
        fTarget = (float)atof(xmlDocument.GetChildData().c_str());

        return true;
    }

    bool TryReadSetPoints(CMarkup& xmlDocument, std::vector<SBodyPoint>& target)
    {
        if (xmlDocument.FindChildElem("Points"))
        {
            xmlDocument.IntoElem();

            while (xmlDocument.FindChildElem("Point"))
            {
                SBodyPoint bodyPoint;

                bodyPoint.fX = (float)atof(xmlDocument.GetChildAttrib("X").c_str());
                bodyPoint.fY = (float)atof(xmlDocument.GetChildAttrib("Y").c_str());
                bodyPoint.fZ = (float)atof(xmlDocument.GetChildAttrib("Z").c_str());

                bodyPoint.virtual_ = (0 != atoi(xmlDocument.GetChildAttrib("Virtual").c_str()));
                bodyPoint.physicalId = atoi(xmlDocument.GetChildAttrib("PhysicalId").c_str());
                bodyPoint.name = xmlDocument.GetChildAttrib("Name");
                target.push_back(bodyPoint);
            }
            xmlDocument.OutOfElem(); // Points

            return true;
        }

        return false;
    }

    bool TryReadSetDataOrigin(CMarkup& xmlDocument, SOrigin& target)
    {
        if (!xmlDocument.FindChildElem("Data_origin"))
        {
            return false;
        }
        target.type = (EOriginType)atoi(xmlDocument.GetChildData().c_str());
        target.position.fX = (float)atof(xmlDocument.GetChildAttrib("X").c_str());
        target.position.fY = (float)atof(xmlDocument.GetChildAttrib("Y").c_str());
        target.position.fZ = (float)atof(xmlDocument.GetChildAttrib("Z").c_str());
        target.relativeBody = atoi(xmlDocument.GetChildAttrib("Relative_body").c_str());

        return true;
    }

    void ReadSetRotations(CMarkup& xmlDocument, SOrigin& target)
    {
        char tmpStr[10];
        for (std::uint32_t i = 0; i < 9; i++)
        {
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            target.rotation[i] = (float)atof(xmlDocument.GetChildAttrib(tmpStr).c_str());
        }
    }

    bool TryReadSetRGBColor(CMarkup& xmlDocument, std::uint32_t& target)
    {
        if (!xmlDocument.FindChildElem("RGBColor"))
        {
            return false;
        }
        target = atoi(xmlDocument.GetChildData().c_str());

        return true;
    }

    bool TryReadSetPointsOld(CMarkup& xmlDocument, std::vector<SBodyPoint>& target)
    {
        target.clear();

        while (xmlDocument.FindChildElem("Point"))
        {
            SBodyPoint point;

            xmlDocument.IntoElem();
            if (!xmlDocument.FindChildElem("X"))
            {
                return false;
            }
            point.fX = (float)atof(xmlDocument.GetChildData().c_str());

            if (!xmlDocument.FindChildElem("Y"))
            {
                return false;
            }
            point.fY = (float)atof(xmlDocument.GetChildData().c_str());

            if (!xmlDocument.FindChildElem("Z"))
            {
                return false;
            }
            point.fZ = (float)atof(xmlDocument.GetChildData().c_str());

            xmlDocument.OutOfElem(); // Point
            target.push_back(point);
        }

        return true;
    }

    bool TryReadSetEuler(CMarkup& xmlDocument, std::string& targetFirst, std::string& targetSecond, std::string& targetThird)
    {
        if (xmlDocument.FindChildElem("Euler"))
        {
            xmlDocument.IntoElem();
            if (!xmlDocument.FindChildElem("First"))
            {
                return false;
            }
            targetFirst = xmlDocument.GetChildData();
            if (!xmlDocument.FindChildElem("Second"))
            {
                return false;
            }
            targetSecond = xmlDocument.GetChildData();
            if (!xmlDocument.FindChildElem("Third"))
            {
                return false;
            }
            targetThird = xmlDocument.GetChildData();
            xmlDocument.OutOfElem(); // Euler
        }

        return true;
    }
}

bool CMarkupDeserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& p6DOFSettings, SSettingsGeneral& pGeneralSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    p6DOFSettings.clear();

    if (xmlDocument.FindChildElem("The_6D"))
    {
        xmlDocument.IntoElem();

        if (mMajorVersion > 1 || mMinorVersion > 20)
        {
            while (xmlDocument.FindChildElem("Body"))
            {
                SSettings6DOFBody bodySettings6Dof;
                SBodyPoint bodyPoint;

                xmlDocument.IntoElem();

                // NOTE: READ-ORDER MATTERS!!!
                if (!TryReadSetName(xmlDocument, bodySettings6Dof.name))
                { // Name --- REQUIRED
                    return false;
                }
                // Enabled --- NOT(!) REQUIRED
                TryReadSetEnabled(mMajorVersion, mMinorVersion, xmlDocument, bodySettings6Dof.enabled);
                if (!TryReadSetColor(xmlDocument, bodySettings6Dof.color)
                    || !TryReadSetMaxResidual(xmlDocument, bodySettings6Dof.maxResidual)
                    || !TryReadSetMinMarkersInBody(xmlDocument, bodySettings6Dof.minMarkersInBody)
                    || !TryReadSetBoneLenTolerance(xmlDocument, bodySettings6Dof.boneLengthTolerance)
                    || !TryReadSetFilter(xmlDocument, bodySettings6Dof.filterPreset))
                { // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                    return false;
                }

                if (xmlDocument.FindChildElem("Mesh"))
                {
                    xmlDocument.IntoElem();

                    if (!TryReadSetName(xmlDocument, bodySettings6Dof.mesh.name)
                        || !TryReadSetPos(xmlDocument, bodySettings6Dof.mesh.position.fX, bodySettings6Dof.mesh.position.fY, bodySettings6Dof.mesh.position.fZ)
                        || !TryReadSetRotation(xmlDocument, bodySettings6Dof.mesh.rotation.fX, bodySettings6Dof.mesh.rotation.fY, bodySettings6Dof.mesh.rotation.fZ)
                        || !TryReadSetScale(xmlDocument, bodySettings6Dof.mesh.scale)
                        || !TryReadSetOpacity(xmlDocument, bodySettings6Dof.mesh.opacity))
                    { // Name, Position, Rotation, Scale, Opacity --- REQUIRED
                        return false;
                    }

                    xmlDocument.OutOfElem(); // Mesh
                }

                // Points --- REQUIRED
                TryReadSetPoints(xmlDocument, bodySettings6Dof.points);
                if (!TryReadSetDataOrigin(xmlDocument, bodySettings6Dof.origin)
                    || !xmlDocument.FindChildElem("Data_orientation")
                    || bodySettings6Dof.origin.type != atoi(xmlDocument.GetChildData().c_str())
                    || bodySettings6Dof.origin.relativeBody != static_cast<std::uint32_t>(atoi(xmlDocument.GetChildAttrib("Relative_body").c_str()))
                    )
                { // Data Orientation, Origin Type / Relative Body --- REQUIRED
                    return false;
                }

                // Rotation values --- NOTE : Does NOT(!) 'Try'; just reads and sets (no boolean return)
                ReadSetRotations(xmlDocument, bodySettings6Dof.origin);

                p6DOFSettings.push_back(bodySettings6Dof);
                xmlDocument.OutOfElem(); // Body

                pDataAvailable = true;
            }
        }
        else
        {
            if (!xmlDocument.FindChildElem("Bodies"))
            {
                return false;
            }
            int nBodies = atoi(xmlDocument.GetChildData().c_str());
            SSettings6DOFBody bodySettings6Dof;

            for (int iBody = 0; iBody < nBodies; iBody++)
            {
                if (!xmlDocument.FindChildElem("Body"))
                {
                    return false;
                }
                xmlDocument.IntoElem();

                if (!TryReadSetName(xmlDocument, bodySettings6Dof.name)
                    || !TryReadSetRGBColor(xmlDocument, bodySettings6Dof.color)
                    || !TryReadSetPointsOld(xmlDocument, bodySettings6Dof.points))
                { // Name, RGBColor, Points(OLD) --- REQUIRED
                    return false;
                }

                p6DOFSettings.push_back(bodySettings6Dof);
                xmlDocument.OutOfElem(); // Body
            }
            if (mMajorVersion > 1 || mMinorVersion > 15)
            {
                if (!TryReadSetEuler(xmlDocument, pGeneralSettings.eulerRotations[0], pGeneralSettings.eulerRotations[1], pGeneralSettings.eulerRotations[2]))
                { // Euler --- REQUIRED
                    return false;
                }
            }
            pDataAvailable = true;
        }
    }

    return true;
} // Read6DOFSettings

bool CMarkupDeserializer::DeserializeGazeVectorSettings(std::vector<SGazeVector>& pGazeVectorSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pGazeVectorSettings.clear();

    //
    // Read gaze vectors
    //
    if (!xmlDocument.FindChildElem("Gaze_Vector"))
    {
        return true; // NO gaze vector data available.
    }
    xmlDocument.IntoElem();

    std::string tGazeVectorName;

    int nGazeVectorCount = 0;

    while (xmlDocument.FindChildElem("Vector"))
    {
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Name"))
        {
            return false;
        }
        tGazeVectorName = xmlDocument.GetChildData();

        float frequency = 0;
        if (xmlDocument.FindChildElem("Frequency"))
        {
            frequency = (float)atof(xmlDocument.GetChildData().c_str());
        }

        bool hwSync = false;
        ReadXmlBool(&xmlDocument, "Hardware_Sync", hwSync);
        bool filter = false;
        ReadXmlBool(&xmlDocument, "Filter", filter);

        pGazeVectorSettings.push_back({ tGazeVectorName, frequency, hwSync, filter });
        nGazeVectorCount++;
        xmlDocument.OutOfElem(); // Vector
    }

    pDataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CMarkupDeserializer::DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& pEyeTrackerSettings,bool& pDataAvailable)
{
    pDataAvailable = false;

    pEyeTrackerSettings.clear();

    if (!xmlDocument.FindChildElem("Eye_Tracker"))
    {
        return true; // NO eye tracker data available.
    }
    xmlDocument.IntoElem();

    std::string tEyeTrackerName;

    int nEyeTrackerCount = 0;

    while (xmlDocument.FindChildElem("Device"))
    {
        xmlDocument.IntoElem();

        if (!xmlDocument.FindChildElem("Name"))
        {
            return false;
        }
        tEyeTrackerName = xmlDocument.GetChildData();

        float frequency = 0;
        if (xmlDocument.FindChildElem("Frequency"))
        {
            frequency = (float)atof(xmlDocument.GetChildData().c_str());
        }

        bool hwSync = false;
        ReadXmlBool(&xmlDocument, "Hardware_Sync", hwSync);

        pEyeTrackerSettings.push_back({ tEyeTrackerName, frequency, hwSync });
        nEyeTrackerCount++;
        xmlDocument.OutOfElem(); // Vector
    }

    pDataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CMarkupDeserializer::DeserializeAnalogSettings(std::vector<SAnalogDevice>& pAnalogDeviceSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pAnalogDeviceSettings.clear();

    if (!xmlDocument.FindChildElem("Analog"))
    {
        // No analog data available.
        return true;
    }

    SAnalogDevice sAnalogDevice;

    xmlDocument.IntoElem();

    if (mMajorVersion == 1 && mMinorVersion == 0)
    {
        sAnalogDevice.nDeviceID = 1;   // Always channel 1
        sAnalogDevice.oName = "AnalogDevice";
        if (!xmlDocument.FindChildElem("Channels"))
        {
            return false;
        }
        sAnalogDevice.nChannels = atoi(xmlDocument.GetChildData().c_str());
        if (!xmlDocument.FindChildElem("Frequency"))
        {
            return false;
        }
        sAnalogDevice.nFrequency = atoi(xmlDocument.GetChildData().c_str());
        if (!xmlDocument.FindChildElem("Unit"))
        {
            return false;
        }
        sAnalogDevice.oUnit = xmlDocument.GetChildData();
        if (!xmlDocument.FindChildElem("Range"))
        {
            return false;
        }
        xmlDocument.IntoElem();
        if (!xmlDocument.FindChildElem("Min"))
        {
            return false;
        }
        sAnalogDevice.fMinRange = (float)atof(xmlDocument.GetChildData().c_str());
        if (!xmlDocument.FindChildElem("Max"))
        {
            return false;
        }
        sAnalogDevice.fMaxRange = (float)atof(xmlDocument.GetChildData().c_str());
        pAnalogDeviceSettings.push_back(sAnalogDevice);
        pDataAvailable = true;
        return true;
    }
    else
    {
        while (xmlDocument.FindChildElem("Device"))
        {
            sAnalogDevice.voLabels.clear();
            sAnalogDevice.voUnits.clear();
            xmlDocument.IntoElem();
            if (!xmlDocument.FindChildElem("Device_ID"))
            {
                xmlDocument.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nDeviceID = atoi(xmlDocument.GetChildData().c_str());

            if (!xmlDocument.FindChildElem("Device_Name"))
            {
                xmlDocument.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.oName = xmlDocument.GetChildData();

            if (!xmlDocument.FindChildElem("Channels"))
            {
                xmlDocument.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nChannels = atoi(xmlDocument.GetChildData().c_str());

            if (!xmlDocument.FindChildElem("Frequency"))
            {
                xmlDocument.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nFrequency = atoi(xmlDocument.GetChildData().c_str());

            if (mMajorVersion == 1 && mMinorVersion < 11)
            {
                if (!xmlDocument.FindChildElem("Unit"))
                {
                    xmlDocument.OutOfElem(); // Device
                    continue;
                }
                sAnalogDevice.oUnit = xmlDocument.GetChildData();
            }
            if (!xmlDocument.FindChildElem("Range"))
            {
                xmlDocument.OutOfElem(); // Device
                continue;
            }
            xmlDocument.IntoElem();

            if (!xmlDocument.FindChildElem("Min"))
            {
                xmlDocument.OutOfElem(); // Device
                xmlDocument.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.fMinRange = (float)atof(xmlDocument.GetChildData().c_str());

            if (!xmlDocument.FindChildElem("Max"))
            {
                xmlDocument.OutOfElem(); // Device
                xmlDocument.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.fMaxRange = (float)atof(xmlDocument.GetChildData().c_str());
            xmlDocument.OutOfElem(); // Range

            if (mMajorVersion == 1 && mMinorVersion < 11)
            {
                for (unsigned int i = 0; i < sAnalogDevice.nChannels; i++)
                {
                    if (xmlDocument.FindChildElem("Label"))
                    {
                        sAnalogDevice.voLabels.push_back(xmlDocument.GetChildData());
                    }
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels)
                {
                    xmlDocument.OutOfElem(); // Device
                    continue;
                }
            }
            else
            {
                while (xmlDocument.FindChildElem("Channel"))
                {
                    xmlDocument.IntoElem();
                    if (xmlDocument.FindChildElem("Label"))
                    {
                        sAnalogDevice.voLabels.push_back(xmlDocument.GetChildData());
                    }
                    if (xmlDocument.FindChildElem("Unit"))
                    {
                        sAnalogDevice.voUnits.push_back(xmlDocument.GetChildData());
                    }
                    xmlDocument.OutOfElem(); // Channel
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels ||
                    sAnalogDevice.voUnits.size() != sAnalogDevice.nChannels)
                {
                    xmlDocument.OutOfElem(); // Device
                    continue;
                }
            }
            xmlDocument.OutOfElem(); // Device
            pAnalogDeviceSettings.push_back(sAnalogDevice);
            pDataAvailable = true;
        }
    }

    return true;
} // ReadAnalogSettings

bool CMarkupDeserializer::DeserializeForceSettings(SSettingsForce& pForceSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pForceSettings.vsForcePlates.clear();

    //
    // Read some force plate parameters
    //
    if (!xmlDocument.FindChildElem("Force"))
    {
        return true;
    }

    xmlDocument.IntoElem();

    SForcePlate forcePlate;
    forcePlate.bValidCalibrationMatrix = false;
    for (int i = 0; i < 12; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            forcePlate.afCalibrationMatrix[i][j] = 0.0f;
        }
    }
    forcePlate.nCalibrationMatrixRows = 6;
    forcePlate.nCalibrationMatrixColumns = 6;

    if (!xmlDocument.FindChildElem("Unit_Length"))
    {
        return false;
    }
    pForceSettings.oUnitLength = xmlDocument.GetChildData();

    if (!xmlDocument.FindChildElem("Unit_Force"))
    {
        return false;
    }
    pForceSettings.oUnitForce = xmlDocument.GetChildData();

    int  iPlate = 1;
    while (xmlDocument.FindChildElem("Plate"))
    {
        //
        // Get name and type of the plates
        //
        xmlDocument.IntoElem(); // "Plate"
        if (xmlDocument.FindChildElem("Force_Plate_Index")) // Version 1.7 and earlier.
        {
            forcePlate.nID = atoi(xmlDocument.GetChildData().c_str());
        }
        else if (xmlDocument.FindChildElem("Plate_ID")) // Version 1.8 and later.
        {
            forcePlate.nID = atoi(xmlDocument.GetChildData().c_str());
        }
        else
        {
            return false;
        }

        if (xmlDocument.FindChildElem("Analog_Device_ID"))
        {
            forcePlate.nAnalogDeviceID = atoi(xmlDocument.GetChildData().c_str());
        }
        else
        {
            forcePlate.nAnalogDeviceID = 0;
        }

        if (!xmlDocument.FindChildElem("Frequency"))
        {
            return false;
        }
        forcePlate.nFrequency = atoi(xmlDocument.GetChildData().c_str());

        if (xmlDocument.FindChildElem("Type"))
        {
            forcePlate.oType = xmlDocument.GetChildData();
        }
        else
        {
            forcePlate.oType = "unknown";
        }

        if (xmlDocument.FindChildElem("Name"))
        {
            forcePlate.oName = xmlDocument.GetChildData();
        }
        else
        {
            forcePlate.oName = CMarkup::Format("#%d", iPlate);
        }

        if (xmlDocument.FindChildElem("Length"))
        {
            forcePlate.fLength = (float)atof(xmlDocument.GetChildData().c_str());
        }
        if (xmlDocument.FindChildElem("Width"))
        {
            forcePlate.fWidth = (float)atof(xmlDocument.GetChildData().c_str());
        }

        if (xmlDocument.FindChildElem("Location"))
        {
            xmlDocument.IntoElem();
            if (xmlDocument.FindChildElem("Corner1"))
            {
                xmlDocument.IntoElem();
                if (xmlDocument.FindChildElem("X"))
                {
                    forcePlate.asCorner[0].fX = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Y"))
                {
                    forcePlate.asCorner[0].fY = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Z"))
                {
                    forcePlate.asCorner[0].fZ = (float)atof(xmlDocument.GetChildData().c_str());
                }
                xmlDocument.OutOfElem();
            }
            if (xmlDocument.FindChildElem("Corner2"))
            {
                xmlDocument.IntoElem();
                if (xmlDocument.FindChildElem("X"))
                {
                    forcePlate.asCorner[1].fX = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Y"))
                {
                    forcePlate.asCorner[1].fY = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Z"))
                {
                    forcePlate.asCorner[1].fZ = (float)atof(xmlDocument.GetChildData().c_str());
                }
                xmlDocument.OutOfElem();
            }
            if (xmlDocument.FindChildElem("Corner3"))
            {
                xmlDocument.IntoElem();
                if (xmlDocument.FindChildElem("X"))
                {
                    forcePlate.asCorner[2].fX = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Y"))
                {
                    forcePlate.asCorner[2].fY = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Z"))
                {
                    forcePlate.asCorner[2].fZ = (float)atof(xmlDocument.GetChildData().c_str());
                }
                xmlDocument.OutOfElem();
            }
            if (xmlDocument.FindChildElem("Corner4"))
            {
                xmlDocument.IntoElem();
                if (xmlDocument.FindChildElem("X"))
                {
                    forcePlate.asCorner[3].fX = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Y"))
                {
                    forcePlate.asCorner[3].fY = (float)atof(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("Z"))
                {
                    forcePlate.asCorner[3].fZ = (float)atof(xmlDocument.GetChildData().c_str());
                }
                xmlDocument.OutOfElem();
            }
            xmlDocument.OutOfElem();
        }

        if (xmlDocument.FindChildElem("Origin"))
        {
            xmlDocument.IntoElem();
            if (xmlDocument.FindChildElem("X"))
            {
                forcePlate.sOrigin.fX = (float)atof(xmlDocument.GetChildData().c_str());
            }
            if (xmlDocument.FindChildElem("Y"))
            {
                forcePlate.sOrigin.fY = (float)atof(xmlDocument.GetChildData().c_str());
            }
            if (xmlDocument.FindChildElem("Z"))
            {
                forcePlate.sOrigin.fZ = (float)atof(xmlDocument.GetChildData().c_str());
            }
            xmlDocument.OutOfElem();
        }

        forcePlate.vChannels.clear();
        if (xmlDocument.FindChildElem("Channels"))
        {
            xmlDocument.IntoElem();
            SForceChannel forceChannel;
            while (xmlDocument.FindChildElem("Channel"))
            {
                xmlDocument.IntoElem();
                if (xmlDocument.FindChildElem("Channel_No"))
                {
                    forceChannel.nChannelNumber = atoi(xmlDocument.GetChildData().c_str());
                }
                if (xmlDocument.FindChildElem("ConversionFactor"))
                {
                    forceChannel.fConversionFactor = (float)atof(xmlDocument.GetChildData().c_str());
                }
                forcePlate.vChannels.push_back(forceChannel);
                xmlDocument.OutOfElem();
            }
            xmlDocument.OutOfElem();
        }

        if (xmlDocument.FindChildElem("Calibration_Matrix"))
        {
            xmlDocument.IntoElem();
            int nRow = 0;

            if (mMajorVersion == 1 && mMinorVersion < 12)
            {
                char strRow[16];
                char strCol[16];
                sprintf(strRow, "Row%d", nRow + 1);
                while (xmlDocument.FindChildElem(strRow))
                {
                    xmlDocument.IntoElem();
                    int nCol = 0;
                    sprintf(strCol, "Col%d", nCol + 1);
                    while (xmlDocument.FindChildElem(strCol))
                    {
                        forcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(xmlDocument.GetChildData().c_str());
                        nCol++;
                        sprintf(strCol, "Col%d", nCol + 1);
                    }
                    forcePlate.nCalibrationMatrixColumns = nCol;

                    nRow++;
                    sprintf(strRow, "Row%d", nRow + 1);
                    xmlDocument.OutOfElem(); // RowX
                }
            }
            else
            {
                //<Rows>
                if (xmlDocument.FindChildElem("Rows"))
                {
                    xmlDocument.IntoElem();

                    while (xmlDocument.FindChildElem("Row"))
                    {
                        xmlDocument.IntoElem();

                        //<Columns>
                        if (xmlDocument.FindChildElem("Columns"))
                        {
                            xmlDocument.IntoElem();

                            int nCol = 0;
                            while (xmlDocument.FindChildElem("Column"))
                            {
                                forcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(xmlDocument.GetChildData().c_str());
                                nCol++;
                            }
                            forcePlate.nCalibrationMatrixColumns = nCol;

                            nRow++;
                            xmlDocument.OutOfElem(); // Columns
                        }
                        xmlDocument.OutOfElem(); // Row
                    }
                    xmlDocument.OutOfElem(); // Rows
                }
            }
            forcePlate.nCalibrationMatrixRows = nRow;
            forcePlate.bValidCalibrationMatrix = true;

            xmlDocument.OutOfElem(); // "Calibration_Matrix"
        }
        xmlDocument.OutOfElem(); // "Plate"

        pDataAvailable = true;
        pForceSettings.vsForcePlates.push_back(forcePlate);
    }

    return true;
} // Read force settings

bool CMarkupDeserializer::DeserializeImageSettings(std::vector<SImageCamera>& pImageSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pImageSettings.clear();

    //
    // Read some Image parameters
    //
    if (!xmlDocument.FindChildElem("Image"))
    {
        return true;
    }
    xmlDocument.IntoElem();

    while (xmlDocument.FindChildElem("Camera"))
    {
        xmlDocument.IntoElem();

        SImageCamera imageCamera;

        if (!xmlDocument.FindChildElem("ID"))
        {
            return false;
        }
        imageCamera.nID = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Enabled"))
        {
            return false;
        }
        std::string str;
        str = ToLower(xmlDocument.GetChildData());

        if (str == "true")
        {
            imageCamera.bEnabled = true;
        }
        else
        {
            imageCamera.bEnabled = false;
        }

        if (!xmlDocument.FindChildElem("Format"))
        {
            return false;
        }
        str = ToLower(xmlDocument.GetChildData());

        if (str == "rawgrayscale")
        {
            imageCamera.eFormat = CRTPacket::FormatRawGrayscale;
        }
        else if (str == "rawbgr")
        {
            imageCamera.eFormat = CRTPacket::FormatRawBGR;
        }
        else if (str == "jpg")
        {
            imageCamera.eFormat = CRTPacket::FormatJPG;
        }
        else if (str == "png")
        {
            imageCamera.eFormat = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!xmlDocument.FindChildElem("Width"))
        {
            return false;
        }
        imageCamera.nWidth = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Height"))
        {
            return false;
        }
        imageCamera.nHeight = atoi(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Left_Crop"))
        {
            return false;
        }
        imageCamera.fCropLeft = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Top_Crop"))
        {
            return false;
        }
        imageCamera.fCropTop = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Right_Crop"))
        {
            return false;
        }
        imageCamera.fCropRight = (float)atof(xmlDocument.GetChildData().c_str());

        if (!xmlDocument.FindChildElem("Bottom_Crop"))
        {
            return false;
        }
        imageCamera.fCropBottom = (float)atof(xmlDocument.GetChildData().c_str());

        xmlDocument.OutOfElem(); // "Camera"

        pImageSettings.push_back(imageCamera);
        pDataAvailable = true;
    }

    return true;
} // ReadImageSettings


bool CMarkupDeserializer::DeserializeSkeletonSettings(bool pSkeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>& mSkeletonSettingsHierarchical, std::vector<SSettingsSkeleton>& mSkeletonSettings, bool& pDataAvailable)
{
    CMarkup xml = xmlDocument;

    pDataAvailable = false;

    mSkeletonSettings.clear();
    mSkeletonSettingsHierarchical.clear();

    int segmentIndex;
    std::map<int, int> segmentIdIndexMap;
    xml.ResetPos();

    xml.FindElem();
    xml.IntoElem();

    if (xml.FindElem("Skeletons"))
    {
        xml.IntoElem();

        if (mMajorVersion > 1 || mMinorVersion > 20)
        {
            while (xml.FindElem("Skeleton"))
            {
                SSettingsSkeletonHierarchical skeletonHierarchical;
                SSettingsSkeleton skeleton;
                segmentIndex = 0;

                skeletonHierarchical.name = xml.GetAttrib("Name");
                skeleton.name = skeletonHierarchical.name;

                xml.IntoElem();

                if (xml.FindElem("Solver"))
                {
                    skeletonHierarchical.rootSegment.solver = xml.GetData();
                }

                if (xml.FindElem("Scale"))
                {
                    if (!ParseString(xml.GetData(), skeletonHierarchical.scale))
                    {
                        sprintf(mErrorStr, "Scale element parse error");
                        return false;
                    }
                }

                if (xml.FindElem("Segments"))
                {
                    xml.IntoElem();

                    std::function<void(SSettingsSkeletonSegmentHierarchical&, std::vector<SSettingsSkeletonSegment>&, const int)> recurseSegments =
                        [&](SSettingsSkeletonSegmentHierarchical& segmentHierarchical, std::vector<SSettingsSkeletonSegment>& segments, const int parentId)
                        {
                            segmentHierarchical.name = xml.GetAttrib("Name");
                            ParseString(xml.GetAttrib("ID"), segmentHierarchical.id);

                            segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                            xml.IntoElem();

                            if (xml.FindElem("Solver"))
                            {
                                segmentHierarchical.solver = xml.GetData();
                            }

                            if (xml.FindElem("Transform"))
                            {
                                xml.IntoElem();
                                segmentHierarchical.position = ReadXMLPosition(xml, "Position");
                                segmentHierarchical.rotation = ReadXMLRotation(xml, "Rotation");
                                xml.OutOfElem(); // Transform
                            }

                            if (xml.FindElem("DefaultTransform"))
                            {
                                xml.IntoElem();
                                segmentHierarchical.defaultPosition = ReadXMLPosition(xml, "Position");
                                segmentHierarchical.defaultRotation = ReadXMLRotation(xml, "Rotation");
                                xml.OutOfElem(); // DefaultTransform
                            }

                            if (xml.FindElem("DegreesOfFreedom"))
                            {
                                xml.IntoElem();
                                ReadXMLDegreesOfFreedom(xml, "RotationX", segmentHierarchical.degreesOfFreedom);
                                ReadXMLDegreesOfFreedom(xml, "RotationY", segmentHierarchical.degreesOfFreedom);
                                ReadXMLDegreesOfFreedom(xml, "RotationZ", segmentHierarchical.degreesOfFreedom);
                                ReadXMLDegreesOfFreedom(xml, "TranslationX", segmentHierarchical.degreesOfFreedom);
                                ReadXMLDegreesOfFreedom(xml, "TranslationY", segmentHierarchical.degreesOfFreedom);
                                ReadXMLDegreesOfFreedom(xml, "TranslationZ", segmentHierarchical.degreesOfFreedom);
                                xml.OutOfElem(); // DegreesOfFreedom
                            }

                            segmentHierarchical.endpoint = ReadXMLPosition(xml, "Endpoint");

                            if (xml.FindElem("Markers"))
                            {
                                xml.IntoElem();

                                while (xml.FindElem("Marker"))
                                {
                                    SMarker marker;

                                    marker.name = xml.GetAttrib("Name");
                                    marker.weight = 1.0;

                                    xml.IntoElem();
                                    marker.position = ReadXMLPosition(xml, "Position");
                                    if (xml.FindElem("Weight"))
                                    {
                                        ParseString(xml.GetData(), marker.weight);
                                    }

                                    xml.OutOfElem(); // Marker

                                    segmentHierarchical.markers.push_back(marker);
                                }

                                xml.OutOfElem(); // Markers
                            }

                            if (xml.FindElem("RigidBodies"))
                            {
                                xml.IntoElem();

                                while (xml.FindElem("RigidBody"))
                                {
                                    SBody body;

                                    body.name = xml.GetAttrib("Name");
                                    body.weight = 1.0;

                                    xml.IntoElem();

                                    if (xml.FindElem("Transform"))
                                    {
                                        xml.IntoElem();
                                        body.position = ReadXMLPosition(xml, "Position");
                                        body.rotation = ReadXMLRotation(xml, "Rotation");
                                        xml.OutOfElem(); // Transform
                                    }
                                    if (xml.FindElem("Weight"))
                                    {
                                        ParseString(xml.GetData(), body.weight);
                                    }

                                    xml.OutOfElem(); // RigidBody

                                    segmentHierarchical.bodies.push_back(body);
                                }

                                xml.OutOfElem(); // RigidBodies
                            }
                            SSettingsSkeletonSegment segment;
                            segment.name = segmentHierarchical.name;
                            segment.id = segmentHierarchical.id;
                            segment.parentId = parentId;
                            segment.parentIndex = (parentId != -1) ? segmentIdIndexMap[parentId] : -1;
                            segment.positionX = (float)segmentHierarchical.defaultPosition.x;
                            segment.positionY = (float)segmentHierarchical.defaultPosition.y;
                            segment.positionZ = (float)segmentHierarchical.defaultPosition.z;
                            segment.rotationX = (float)segmentHierarchical.defaultRotation.x;
                            segment.rotationY = (float)segmentHierarchical.defaultRotation.y;
                            segment.rotationZ = (float)segmentHierarchical.defaultRotation.z;
                            segment.rotationW = (float)segmentHierarchical.defaultRotation.w;

                            segments.push_back(segment);

                            while (xml.FindElem("Segment"))
                            {
                                SSettingsSkeletonSegmentHierarchical childSegment;
                                recurseSegments(childSegment, skeleton.segments, segmentHierarchical.id);
                                segmentHierarchical.segments.push_back(childSegment);
                            }
                            xml.OutOfElem();
                        };

                    if (xml.FindElem("Segment"))
                    {
                        recurseSegments(skeletonHierarchical.rootSegment, skeleton.segments, -1);
                    }
                    xml.OutOfElem(); // Segments
                }
                xml.OutOfElem(); // Skeleton
                mSkeletonSettingsHierarchical.push_back(skeletonHierarchical);
                mSkeletonSettings.push_back(skeleton);
            }
            pDataAvailable = true;
        }
        else
        {
            while (xml.FindElem("Skeleton"))
            {
                SSettingsSkeleton skeleton;
                segmentIndex = 0;

                skeleton.name = xml.GetAttrib("Name");
                xml.IntoElem();

                while (xml.FindElem("Segment"))
                {
                    SSettingsSkeletonSegment segment;

                    segment.name = xml.GetAttrib("Name");
                    if (segment.name.size() == 0 || sscanf(xml.GetAttrib("ID").c_str(), "%u", &segment.id) != 1)
                    {
                        return false;
                    }

                    segmentIdIndexMap[segment.id] = segmentIndex++;

                    int parentId;
                    if (sscanf(xml.GetAttrib("Parent_ID").c_str(), "%d", &parentId) != 1)
                    {
                        segment.parentId = -1;
                        segment.parentIndex = -1;
                    }
                    else if (segmentIdIndexMap.count(parentId) > 0)
                    {
                        segment.parentId = parentId;
                        segment.parentIndex = segmentIdIndexMap[parentId];
                    }

                    xml.IntoElem();

                    if (xml.FindElem("Position"))
                    {
                        ParseString(xml.GetAttrib("X"), segment.positionX);
                        ParseString(xml.GetAttrib("Y"), segment.positionY);
                        ParseString(xml.GetAttrib("Z"), segment.positionZ);
                    }

                    if (xml.FindElem("Rotation"))
                    {
                        ParseString(xml.GetAttrib("X"), segment.rotationX);
                        ParseString(xml.GetAttrib("Y"), segment.rotationY);
                        ParseString(xml.GetAttrib("Z"), segment.rotationZ);
                        ParseString(xml.GetAttrib("W"), segment.rotationW);
                    }

                    skeleton.segments.push_back(segment);

                    xml.OutOfElem(); // Segment
                }

                mSkeletonSettings.push_back(skeleton);

                xml.OutOfElem(); // Skeleton
            }
        }
        xml.OutOfElem(); // Skeletons
        pDataAvailable = true;
    }
    return true;
} // ReadSkeletonSettings

namespace 
{
    bool ReadXmlFov(std::string name, CMarkup& xmlDocument, SCalibrationFov& fov)
    {
        if (!xmlDocument.FindChildElem(name.c_str()))
        {
            return false;
        }
        fov.left = std::stoul(xmlDocument.GetChildAttrib("left"));
        fov.top = std::stoul(xmlDocument.GetChildAttrib("top"));
        fov.right = std::stoul(xmlDocument.GetChildAttrib("right"));
        fov.bottom = std::stoul(xmlDocument.GetChildAttrib("bottom"));

        return true;
    }
}

bool CMarkupDeserializer::DeserializeCalibrationSettings(SCalibration& calibrationSettings)
{
    SCalibration settings;

    if (!xmlDocument.FindChildElem("calibration"))
    {
        sprintf(mErrorStr, "Missing calibration element");
        return false;
    }
    xmlDocument.IntoElem();

    try
    {
        std::string resultStr = ToLower(xmlDocument.GetAttrib("calibrated"));

        settings.calibrated = (resultStr == "true");
        settings.source = xmlDocument.GetAttrib("source");
        settings.created = xmlDocument.GetAttrib("created");
        settings.qtm_version = xmlDocument.GetAttrib("qtm-version");
        std::string typeStr = xmlDocument.GetAttrib("type");
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
            settings.refit_residual = std::stod(xmlDocument.GetAttrib("refit-residual"));
        }
        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = std::stod(xmlDocument.GetAttrib("wandLength"));
            settings.max_frames = std::stoul(xmlDocument.GetAttrib("maximumFrames"));
            settings.short_arm_end = std::stod(xmlDocument.GetAttrib("shortArmEnd"));
            settings.long_arm_end = std::stod(xmlDocument.GetAttrib("longArmEnd"));
            settings.long_arm_middle = std::stod(xmlDocument.GetAttrib("longArmMiddle"));

            if (!xmlDocument.FindChildElem("results"))
            {
                return false;
            }

            settings.result_std_dev = std::stod(xmlDocument.GetChildAttrib("std-dev"));
            settings.result_min_max_diff = std::stod(xmlDocument.GetChildAttrib("min-max-diff"));
            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = std::stod(xmlDocument.GetChildAttrib("refit-residual"));
                settings.result_consecutive = std::stoul(xmlDocument.GetChildAttrib("consecutive"));
            }
        }

        if (!xmlDocument.FindChildElem("cameras"))
        {
            return false;
        }
        xmlDocument.IntoElem();

        while (xmlDocument.FindChildElem("camera"))
        {
            xmlDocument.IntoElem();
            SCalibrationCamera camera;
            camera.active = std::stod(xmlDocument.GetAttrib("active")) != 0;

            std::string calibratedStr = ToLower(xmlDocument.GetAttrib("calibrated"));

            camera.calibrated = (calibratedStr == "true");
            camera.message = xmlDocument.GetAttrib("message");

            camera.point_count = std::stoul(xmlDocument.GetAttrib("point-count"));
            camera.avg_residual = std::stod(xmlDocument.GetAttrib("avg-residual"));
            camera.serial = std::stoul(xmlDocument.GetAttrib("serial"));
            camera.model = xmlDocument.GetAttrib("model");
            camera.view_rotation = std::stoul(xmlDocument.GetAttrib("viewrotation"));
            if (!ReadXmlFov("fov_marker", xmlDocument, camera.fov_marker))
            {
                return false;
            }
            if (!ReadXmlFov("fov_marker_max", xmlDocument, camera.fov_marker_max))
            {
                return false;
            }
            if (!ReadXmlFov("fov_video", xmlDocument, camera.fov_video))
            {
                return false;
            }
            if (!ReadXmlFov("fov_video_max", xmlDocument, camera.fov_video_max))
            {
                return false;
            }
            if (!xmlDocument.FindChildElem("transform"))
            {
                return false;
            }
            camera.transform.x = std::stod(xmlDocument.GetChildAttrib("x"));
            camera.transform.y = std::stod(xmlDocument.GetChildAttrib("y"));
            camera.transform.z = std::stod(xmlDocument.GetChildAttrib("z"));
            camera.transform.r11 = std::stod(xmlDocument.GetChildAttrib("r11"));
            camera.transform.r12 = std::stod(xmlDocument.GetChildAttrib("r12"));
            camera.transform.r13 = std::stod(xmlDocument.GetChildAttrib("r13"));
            camera.transform.r21 = std::stod(xmlDocument.GetChildAttrib("r21"));
            camera.transform.r22 = std::stod(xmlDocument.GetChildAttrib("r22"));
            camera.transform.r23 = std::stod(xmlDocument.GetChildAttrib("r23"));
            camera.transform.r31 = std::stod(xmlDocument.GetChildAttrib("r31"));
            camera.transform.r32 = std::stod(xmlDocument.GetChildAttrib("r32"));
            camera.transform.r33 = std::stod(xmlDocument.GetChildAttrib("r33"));

            if (!xmlDocument.FindChildElem("intrinsic"))
            {
                return false;
            }

            auto focalLength = xmlDocument.GetChildAttrib("focallength");
            try
            {
                camera.intrinsic.focal_length = std::stod(focalLength);
            }
            catch (const std::invalid_argument&)
            {
                camera.intrinsic.focal_length = 0;
            }

            camera.intrinsic.sensor_min_u = std::stod(xmlDocument.GetChildAttrib("sensorMinU"));
            camera.intrinsic.sensor_max_u = std::stod(xmlDocument.GetChildAttrib("sensorMaxU"));
            camera.intrinsic.sensor_min_v = std::stod(xmlDocument.GetChildAttrib("sensorMinV"));
            camera.intrinsic.sensor_max_v = std::stod(xmlDocument.GetChildAttrib("sensorMaxV"));
            camera.intrinsic.focal_length_u = std::stod(xmlDocument.GetChildAttrib("focalLengthU"));
            camera.intrinsic.focal_length_v = std::stod(xmlDocument.GetChildAttrib("focalLengthV"));
            camera.intrinsic.center_point_u = std::stod(xmlDocument.GetChildAttrib("centerPointU"));
            camera.intrinsic.center_point_v = std::stod(xmlDocument.GetChildAttrib("centerPointV"));
            camera.intrinsic.skew = std::stod(xmlDocument.GetChildAttrib("skew"));
            camera.intrinsic.radial_distortion_1 = std::stod(xmlDocument.GetChildAttrib("radialDistortion1"));
            camera.intrinsic.radial_distortion_2 = std::stod(xmlDocument.GetChildAttrib("radialDistortion2"));
            camera.intrinsic.radial_distortion_3 = std::stod(xmlDocument.GetChildAttrib("radialDistortion3"));
            camera.intrinsic.tangental_distortion_1 = std::stod(xmlDocument.GetChildAttrib("tangentalDistortion1"));
            camera.intrinsic.tangental_distortion_2 = std::stod(xmlDocument.GetChildAttrib("tangentalDistortion2"));
            xmlDocument.OutOfElem(); // camera
            settings.cameras.push_back(camera);
        }
        xmlDocument.OutOfElem(); // cameras
    }
    catch (...)
    {
        return false;
    }

    xmlDocument.OutOfElem(); // calibration

    calibrationSettings = settings;
    return true;
}


SPosition CMarkupDeserializer::DeserializeXMLPosition(CMarkup& xml, const std::string& element)
{
    SPosition position;

    if (xml.FindElem(element.c_str()))
    {
        ParseString(xml.GetAttrib("X"), position.x);
        ParseString(xml.GetAttrib("Y"), position.y);
        ParseString(xml.GetAttrib("Z"), position.z);
        xml.ResetMainPos();
    }
    return position;
}


SRotation CMarkupDeserializer::DeserializeXMLRotation(CMarkup& xml, const std::string& element)
{
    SRotation rotation;

    if (xml.FindElem(element.c_str()))
    {
        ParseString(xml.GetAttrib("X"), rotation.x);
        ParseString(xml.GetAttrib("Y"), rotation.y);
        ParseString(xml.GetAttrib("Z"), rotation.z);
        ParseString(xml.GetAttrib("W"), rotation.w);
        xml.ResetMainPos();
    }
    return rotation;
}

CMarkupSerializer::CMarkupSerializer(std::uint32_t pMajorVersion, std::uint32_t pMinorVersion)
    : mMajorVersion(pMajorVersion), mMinorVersion(pMinorVersion)
{
}

std::string CMarkupSerializer::SetGeneralSettings(const unsigned int* pnCaptureFrequency,
                                                  const float* pfCaptureTime, const bool* pbStartOnExtTrig, const bool* pStartOnTrigNO, const bool* pStartOnTrigNC,
                                                  const bool* pStartOnTrigSoftware, const EProcessingActions* peProcessingActions,
                                                  const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    if (pnCaptureFrequency)
    {
        AddXMLElementUnsignedInt(&xmlDocument, "Frequency", pnCaptureFrequency);
    }
    if (pfCaptureTime)
    {
        AddXMLElementFloat(&xmlDocument, "Capture_Time", pfCaptureTime, 3);
    }
    if (pbStartOnExtTrig)
    {
        AddXMLElementBool(&xmlDocument, "Start_On_External_Trigger", pbStartOnExtTrig);
        if (mMajorVersion > 1 || mMinorVersion > 14)
        {
            AddXMLElementBool(&xmlDocument, "Start_On_Trigger_NO", pStartOnTrigNO);
            AddXMLElementBool(&xmlDocument, "Start_On_Trigger_NC", pStartOnTrigNC);
            AddXMLElementBool(&xmlDocument, "Start_On_Trigger_Software", pStartOnTrigSoftware);
        }
    }

    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActions[3] = { peProcessingActions, peRtProcessingActions, peReprocessingActions };

    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActions[i])
        {
            xmlDocument.AddElem(processings[i]);
            xmlDocument.IntoElem();

            if (mMajorVersion > 1 || mMinorVersion > 13)
            {
                AddXMLElementBool(&xmlDocument, "PreProcessing2D", (*processingActions[i] & ProcessingPreProcess2D) != 0);
            }
            if (*processingActions[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                xmlDocument.AddElem("Tracking", "2D");
            }
            else if (*processingActions[i] & ProcessingTracking3D)
            {
                xmlDocument.AddElem("Tracking", "3D");
            }
            else
            {
                xmlDocument.AddElem("Tracking", "False");
            }
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&xmlDocument, "TwinSystemMerge", (*processingActions[i] & ProcessingTwinSystemMerge) != 0);
                AddXMLElementBool(&xmlDocument, "SplineFill", (*processingActions[i] & ProcessingSplineFill) != 0);
            }
            AddXMLElementBool(&xmlDocument, "AIM", (*processingActions[i] & ProcessingAIM) != 0);
            AddXMLElementBool(&xmlDocument, "Track6DOF", (*processingActions[i] & Processing6DOFTracking) != 0);
            AddXMLElementBool(&xmlDocument, "ForceData", (*processingActions[i] & ProcessingForceData) != 0);
            AddXMLElementBool(&xmlDocument, "GazeVector", (*processingActions[i] & ProcessingGazeVector) != 0);
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&xmlDocument, "ExportTSV", (*processingActions[i] & ProcessingExportTSV) != 0);
                AddXMLElementBool(&xmlDocument, "ExportC3D", (*processingActions[i] & ProcessingExportC3D) != 0);
                AddXMLElementBool(&xmlDocument, "ExportMatlabFile", (*processingActions[i] & ProcessingExportMatlabFile) != 0);
                AddXMLElementBool(&xmlDocument, "ExportAviFile", (*processingActions[i] & ProcessingExportAviFile) != 0);
            }
            xmlDocument.OutOfElem(); // Processing_Actions
        }
    }
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetExtTimeBaseSettings(const bool* pbEnabled, const ESignalSource* peSignalSource,
    const bool* pbSignalModePeriodic, const unsigned int* pnFreqMultiplier, const unsigned int* pnFreqDivisor,
    const unsigned int* pnFreqTolerance, const float* pfNominalFrequency, const bool* pbNegativeEdge,
    const unsigned int* pnSignalShutterDelay, const float* pfNonPeriodicTimeout)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("External_Time_Base");
    xmlDocument.IntoElem();

    AddXMLElementBool(&xmlDocument, "Enabled", pbEnabled);

    if (peSignalSource)
    {
        switch (*peSignalSource)
        {
        case SourceControlPort:
            xmlDocument.AddElem("Signal_Source", "Control port");
            break;
        case SourceIRReceiver:
            xmlDocument.AddElem("Signal_Source", "IR receiver");
            break;
        case SourceSMPTE:
            xmlDocument.AddElem("Signal_Source", "SMPTE");
            break;
        case SourceVideoSync:
            xmlDocument.AddElem("Signal_Source", "Video sync");
            break;
        case SourceIRIG:
            xmlDocument.AddElem("Signal_Source", "IRIG");
            break;
        }
    }

    AddXMLElementBool(&xmlDocument, "Signal_Mode", pbSignalModePeriodic, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(&xmlDocument, "Frequency_Multiplier", pnFreqMultiplier);
    AddXMLElementUnsignedInt(&xmlDocument, "Frequency_Divisor", pnFreqDivisor);
    AddXMLElementUnsignedInt(&xmlDocument, "Frequency_Tolerance", pnFreqTolerance);

    if (pfNominalFrequency)
    {
        if (*pfNominalFrequency < 0)
        {
            xmlDocument.AddElem("Nominal_Frequency", "None");
        }
        else
        {
            AddXMLElementFloat(&xmlDocument, "Nominal_Frequency", pfNominalFrequency, 3);
        }
    }

    AddXMLElementBool(&xmlDocument, "Signal_Edge", pbNegativeEdge, "Negative", "Positive");
    AddXMLElementUnsignedInt(&xmlDocument, "Signal_Shutter_Delay", pnSignalShutterDelay);
    AddXMLElementFloat(&xmlDocument, "Non_Periodic_Timeout", pfNonPeriodicTimeout, 3);

    xmlDocument.OutOfElem(); // External_Time_Base            
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("External_Timestamp");
    xmlDocument.IntoElem();

    AddXMLElementBool(&xmlDocument, "Enabled", timestampSettings.bEnabled);

    switch (timestampSettings.nType)
    {
    default:
    case ETimestampType::Timestamp_SMPTE:
        xmlDocument.AddElem("Type", "SMPTE");
        break;
    case ETimestampType::Timestamp_IRIG:
        xmlDocument.AddElem("Type", "IRIG");
        break;
    case ETimestampType::Timestamp_CameraTime:
        xmlDocument.AddElem("Type", "CameraTime");
        break;
    }
    AddXMLElementUnsignedInt(&xmlDocument, "Frequency", timestampSettings.nFrequency);

    xmlDocument.OutOfElem(); // Timestamp
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetCameraSettings(const unsigned int pCameraId, const ECameraMode* peMode,
    const float* pfMarkerExposure, const float* pfMarkerThreshold, const int* pnOrientation)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);

    if (peMode)
    {
        switch (*peMode)
        {
        case ModeMarker:
            xmlDocument.AddElem("Mode", "Marker");
            break;
        case ModeMarkerIntensity:
            xmlDocument.AddElem("Mode", "Marker Intensity");
            break;
        case ModeVideo:
            xmlDocument.AddElem("Mode", "Video");
            break;
        }
    }
    AddXMLElementFloat(&xmlDocument, "Marker_Exposure", pfMarkerExposure);
    AddXMLElementFloat(&xmlDocument, "Marker_Threshold", pfMarkerThreshold);
    AddXMLElementInt(&xmlDocument, "Orientation", pnOrientation);

    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetCameraVideoSettings(const unsigned int pCameraId,
    const EVideoResolution* eVideoResolution, const EVideoAspectRatio* eVideoAspectRatio,
    const unsigned int* pnVideoFrequency, const float* pfVideoExposure, const float* pfVideoFlashTime)
{
    CMarkup xmlDocument;
    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);
    if (eVideoResolution)
    {
        switch (*eVideoResolution)
        {
        case VideoResolution1440p:
            xmlDocument.AddElem("Video_Resolution", "1440p");
            break;
        case VideoResolution1080p:
            xmlDocument.AddElem("Video_Resolution", "1080p");
            break;
        case VideoResolution720p:
            xmlDocument.AddElem("Video_Resolution", "720p");
            break;
        case VideoResolution540p:
            xmlDocument.AddElem("Video_Resolution", "540p");
            break;
        case VideoResolution480p:
            xmlDocument.AddElem("Video_Resolution", "480p");
            break;
        case VideoResolutionNone:
            break;
        }
    }
    if (eVideoAspectRatio)
    {
        switch (*eVideoAspectRatio)
        {
        case VideoAspectRatio16x9:
            xmlDocument.AddElem("Video_Aspect_Ratio", "16x9");
            break;
        case VideoAspectRatio4x3:
            xmlDocument.AddElem("Video_Aspect_Ratio", "4x3");
            break;
        case VideoAspectRatio1x1:
            xmlDocument.AddElem("Video_Aspect_Ratio", "1x1");
            break;
        case VideoAspectRatioNone:
            break;
        }
    }
    AddXMLElementUnsignedInt(&xmlDocument, "Video_Frequency", pnVideoFrequency);
    AddXMLElementFloat(&xmlDocument, "Video_Exposure", pfVideoExposure);
    AddXMLElementFloat(&xmlDocument, "Video_Flash_Time", pfVideoFlashTime);

    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetCameraSyncOutSettings(const unsigned int pCameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* peSyncOutMode, const unsigned int* pnSyncOutValue, const float* pfSyncOutDutyCycle,
    const bool* pbSyncOutNegativePolarity)
{
    CMarkup xmlDocument;
    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);

    int port = portNumber - 1;
    if (((port == 0 || port == 1) && peSyncOutMode) || (port == 2))
    {
        xmlDocument.AddElem(port == 0 ? "Sync_Out" : (port == 1 ? "Sync_Out2" : "Sync_Out_MT"));
        xmlDocument.IntoElem();

        if (port == 0 || port == 1)
        {
            switch (*peSyncOutMode)
            {
            case ModeShutterOut:
                xmlDocument.AddElem("Mode", "Shutter out");
                break;
            case ModeMultiplier:
                xmlDocument.AddElem("Mode", "Multiplier");
                break;
            case ModeDivisor:
                xmlDocument.AddElem("Mode", "Divisor");
                break;
            case ModeIndependentFreq:
                xmlDocument.AddElem("Mode", "Camera independent");
                break;
            case ModeMeasurementTime:
                xmlDocument.AddElem("Mode", "Measurement time");
                break;
            case ModeFixed100Hz:
                xmlDocument.AddElem("Mode", "Continuous 100Hz");
                break;
            case ModeSystemLiveTime:
                xmlDocument.AddElem("Mode", "System live time");
                break;
            default:
                return false; // Should never happen
            }

            if (*peSyncOutMode == ModeMultiplier ||
                *peSyncOutMode == ModeDivisor ||
                *peSyncOutMode == ModeIndependentFreq)
            {
                if (pnSyncOutValue)
                {
                    AddXMLElementUnsignedInt(&xmlDocument, "Value", pnSyncOutValue);
                }
                if (pfSyncOutDutyCycle)
                {
                    AddXMLElementFloat(&xmlDocument, "Duty_Cycle", pfSyncOutDutyCycle, 3);
                }
            }
        }
        if (pbSyncOutNegativePolarity && (port == 2 ||
            (peSyncOutMode && *peSyncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(&xmlDocument, "Signal_Polarity", pbSyncOutNegativePolarity, "Negative", "Positive");
        }
        xmlDocument.OutOfElem(); // Sync_Out
    }
    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus,
    const float pAperture)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);

    xmlDocument.AddElem("LensControl");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Focus");
    xmlDocument.AddAttrib("Value", CMarkup::Format("%f", pFocus).c_str());
    xmlDocument.AddElem("Aperture");
    xmlDocument.AddAttrib("Value", CMarkup::Format("%f", pAperture).c_str());

    xmlDocument.OutOfElem(); // LensControl
    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure,
    const float pCompensation)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);

    xmlDocument.AddElem("LensControl");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("AutoExposure");
    xmlDocument.AddAttrib("Enabled", pAutoExposure ? "true" : "false");
    xmlDocument.AddAttrib("Compensation", CMarkup::Format("%f", pCompensation).c_str());

    xmlDocument.OutOfElem(); // AutoExposure
    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("General");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);

    xmlDocument.AddElem("AutoWhiteBalance", pEnable ? "true" : "false");

    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // General
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetImageSettings(const unsigned int pCameraId, const bool* pbEnable,
    const CRTPacket::EImageFormat* peFormat, const unsigned int* pnWidth, const unsigned int* pnHeight,
    const float* pfLeftCrop, const float* pfTopCrop, const float* pfRightCrop, const float* pfBottomCrop)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("Image");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Camera");
    xmlDocument.IntoElem();

    AddXMLElementUnsignedInt(&xmlDocument, "ID", &pCameraId);

    AddXMLElementBool(&xmlDocument, "Enabled", pbEnable);

    if (peFormat)
    {
        switch (*peFormat)
        {
        case CRTPacket::FormatRawGrayscale:
            xmlDocument.AddElem("Format", "RAWGrayscale");
            break;
        case CRTPacket::FormatRawBGR:
            xmlDocument.AddElem("Format", "RAWBGR");
            break;
        case CRTPacket::FormatJPG:
            xmlDocument.AddElem("Format", "JPG");
            break;
        case CRTPacket::FormatPNG:
            xmlDocument.AddElem("Format", "PNG");
            break;
        }
    }
    AddXMLElementUnsignedInt(&xmlDocument, "Width", pnWidth);
    AddXMLElementUnsignedInt(&xmlDocument, "Height", pnHeight);
    AddXMLElementFloat(&xmlDocument, "Left_Crop", pfLeftCrop);
    AddXMLElementFloat(&xmlDocument, "Top_Crop", pfTopCrop);
    AddXMLElementFloat(&xmlDocument, "Right_Crop", pfRightCrop);
    AddXMLElementFloat(&xmlDocument, "Bottom_Crop", pfBottomCrop);

    xmlDocument.OutOfElem(); // Camera
    xmlDocument.OutOfElem(); // Image
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetForceSettings(const unsigned int pPlateId, const SPoint* pCorner1,
    const SPoint* pCorner2, const SPoint* pCorner3, const SPoint* pCorner4)
{
    CMarkup xmlDocument;
    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("Force");
    xmlDocument.IntoElem();

    xmlDocument.AddElem("Plate");
    xmlDocument.IntoElem();

    if (mMajorVersion > 1 || mMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(&xmlDocument, "Plate_ID", &pPlateId);
    }
    else
    {
        AddXMLElementUnsignedInt(&xmlDocument, "Force_Plate_Index", &pPlateId);
    }
    if (pCorner1)
    {
        xmlDocument.AddElem("Corner1");
        xmlDocument.IntoElem();
        AddXMLElementFloat(&xmlDocument, "X", &(pCorner1->fX));
        AddXMLElementFloat(&xmlDocument, "Y", &(pCorner1->fY));
        AddXMLElementFloat(&xmlDocument, "Z", &(pCorner1->fZ));
        xmlDocument.OutOfElem(); // Corner1
    }
    if (pCorner2)
    {
        xmlDocument.AddElem("Corner2");
        xmlDocument.IntoElem();
        AddXMLElementFloat(&xmlDocument, "X", &(pCorner2->fX));
        AddXMLElementFloat(&xmlDocument, "Y", &(pCorner2->fY));
        AddXMLElementFloat(&xmlDocument, "Z", &(pCorner2->fZ));
        xmlDocument.OutOfElem(); // Corner2
    }
    if (pCorner3)
    {
        xmlDocument.AddElem("Corner3");
        xmlDocument.IntoElem();
        AddXMLElementFloat(&xmlDocument, "X", &(pCorner3->fX));
        AddXMLElementFloat(&xmlDocument, "Y", &(pCorner3->fY));
        AddXMLElementFloat(&xmlDocument, "Z", &(pCorner3->fZ));
        xmlDocument.OutOfElem(); // Corner3
    }
    if (pCorner4)
    {
        xmlDocument.AddElem("Corner4");
        xmlDocument.IntoElem();
        AddXMLElementFloat(&xmlDocument, "X", &(pCorner4->fX));
        AddXMLElementFloat(&xmlDocument, "Y", &(pCorner4->fY));
        AddXMLElementFloat(&xmlDocument, "Z", &(pCorner4->fZ));
        xmlDocument.OutOfElem(); // Corner4
    }
    xmlDocument.OutOfElem(); // Plate

    xmlDocument.OutOfElem(); // Force
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs)
{
    CMarkup xmlDocument;

    xmlDocument.AddElem("QTM_Settings");
    xmlDocument.IntoElem();
    xmlDocument.AddElem("The_6D");
    xmlDocument.IntoElem();

    for (auto& body : pSettings6Dofs)
    {
        xmlDocument.AddElem("Body");
        xmlDocument.IntoElem();
        xmlDocument.AddElem("Name", body.name.c_str());
        xmlDocument.AddElem("Enabled", body.enabled ? "true" : "false");
        xmlDocument.AddElem("Color");
        xmlDocument.AddAttrib("R", std::to_string(body.color & 0xff).c_str());
        xmlDocument.AddAttrib("G", std::to_string((body.color >> 8) & 0xff).c_str());
        xmlDocument.AddAttrib("B", std::to_string((body.color >> 16) & 0xff).c_str());
        xmlDocument.AddElem("MaximumResidual", std::to_string(body.maxResidual).c_str());
        xmlDocument.AddElem("MinimumMarkersInBody", std::to_string(body.minMarkersInBody).c_str());
        xmlDocument.AddElem("BoneLengthTolerance", std::to_string(body.boneLengthTolerance).c_str());
        xmlDocument.AddElem("Filter");
        xmlDocument.AddAttrib("Preset", body.filterPreset.c_str());

        if (!body.mesh.name.empty())
        {
            xmlDocument.AddElem("Mesh");
            xmlDocument.IntoElem();
            xmlDocument.AddElem("Name", body.mesh.name.c_str());
            xmlDocument.AddElem("Position");
            xmlDocument.AddAttrib("X", std::to_string(body.mesh.position.fX).c_str());
            xmlDocument.AddAttrib("Y", std::to_string(body.mesh.position.fY).c_str());
            xmlDocument.AddAttrib("Z", std::to_string(body.mesh.position.fZ).c_str());
            xmlDocument.AddElem("Rotation");
            xmlDocument.AddAttrib("X", std::to_string(body.mesh.rotation.fX).c_str());
            xmlDocument.AddAttrib("Y", std::to_string(body.mesh.rotation.fY).c_str());
            xmlDocument.AddAttrib("Z", std::to_string(body.mesh.rotation.fZ).c_str());
            xmlDocument.AddElem("Scale", std::to_string(body.mesh.scale).c_str());
            xmlDocument.AddElem("Opacity", std::to_string(body.mesh.opacity).c_str());
            xmlDocument.OutOfElem(); // Mesh
        }

        if (!body.points.empty())
        {
            xmlDocument.AddElem("Points");
            xmlDocument.IntoElem();
            for (auto& point : body.points)
            {
                xmlDocument.AddElem("Point");
                xmlDocument.AddAttrib("X", std::to_string(point.fX).c_str());
                xmlDocument.AddAttrib("Y", std::to_string(point.fY).c_str());
                xmlDocument.AddAttrib("Z", std::to_string(point.fZ).c_str());
                xmlDocument.AddAttrib("Virtual", point.virtual_ ? "1" : "0");
                xmlDocument.AddAttrib("PhysicalId", std::to_string(point.physicalId).c_str());
                xmlDocument.AddAttrib("Name", point.name.c_str());
            }
            xmlDocument.OutOfElem(); // Points
        }
        xmlDocument.AddElem("Data_origin", std::to_string(body.origin.type).c_str());
        xmlDocument.AddAttrib("X", std::to_string(body.origin.position.fX).c_str());
        xmlDocument.AddAttrib("Y", std::to_string(body.origin.position.fY).c_str());
        xmlDocument.AddAttrib("Z", std::to_string(body.origin.position.fZ).c_str());
        xmlDocument.AddAttrib("Relative_body", std::to_string(body.origin.relativeBody).c_str());
        xmlDocument.AddElem("Data_orientation", std::to_string(body.origin.type).c_str());
        for (std::uint32_t i = 0; i < 9; i++)
        {
            char tmpStr[16];
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            xmlDocument.AddAttrib(tmpStr, std::to_string(body.origin.rotation[i]).c_str());
        }
        xmlDocument.AddAttrib("Relative_body", std::to_string(body.origin.relativeBody).c_str());

        xmlDocument.OutOfElem(); // Body
    }
    xmlDocument.OutOfElem(); // The_6D
    xmlDocument.OutOfElem(); // QTM_Settings

    return xmlDocument.GetDoc();
}

std::string CMarkupSerializer::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& pSettingsSkeletons)
{
    CMarkup xml;

    xml.AddElem("QTM_Settings");
    xml.IntoElem();
    xml.AddElem("Skeletons");
    xml.IntoElem();

    for (auto& skeleton : pSettingsSkeletons)
    {
        xml.AddElem("Skeleton");
        xml.SetAttrib("Name", skeleton.name.c_str());
        xml.IntoElem();
        if (mMajorVersion == 1 && mMinorVersion < 22)
        {
            xml.AddElem("Solver", skeleton.rootSegment.solver.c_str());
        }
        xml.AddElem("Scale", std::to_string(skeleton.scale).c_str());
        xml.AddElem("Segments");
        xml.IntoElem();

        std::function<void(const SSettingsSkeletonSegmentHierarchical)> recurseSegments = [&](const SSettingsSkeletonSegmentHierarchical& segment)
            {
                xml.AddElem("Segment");
                xml.SetAttrib("Name", segment.name.c_str());
                xml.IntoElem();
                {
                    if (mMajorVersion > 1 || mMinorVersion > 21)
                    {
                        xml.AddElem("Solver", segment.solver.c_str());
                    }
                    if (!std::isnan(segment.position.x))
                    {
                        AddXMLElementTransform(xml, "Transform", segment.position, segment.rotation);
                    }
                    if (!std::isnan(segment.defaultPosition.x))
                    {
                        AddXMLElementTransform(xml, "DefaultTransform", segment.defaultPosition, segment.defaultRotation);
                    }
                    xml.AddElem("DegreesOfFreedom");
                    xml.IntoElem();
                    for (auto dof : segment.degreesOfFreedom)
                    {
                        AddXMLElementDOF(xml, SkeletonDofToStringSettings(dof.type), dof);
                    }
                    xml.OutOfElem(); // DegreesOfFreedom

                    xml.AddElem("Endpoint");
                    {
                        if (!std::isnan(segment.endpoint.x) && !std::isnan(segment.endpoint.y) && !std::isnan(segment.endpoint.z))
                        {
                            xml.AddAttrib("X", std::to_string(segment.endpoint.x).c_str());
                            xml.AddAttrib("Y", std::to_string(segment.endpoint.y).c_str());
                            xml.AddAttrib("Z", std::to_string(segment.endpoint.z).c_str());
                        }
                    }

                    xml.AddElem("Markers");
                    xml.IntoElem();
                    {
                        for (const auto& marker : segment.markers)
                        {
                            xml.AddElem("Marker");
                            xml.AddAttrib("Name", marker.name.c_str());
                            xml.IntoElem();
                            {
                                xml.AddElem("Position");
                                xml.AddAttrib("X", std::to_string(marker.position.x).c_str());
                                xml.AddAttrib("Y", std::to_string(marker.position.y).c_str());
                                xml.AddAttrib("Z", std::to_string(marker.position.z).c_str());
                                xml.AddElem("Weight", std::to_string(marker.weight).c_str());
                            }
                            xml.OutOfElem(); // Marker
                        }
                    }
                    xml.OutOfElem(); // MarkerS

                    xml.AddElem("RigidBodies");
                    xml.IntoElem();
                    {
                        for (const auto& rigidBody : segment.bodies)
                        {
                            xml.AddElem("RigidBody");
                            xml.AddAttrib("Name", rigidBody.name.c_str());
                            xml.IntoElem();

                            xml.AddElem("Transform");
                            xml.IntoElem();

                            xml.AddElem("Position");
                            xml.AddAttrib("X", std::to_string(rigidBody.position.x).c_str());
                            xml.AddAttrib("Y", std::to_string(rigidBody.position.y).c_str());
                            xml.AddAttrib("Z", std::to_string(rigidBody.position.z).c_str());
                            xml.AddElem("Rotation");
                            xml.AddAttrib("X", std::to_string(rigidBody.rotation.x).c_str());
                            xml.AddAttrib("Y", std::to_string(rigidBody.rotation.y).c_str());
                            xml.AddAttrib("Z", std::to_string(rigidBody.rotation.z).c_str());
                            xml.AddAttrib("W", std::to_string(rigidBody.rotation.w).c_str());
                            xml.OutOfElem(); // Transform

                            xml.AddElem("Weight", std::to_string(rigidBody.weight).c_str());
                            xml.OutOfElem(); // RigidBody
                        }
                    }
                    xml.OutOfElem(); // RigidBodies
                }
                xml.OutOfElem(); // Segment

                for (const auto& childSegment : segment.segments)
                {
                    xml.IntoElem();
                    recurseSegments(childSegment);
                    xml.OutOfElem();
                }
            };
        recurseSegments(skeleton.rootSegment);

        xml.OutOfElem(); // Segments
        xml.OutOfElem(); // Skeleton
    }
    xml.OutOfElem(); // Skeleton

    return xml.GetDoc();
}


bool CMarkupDeserializer::ReadXMLDegreesOfFreedom(CMarkup& xml, const std::string& element, std::vector<SDegreeOfFreedom>& degreesOfFreedom)
{
    SDegreeOfFreedom degreeOfFreedom;

    if (xml.FindElem(element.c_str()))
    {
        degreeOfFreedom.type = SkeletonStringToDofSettings(element);
        ParseString(xml.GetAttrib("LowerBound"), degreeOfFreedom.lowerBound);
        ParseString(xml.GetAttrib("UpperBound"), degreeOfFreedom.upperBound);
        xml.IntoElem();
        if (xml.FindElem("Constraint"))
        {
            ParseString(xml.GetAttrib("LowerBound"), degreeOfFreedom.lowerBound);
            ParseString(xml.GetAttrib("UpperBound"), degreeOfFreedom.upperBound);
        }
        if (xml.FindElem("Couplings"))
        {
            xml.IntoElem();
            while (xml.FindElem("Coupling"))
            {
                SCoupling coupling;
                coupling.segment = xml.GetAttrib("Segment");
                auto dof = xml.GetAttrib("DegreeOfFreedom");
                coupling.degreeOfFreedom = SkeletonStringToDofSettings(dof);
                ParseString(xml.GetAttrib("Coefficient"), coupling.coefficient);
                degreeOfFreedom.couplings.push_back(coupling);
            }
            xml.OutOfElem();
        }
        if (xml.FindElem("Goal"))
        {
            ParseString(xml.GetAttrib("Value"), degreeOfFreedom.goalValue);
            ParseString(xml.GetAttrib("Weight"), degreeOfFreedom.goalWeight);
        }
        xml.OutOfElem();
        xml.ResetMainPos();

        degreesOfFreedom.push_back(degreeOfFreedom);
        return true;
    }

    return false;
}

