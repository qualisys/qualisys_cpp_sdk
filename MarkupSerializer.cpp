#define _CRT_SECURE_NO_WARNINGS

#include "MarkupSerializer.h"

#include <algorithm>
#include <map>

#include "Settings.h"
#include "Markup.h"
#include <functional>
#include <stdexcept>

using namespace CRTProtocolNs;

void CMarkupSerializer::AddXMLElementBool(CMarkup* oXML, const char* tTag, const bool* pbValue, const char* tTrue, const char* tFalse)
{
    if (pbValue)
    {
        oXML->AddElem(tTag, *pbValue ? tTrue : tFalse);
    }
}


void CMarkupSerializer::AddXMLElementBool(CMarkup* oXML, const char* tTag, const bool pbValue, const char* tTrue, const char* tFalse)
{
    oXML->AddElem(tTag, pbValue ? tTrue : tFalse);
}


void CMarkupSerializer::AddXMLElementInt(CMarkup* oXML, const char* tTag, const int* pnValue)
{
    if (pnValue)
    {
        std::string tVal;

        tVal = CMarkup::Format("%d", *pnValue);
        oXML->AddElem(tTag, tVal.c_str());
    }
}


void CMarkupSerializer::AddXMLElementUnsignedInt(CMarkup* oXML, const char* tTag, const unsigned int value)
{
    std::string tVal = CMarkup::Format("%u", value);
    oXML->AddElem(tTag, tVal.c_str());
}

void CMarkupSerializer::AddXMLElementUnsignedInt(CMarkup* oXML, const char* tTag, const unsigned int* pnValue)
{
    if (pnValue)
    {
        AddXMLElementUnsignedInt(oXML, tTag, *pnValue);
    }
}

void CMarkupSerializer::AddXMLElementFloat(CMarkup* oXML, const char* tTag, const float* pfValue, unsigned int pnDecimals)
{
    if (pfValue)
    {
        std::string tVal;
        char fFormat[10];

        sprintf(fFormat, "%%.%df", pnDecimals);
        tVal = CMarkup::Format(fFormat, *pfValue);
        oXML->AddElem(tTag, tVal.c_str());
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
        if (mnMajorVersion > 1 || mnMinorVersion > 21)
        {
            xml.IntoElem();
            xml.AddElem("Constraint");
        }
        xml.AddAttrib("LowerBound", std::to_string(degreeOfFreedoms.lowerBound).c_str());
        xml.AddAttrib("UpperBound", std::to_string(degreeOfFreedoms.upperBound).c_str());
    }

    if (std::isnan(degreeOfFreedoms.lowerBound) || std::isnan(degreeOfFreedoms.upperBound) || (mnMajorVersion == 1 && mnMinorVersion < 22))
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


CMarkupDeserializer::CMarkupDeserializer(const char* data, std::uint32_t versionMajor, std::uint32_t versionMinor)
    : mnMajorVersion(versionMajor), mnMinorVersion(versionMinor), maErrorStr{0}, oXML(data)
{
}

bool CMarkupDeserializer::DeserializeGeneralSettings(SSettingsGeneral& msGeneralSettings)
{
    std::string             tStr;

    msGeneralSettings.vsCameras.clear();

    // ==================== General ====================
    if (!oXML.FindChildElem("General"))
    {
        return false;
    }
    oXML.IntoElem();

    if (!oXML.FindChildElem("Frequency"))
    {
        return false;
    }
    msGeneralSettings.nCaptureFrequency = atoi(oXML.GetChildData().c_str());

    if (!oXML.FindChildElem("Capture_Time"))
    {
        return false;
    }
    msGeneralSettings.fCaptureTime = (float)atof(oXML.GetChildData().c_str());

    // Refactored variant of all this copy/paste code. TODO: Refactor everything else.
    if (!ReadXmlBool(&oXML, "Start_On_External_Trigger", msGeneralSettings.bStartOnExternalTrigger))
    {
        return false;
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 14)
    {
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_NO", msGeneralSettings.bStartOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_NC", msGeneralSettings.bStartOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_Software", msGeneralSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }

    // ==================== External time base ====================
    if (!oXML.FindChildElem("External_Time_Base"))
    {
        return false;
    }
    oXML.IntoElem();

    if (!oXML.FindChildElem("Enabled"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    msGeneralSettings.sExternalTimebase.bEnabled = (tStr == "true");

    if (!oXML.FindChildElem("Signal_Source"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    if (tStr == "control port")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceControlPort;
    }
    else if (tStr == "ir receiver")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceIRReceiver;
    }
    else if (tStr == "smpte")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceSMPTE;
    }
    else if (tStr == "irig")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceIRIG;
    }
    else if (tStr == "video sync")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceVideoSync;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Signal_Mode"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    if (tStr == "periodic")
    {
        msGeneralSettings.sExternalTimebase.bSignalModePeriodic = true;
    }
    else if (tStr == "non-periodic")
    {
        msGeneralSettings.sExternalTimebase.bSignalModePeriodic = false;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Frequency_Multiplier"))
    {
        return false;
    }
    unsigned int nMultiplier;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nMultiplier) == 1)
    {
        msGeneralSettings.sExternalTimebase.nFreqMultiplier = nMultiplier;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Frequency_Divisor"))
    {
        return false;
    }
    unsigned int nDivisor;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nDivisor) == 1)
    {
        msGeneralSettings.sExternalTimebase.nFreqDivisor = nDivisor;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Frequency_Tolerance"))
    {
        return false;
    }
    unsigned int nTolerance;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nTolerance) == 1)
    {
        msGeneralSettings.sExternalTimebase.nFreqTolerance = nTolerance;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Nominal_Frequency"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());

    if (tStr == "none")
    {
        msGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
    }
    else
    {
        float fFrequency;
        if (sscanf(tStr.c_str(), "%f", &fFrequency) == 1)
        {
            msGeneralSettings.sExternalTimebase.fNominalFrequency = fFrequency;
        }
        else
        {
            return false;
        }
    }

    if (!oXML.FindChildElem("Signal_Edge"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    if (tStr == "negative")
    {
        msGeneralSettings.sExternalTimebase.bNegativeEdge = true;
    }
    else if (tStr == "positive")
    {
        msGeneralSettings.sExternalTimebase.bNegativeEdge = false;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Signal_Shutter_Delay"))
    {
        return false;
    }
    unsigned int nDelay;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nDelay) == 1)
    {
        msGeneralSettings.sExternalTimebase.nSignalShutterDelay = nDelay;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Non_Periodic_Timeout"))
    {
        return false;
    }
    float fTimeout;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%f", &fTimeout) == 1)
    {
        msGeneralSettings.sExternalTimebase.fNonPeriodicTimeout = fTimeout;
    }
    else
    {
        return false;
    }

    oXML.OutOfElem(); // External_Time_Base


    // External_Timestamp
    if (oXML.FindChildElem("External_Timestamp"))
    {
        oXML.IntoElem();

        if (oXML.FindChildElem("Enabled"))
        {
            tStr = ToLower(oXML.GetChildData());
            msGeneralSettings.sTimestamp.bEnabled = (tStr == "true");
        }
        if (oXML.FindChildElem("Type"))
        {
            tStr = ToLower(oXML.GetChildData());
            if (tStr == "smpte")
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_SMPTE;
            }
            else if (tStr == "irig")
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_IRIG;
            }
            else
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_CameraTime;
            }
        }
        if (oXML.FindChildElem("Frequency"))
        {
            unsigned int timestampFrequency;
            tStr = oXML.GetChildData();
            if (sscanf(tStr.c_str(), "%u", &timestampFrequency) == 1)
            {
                msGeneralSettings.sTimestamp.nFrequency = timestampFrequency;
            }
        }
        oXML.OutOfElem();
    }
    // External_Timestamp


    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    EProcessingActions* processingActions[3] =
    {
        &msGeneralSettings.eProcessingActions,
        &msGeneralSettings.eRtProcessingActions,
        &msGeneralSettings.eReprocessingActions
    };
    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================
        if (!oXML.FindChildElem(processings[i]))
        {
            return false;
        }
        oXML.IntoElem();

        *processingActions[i] = ProcessingNone;

        if (mnMajorVersion > 1 || mnMinorVersion > 13)
        {
            if (!oXML.FindChildElem("PreProcessing2D"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingPreProcess2D);
            }
        }

        if (!oXML.FindChildElem("Tracking"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());
        if (tStr == "3d")
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking3D);
        }
        else if (tStr == "2d" && i != 1) // i != 1 => Not RtProcessingSettings
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking2D);
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!oXML.FindChildElem("TwinSystemMerge"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTwinSystemMerge);
            }

            if (!oXML.FindChildElem("SplineFill"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingSplineFill);
            }
        }

        if (!oXML.FindChildElem("AIM"))
        {
            return false;
        }
        if (CompareNoCase(oXML.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingAIM);
        }

        if (!oXML.FindChildElem("Track6DOF"))
        {
            return false;
        }
        if (CompareNoCase(oXML.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + Processing6DOFTracking);
        }

        if (!oXML.FindChildElem("ForceData"))
        {
            return false;
        }
        if (CompareNoCase(oXML.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingForceData);
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            if (!oXML.FindChildElem("GazeVector"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingGazeVector);
            }
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!oXML.FindChildElem("ExportTSV"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportTSV);
            }

            if (!oXML.FindChildElem("ExportC3D"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportC3D);
            }

            if (!oXML.FindChildElem("ExportMatlabFile"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportMatlabFile);
            }

            if (mnMajorVersion > 1 || mnMinorVersion > 11)
            {
                if (!oXML.FindChildElem("ExportAviFile"))
                {
                    return false;
                }
                if (CompareNoCase(oXML.GetChildData(), "true"))
                {
                    *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportAviFile);
                }
            }
        }
        oXML.OutOfElem(); // Processing_Actions
    }

    if (oXML.FindChildElem("EulerAngles"))
    {
        oXML.IntoElem();
        msGeneralSettings.eulerRotations[0] = oXML.GetAttrib("First");
        msGeneralSettings.eulerRotations[1] = oXML.GetAttrib("Second");
        msGeneralSettings.eulerRotations[2] = oXML.GetAttrib("Third");
        oXML.OutOfElem();
    }

    SSettingsGeneralCamera sCameraSettings;

    while (oXML.FindChildElem("Camera"))
    {
        oXML.IntoElem();

        if (!oXML.FindChildElem("ID"))
        {
            return false;
        }
        sCameraSettings.nID = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Model"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());

        if (tStr == "macreflex")
        {
            sCameraSettings.eModel = ModelMacReflex;
        }
        else if (tStr == "proreflex 120")
        {
            sCameraSettings.eModel = ModelProReflex120;
        }
        else if (tStr == "proreflex 240")
        {
            sCameraSettings.eModel = ModelProReflex240;
        }
        else if (tStr == "proreflex 500")
        {
            sCameraSettings.eModel = ModelProReflex500;
        }
        else if (tStr == "proreflex 1000")
        {
            sCameraSettings.eModel = ModelProReflex1000;
        }
        else if (tStr == "oqus 100")
        {
            sCameraSettings.eModel = ModelOqus100;
        }
        else if (tStr == "oqus 200" || tStr == "oqus 200 c")
        {
            sCameraSettings.eModel = ModelOqus200C;
        }
        else if (tStr == "oqus 300")
        {
            sCameraSettings.eModel = ModelOqus300;
        }
        else if (tStr == "oqus 300 plus")
        {
            sCameraSettings.eModel = ModelOqus300Plus;
        }
        else if (tStr == "oqus 400")
        {
            sCameraSettings.eModel = ModelOqus400;
        }
        else if (tStr == "oqus 500")
        {
            sCameraSettings.eModel = ModelOqus500;
        }
        else if (tStr == "oqus 500 plus")
        {
            sCameraSettings.eModel = ModelOqus500Plus;
        }
        else if (tStr == "oqus 700")
        {
            sCameraSettings.eModel = ModelOqus700;
        }
        else if (tStr == "oqus 700 plus")
        {
            sCameraSettings.eModel = ModelOqus700Plus;
        }
        else if (tStr == "oqus 600 plus")
        {
            sCameraSettings.eModel = ModelOqus600Plus;
        }
        else if (tStr == "miqus m1")
        {
            sCameraSettings.eModel = ModelMiqusM1;
        }
        else if (tStr == "miqus m3")
        {
            sCameraSettings.eModel = ModelMiqusM3;
        }
        else if (tStr == "miqus m5")
        {
            sCameraSettings.eModel = ModelMiqusM5;
        }
        else if (tStr == "miqus sync unit")
        {
            sCameraSettings.eModel = ModelMiqusSyncUnit;
        }
        else if (tStr == "miqus video")
        {
            sCameraSettings.eModel = ModelMiqusVideo;
        }
        else if (tStr == "miqus video color")
        {
            sCameraSettings.eModel = ModelMiqusVideoColor;
        }
        else if (tStr == "miqus hybrid")
        {
            sCameraSettings.eModel = ModelMiqusHybrid;
        }
        else if (tStr == "miqus video color plus")
        {
            sCameraSettings.eModel = ModelMiqusVideoColorPlus;
        }
        else if (tStr == "arqus a5")
        {
            sCameraSettings.eModel = ModelArqusA5;
        }
        else if (tStr == "arqus a9")
        {
            sCameraSettings.eModel = ModelArqusA9;
        }
        else if (tStr == "arqus a12")
        {
            sCameraSettings.eModel = ModelArqusA12;
        }
        else if (tStr == "arqus a26")
        {
            sCameraSettings.eModel = ModelArqusA26;
        }
        else
        {
            sCameraSettings.eModel = ModelUnknown;
        }

        // Only available from protocol version 1.10 and later.
        if (oXML.FindChildElem("Underwater"))
        {
            tStr = ToLower(oXML.GetChildData());
            sCameraSettings.bUnderwater = (tStr == "true");
        }

        if (oXML.FindChildElem("Supports_HW_Sync"))
        {
            tStr = ToLower(oXML.GetChildData());
            sCameraSettings.bSupportsHwSync = (tStr == "true");
        }

        if (!oXML.FindChildElem("Serial"))
        {
            return false;
        }
        sCameraSettings.nSerial = atoi(oXML.GetChildData().c_str());

        // ==================== Camera Mode ====================
        if (!oXML.FindChildElem("Mode"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());
        if (tStr == "marker")
        {
            sCameraSettings.eMode = ModeMarker;
        }
        else if (tStr == "marker intensity")
        {
            sCameraSettings.eMode = ModeMarkerIntensity;
        }
        else if (tStr == "video")
        {
            sCameraSettings.eMode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            // ==================== Video frequency ====================
            if (!oXML.FindChildElem("Video_Frequency"))
            {
                return false;
            }
            sCameraSettings.nVideoFrequency = atoi(oXML.GetChildData().c_str());
        }

        // ==================== Video Resolution ====================
        if (oXML.FindChildElem("Video_Resolution"))
        {
            tStr = ToLower(oXML.GetChildData());
            if (tStr == "1440p")
            {
                sCameraSettings.eVideoResolution = VideoResolution1440p;
            }
            else if (tStr == "1080p")
            {
                sCameraSettings.eVideoResolution = VideoResolution1080p;
            }
            else if (tStr == "720p")
            {
                sCameraSettings.eVideoResolution = VideoResolution720p;
            }
            else if (tStr == "540p")
            {
                sCameraSettings.eVideoResolution = VideoResolution540p;
            }
            else if (tStr == "480p")
            {
                sCameraSettings.eVideoResolution = VideoResolution480p;
            }
            else
            {
                return false;
            }
        }
        else
        {
            sCameraSettings.eVideoResolution = VideoResolutionNone;
        }

        // ==================== Video AspectRatio ====================
        if (oXML.FindChildElem("Video_Aspect_Ratio"))
        {
            tStr = ToLower(oXML.GetChildData());
            if (tStr == "16x9")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
            }
            else if (tStr == "4x3")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
            }
            else if (tStr == "1x1")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
            }
        }
        else
        {
            sCameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
        }

        // ==================== Video exposure ====================
        if (!oXML.FindChildElem("Video_Exposure"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nVideoExposure = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nVideoExposureMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nVideoExposureMax = atoi(oXML.GetChildData().c_str());
        oXML.OutOfElem(); // Video_Exposure

        // ==================== Video flash time ====================
        if (!oXML.FindChildElem("Video_Flash_Time"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nVideoFlashTime = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nVideoFlashTimeMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nVideoFlashTimeMax = atoi(oXML.GetChildData().c_str());
        oXML.OutOfElem(); // Video_Flash_Time

        // ==================== Marker exposure ====================
        if (!oXML.FindChildElem("Marker_Exposure"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nMarkerExposure = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nMarkerExposureMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nMarkerExposureMax = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_Exposure

        // ==================== Marker threshold ====================
        if (!oXML.FindChildElem("Marker_Threshold"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nMarkerThreshold = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nMarkerThresholdMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nMarkerThresholdMax = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_Threshold

        // ==================== Position ====================
        if (!oXML.FindChildElem("Position"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("X"))
        {
            return false;
        }
        sCameraSettings.fPositionX = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Y"))
        {
            return false;
        }
        sCameraSettings.fPositionY = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Z"))
        {
            return false;
        }
        sCameraSettings.fPositionZ = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_1"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[0][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_1"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[1][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_1"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[2][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_2"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[0][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_2"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[1][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_2"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[2][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_3"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[0][2] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_3"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[1][2] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_3"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[2][2] = (float)atof(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Position


        if (!oXML.FindChildElem("Orientation"))
        {
            return false;
        }
        sCameraSettings.nOrientation = atoi(oXML.GetChildData().c_str());

        // ==================== Marker resolution ====================
        if (!oXML.FindChildElem("Marker_Res"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sCameraSettings.nMarkerResolutionWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sCameraSettings.nMarkerResolutionHeight = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_Res

        // ==================== Video resolution ====================
        if (!oXML.FindChildElem("Video_Res"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sCameraSettings.nVideoResolutionWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sCameraSettings.nVideoResolutionHeight = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Video_Res

        // ==================== Marker FOV ====================
        if (!oXML.FindChildElem("Marker_FOV"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Left"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVLeft = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVTop = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVRight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVBottom = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_FOV

        // ==================== Video FOV ====================
        if (!oXML.FindChildElem("Video_FOV"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Left"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVLeft = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVTop = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVRight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVBottom = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Video_FOV

        // ==================== Sync out ====================
        // Only available from protocol version 1.10 and later.
        for (int port = 0; port < 3; port++)
        {
            char syncOutStr[16];
            sprintf(syncOutStr, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));
            if (oXML.FindChildElem(syncOutStr))
            {
                oXML.IntoElem();

                if (port < 2)
                {
                    if (!oXML.FindChildElem("Mode"))
                    {
                        return false;
                    }
                    tStr = ToLower(oXML.GetChildData());
                    if (tStr == "shutter out")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeShutterOut;
                    }
                    else if (tStr == "multiplier")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeMultiplier;
                    }
                    else if (tStr == "divisor")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeDivisor;
                    }
                    else if (tStr == "camera independent")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (tStr == "measurement time")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (tStr == "continuous 100hz")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
                    }
                    else if (tStr == "system live time")
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
                        if (!oXML.FindChildElem("Value"))
                        {
                            return false;
                        }
                        sCameraSettings.nSyncOutValue[port] = atoi(oXML.GetChildData().c_str());

                        if (!oXML.FindChildElem("Duty_Cycle"))
                        {
                            return false;
                        }
                        sCameraSettings.fSyncOutDutyCycle[port] = (float)atof(oXML.GetChildData().c_str());
                    }
                }
                if (port == 2 ||
                    (sCameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    if (!oXML.FindChildElem("Signal_Polarity"))
                    {
                        return false;
                    }
                    if (CompareNoCase(oXML.GetChildData(), "negative"))
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = true;
                    }
                    else
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = false;
                    }
                }
                oXML.OutOfElem(); // Sync_Out
            }
            else
            {
                sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                sCameraSettings.nSyncOutValue[port] = 0;
                sCameraSettings.fSyncOutDutyCycle[port] = 0;
                sCameraSettings.bSyncOutNegativePolarity[port] = false;
            }
        }

        if (oXML.FindChildElem("LensControl"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Focus"))
            {
                oXML.IntoElem();
                float focus;
                if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &focus) == 1)
                {
                    sCameraSettings.fFocus = focus;
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Aperture"))
            {
                oXML.IntoElem();
                float aperture;
                if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &aperture) == 1)
                {
                    sCameraSettings.fAperture = aperture;
                }
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }
        else
        {
            sCameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            sCameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        if (oXML.FindChildElem("AutoExposure"))
        {
            oXML.IntoElem();
            if (CompareNoCase(oXML.GetAttrib("Enabled"), "true"))
            {
                sCameraSettings.autoExposureEnabled = true;
            }
            float autoExposureCompensation;
            if (sscanf(oXML.GetAttrib("Compensation").c_str(), "%f", &autoExposureCompensation) == 1)
            {
                sCameraSettings.autoExposureCompensation = autoExposureCompensation;
            }
            oXML.OutOfElem();
        }
        else
        {
            sCameraSettings.autoExposureEnabled = false;
            sCameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        if (oXML.FindChildElem("AutoWhiteBalance"))
        {
            sCameraSettings.autoWhiteBalance = CompareNoCase(oXML.GetChildData().c_str(), "true") ? 1 : 0;
        }
        else
        {
            sCameraSettings.autoWhiteBalance = -1;
        }

        oXML.OutOfElem(); // Camera

        msGeneralSettings.vsCameras.push_back(sCameraSettings);
    }

    return true;

}

bool CMarkupDeserializer::Deserialize3DSettings(SSettings3D& ms3DSettings, bool& bDataAvailable)
{
    std::string tStr;

    bDataAvailable = false;

    ms3DSettings.s3DLabels.clear();
    ms3DSettings.pCalibrationTime[0] = 0;

    if (!oXML.FindChildElem("The_3D"))
    {
        // No 3D data available.
        return true;
    }
    oXML.IntoElem();

    if (!oXML.FindChildElem("AxisUpwards"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());

    if (tStr == "+x")
    {
        ms3DSettings.eAxisUpwards = XPos;
    }
    else if (tStr == "-x")
    {
        ms3DSettings.eAxisUpwards = XNeg;
    }
    else if (tStr == "+y")
    {
        ms3DSettings.eAxisUpwards = YPos;
    }
    else if (tStr == "-y")
    {
        ms3DSettings.eAxisUpwards = YNeg;
    }
    else if (tStr == "+z")
    {
        ms3DSettings.eAxisUpwards = ZPos;
    }
    else if (tStr == "-z")
    {
        ms3DSettings.eAxisUpwards = ZNeg;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("CalibrationTime"))
    {
        return false;
    }
    tStr = oXML.GetChildData();
    strcpy(ms3DSettings.pCalibrationTime, tStr.c_str());

    if (!oXML.FindChildElem("Labels"))
    {
        return false;
    }
    unsigned int nNumberOfLabels = atoi(oXML.GetChildData().c_str());

    ms3DSettings.s3DLabels.resize(nNumberOfLabels);
    SSettings3DLabel sLabel;

    for (unsigned int iLabel = 0; iLabel < nNumberOfLabels; iLabel++)
    {
        if (oXML.FindChildElem("Label"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Name"))
            {
                sLabel.oName = oXML.GetChildData();
                if (oXML.FindChildElem("RGBColor"))
                {
                    sLabel.nRGBColor = atoi(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Trajectory_Type"))
                {
                    sLabel.type = oXML.GetChildData();
                }
                ms3DSettings.s3DLabels[iLabel] = sLabel;
            }
            oXML.OutOfElem();
        }
        else
        {
            return false;
        }
    }

    ms3DSettings.sBones.clear();
    if (oXML.FindChildElem("Bones"))
    {
        oXML.IntoElem();
        while (oXML.FindChildElem("Bone"))
        {
            oXML.IntoElem();
            SSettingsBone bone = { };
            bone.fromName = oXML.GetAttrib("From").c_str();
            bone.toName = oXML.GetAttrib("To").c_str();

            auto colorString = oXML.GetAttrib("Color");
            if (!colorString.empty())
            {
                bone.color = atoi(colorString.c_str());
            }
            ms3DSettings.sBones.push_back(bone);
            oXML.OutOfElem();
        }
        oXML.OutOfElem();
    }

    bDataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryReadSetEnabled(const int nMajorVer, const int nMinorVer, CMarkup& oXML, bool& bTarget)
    {
        if (nMajorVer > 1 || nMinorVer > 23)
        {
            if (!oXML.FindChildElem("Enabled"))
            {
                bTarget = true;
                return true;
            }

            bTarget = oXML.GetChildData() == "true" ? true : false;
            return false;
        }

        return false;
    }

    bool TryReadSetName(CMarkup& oXML, std::string& sTarget)
    {
        if (!oXML.FindChildElem("Name"))
        {
            return false;
        }
        sTarget = oXML.GetChildData();
        return true;
    }

    bool TryReadSetColor(CMarkup& oXML, std::uint32_t& nTarget)
    {
        if (!oXML.FindChildElem("Color"))
        {
            return false;
        }
        std::uint32_t colorR = atoi(oXML.GetChildAttrib("R").c_str());
        std::uint32_t colorG = atoi(oXML.GetChildAttrib("G").c_str());
        std::uint32_t colorB = atoi(oXML.GetChildAttrib("B").c_str());
        nTarget = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);

        return true;
    }

    bool TryReadSetMaxResidual(CMarkup& oXML, float& fTarget)
    {
        if (!oXML.FindChildElem("MaximumResidual"))
        {
            return false;
        }
        fTarget = (float)atof(oXML.GetChildData().c_str());

        return true;
    }

    bool TryReadSetMinMarkersInBody(CMarkup& oXML, std::uint32_t& nTarget)
    {
        if (!oXML.FindChildElem("MinimumMarkersInBody"))
        {
            return false;
        }
        nTarget = atoi(oXML.GetChildData().c_str());

        return true;
    }

    bool TryReadSetBoneLenTolerance(CMarkup& oXML, float& fTarget)
    {
        if (!oXML.FindChildElem("BoneLengthTolerance"))
        {
            return false;
        }
        fTarget = (float)atof(oXML.GetChildData().c_str());

        return true;
    }

    bool TryReadSetFilter(CMarkup& oXML, std::string& sTarget)
    {
        if (!oXML.FindChildElem("Filter"))
        {
            return false;
        }
        sTarget = oXML.GetChildAttrib("Preset");

        return true;
    }

    bool TryReadSetPos(CMarkup& oXML, float& fTargetX, float& fTargetY, float& fTargetZ)
    {
        if (!oXML.FindChildElem("Position"))
        {
            return false;
        }
        fTargetX = (float)atof(oXML.GetChildAttrib("X").c_str());
        fTargetY = (float)atof(oXML.GetChildAttrib("Y").c_str());
        fTargetZ = (float)atof(oXML.GetChildAttrib("Z").c_str());

        return true;
    }

    bool TryReadSetRotation(CMarkup& oXML, float& fTargetX, float& fTargetY, float& fTargetZ)
    {
        if (!oXML.FindChildElem("Rotation"))
        {
            return false;
        }
        fTargetX = (float)atof(oXML.GetChildAttrib("X").c_str());
        fTargetY = (float)atof(oXML.GetChildAttrib("Y").c_str());
        fTargetZ = (float)atof(oXML.GetChildAttrib("Z").c_str());

        return true;
    }

    bool TryReadSetScale(CMarkup& oXML, float& fTarget)
    {
        if (!oXML.FindChildElem("Scale"))
        {
            return false;
        }
        fTarget = (float)atof(oXML.GetChildData().c_str());

        return true;
    }

    bool TryReadSetOpacity(CMarkup& oXML, float& fTarget)
    {
        if (!oXML.FindChildElem("Opacity"))
        {
            return false;
        }
        fTarget = (float)atof(oXML.GetChildData().c_str());

        return true;
    }

    bool TryReadSetPoints(CMarkup& oXML, std::vector<SBodyPoint>& vTarget)
    {
        if (oXML.FindChildElem("Points"))
        {
            oXML.IntoElem();

            while (oXML.FindChildElem("Point"))
            {
                SBodyPoint sBodyPoint;

                sBodyPoint.fX = (float)atof(oXML.GetChildAttrib("X").c_str());
                sBodyPoint.fY = (float)atof(oXML.GetChildAttrib("Y").c_str());
                sBodyPoint.fZ = (float)atof(oXML.GetChildAttrib("Z").c_str());

                sBodyPoint.virtual_ = (0 != atoi(oXML.GetChildAttrib("Virtual").c_str()));
                sBodyPoint.physicalId = atoi(oXML.GetChildAttrib("PhysicalId").c_str());
                sBodyPoint.name = oXML.GetChildAttrib("Name");
                vTarget.push_back(sBodyPoint);
            }
            oXML.OutOfElem(); // Points

            return true;
        }

        return false;
    }

    bool TryReadSetDataOrigin(CMarkup& oXML, SOrigin& oTarget)
    {
        if (!oXML.FindChildElem("Data_origin"))
        {
            return false;
        }
        oTarget.type = (EOriginType)atoi(oXML.GetChildData().c_str());
        oTarget.position.fX = (float)atof(oXML.GetChildAttrib("X").c_str());
        oTarget.position.fY = (float)atof(oXML.GetChildAttrib("Y").c_str());
        oTarget.position.fZ = (float)atof(oXML.GetChildAttrib("Z").c_str());
        oTarget.relativeBody = atoi(oXML.GetChildAttrib("Relative_body").c_str());

        return true;
    }

    void ReadSetRotations(CMarkup& oXML, SOrigin& oTarget)
    {
        char tmpStr[10];
        for (std::uint32_t i = 0; i < 9; i++)
        {
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            oTarget.rotation[i] = (float)atof(oXML.GetChildAttrib(tmpStr).c_str());
        }
    }

    bool TryReadSetRGBColor(CMarkup& oXML, std::uint32_t& oTarget)
    {
        if (!oXML.FindChildElem("RGBColor"))
        {
            return false;
        }
        oTarget = atoi(oXML.GetChildData().c_str());

        return true;
    }

    bool TryReadSetPointsOld(CMarkup& oXML, std::vector<SBodyPoint>& vTarget)
    {
        vTarget.clear();

        while (oXML.FindChildElem("Point"))
        {
            SBodyPoint sPoint;

            oXML.IntoElem();
            if (!oXML.FindChildElem("X"))
            {
                return false;
            }
            sPoint.fX = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Y"))
            {
                return false;
            }
            sPoint.fY = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Z"))
            {
                return false;
            }
            sPoint.fZ = (float)atof(oXML.GetChildData().c_str());

            oXML.OutOfElem(); // Point
            vTarget.push_back(sPoint);
        }

        return true;
    }

    bool TryReadSetEuler(CMarkup& oXML, std::string& sTargetFirst, std::string& sTargetSecond, std::string& sTargetThird)
    {
        if (oXML.FindChildElem("Euler"))
        {
            oXML.IntoElem();
            if (!oXML.FindChildElem("First"))
            {
                return false;
            }
            sTargetFirst = oXML.GetChildData();
            if (!oXML.FindChildElem("Second"))
            {
                return false;
            }
            sTargetSecond = oXML.GetChildData();
            if (!oXML.FindChildElem("Third"))
            {
                return false;
            }
            sTargetThird = oXML.GetChildData();
            oXML.OutOfElem(); // Euler
        }

        return true;
    }
}

bool CMarkupDeserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& m6DOFSettings, bool& bDataAvailable)
{
    bDataAvailable = false;

    m6DOFSettings.clear();

    if (oXML.FindChildElem("The_6D"))
    {
        oXML.IntoElem();

        if (mnMajorVersion > 1 || mnMinorVersion > 20)
        {
            while (oXML.FindChildElem("Body"))
            {
                SSettings6DOFBody s6DOFBodySettings;
                SBodyPoint sBodyPoint;

                oXML.IntoElem();

                // NOTE: READ-ORDER MATTERS!!!
                if (!TryReadSetName(oXML, s6DOFBodySettings.name))
                { // Name --- REQUIRED
                    return false;
                }
                // Enabled --- NOT(!) REQUIRED
                TryReadSetEnabled(mnMajorVersion, mnMinorVersion, oXML, s6DOFBodySettings.enabled);
                if (!TryReadSetColor(oXML, s6DOFBodySettings.color)
                    || !TryReadSetMaxResidual(oXML, s6DOFBodySettings.maxResidual)
                    || !TryReadSetMinMarkersInBody(oXML, s6DOFBodySettings.minMarkersInBody)
                    || !TryReadSetBoneLenTolerance(oXML, s6DOFBodySettings.boneLengthTolerance)
                    || !TryReadSetFilter(oXML, s6DOFBodySettings.filterPreset))
                { // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                    return false;
                }

                if (oXML.FindChildElem("Mesh"))
                {
                    oXML.IntoElem();

                    if (!TryReadSetName(oXML, s6DOFBodySettings.mesh.name)
                        || !TryReadSetPos(oXML, s6DOFBodySettings.mesh.position.fX, s6DOFBodySettings.mesh.position.fY, s6DOFBodySettings.mesh.position.fZ)
                        || !TryReadSetRotation(oXML, s6DOFBodySettings.mesh.rotation.fX, s6DOFBodySettings.mesh.rotation.fY, s6DOFBodySettings.mesh.rotation.fZ)
                        || !TryReadSetScale(oXML, s6DOFBodySettings.mesh.scale)
                        || !TryReadSetOpacity(oXML, s6DOFBodySettings.mesh.opacity))
                    { // Name, Position, Rotation, Scale, Opacity --- REQUIRED
                        return false;
                    }

                    oXML.OutOfElem(); // Mesh
                }

                // Points --- REQUIRED
                TryReadSetPoints(oXML, s6DOFBodySettings.points);
                if (!TryReadSetDataOrigin(oXML, s6DOFBodySettings.origin)
                    || !oXML.FindChildElem("Data_orientation")
                    || s6DOFBodySettings.origin.type != atoi(oXML.GetChildData().c_str())
                    || s6DOFBodySettings.origin.relativeBody != static_cast<std::uint32_t>(atoi(oXML.GetChildAttrib("Relative_body").c_str()))
                    )
                { // Data Orientation, Origin Type / Relative Body --- REQUIRED
                    return false;
                }

                // Rotation values --- NOTE : Does NOT(!) 'Try'; just reads and sets (no boolean return)
                ReadSetRotations(oXML, s6DOFBodySettings.origin);

                m6DOFSettings.push_back(s6DOFBodySettings);
                oXML.OutOfElem(); // Body

                bDataAvailable = true;
            }
        }
        else
        {
            if (!oXML.FindChildElem("Bodies"))
            {
                return false;
            }
            int nBodies = atoi(oXML.GetChildData().c_str());
            SSettings6DOFBody s6DOFBodySettings;

            for (int iBody = 0; iBody < nBodies; iBody++)
            {
                if (!oXML.FindChildElem("Body"))
                {
                    return false;
                }
                oXML.IntoElem();

                if (!TryReadSetName(oXML, s6DOFBodySettings.name)
                    || !TryReadSetRGBColor(oXML, s6DOFBodySettings.color)
                    || !TryReadSetPointsOld(oXML, s6DOFBodySettings.points))
                { // Name, RGBColor, Points(OLD) --- REQUIRED
                    return false;
                }

                m6DOFSettings.push_back(s6DOFBodySettings);
                oXML.OutOfElem(); // Body
            }
            if (mnMajorVersion > 1 || mnMinorVersion > 15)
            {
                // TODO: Figure this out
                //if (!TryReadSetEuler(oXML, msGeneralSettings.eulerRotations[0], msGeneralSettings.eulerRotations[1], msGeneralSettings.eulerRotations[2]))
                //{ // Euler --- REQUIRED
                    return false;
                //}
            }
            bDataAvailable = true;
        }
    }

    return true;
} // Read6DOFSettings

bool CMarkupDeserializer::DeserializeGazeVectorSettings(std::vector<SGazeVector>& mvsGazeVectorSettings, bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsGazeVectorSettings.clear();

    //
    // Read gaze vectors
    //
    if (!oXML.FindChildElem("Gaze_Vector"))
    {
        return true; // NO gaze vector data available.
    }
    oXML.IntoElem();

    std::string tGazeVectorName;

    int nGazeVectorCount = 0;

    while (oXML.FindChildElem("Vector"))
    {
        oXML.IntoElem();

        if (!oXML.FindChildElem("Name"))
        {
            return false;
        }
        tGazeVectorName = oXML.GetChildData();

        float frequency = 0;
        if (oXML.FindChildElem("Frequency"))
        {
            frequency = (float)atof(oXML.GetChildData().c_str());
        }

        bool hwSync = false;
        ReadXmlBool(&oXML, "Hardware_Sync", hwSync);
        bool filter = false;
        ReadXmlBool(&oXML, "Filter", filter);

        mvsGazeVectorSettings.push_back({ tGazeVectorName, frequency, hwSync, filter });
        nGazeVectorCount++;
        oXML.OutOfElem(); // Vector
    }

    bDataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CMarkupDeserializer::DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& mvsEyeTrackerSettings,bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsEyeTrackerSettings.clear();

    if (!oXML.FindChildElem("Eye_Tracker"))
    {
        return true; // NO eye tracker data available.
    }
    oXML.IntoElem();

    std::string tEyeTrackerName;

    int nEyeTrackerCount = 0;

    while (oXML.FindChildElem("Device"))
    {
        oXML.IntoElem();

        if (!oXML.FindChildElem("Name"))
        {
            return false;
        }
        tEyeTrackerName = oXML.GetChildData();

        float frequency = 0;
        if (oXML.FindChildElem("Frequency"))
        {
            frequency = (float)atof(oXML.GetChildData().c_str());
        }

        bool hwSync = false;
        ReadXmlBool(&oXML, "Hardware_Sync", hwSync);

        mvsEyeTrackerSettings.push_back({ tEyeTrackerName, frequency, hwSync });
        nEyeTrackerCount++;
        oXML.OutOfElem(); // Vector
    }

    bDataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CMarkupDeserializer::DeserializeAnalogSettings(std::vector<SAnalogDevice>& mvsAnalogDeviceSettings, bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsAnalogDeviceSettings.clear();

    if (!oXML.FindChildElem("Analog"))
    {
        // No analog data available.
        return true;
    }

    SAnalogDevice sAnalogDevice;

    oXML.IntoElem();

    if (mnMajorVersion == 1 && mnMinorVersion == 0)
    {
        sAnalogDevice.nDeviceID = 1;   // Always channel 1
        sAnalogDevice.oName = "AnalogDevice";
        if (!oXML.FindChildElem("Channels"))
        {
            return false;
        }
        sAnalogDevice.nChannels = atoi(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Frequency"))
        {
            return false;
        }
        sAnalogDevice.nFrequency = atoi(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Unit"))
        {
            return false;
        }
        sAnalogDevice.oUnit = oXML.GetChildData();
        if (!oXML.FindChildElem("Range"))
        {
            return false;
        }
        oXML.IntoElem();
        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sAnalogDevice.fMinRange = (float)atof(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sAnalogDevice.fMaxRange = (float)atof(oXML.GetChildData().c_str());
        mvsAnalogDeviceSettings.push_back(sAnalogDevice);
        bDataAvailable = true;
        return true;
    }
    else
    {
        while (oXML.FindChildElem("Device"))
        {
            sAnalogDevice.voLabels.clear();
            sAnalogDevice.voUnits.clear();
            oXML.IntoElem();
            if (!oXML.FindChildElem("Device_ID"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nDeviceID = atoi(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Device_Name"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.oName = oXML.GetChildData();

            if (!oXML.FindChildElem("Channels"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nChannels = atoi(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Frequency"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nFrequency = atoi(oXML.GetChildData().c_str());

            if (mnMajorVersion == 1 && mnMinorVersion < 11)
            {
                if (!oXML.FindChildElem("Unit"))
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
                sAnalogDevice.oUnit = oXML.GetChildData();
            }
            if (!oXML.FindChildElem("Range"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            oXML.IntoElem();

            if (!oXML.FindChildElem("Min"))
            {
                oXML.OutOfElem(); // Device
                oXML.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.fMinRange = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Max"))
            {
                oXML.OutOfElem(); // Device
                oXML.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.fMaxRange = (float)atof(oXML.GetChildData().c_str());
            oXML.OutOfElem(); // Range

            if (mnMajorVersion == 1 && mnMinorVersion < 11)
            {
                for (unsigned int i = 0; i < sAnalogDevice.nChannels; i++)
                {
                    if (oXML.FindChildElem("Label"))
                    {
                        sAnalogDevice.voLabels.push_back(oXML.GetChildData());
                    }
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels)
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
            }
            else
            {
                while (oXML.FindChildElem("Channel"))
                {
                    oXML.IntoElem();
                    if (oXML.FindChildElem("Label"))
                    {
                        sAnalogDevice.voLabels.push_back(oXML.GetChildData());
                    }
                    if (oXML.FindChildElem("Unit"))
                    {
                        sAnalogDevice.voUnits.push_back(oXML.GetChildData());
                    }
                    oXML.OutOfElem(); // Channel
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels ||
                    sAnalogDevice.voUnits.size() != sAnalogDevice.nChannels)
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
            }
            oXML.OutOfElem(); // Device
            mvsAnalogDeviceSettings.push_back(sAnalogDevice);
            bDataAvailable = true;
        }
    }

    return true;
} // ReadAnalogSettings

bool CMarkupDeserializer::DeserializeForceSettings(SSettingsForce& msForceSettings, bool& bDataAvailable)
{
    bDataAvailable = false;

    msForceSettings.vsForcePlates.clear();

    //
    // Read some force plate parameters
    //
    if (!oXML.FindChildElem("Force"))
    {
        return true;
    }

    oXML.IntoElem();

    SForcePlate sForcePlate;
    sForcePlate.bValidCalibrationMatrix = false;
    for (int i = 0; i < 12; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            sForcePlate.afCalibrationMatrix[i][j] = 0.0f;
        }
    }
    sForcePlate.nCalibrationMatrixRows = 6;
    sForcePlate.nCalibrationMatrixColumns = 6;

    if (!oXML.FindChildElem("Unit_Length"))
    {
        return false;
    }
    msForceSettings.oUnitLength = oXML.GetChildData();

    if (!oXML.FindChildElem("Unit_Force"))
    {
        return false;
    }
    msForceSettings.oUnitForce = oXML.GetChildData();

    int  iPlate = 1;
    while (oXML.FindChildElem("Plate"))
    {
        //
        // Get name and type of the plates
        //
        oXML.IntoElem(); // "Plate"
        if (oXML.FindChildElem("Force_Plate_Index")) // Version 1.7 and earlier.
        {
            sForcePlate.nID = atoi(oXML.GetChildData().c_str());
        }
        else if (oXML.FindChildElem("Plate_ID")) // Version 1.8 and later.
        {
            sForcePlate.nID = atoi(oXML.GetChildData().c_str());
        }
        else
        {
            return false;
        }

        if (oXML.FindChildElem("Analog_Device_ID"))
        {
            sForcePlate.nAnalogDeviceID = atoi(oXML.GetChildData().c_str());
        }
        else
        {
            sForcePlate.nAnalogDeviceID = 0;
        }

        if (!oXML.FindChildElem("Frequency"))
        {
            return false;
        }
        sForcePlate.nFrequency = atoi(oXML.GetChildData().c_str());

        if (oXML.FindChildElem("Type"))
        {
            sForcePlate.oType = oXML.GetChildData();
        }
        else
        {
            sForcePlate.oType = "unknown";
        }

        if (oXML.FindChildElem("Name"))
        {
            sForcePlate.oName = oXML.GetChildData();
        }
        else
        {
            sForcePlate.oName = CMarkup::Format("#%d", iPlate);
        }

        if (oXML.FindChildElem("Length"))
        {
            sForcePlate.fLength = (float)atof(oXML.GetChildData().c_str());
        }
        if (oXML.FindChildElem("Width"))
        {
            sForcePlate.fWidth = (float)atof(oXML.GetChildData().c_str());
        }

        if (oXML.FindChildElem("Location"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Corner1"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[0].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[0].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[0].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner2"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[1].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[1].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[1].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner3"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[2].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[2].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[2].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner4"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[3].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[3].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[3].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }

        if (oXML.FindChildElem("Origin"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("X"))
            {
                sForcePlate.sOrigin.fX = (float)atof(oXML.GetChildData().c_str());
            }
            if (oXML.FindChildElem("Y"))
            {
                sForcePlate.sOrigin.fY = (float)atof(oXML.GetChildData().c_str());
            }
            if (oXML.FindChildElem("Z"))
            {
                sForcePlate.sOrigin.fZ = (float)atof(oXML.GetChildData().c_str());
            }
            oXML.OutOfElem();
        }

        sForcePlate.vChannels.clear();
        if (oXML.FindChildElem("Channels"))
        {
            oXML.IntoElem();
            SForceChannel sForceChannel;
            while (oXML.FindChildElem("Channel"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("Channel_No"))
                {
                    sForceChannel.nChannelNumber = atoi(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("ConversionFactor"))
                {
                    sForceChannel.fConversionFactor = (float)atof(oXML.GetChildData().c_str());
                }
                sForcePlate.vChannels.push_back(sForceChannel);
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }

        if (oXML.FindChildElem("Calibration_Matrix"))
        {
            oXML.IntoElem();
            int nRow = 0;

            if (mnMajorVersion == 1 && mnMinorVersion < 12)
            {
                char strRow[16];
                char strCol[16];
                sprintf(strRow, "Row%d", nRow + 1);
                while (oXML.FindChildElem(strRow))
                {
                    oXML.IntoElem();
                    int nCol = 0;
                    sprintf(strCol, "Col%d", nCol + 1);
                    while (oXML.FindChildElem(strCol))
                    {
                        sForcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
                        nCol++;
                        sprintf(strCol, "Col%d", nCol + 1);
                    }
                    sForcePlate.nCalibrationMatrixColumns = nCol;

                    nRow++;
                    sprintf(strRow, "Row%d", nRow + 1);
                    oXML.OutOfElem(); // RowX
                }
            }
            else
            {
                //<Rows>
                if (oXML.FindChildElem("Rows"))
                {
                    oXML.IntoElem();

                    while (oXML.FindChildElem("Row"))
                    {
                        oXML.IntoElem();

                        //<Columns>
                        if (oXML.FindChildElem("Columns"))
                        {
                            oXML.IntoElem();

                            int nCol = 0;
                            while (oXML.FindChildElem("Column"))
                            {
                                sForcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
                                nCol++;
                            }
                            sForcePlate.nCalibrationMatrixColumns = nCol;

                            nRow++;
                            oXML.OutOfElem(); // Columns
                        }
                        oXML.OutOfElem(); // Row
                    }
                    oXML.OutOfElem(); // Rows
                }
            }
            sForcePlate.nCalibrationMatrixRows = nRow;
            sForcePlate.bValidCalibrationMatrix = true;

            oXML.OutOfElem(); // "Calibration_Matrix"
        }
        oXML.OutOfElem(); // "Plate"

        bDataAvailable = true;
        msForceSettings.vsForcePlates.push_back(sForcePlate);
    }

    return true;
} // Read force settings

bool CMarkupDeserializer::DeserializeImageSettings(std::vector<SImageCamera>& mvsImageSettings, bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsImageSettings.clear();

    //
    // Read some Image parameters
    //
    if (!oXML.FindChildElem("Image"))
    {
        return true;
    }
    oXML.IntoElem();

    while (oXML.FindChildElem("Camera"))
    {
        oXML.IntoElem();

        SImageCamera sImageCamera;

        if (!oXML.FindChildElem("ID"))
        {
            return false;
        }
        sImageCamera.nID = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Enabled"))
        {
            return false;
        }
        std::string tStr;
        tStr = ToLower(oXML.GetChildData());

        if (tStr == "true")
        {
            sImageCamera.bEnabled = true;
        }
        else
        {
            sImageCamera.bEnabled = false;
        }

        if (!oXML.FindChildElem("Format"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());

        if (tStr == "rawgrayscale")
        {
            sImageCamera.eFormat = CRTPacket::FormatRawGrayscale;
        }
        else if (tStr == "rawbgr")
        {
            sImageCamera.eFormat = CRTPacket::FormatRawBGR;
        }
        else if (tStr == "jpg")
        {
            sImageCamera.eFormat = CRTPacket::FormatJPG;
        }
        else if (tStr == "png")
        {
            sImageCamera.eFormat = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sImageCamera.nWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sImageCamera.nHeight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Left_Crop"))
        {
            return false;
        }
        sImageCamera.fCropLeft = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top_Crop"))
        {
            return false;
        }
        sImageCamera.fCropTop = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right_Crop"))
        {
            return false;
        }
        sImageCamera.fCropRight = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom_Crop"))
        {
            return false;
        }
        sImageCamera.fCropBottom = (float)atof(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // "Camera"

        mvsImageSettings.push_back(sImageCamera);
        bDataAvailable = true;
    }

    return true;
} // ReadImageSettings


bool CMarkupDeserializer::DeserializeSkeletonSettings(bool skeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>& mSkeletonSettingsHierarchical, std::vector<SSettingsSkeleton>& mSkeletonSettings, bool& dataAvailable)
{
    CMarkup xml = oXML;

    dataAvailable = false;

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

        if (mnMajorVersion > 1 || mnMinorVersion > 20)
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
                        sprintf(maErrorStr, "Scale element parse error");
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
            dataAvailable = true;
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
        dataAvailable = true;
    }
    return true;
} // ReadSkeletonSettings

namespace 
{
    bool ReadXmlFov(std::string name, CMarkup& oXML, SCalibrationFov& fov)
    {
        if (!oXML.FindChildElem(name.c_str()))
        {
            return false;
        }
        fov.left = std::stoul(oXML.GetChildAttrib("left"));
        fov.top = std::stoul(oXML.GetChildAttrib("top"));
        fov.right = std::stoul(oXML.GetChildAttrib("right"));
        fov.bottom = std::stoul(oXML.GetChildAttrib("bottom"));

        return true;
    }
}

bool CMarkupDeserializer::DeserializeCalibrationSettings(SCalibration& mCalibrationSettings)
{
    SCalibration settings;

    if (!oXML.FindChildElem("calibration"))
    {
        sprintf(maErrorStr, "Missing calibration element");
        return false;
    }
    oXML.IntoElem();

    try
    {
        std::string resultStr = ToLower(oXML.GetAttrib("calibrated"));

        settings.calibrated = (resultStr == "true");
        settings.source = oXML.GetAttrib("source");
        settings.created = oXML.GetAttrib("created");
        settings.qtm_version = oXML.GetAttrib("qtm-version");
        std::string typeStr = oXML.GetAttrib("type");
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
            settings.refit_residual = std::stod(oXML.GetAttrib("refit-residual"));
        }
        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = std::stod(oXML.GetAttrib("wandLength"));
            settings.max_frames = std::stoul(oXML.GetAttrib("maximumFrames"));
            settings.short_arm_end = std::stod(oXML.GetAttrib("shortArmEnd"));
            settings.long_arm_end = std::stod(oXML.GetAttrib("longArmEnd"));
            settings.long_arm_middle = std::stod(oXML.GetAttrib("longArmMiddle"));

            if (!oXML.FindChildElem("results"))
            {
                return false;
            }

            settings.result_std_dev = std::stod(oXML.GetChildAttrib("std-dev"));
            settings.result_min_max_diff = std::stod(oXML.GetChildAttrib("min-max-diff"));
            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = std::stod(oXML.GetChildAttrib("refit-residual"));
                settings.result_consecutive = std::stoul(oXML.GetChildAttrib("consecutive"));
            }
        }

        if (!oXML.FindChildElem("cameras"))
        {
            return false;
        }
        oXML.IntoElem();

        while (oXML.FindChildElem("camera"))
        {
            oXML.IntoElem();
            SCalibrationCamera camera;
            camera.active = std::stod(oXML.GetAttrib("active")) != 0;

            std::string calibratedStr = ToLower(oXML.GetAttrib("calibrated"));

            camera.calibrated = (calibratedStr == "true");
            camera.message = oXML.GetAttrib("message");

            camera.point_count = std::stoul(oXML.GetAttrib("point-count"));
            camera.avg_residual = std::stod(oXML.GetAttrib("avg-residual"));
            camera.serial = std::stoul(oXML.GetAttrib("serial"));
            camera.model = oXML.GetAttrib("model");
            camera.view_rotation = std::stoul(oXML.GetAttrib("viewrotation"));
            if (!ReadXmlFov("fov_marker", oXML, camera.fov_marker))
            {
                return false;
            }
            if (!ReadXmlFov("fov_marker_max", oXML, camera.fov_marker_max))
            {
                return false;
            }
            if (!ReadXmlFov("fov_video", oXML, camera.fov_video))
            {
                return false;
            }
            if (!ReadXmlFov("fov_video_max", oXML, camera.fov_video_max))
            {
                return false;
            }
            if (!oXML.FindChildElem("transform"))
            {
                return false;
            }
            camera.transform.x = std::stod(oXML.GetChildAttrib("x"));
            camera.transform.y = std::stod(oXML.GetChildAttrib("y"));
            camera.transform.z = std::stod(oXML.GetChildAttrib("z"));
            camera.transform.r11 = std::stod(oXML.GetChildAttrib("r11"));
            camera.transform.r12 = std::stod(oXML.GetChildAttrib("r12"));
            camera.transform.r13 = std::stod(oXML.GetChildAttrib("r13"));
            camera.transform.r21 = std::stod(oXML.GetChildAttrib("r21"));
            camera.transform.r22 = std::stod(oXML.GetChildAttrib("r22"));
            camera.transform.r23 = std::stod(oXML.GetChildAttrib("r23"));
            camera.transform.r31 = std::stod(oXML.GetChildAttrib("r31"));
            camera.transform.r32 = std::stod(oXML.GetChildAttrib("r32"));
            camera.transform.r33 = std::stod(oXML.GetChildAttrib("r33"));

            if (!oXML.FindChildElem("intrinsic"))
            {
                return false;
            }

            auto focalLength = oXML.GetChildAttrib("focallength");
            try
            {
                camera.intrinsic.focal_length = std::stod(focalLength);
            }
            catch (const std::invalid_argument&)
            {
                camera.intrinsic.focal_length = 0;
            }

            camera.intrinsic.sensor_min_u = std::stod(oXML.GetChildAttrib("sensorMinU"));
            camera.intrinsic.sensor_max_u = std::stod(oXML.GetChildAttrib("sensorMaxU"));
            camera.intrinsic.sensor_min_v = std::stod(oXML.GetChildAttrib("sensorMinV"));
            camera.intrinsic.sensor_max_v = std::stod(oXML.GetChildAttrib("sensorMaxV"));
            camera.intrinsic.focal_length_u = std::stod(oXML.GetChildAttrib("focalLengthU"));
            camera.intrinsic.focal_length_v = std::stod(oXML.GetChildAttrib("focalLengthV"));
            camera.intrinsic.center_point_u = std::stod(oXML.GetChildAttrib("centerPointU"));
            camera.intrinsic.center_point_v = std::stod(oXML.GetChildAttrib("centerPointV"));
            camera.intrinsic.skew = std::stod(oXML.GetChildAttrib("skew"));
            camera.intrinsic.radial_distortion_1 = std::stod(oXML.GetChildAttrib("radialDistortion1"));
            camera.intrinsic.radial_distortion_2 = std::stod(oXML.GetChildAttrib("radialDistortion2"));
            camera.intrinsic.radial_distortion_3 = std::stod(oXML.GetChildAttrib("radialDistortion3"));
            camera.intrinsic.tangental_distortion_1 = std::stod(oXML.GetChildAttrib("tangentalDistortion1"));
            camera.intrinsic.tangental_distortion_2 = std::stod(oXML.GetChildAttrib("tangentalDistortion2"));
            oXML.OutOfElem(); // camera
            settings.cameras.push_back(camera);
        }
        oXML.OutOfElem(); // cameras
    }
    catch (...)
    {
        return false;
    }

    oXML.OutOfElem(); // calibration

    mCalibrationSettings = settings;
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

CMarkupSerializer::CMarkupSerializer(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mnMajorVersion(majorVersion), mnMinorVersion(minorVersion)
{
}

std::string CMarkupSerializer::SetGeneralSettings(const unsigned int* pnCaptureFrequency,
                                                  const float* pfCaptureTime, const bool* pbStartOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC,
                                                  const bool* startOnTrigSoftware, const EProcessingActions* peProcessingActions,
                                                  const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    if (pnCaptureFrequency)
    {
        AddXMLElementUnsignedInt(&oXML, "Frequency", pnCaptureFrequency);
    }
    if (pfCaptureTime)
    {
        AddXMLElementFloat(&oXML, "Capture_Time", pfCaptureTime, 3);
    }
    if (pbStartOnExtTrig)
    {
        AddXMLElementBool(&oXML, "Start_On_External_Trigger", pbStartOnExtTrig);
        if (mnMajorVersion > 1 || mnMinorVersion > 14)
        {
            AddXMLElementBool(&oXML, "Start_On_Trigger_NO", startOnTrigNO);
            AddXMLElementBool(&oXML, "Start_On_Trigger_NC", startOnTrigNC);
            AddXMLElementBool(&oXML, "Start_On_Trigger_Software", startOnTrigSoftware);
        }
    }

    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActions[3] = { peProcessingActions, peRtProcessingActions, peReprocessingActions };

    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActions[i])
        {
            oXML.AddElem(processings[i]);
            oXML.IntoElem();

            if (mnMajorVersion > 1 || mnMinorVersion > 13)
            {
                AddXMLElementBool(&oXML, "PreProcessing2D", (*processingActions[i] & ProcessingPreProcess2D) != 0);
            }
            if (*processingActions[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                oXML.AddElem("Tracking", "2D");
            }
            else if (*processingActions[i] & ProcessingTracking3D)
            {
                oXML.AddElem("Tracking", "3D");
            }
            else
            {
                oXML.AddElem("Tracking", "False");
            }
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&oXML, "TwinSystemMerge", (*processingActions[i] & ProcessingTwinSystemMerge) != 0);
                AddXMLElementBool(&oXML, "SplineFill", (*processingActions[i] & ProcessingSplineFill) != 0);
            }
            AddXMLElementBool(&oXML, "AIM", (*processingActions[i] & ProcessingAIM) != 0);
            AddXMLElementBool(&oXML, "Track6DOF", (*processingActions[i] & Processing6DOFTracking) != 0);
            AddXMLElementBool(&oXML, "ForceData", (*processingActions[i] & ProcessingForceData) != 0);
            AddXMLElementBool(&oXML, "GazeVector", (*processingActions[i] & ProcessingGazeVector) != 0);
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&oXML, "ExportTSV", (*processingActions[i] & ProcessingExportTSV) != 0);
                AddXMLElementBool(&oXML, "ExportC3D", (*processingActions[i] & ProcessingExportC3D) != 0);
                AddXMLElementBool(&oXML, "ExportMatlabFile", (*processingActions[i] & ProcessingExportMatlabFile) != 0);
                AddXMLElementBool(&oXML, "ExportAviFile", (*processingActions[i] & ProcessingExportAviFile) != 0);
            }
            oXML.OutOfElem(); // Processing_Actions
        }
    }
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetExtTimeBaseSettings(const bool* pbEnabled, const ESignalSource* peSignalSource,
    const bool* pbSignalModePeriodic, const unsigned int* pnFreqMultiplier, const unsigned int* pnFreqDivisor,
    const unsigned int* pnFreqTolerance, const float* pfNominalFrequency, const bool* pbNegativeEdge,
    const unsigned int* pnSignalShutterDelay, const float* pfNonPeriodicTimeout)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();
    oXML.AddElem("External_Time_Base");
    oXML.IntoElem();

    AddXMLElementBool(&oXML, "Enabled", pbEnabled);

    if (peSignalSource)
    {
        switch (*peSignalSource)
        {
        case SourceControlPort:
            oXML.AddElem("Signal_Source", "Control port");
            break;
        case SourceIRReceiver:
            oXML.AddElem("Signal_Source", "IR receiver");
            break;
        case SourceSMPTE:
            oXML.AddElem("Signal_Source", "SMPTE");
            break;
        case SourceVideoSync:
            oXML.AddElem("Signal_Source", "Video sync");
            break;
        case SourceIRIG:
            oXML.AddElem("Signal_Source", "IRIG");
            break;
        }
    }

    AddXMLElementBool(&oXML, "Signal_Mode", pbSignalModePeriodic, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(&oXML, "Frequency_Multiplier", pnFreqMultiplier);
    AddXMLElementUnsignedInt(&oXML, "Frequency_Divisor", pnFreqDivisor);
    AddXMLElementUnsignedInt(&oXML, "Frequency_Tolerance", pnFreqTolerance);

    if (pfNominalFrequency)
    {
        if (*pfNominalFrequency < 0)
        {
            oXML.AddElem("Nominal_Frequency", "None");
        }
        else
        {
            AddXMLElementFloat(&oXML, "Nominal_Frequency", pfNominalFrequency, 3);
        }
    }

    AddXMLElementBool(&oXML, "Signal_Edge", pbNegativeEdge, "Negative", "Positive");
    AddXMLElementUnsignedInt(&oXML, "Signal_Shutter_Delay", pnSignalShutterDelay);
    AddXMLElementFloat(&oXML, "Non_Periodic_Timeout", pfNonPeriodicTimeout, 3);

    oXML.OutOfElem(); // External_Time_Base            
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();
    oXML.AddElem("External_Timestamp");
    oXML.IntoElem();

    AddXMLElementBool(&oXML, "Enabled", timestampSettings.bEnabled);

    switch (timestampSettings.nType)
    {
    default:
    case ETimestampType::Timestamp_SMPTE:
        oXML.AddElem("Type", "SMPTE");
        break;
    case ETimestampType::Timestamp_IRIG:
        oXML.AddElem("Type", "IRIG");
        break;
    case ETimestampType::Timestamp_CameraTime:
        oXML.AddElem("Type", "CameraTime");
        break;
    }
    AddXMLElementUnsignedInt(&oXML, "Frequency", timestampSettings.nFrequency);

    oXML.OutOfElem(); // Timestamp
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraSettings(const unsigned int nCameraID, const ECameraMode* peMode,
    const float* pfMarkerExposure, const float* pfMarkerThreshold, const int* pnOrientation)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    if (peMode)
    {
        switch (*peMode)
        {
        case ModeMarker:
            oXML.AddElem("Mode", "Marker");
            break;
        case ModeMarkerIntensity:
            oXML.AddElem("Mode", "Marker Intensity");
            break;
        case ModeVideo:
            oXML.AddElem("Mode", "Video");
            break;
        }
    }
    AddXMLElementFloat(&oXML, "Marker_Exposure", pfMarkerExposure);
    AddXMLElementFloat(&oXML, "Marker_Threshold", pfMarkerThreshold);
    AddXMLElementInt(&oXML, "Orientation", pnOrientation);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraVideoSettings(const unsigned int nCameraID,
    const EVideoResolution* eVideoResolution, const EVideoAspectRatio* eVideoAspectRatio,
    const unsigned int* pnVideoFrequency, const float* pfVideoExposure, const float* pfVideoFlashTime)
{
    CMarkup oXML;
    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);
    if (eVideoResolution)
    {
        switch (*eVideoResolution)
        {
        case VideoResolution1440p:
            oXML.AddElem("Video_Resolution", "1440p");
            break;
        case VideoResolution1080p:
            oXML.AddElem("Video_Resolution", "1080p");
            break;
        case VideoResolution720p:
            oXML.AddElem("Video_Resolution", "720p");
            break;
        case VideoResolution540p:
            oXML.AddElem("Video_Resolution", "540p");
            break;
        case VideoResolution480p:
            oXML.AddElem("Video_Resolution", "480p");
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
            oXML.AddElem("Video_Aspect_Ratio", "16x9");
            break;
        case VideoAspectRatio4x3:
            oXML.AddElem("Video_Aspect_Ratio", "4x3");
            break;
        case VideoAspectRatio1x1:
            oXML.AddElem("Video_Aspect_Ratio", "1x1");
            break;
        case VideoAspectRatioNone:
            break;
        }
    }
    AddXMLElementUnsignedInt(&oXML, "Video_Frequency", pnVideoFrequency);
    AddXMLElementFloat(&oXML, "Video_Exposure", pfVideoExposure);
    AddXMLElementFloat(&oXML, "Video_Flash_Time", pfVideoFlashTime);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraSyncOutSettings(const unsigned int nCameraID, const unsigned int portNumber,
    const ESyncOutFreqMode* peSyncOutMode, const unsigned int* pnSyncOutValue, const float* pfSyncOutDutyCycle,
    const bool* pbSyncOutNegativePolarity)
{
    CMarkup oXML;
    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    int port = portNumber - 1;
    if (((port == 0 || port == 1) && peSyncOutMode) || (port == 2))
    {
        oXML.AddElem(port == 0 ? "Sync_Out" : (port == 1 ? "Sync_Out2" : "Sync_Out_MT"));
        oXML.IntoElem();

        if (port == 0 || port == 1)
        {
            switch (*peSyncOutMode)
            {
            case ModeShutterOut:
                oXML.AddElem("Mode", "Shutter out");
                break;
            case ModeMultiplier:
                oXML.AddElem("Mode", "Multiplier");
                break;
            case ModeDivisor:
                oXML.AddElem("Mode", "Divisor");
                break;
            case ModeIndependentFreq:
                oXML.AddElem("Mode", "Camera independent");
                break;
            case ModeMeasurementTime:
                oXML.AddElem("Mode", "Measurement time");
                break;
            case ModeFixed100Hz:
                oXML.AddElem("Mode", "Continuous 100Hz");
                break;
            case ModeSystemLiveTime:
                oXML.AddElem("Mode", "System live time");
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
                    AddXMLElementUnsignedInt(&oXML, "Value", pnSyncOutValue);
                }
                if (pfSyncOutDutyCycle)
                {
                    AddXMLElementFloat(&oXML, "Duty_Cycle", pfSyncOutDutyCycle, 3);
                }
            }
        }
        if (pbSyncOutNegativePolarity && (port == 2 ||
            (peSyncOutMode && *peSyncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(&oXML, "Signal_Polarity", pbSyncOutNegativePolarity, "Negative", "Positive");
        }
        oXML.OutOfElem(); // Sync_Out
    }
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraLensControlSettings(const unsigned int nCameraID, const float focus,
    const float aperture)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    oXML.AddElem("LensControl");
    oXML.IntoElem();

    oXML.AddElem("Focus");
    oXML.AddAttrib("Value", CMarkup::Format("%f", focus).c_str());
    oXML.AddElem("Aperture");
    oXML.AddAttrib("Value", CMarkup::Format("%f", aperture).c_str());

    oXML.OutOfElem(); // LensControl
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraAutoExposureSettings(const unsigned int nCameraID, const bool autoExposure,
    const float compensation)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    oXML.AddElem("LensControl");
    oXML.IntoElem();

    oXML.AddElem("AutoExposure");
    oXML.AddAttrib("Enabled", autoExposure ? "true" : "false");
    oXML.AddAttrib("Compensation", CMarkup::Format("%f", compensation).c_str());

    oXML.OutOfElem(); // AutoExposure
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraAutoWhiteBalance(const unsigned int nCameraID, const bool enable)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    oXML.AddElem("AutoWhiteBalance", enable ? "true" : "false");

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetImageSettings(const unsigned int nCameraID, const bool* pbEnable,
    const CRTPacket::EImageFormat* peFormat, const unsigned int* pnWidth, const unsigned int* pnHeight,
    const float* pfLeftCrop, const float* pfTopCrop, const float* pfRightCrop, const float* pfBottomCrop)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("Image");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    AddXMLElementBool(&oXML, "Enabled", pbEnable);

    if (peFormat)
    {
        switch (*peFormat)
        {
        case CRTPacket::FormatRawGrayscale:
            oXML.AddElem("Format", "RAWGrayscale");
            break;
        case CRTPacket::FormatRawBGR:
            oXML.AddElem("Format", "RAWBGR");
            break;
        case CRTPacket::FormatJPG:
            oXML.AddElem("Format", "JPG");
            break;
        case CRTPacket::FormatPNG:
            oXML.AddElem("Format", "PNG");
            break;
        }
    }
    AddXMLElementUnsignedInt(&oXML, "Width", pnWidth);
    AddXMLElementUnsignedInt(&oXML, "Height", pnHeight);
    AddXMLElementFloat(&oXML, "Left_Crop", pfLeftCrop);
    AddXMLElementFloat(&oXML, "Top_Crop", pfTopCrop);
    AddXMLElementFloat(&oXML, "Right_Crop", pfRightCrop);
    AddXMLElementFloat(&oXML, "Bottom_Crop", pfBottomCrop);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // Image
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetForceSettings(const unsigned int nPlateID, const SPoint* psCorner1,
    const SPoint* psCorner2, const SPoint* psCorner3, const SPoint* psCorner4)
{
    CMarkup oXML;
    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("Force");
    oXML.IntoElem();

    oXML.AddElem("Plate");
    oXML.IntoElem();

    if (mnMajorVersion > 1 || mnMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(&oXML, "Plate_ID", &nPlateID);
    }
    else
    {
        AddXMLElementUnsignedInt(&oXML, "Force_Plate_Index", &nPlateID);
    }
    if (psCorner1)
    {
        oXML.AddElem("Corner1");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(psCorner1->fX));
        AddXMLElementFloat(&oXML, "Y", &(psCorner1->fY));
        AddXMLElementFloat(&oXML, "Z", &(psCorner1->fZ));
        oXML.OutOfElem(); // Corner1
    }
    if (psCorner2)
    {
        oXML.AddElem("Corner2");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(psCorner2->fX));
        AddXMLElementFloat(&oXML, "Y", &(psCorner2->fY));
        AddXMLElementFloat(&oXML, "Z", &(psCorner2->fZ));
        oXML.OutOfElem(); // Corner2
    }
    if (psCorner3)
    {
        oXML.AddElem("Corner3");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(psCorner3->fX));
        AddXMLElementFloat(&oXML, "Y", &(psCorner3->fY));
        AddXMLElementFloat(&oXML, "Z", &(psCorner3->fZ));
        oXML.OutOfElem(); // Corner3
    }
    if (psCorner4)
    {
        oXML.AddElem("Corner4");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(psCorner4->fX));
        AddXMLElementFloat(&oXML, "Y", &(psCorner4->fY));
        AddXMLElementFloat(&oXML, "Z", &(psCorner4->fZ));
        oXML.OutOfElem(); // Corner4
    }
    oXML.OutOfElem(); // Plate

    oXML.OutOfElem(); // Force
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("The_6D");
    oXML.IntoElem();

    for (auto& body : settings)
    {
        oXML.AddElem("Body");
        oXML.IntoElem();
        oXML.AddElem("Name", body.name.c_str());
        oXML.AddElem("Enabled", body.enabled ? "true" : "false");
        oXML.AddElem("Color");
        oXML.AddAttrib("R", std::to_string(body.color & 0xff).c_str());
        oXML.AddAttrib("G", std::to_string((body.color >> 8) & 0xff).c_str());
        oXML.AddAttrib("B", std::to_string((body.color >> 16) & 0xff).c_str());
        oXML.AddElem("MaximumResidual", std::to_string(body.maxResidual).c_str());
        oXML.AddElem("MinimumMarkersInBody", std::to_string(body.minMarkersInBody).c_str());
        oXML.AddElem("BoneLengthTolerance", std::to_string(body.boneLengthTolerance).c_str());
        oXML.AddElem("Filter");
        oXML.AddAttrib("Preset", body.filterPreset.c_str());

        if (!body.mesh.name.empty())
        {
            oXML.AddElem("Mesh");
            oXML.IntoElem();
            oXML.AddElem("Name", body.mesh.name.c_str());
            oXML.AddElem("Position");
            oXML.AddAttrib("X", std::to_string(body.mesh.position.fX).c_str());
            oXML.AddAttrib("Y", std::to_string(body.mesh.position.fY).c_str());
            oXML.AddAttrib("Z", std::to_string(body.mesh.position.fZ).c_str());
            oXML.AddElem("Rotation");
            oXML.AddAttrib("X", std::to_string(body.mesh.rotation.fX).c_str());
            oXML.AddAttrib("Y", std::to_string(body.mesh.rotation.fY).c_str());
            oXML.AddAttrib("Z", std::to_string(body.mesh.rotation.fZ).c_str());
            oXML.AddElem("Scale", std::to_string(body.mesh.scale).c_str());
            oXML.AddElem("Opacity", std::to_string(body.mesh.opacity).c_str());
            oXML.OutOfElem(); // Mesh
        }

        if (!body.points.empty())
        {
            oXML.AddElem("Points");
            oXML.IntoElem();
            for (auto& point : body.points)
            {
                oXML.AddElem("Point");
                oXML.AddAttrib("X", std::to_string(point.fX).c_str());
                oXML.AddAttrib("Y", std::to_string(point.fY).c_str());
                oXML.AddAttrib("Z", std::to_string(point.fZ).c_str());
                oXML.AddAttrib("Virtual", point.virtual_ ? "1" : "0");
                oXML.AddAttrib("PhysicalId", std::to_string(point.physicalId).c_str());
                oXML.AddAttrib("Name", point.name.c_str());
            }
            oXML.OutOfElem(); // Points
        }
        oXML.AddElem("Data_origin", std::to_string(body.origin.type).c_str());
        oXML.AddAttrib("X", std::to_string(body.origin.position.fX).c_str());
        oXML.AddAttrib("Y", std::to_string(body.origin.position.fY).c_str());
        oXML.AddAttrib("Z", std::to_string(body.origin.position.fZ).c_str());
        oXML.AddAttrib("Relative_body", std::to_string(body.origin.relativeBody).c_str());
        oXML.AddElem("Data_orientation", std::to_string(body.origin.type).c_str());
        for (std::uint32_t i = 0; i < 9; i++)
        {
            char tmpStr[16];
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            oXML.AddAttrib(tmpStr, std::to_string(body.origin.rotation[i]).c_str());
        }
        oXML.AddAttrib("Relative_body", std::to_string(body.origin.relativeBody).c_str());

        oXML.OutOfElem(); // Body
    }
    oXML.OutOfElem(); // The_6D
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons)
{
    CMarkup xml;

    xml.AddElem("QTM_Settings");
    xml.IntoElem();
    xml.AddElem("Skeletons");
    xml.IntoElem();

    for (auto& skeleton : skeletons)
    {
        xml.AddElem("Skeleton");
        xml.SetAttrib("Name", skeleton.name.c_str());
        xml.IntoElem();
        if (mnMajorVersion == 1 && mnMinorVersion < 22)
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
                    if (mnMajorVersion > 1 || mnMinorVersion > 21)
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

