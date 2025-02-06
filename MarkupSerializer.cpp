#define _CRT_SECURE_NO_WARNINGS

#include "MarkupSerializer.h"

#include <algorithm>
#include <map>

#include "Settings.h"
#include "Markup.h"
#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

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


CMarkupDeserializer::CMarkupDeserializer(const char* data, std::uint32_t pMajorVersion, std::uint32_t pMinorVersion)
    : mMajorVersion(pMajorVersion), mMinorVersion(pMinorVersion), mErrorStr{0}, oXML(data)
{
}

bool CMarkupDeserializer::DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings)
{
    std::string             str;

    pGeneralSettings.cameras.clear();

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
    pGeneralSettings.captureFrequency = atoi(oXML.GetChildData().c_str());

    if (!oXML.FindChildElem("Capture_Time"))
    {
        return false;
    }
    pGeneralSettings.captureTime = (float)atof(oXML.GetChildData().c_str());

    // Refactored variant of all this copy/paste code. TODO: Refactor everything else.
    if (!ReadXmlBool(&oXML, "Start_On_External_Trigger", pGeneralSettings.startOnExternalTrigger))
    {
        return false;
    }
    if (mMajorVersion > 1 || mMinorVersion > 14)
    {
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_NO", pGeneralSettings.startOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_NC", pGeneralSettings.startOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_Software", pGeneralSettings.startOnTrigSoftware))
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
    str = ToLower(oXML.GetChildData());
    pGeneralSettings.externalTimebase.enabled = (str == "true");

    if (!oXML.FindChildElem("Signal_Source"))
    {
        return false;
    }
    str = ToLower(oXML.GetChildData());
    if (str == "control port")
    {
        pGeneralSettings.externalTimebase.signalSource = SourceControlPort;
    }
    else if (str == "ir receiver")
    {
        pGeneralSettings.externalTimebase.signalSource = SourceIRReceiver;
    }
    else if (str == "smpte")
    {
        pGeneralSettings.externalTimebase.signalSource = SourceSMPTE;
    }
    else if (str == "irig")
    {
        pGeneralSettings.externalTimebase.signalSource = SourceIRIG;
    }
    else if (str == "video sync")
    {
        pGeneralSettings.externalTimebase.signalSource = SourceVideoSync;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Signal_Mode"))
    {
        return false;
    }
    str = ToLower(oXML.GetChildData());
    if (str == "periodic")
    {
        pGeneralSettings.externalTimebase.signalModePeriodic = true;
    }
    else if (str == "non-periodic")
    {
        pGeneralSettings.externalTimebase.signalModePeriodic = false;
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
    str = oXML.GetChildData();
    if (sscanf(str.c_str(), "%u", &nMultiplier) == 1)
    {
        pGeneralSettings.externalTimebase.freqMultiplier = nMultiplier;
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
    str = oXML.GetChildData();
    if (sscanf(str.c_str(), "%u", &nDivisor) == 1)
    {
        pGeneralSettings.externalTimebase.freqDivisor = nDivisor;
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
    str = oXML.GetChildData();
    if (sscanf(str.c_str(), "%u", &nTolerance) == 1)
    {
        pGeneralSettings.externalTimebase.freqTolerance = nTolerance;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Nominal_Frequency"))
    {
        return false;
    }
    str = ToLower(oXML.GetChildData());

    if (str == "none")
    {
        pGeneralSettings.externalTimebase.nominalFrequency = -1; // -1 = disabled
    }
    else
    {
        float fFrequency;
        if (sscanf(str.c_str(), "%f", &fFrequency) == 1)
        {
            pGeneralSettings.externalTimebase.nominalFrequency = fFrequency;
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
    str = ToLower(oXML.GetChildData());
    if (str == "negative")
    {
        pGeneralSettings.externalTimebase.negativeEdge = true;
    }
    else if (str == "positive")
    {
        pGeneralSettings.externalTimebase.negativeEdge = false;
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
    str = oXML.GetChildData();
    if (sscanf(str.c_str(), "%u", &nDelay) == 1)
    {
        pGeneralSettings.externalTimebase.signalShutterDelay = nDelay;
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
    str = oXML.GetChildData();
    if (sscanf(str.c_str(), "%f", &fTimeout) == 1)
    {
        pGeneralSettings.externalTimebase.nonPeriodicTimeout = fTimeout;
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
            str = ToLower(oXML.GetChildData());
            pGeneralSettings.timestamp.enabled = (str == "true");
        }
        if (oXML.FindChildElem("Type"))
        {
            str = ToLower(oXML.GetChildData());
            if (str == "smpte")
            {
                pGeneralSettings.timestamp.type = Timestamp_SMPTE;
            }
            else if (str == "irig")
            {
                pGeneralSettings.timestamp.type = Timestamp_IRIG;
            }
            else
            {
                pGeneralSettings.timestamp.type = Timestamp_CameraTime;
            }
        }
        if (oXML.FindChildElem("Frequency"))
        {
            unsigned int timestampFrequency;
            str = oXML.GetChildData();
            if (sscanf(str.c_str(), "%u", &timestampFrequency) == 1)
            {
                pGeneralSettings.timestamp.frequency = timestampFrequency;
            }
        }
        oXML.OutOfElem();
    }
    // External_Timestamp


    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    EProcessingActions* processingActions[3] =
    {
        &pGeneralSettings.processingActions,
        &pGeneralSettings.rtProcessingActions,
        &pGeneralSettings.reprocessingActions
    };
    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================
        if (!oXML.FindChildElem(processings[i]))
        {
            return false;
        }
        oXML.IntoElem();

        *processingActions[i] = ProcessingNone;

        if (mMajorVersion > 1 || mMinorVersion > 13)
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
        str = ToLower(oXML.GetChildData());
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

        if (mMajorVersion > 1 || mMinorVersion > 11)
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

            if (mMajorVersion > 1 || mMinorVersion > 11)
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
        pGeneralSettings.eulerRotations[0] = oXML.GetAttrib("First");
        pGeneralSettings.eulerRotations[1] = oXML.GetAttrib("Second");
        pGeneralSettings.eulerRotations[2] = oXML.GetAttrib("Third");
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
        sCameraSettings.id = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Model"))
        {
            return false;
        }
        str = ToLower(oXML.GetChildData());

        if (str == "macreflex")
        {
            sCameraSettings.model = ModelMacReflex;
        }
        else if (str == "proreflex 120")
        {
            sCameraSettings.model = ModelProReflex120;
        }
        else if (str == "proreflex 240")
        {
            sCameraSettings.model = ModelProReflex240;
        }
        else if (str == "proreflex 500")
        {
            sCameraSettings.model = ModelProReflex500;
        }
        else if (str == "proreflex 1000")
        {
            sCameraSettings.model = ModelProReflex1000;
        }
        else if (str == "oqus 100")
        {
            sCameraSettings.model = ModelOqus100;
        }
        else if (str == "oqus 200" || str == "oqus 200 c")
        {
            sCameraSettings.model = ModelOqus200C;
        }
        else if (str == "oqus 300")
        {
            sCameraSettings.model = ModelOqus300;
        }
        else if (str == "oqus 300 plus")
        {
            sCameraSettings.model = ModelOqus300Plus;
        }
        else if (str == "oqus 400")
        {
            sCameraSettings.model = ModelOqus400;
        }
        else if (str == "oqus 500")
        {
            sCameraSettings.model = ModelOqus500;
        }
        else if (str == "oqus 500 plus")
        {
            sCameraSettings.model = ModelOqus500Plus;
        }
        else if (str == "oqus 700")
        {
            sCameraSettings.model = ModelOqus700;
        }
        else if (str == "oqus 700 plus")
        {
            sCameraSettings.model = ModelOqus700Plus;
        }
        else if (str == "oqus 600 plus")
        {
            sCameraSettings.model = ModelOqus600Plus;
        }
        else if (str == "miqus m1")
        {
            sCameraSettings.model = ModelMiqusM1;
        }
        else if (str == "miqus m3")
        {
            sCameraSettings.model = ModelMiqusM3;
        }
        else if (str == "miqus m5")
        {
            sCameraSettings.model = ModelMiqusM5;
        }
        else if (str == "miqus sync unit")
        {
            sCameraSettings.model = ModelMiqusSyncUnit;
        }
        else if (str == "miqus video")
        {
            sCameraSettings.model = ModelMiqusVideo;
        }
        else if (str == "miqus video color")
        {
            sCameraSettings.model = ModelMiqusVideoColor;
        }
        else if (str == "miqus hybrid")
        {
            sCameraSettings.model = ModelMiqusHybrid;
        }
        else if (str == "miqus video color plus")
        {
            sCameraSettings.model = ModelMiqusVideoColorPlus;
        }
        else if (str == "arqus a5")
        {
            sCameraSettings.model = ModelArqusA5;
        }
        else if (str == "arqus a9")
        {
            sCameraSettings.model = ModelArqusA9;
        }
        else if (str == "arqus a12")
        {
            sCameraSettings.model = ModelArqusA12;
        }
        else if (str == "arqus a26")
        {
            sCameraSettings.model = ModelArqusA26;
        }
        else
        {
            sCameraSettings.model = ModelUnknown;
        }

        // Only available from protocol version 1.10 and later.
        if (oXML.FindChildElem("Underwater"))
        {
            str = ToLower(oXML.GetChildData());
            sCameraSettings.underwater = (str == "true");
        }

        if (oXML.FindChildElem("Supports_HW_Sync"))
        {
            str = ToLower(oXML.GetChildData());
            sCameraSettings.supportsHwSync = (str == "true");
        }

        if (!oXML.FindChildElem("Serial"))
        {
            return false;
        }
        sCameraSettings.serial = atoi(oXML.GetChildData().c_str());

        // ==================== Camera Mode ====================
        if (!oXML.FindChildElem("Mode"))
        {
            return false;
        }
        str = ToLower(oXML.GetChildData());
        if (str == "marker")
        {
            sCameraSettings.mode = ModeMarker;
        }
        else if (str == "marker intensity")
        {
            sCameraSettings.mode = ModeMarkerIntensity;
        }
        else if (str == "video")
        {
            sCameraSettings.mode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mMajorVersion > 1 || mMinorVersion > 11)
        {
            // ==================== Video frequency ====================
            if (!oXML.FindChildElem("Video_Frequency"))
            {
                return false;
            }
            sCameraSettings.videoFrequency = atoi(oXML.GetChildData().c_str());
        }

        // ==================== Video Resolution ====================
        if (oXML.FindChildElem("Video_Resolution"))
        {
            str = ToLower(oXML.GetChildData());
            if (str == "1440p")
            {
                sCameraSettings.videoResolution = VideoResolution1440p;
            }
            else if (str == "1080p")
            {
                sCameraSettings.videoResolution = VideoResolution1080p;
            }
            else if (str == "720p")
            {
                sCameraSettings.videoResolution = VideoResolution720p;
            }
            else if (str == "540p")
            {
                sCameraSettings.videoResolution = VideoResolution540p;
            }
            else if (str == "480p")
            {
                sCameraSettings.videoResolution = VideoResolution480p;
            }
            else
            {
                return false;
            }
        }
        else
        {
            sCameraSettings.videoResolution = VideoResolutionNone;
        }

        // ==================== Video AspectRatio ====================
        if (oXML.FindChildElem("Video_Aspect_Ratio"))
        {
            str = ToLower(oXML.GetChildData());
            if (str == "16x9")
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatio16x9;
            }
            else if (str == "4x3")
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatio4x3;
            }
            else if (str == "1x1")
            {
                sCameraSettings.videoAspectRatio = VideoAspectRatio1x1;
            }
        }
        else
        {
            sCameraSettings.videoAspectRatio = VideoAspectRatioNone;
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
        sCameraSettings.videoExposure = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.videoExposureMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.videoExposureMax = atoi(oXML.GetChildData().c_str());
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
        sCameraSettings.videoFlashTime = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.videoFlashTimeMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.videoFlashTimeMax = atoi(oXML.GetChildData().c_str());
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
        sCameraSettings.markerExposure = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.markerExposureMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.markerExposureMax = atoi(oXML.GetChildData().c_str());

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
        sCameraSettings.markerThreshold = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.markerThresholdMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.markerThresholdMax = atoi(oXML.GetChildData().c_str());

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
        sCameraSettings.positionX = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Y"))
        {
            return false;
        }
        sCameraSettings.positionY = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Z"))
        {
            return false;
        }
        sCameraSettings.positionZ = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_1"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[0][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_1"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[1][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_1"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[2][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_2"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[0][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_2"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[1][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_2"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[2][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_3"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[0][2] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_3"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[1][2] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_3"))
        {
            return false;
        }
        sCameraSettings.positionRotMatrix[2][2] = (float)atof(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Position


        if (!oXML.FindChildElem("Orientation"))
        {
            return false;
        }
        sCameraSettings.orientation = atoi(oXML.GetChildData().c_str());

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
        sCameraSettings.markerResolutionWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sCameraSettings.markerResolutionHeight = atoi(oXML.GetChildData().c_str());

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
        sCameraSettings.videoResolutionWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sCameraSettings.videoResolutionHeight = atoi(oXML.GetChildData().c_str());

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
        sCameraSettings.markerFOVLeft = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top"))
        {
            return false;
        }
        sCameraSettings.markerFOVTop = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right"))
        {
            return false;
        }
        sCameraSettings.markerFOVRight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom"))
        {
            return false;
        }
        sCameraSettings.markerFOVBottom = atoi(oXML.GetChildData().c_str());

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
        sCameraSettings.videoFOVLeft = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top"))
        {
            return false;
        }
        sCameraSettings.videoFOVTop = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right"))
        {
            return false;
        }
        sCameraSettings.videoFOVRight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom"))
        {
            return false;
        }
        sCameraSettings.videoFOVBottom = atoi(oXML.GetChildData().c_str());

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
                    str = ToLower(oXML.GetChildData());
                    if (str == "shutter out")
                    {
                        sCameraSettings.syncOutMode[port] = ModeShutterOut;
                    }
                    else if (str == "multiplier")
                    {
                        sCameraSettings.syncOutMode[port] = ModeMultiplier;
                    }
                    else if (str == "divisor")
                    {
                        sCameraSettings.syncOutMode[port] = ModeDivisor;
                    }
                    else if (str == "camera independent")
                    {
                        sCameraSettings.syncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (str == "measurement time")
                    {
                        sCameraSettings.syncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (str == "continuous 100hz")
                    {
                        sCameraSettings.syncOutMode[port] = ModeFixed100Hz;
                    }
                    else if (str == "system live time")
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
                        if (!oXML.FindChildElem("Value"))
                        {
                            return false;
                        }
                        sCameraSettings.syncOutValue[port] = atoi(oXML.GetChildData().c_str());

                        if (!oXML.FindChildElem("Duty_Cycle"))
                        {
                            return false;
                        }
                        sCameraSettings.syncOutDutyCycle[port] = (float)atof(oXML.GetChildData().c_str());
                    }
                }
                if (port == 2 ||
                    (sCameraSettings.syncOutMode[port] != ModeFixed100Hz))
                {
                    if (!oXML.FindChildElem("Signal_Polarity"))
                    {
                        return false;
                    }
                    if (CompareNoCase(oXML.GetChildData(), "negative"))
                    {
                        sCameraSettings.syncOutNegativePolarity[port] = true;
                    }
                    else
                    {
                        sCameraSettings.syncOutNegativePolarity[port] = false;
                    }
                }
                oXML.OutOfElem(); // Sync_Out
            }
            else
            {
                sCameraSettings.syncOutMode[port] = ModeIndependentFreq;
                sCameraSettings.syncOutValue[port] = 0;
                sCameraSettings.syncOutDutyCycle[port] = 0;
                sCameraSettings.syncOutNegativePolarity[port] = false;
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
                    sCameraSettings.focus = focus;
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Aperture"))
            {
                oXML.IntoElem();
                float aperture;
                if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &aperture) == 1)
                {
                    sCameraSettings.aperture = aperture;
                }
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }
        else
        {
            sCameraSettings.focus = std::numeric_limits<float>::quiet_NaN();
            sCameraSettings.aperture = std::numeric_limits<float>::quiet_NaN();
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

        pGeneralSettings.cameras.push_back(sCameraSettings);
    }

    return true;

}

bool CMarkupDeserializer::Deserialize3DSettings(SSettings3D& p3dSettings, bool& pDataAvailable)
{
    std::string str;

    pDataAvailable = false;

    p3dSettings.labels3D.clear();
    p3dSettings.calibrationTime[0] = 0;

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
    str = ToLower(oXML.GetChildData());

    if (str == "+x")
    {
        p3dSettings.axisUpwards = XPos;
    }
    else if (str == "-x")
    {
        p3dSettings.axisUpwards = XNeg;
    }
    else if (str == "+y")
    {
        p3dSettings.axisUpwards = YPos;
    }
    else if (str == "-y")
    {
        p3dSettings.axisUpwards = YNeg;
    }
    else if (str == "+z")
    {
        p3dSettings.axisUpwards = ZPos;
    }
    else if (str == "-z")
    {
        p3dSettings.axisUpwards = ZNeg;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("CalibrationTime"))
    {
        return false;
    }
    str = oXML.GetChildData();
    strcpy(p3dSettings.calibrationTime, str.c_str());

    if (!oXML.FindChildElem("Labels"))
    {
        return false;
    }
    unsigned int nNumberOfLabels = atoi(oXML.GetChildData().c_str());

    p3dSettings.labels3D.resize(nNumberOfLabels);
    SSettings3DLabel sLabel;

    for (unsigned int iLabel = 0; iLabel < nNumberOfLabels; iLabel++)
    {
        if (oXML.FindChildElem("Label"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Name"))
            {
                sLabel.name = oXML.GetChildData();
                if (oXML.FindChildElem("RGBColor"))
                {
                    sLabel.rgbColor = atoi(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Trajectory_Type"))
                {
                    sLabel.type = oXML.GetChildData();
                }
                p3dSettings.labels3D[iLabel] = sLabel;
            }
            oXML.OutOfElem();
        }
        else
        {
            return false;
        }
    }

    p3dSettings.bones.clear();
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
            p3dSettings.bones.push_back(bone);
            oXML.OutOfElem();
        }
        oXML.OutOfElem();
    }

    pDataAvailable = true;
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
                SBodyPoint bodyPoint;

                bodyPoint.x = (float)atof(oXML.GetChildAttrib("X").c_str());
                bodyPoint.y = (float)atof(oXML.GetChildAttrib("Y").c_str());
                bodyPoint.z = (float)atof(oXML.GetChildAttrib("Z").c_str());

                bodyPoint.virtual_ = (0 != atoi(oXML.GetChildAttrib("Virtual").c_str()));
                bodyPoint.physicalId = atoi(oXML.GetChildAttrib("PhysicalId").c_str());
                bodyPoint.name = oXML.GetChildAttrib("Name");
                vTarget.push_back(bodyPoint);
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
        oTarget.position.x = (float)atof(oXML.GetChildAttrib("X").c_str());
        oTarget.position.y = (float)atof(oXML.GetChildAttrib("Y").c_str());
        oTarget.position.z = (float)atof(oXML.GetChildAttrib("Z").c_str());
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
            SBodyPoint point;

            oXML.IntoElem();
            if (!oXML.FindChildElem("X"))
            {
                return false;
            }
            point.x = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Y"))
            {
                return false;
            }
            point.y = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Z"))
            {
                return false;
            }
            point.z = (float)atof(oXML.GetChildData().c_str());

            oXML.OutOfElem(); // Point
            vTarget.push_back(point);
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

bool CMarkupDeserializer::Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& p6DOFSettings, SSettingsGeneral& pGeneralSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    p6DOFSettings.clear();

    if (oXML.FindChildElem("The_6D"))
    {
        oXML.IntoElem();

        if (mMajorVersion > 1 || mMinorVersion > 20)
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
                TryReadSetEnabled(mMajorVersion, mMinorVersion, oXML, s6DOFBodySettings.enabled);
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
                        || !TryReadSetPos(oXML, s6DOFBodySettings.mesh.position.x, s6DOFBodySettings.mesh.position.y, s6DOFBodySettings.mesh.position.z)
                        || !TryReadSetRotation(oXML, s6DOFBodySettings.mesh.rotation.x, s6DOFBodySettings.mesh.rotation.y, s6DOFBodySettings.mesh.rotation.z)
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

                p6DOFSettings.push_back(s6DOFBodySettings);
                oXML.OutOfElem(); // Body

                pDataAvailable = true;
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

                p6DOFSettings.push_back(s6DOFBodySettings);
                oXML.OutOfElem(); // Body
            }
            if (mMajorVersion > 1 || mMinorVersion > 15)
            {
                if (!TryReadSetEuler(oXML, pGeneralSettings.eulerRotations[0], pGeneralSettings.eulerRotations[1], pGeneralSettings.eulerRotations[2]))
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

        pGazeVectorSettings.push_back({ tGazeVectorName, frequency, hwSync, filter });
        nGazeVectorCount++;
        oXML.OutOfElem(); // Vector
    }

    pDataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CMarkupDeserializer::DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& pEyeTrackerSettings,bool& pDataAvailable)
{
    pDataAvailable = false;

    pEyeTrackerSettings.clear();

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

        pEyeTrackerSettings.push_back({ tEyeTrackerName, frequency, hwSync });
        nEyeTrackerCount++;
        oXML.OutOfElem(); // Vector
    }

    pDataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CMarkupDeserializer::DeserializeAnalogSettings(std::vector<SAnalogDevice>& pAnalogDeviceSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pAnalogDeviceSettings.clear();

    if (!oXML.FindChildElem("Analog"))
    {
        // No analog data available.
        return true;
    }

    SAnalogDevice sAnalogDevice;

    oXML.IntoElem();

    if (mMajorVersion == 1 && mMinorVersion == 0)
    {
        sAnalogDevice.deviceID = 1;   // Always channel 1
        sAnalogDevice.name = "AnalogDevice";
        if (!oXML.FindChildElem("Channels"))
        {
            return false;
        }
        sAnalogDevice.channels = atoi(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Frequency"))
        {
            return false;
        }
        sAnalogDevice.frequency = atoi(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Unit"))
        {
            return false;
        }
        sAnalogDevice.unit = oXML.GetChildData();
        if (!oXML.FindChildElem("Range"))
        {
            return false;
        }
        oXML.IntoElem();
        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sAnalogDevice.minRange = (float)atof(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sAnalogDevice.maxRange = (float)atof(oXML.GetChildData().c_str());
        pAnalogDeviceSettings.push_back(sAnalogDevice);
        pDataAvailable = true;
        return true;
    }
    else
    {
        while (oXML.FindChildElem("Device"))
        {
            sAnalogDevice.labels.clear();
            sAnalogDevice.units.clear();
            oXML.IntoElem();
            if (!oXML.FindChildElem("Device_ID"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.deviceID = atoi(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Device_Name"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.name = oXML.GetChildData();

            if (!oXML.FindChildElem("Channels"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.channels = atoi(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Frequency"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.frequency = atoi(oXML.GetChildData().c_str());

            if (mMajorVersion == 1 && mMinorVersion < 11)
            {
                if (!oXML.FindChildElem("Unit"))
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
                sAnalogDevice.unit = oXML.GetChildData();
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
            sAnalogDevice.minRange = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Max"))
            {
                oXML.OutOfElem(); // Device
                oXML.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.maxRange = (float)atof(oXML.GetChildData().c_str());
            oXML.OutOfElem(); // Range

            if (mMajorVersion == 1 && mMinorVersion < 11)
            {
                for (unsigned int i = 0; i < sAnalogDevice.channels; i++)
                {
                    if (oXML.FindChildElem("Label"))
                    {
                        sAnalogDevice.labels.push_back(oXML.GetChildData());
                    }
                }
                if (sAnalogDevice.labels.size() != sAnalogDevice.channels)
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
                        sAnalogDevice.labels.push_back(oXML.GetChildData());
                    }
                    if (oXML.FindChildElem("Unit"))
                    {
                        sAnalogDevice.units.push_back(oXML.GetChildData());
                    }
                    oXML.OutOfElem(); // Channel
                }
                if (sAnalogDevice.labels.size() != sAnalogDevice.channels ||
                    sAnalogDevice.units.size() != sAnalogDevice.channels)
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
            }
            oXML.OutOfElem(); // Device
            pAnalogDeviceSettings.push_back(sAnalogDevice);
            pDataAvailable = true;
        }
    }

    return true;
} // ReadAnalogSettings

bool CMarkupDeserializer::DeserializeForceSettings(SSettingsForce& pForceSettings, bool& pDataAvailable)
{
    pDataAvailable = false;

    pForceSettings.forcePlates.clear();

    //
    // Read some force plate parameters
    //
    if (!oXML.FindChildElem("Force"))
    {
        return true;
    }

    oXML.IntoElem();

    SForcePlate sForcePlate;
    sForcePlate.validCalibrationMatrix = false;
    for (int i = 0; i < 12; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            sForcePlate.calibrationMatrix[i][j] = 0.0f;
        }
    }
    sForcePlate.calibrationMatrixRows = 6;
    sForcePlate.calibrationMatrixColumns = 6;

    if (!oXML.FindChildElem("Unit_Length"))
    {
        return false;
    }
    pForceSettings.unitLength = oXML.GetChildData();

    if (!oXML.FindChildElem("Unit_Force"))
    {
        return false;
    }
    pForceSettings.unitForce = oXML.GetChildData();

    int  iPlate = 1;
    while (oXML.FindChildElem("Plate"))
    {
        //
        // Get name and type of the plates
        //
        oXML.IntoElem(); // "Plate"
        if (oXML.FindChildElem("Force_Plate_Index")) // Version 1.7 and earlier.
        {
            sForcePlate.id = atoi(oXML.GetChildData().c_str());
        }
        else if (oXML.FindChildElem("Plate_ID")) // Version 1.8 and later.
        {
            sForcePlate.id = atoi(oXML.GetChildData().c_str());
        }
        else
        {
            return false;
        }

        if (oXML.FindChildElem("Analog_Device_ID"))
        {
            sForcePlate.analogDeviceID = atoi(oXML.GetChildData().c_str());
        }
        else
        {
            sForcePlate.analogDeviceID = 0;
        }

        if (!oXML.FindChildElem("Frequency"))
        {
            return false;
        }
        sForcePlate.frequency = atoi(oXML.GetChildData().c_str());

        if (oXML.FindChildElem("Type"))
        {
            sForcePlate.type = oXML.GetChildData();
        }
        else
        {
            sForcePlate.type = "unknown";
        }

        if (oXML.FindChildElem("Name"))
        {
            sForcePlate.name = oXML.GetChildData();
        }
        else
        {
            sForcePlate.name = CMarkup::Format("#%d", iPlate);
        }

        if (oXML.FindChildElem("Length"))
        {
            sForcePlate.length = (float)atof(oXML.GetChildData().c_str());
        }
        if (oXML.FindChildElem("Width"))
        {
            sForcePlate.width = (float)atof(oXML.GetChildData().c_str());
        }

        if (oXML.FindChildElem("Location"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Corner1"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.corner[0].x = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.corner[0].y = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.corner[0].z = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner2"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.corner[1].x = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.corner[1].y = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.corner[1].z = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner3"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.corner[2].x = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.corner[2].y = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.corner[2].z = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner4"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.corner[3].x = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.corner[3].y = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.corner[3].z = (float)atof(oXML.GetChildData().c_str());
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
                sForcePlate.origin.x = (float)atof(oXML.GetChildData().c_str());
            }
            if (oXML.FindChildElem("Y"))
            {
                sForcePlate.origin.y = (float)atof(oXML.GetChildData().c_str());
            }
            if (oXML.FindChildElem("Z"))
            {
                sForcePlate.origin.z = (float)atof(oXML.GetChildData().c_str());
            }
            oXML.OutOfElem();
        }

        sForcePlate.channels.clear();
        if (oXML.FindChildElem("Channels"))
        {
            oXML.IntoElem();
            SForceChannel sForceChannel;
            while (oXML.FindChildElem("Channel"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("Channel_No"))
                {
                    sForceChannel.channelNumber = atoi(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("ConversionFactor"))
                {
                    sForceChannel.conversionFactor = (float)atof(oXML.GetChildData().c_str());
                }
                sForcePlate.channels.push_back(sForceChannel);
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }

        if (oXML.FindChildElem("Calibration_Matrix"))
        {
            oXML.IntoElem();
            int nRow = 0;

            if (mMajorVersion == 1 && mMinorVersion < 12)
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
                        sForcePlate.calibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
                        nCol++;
                        sprintf(strCol, "Col%d", nCol + 1);
                    }
                    sForcePlate.calibrationMatrixColumns = nCol;

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
                                sForcePlate.calibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
                                nCol++;
                            }
                            sForcePlate.calibrationMatrixColumns = nCol;

                            nRow++;
                            oXML.OutOfElem(); // Columns
                        }
                        oXML.OutOfElem(); // Row
                    }
                    oXML.OutOfElem(); // Rows
                }
            }
            sForcePlate.calibrationMatrixRows = nRow;
            sForcePlate.validCalibrationMatrix = true;

            oXML.OutOfElem(); // "Calibration_Matrix"
        }
        oXML.OutOfElem(); // "Plate"

        pDataAvailable = true;
        pForceSettings.forcePlates.push_back(sForcePlate);
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
        sImageCamera.id = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Enabled"))
        {
            return false;
        }
        std::string str;
        str = ToLower(oXML.GetChildData());

        if (str == "true")
        {
            sImageCamera.enabled = true;
        }
        else
        {
            sImageCamera.enabled = false;
        }

        if (!oXML.FindChildElem("Format"))
        {
            return false;
        }
        str = ToLower(oXML.GetChildData());

        if (str == "rawgrayscale")
        {
            sImageCamera.format = CRTPacket::FormatRawGrayscale;
        }
        else if (str == "rawbgr")
        {
            sImageCamera.format = CRTPacket::FormatRawBGR;
        }
        else if (str == "jpg")
        {
            sImageCamera.format = CRTPacket::FormatJPG;
        }
        else if (str == "png")
        {
            sImageCamera.format = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sImageCamera.width = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sImageCamera.height = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Left_Crop"))
        {
            return false;
        }
        sImageCamera.cropLeft = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top_Crop"))
        {
            return false;
        }
        sImageCamera.cropTop = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right_Crop"))
        {
            return false;
        }
        sImageCamera.cropRight = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom_Crop"))
        {
            return false;
        }
        sImageCamera.cropBottom = (float)atof(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // "Camera"

        pImageSettings.push_back(sImageCamera);
        pDataAvailable = true;
    }

    return true;
} // ReadImageSettings


bool CMarkupDeserializer::DeserializeSkeletonSettings(bool pSkeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>& mSkeletonSettingsHierarchical, std::vector<SSettingsSkeleton>& mSkeletonSettings, bool& pDataAvailable)
{
    CMarkup xml = oXML;

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

bool CMarkupDeserializer::DeserializeCalibrationSettings(SCalibration& pCalibrationSettings)
{
    SCalibration settings;

    if (!oXML.FindChildElem("calibration"))
    {
        sprintf(mErrorStr, "Missing calibration element");
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

    pCalibrationSettings = settings;
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

std::string CMarkupSerializer::SetGeneralSettings(const unsigned int* captureFrequency,
                                                  const float* captureTime, const bool* startOnExtTrig, const bool* pStartOnTrigNO, const bool* pStartOnTrigNC,
                                                  const bool* pStartOnTrigSoftware, const EProcessingActions* processingActions,
                                                  const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    if (captureFrequency)
    {
        AddXMLElementUnsignedInt(&oXML, "Frequency", captureFrequency);
    }
    if (captureTime)
    {
        AddXMLElementFloat(&oXML, "Capture_Time", captureTime, 3);
    }
    if (startOnExtTrig)
    {
        AddXMLElementBool(&oXML, "Start_On_External_Trigger", startOnExtTrig);
        if (mMajorVersion > 1 || mMinorVersion > 14)
        {
            AddXMLElementBool(&oXML, "Start_On_Trigger_NO", pStartOnTrigNO);
            AddXMLElementBool(&oXML, "Start_On_Trigger_NC", pStartOnTrigNC);
            AddXMLElementBool(&oXML, "Start_On_Trigger_Software", pStartOnTrigSoftware);
        }
    }

    const char* processingActionTags[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActionSets[3] = { processingActions, rtProcessingActions, reprocessingActions };

    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActionSets[i])
        {
            oXML.AddElem(processingActionTags[i]);
            oXML.IntoElem();

            if (mMajorVersion > 1 || mMinorVersion > 13)
            {
                AddXMLElementBool(&oXML, "PreProcessing2D", (*processingActionSets[i] & ProcessingPreProcess2D) != 0);
            }
            if (*processingActionSets[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                oXML.AddElem("Tracking", "2D");
            }
            else if (*processingActionSets[i] & ProcessingTracking3D)
            {
                oXML.AddElem("Tracking", "3D");
            }
            else
            {
                oXML.AddElem("Tracking", "False");
            }
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&oXML, "TwinSystemMerge", (*processingActionSets[i] & ProcessingTwinSystemMerge) != 0);
                AddXMLElementBool(&oXML, "SplineFill", (*processingActionSets[i] & ProcessingSplineFill) != 0);
            }
            AddXMLElementBool(&oXML, "AIM", (*processingActionSets[i] & ProcessingAIM) != 0);
            AddXMLElementBool(&oXML, "Track6DOF", (*processingActionSets[i] & Processing6DOFTracking) != 0);
            AddXMLElementBool(&oXML, "ForceData", (*processingActionSets[i] & ProcessingForceData) != 0);
            AddXMLElementBool(&oXML, "GazeVector", (*processingActionSets[i] & ProcessingGazeVector) != 0);
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&oXML, "ExportTSV", (*processingActionSets[i] & ProcessingExportTSV) != 0);
                AddXMLElementBool(&oXML, "ExportC3D", (*processingActionSets[i] & ProcessingExportC3D) != 0);
                AddXMLElementBool(&oXML, "ExportMatlabFile", (*processingActionSets[i] & ProcessingExportMatlabFile) != 0);
                AddXMLElementBool(&oXML, "ExportAviFile", (*processingActionSets[i] & ProcessingExportAviFile) != 0);
            }
            oXML.OutOfElem(); // Processing_Actions
        }
    }
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource,
    const bool* signalModePeriodic, const unsigned int* freqMultiplier, const unsigned int* freqDivisor,
    const unsigned int* freqTolerance, const float* nominalFrequency, const bool* negativeEdge,
    const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();
    oXML.AddElem("External_Time_Base");
    oXML.IntoElem();

    AddXMLElementBool(&oXML, "Enabled", enabled);

    if (signalSource)
    {
        switch (*signalSource)
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

    AddXMLElementBool(&oXML, "Signal_Mode", signalModePeriodic, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(&oXML, "Frequency_Multiplier", freqMultiplier);
    AddXMLElementUnsignedInt(&oXML, "Frequency_Divisor", freqDivisor);
    AddXMLElementUnsignedInt(&oXML, "Frequency_Tolerance", freqTolerance);

    if (nominalFrequency)
    {
        if (*nominalFrequency < 0)
        {
            oXML.AddElem("Nominal_Frequency", "None");
        }
        else
        {
            AddXMLElementFloat(&oXML, "Nominal_Frequency", nominalFrequency, 3);
        }
    }

    AddXMLElementBool(&oXML, "Signal_Edge", negativeEdge, "Negative", "Positive");
    AddXMLElementUnsignedInt(&oXML, "Signal_Shutter_Delay", signalShutterDelay);
    AddXMLElementFloat(&oXML, "Non_Periodic_Timeout", nonPeriodicTimeout, 3);

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

    AddXMLElementBool(&oXML, "Enabled", timestampSettings.enabled);

    switch (timestampSettings.type)
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
    AddXMLElementUnsignedInt(&oXML, "Frequency", timestampSettings.frequency);

    oXML.OutOfElem(); // Timestamp
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraSettings(const unsigned int pCameraId, const ECameraMode* mode,
    const float* markerExposure, const float* markerThreshold, const int* orientation)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    if (mode)
    {
        switch (*mode)
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
    AddXMLElementFloat(&oXML, "Marker_Exposure", markerExposure);
    AddXMLElementFloat(&oXML, "Marker_Threshold", markerThreshold);
    AddXMLElementInt(&oXML, "Orientation", orientation);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraVideoSettings(const unsigned int pCameraId,
    const EVideoResolution* videoResolution, const EVideoAspectRatio* videoAspectRatio,
    const unsigned int* videoFrequency, const float* videoExposure, const float* videoFlashTime)
{
    CMarkup oXML;
    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);
    if (videoResolution)
    {
        switch (*videoResolution)
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
    if (videoAspectRatio)
    {
        switch (*videoAspectRatio)
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
    AddXMLElementUnsignedInt(&oXML, "Video_Frequency", videoFrequency);
    AddXMLElementFloat(&oXML, "Video_Exposure", videoExposure);
    AddXMLElementFloat(&oXML, "Video_Flash_Time", videoFlashTime);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraSyncOutSettings(const unsigned int pCameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* syncOutMode, const unsigned int* syncOutValue, const float* syncOutDutyCycle,
    const bool* syncOutNegativePolarity)
{
    CMarkup oXML;
    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    int port = portNumber - 1;
    if (((port == 0 || port == 1) && syncOutMode) || (port == 2))
    {
        oXML.AddElem(port == 0 ? "Sync_Out" : (port == 1 ? "Sync_Out2" : "Sync_Out_MT"));
        oXML.IntoElem();

        if (port == 0 || port == 1)
        {
            switch (*syncOutMode)
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

            if (*syncOutMode == ModeMultiplier ||
                *syncOutMode == ModeDivisor ||
                *syncOutMode == ModeIndependentFreq)
            {
                if (syncOutValue)
                {
                    AddXMLElementUnsignedInt(&oXML, "Value", syncOutValue);
                }
                if (syncOutDutyCycle)
                {
                    AddXMLElementFloat(&oXML, "Duty_Cycle", syncOutDutyCycle, 3);
                }
            }
        }
        if (syncOutNegativePolarity && (port == 2 ||
            (syncOutMode && *syncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(&oXML, "Signal_Polarity", syncOutNegativePolarity, "Negative", "Positive");
        }
        oXML.OutOfElem(); // Sync_Out
    }
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus,
    const float pAperture)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    oXML.AddElem("LensControl");
    oXML.IntoElem();

    oXML.AddElem("Focus");
    oXML.AddAttrib("Value", CMarkup::Format("%f", pFocus).c_str());
    oXML.AddElem("Aperture");
    oXML.AddAttrib("Value", CMarkup::Format("%f", pAperture).c_str());

    oXML.OutOfElem(); // LensControl
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure,
    const float pCompensation)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    oXML.AddElem("LensControl");
    oXML.IntoElem();

    oXML.AddElem("AutoExposure");
    oXML.AddAttrib("Enabled", pAutoExposure ? "true" : "false");
    oXML.AddAttrib("Compensation", CMarkup::Format("%f", pCompensation).c_str());

    oXML.OutOfElem(); // AutoExposure
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    oXML.AddElem("AutoWhiteBalance", pEnable ? "true" : "false");

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetImageSettings(const unsigned int pCameraId, const bool* enable,
    const CRTPacket::EImageFormat* format, const unsigned int* width, const unsigned int* height,
    const float* leftCrop, const float* topCrop, const float* rightCrop, const float* bottomCrop)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("Image");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &pCameraId);

    AddXMLElementBool(&oXML, "Enabled", enable);

    if (format)
    {
        switch (*format)
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
    AddXMLElementUnsignedInt(&oXML, "Width", width);
    AddXMLElementUnsignedInt(&oXML, "Height", height);
    AddXMLElementFloat(&oXML, "Left_Crop", leftCrop);
    AddXMLElementFloat(&oXML, "Top_Crop", topCrop);
    AddXMLElementFloat(&oXML, "Right_Crop", rightCrop);
    AddXMLElementFloat(&oXML, "Bottom_Crop", bottomCrop);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // Image
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::SetForceSettings(const unsigned int pPlateId, const SPoint* pCorner1,
    const SPoint* pCorner2, const SPoint* pCorner3, const SPoint* pCorner4)
{
    CMarkup oXML;
    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("Force");
    oXML.IntoElem();

    oXML.AddElem("Plate");
    oXML.IntoElem();

    if (mMajorVersion > 1 || mMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(&oXML, "Plate_ID", &pPlateId);
    }
    else
    {
        AddXMLElementUnsignedInt(&oXML, "Force_Plate_Index", &pPlateId);
    }
    if (pCorner1)
    {
        oXML.AddElem("Corner1");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(pCorner1->x));
        AddXMLElementFloat(&oXML, "Y", &(pCorner1->y));
        AddXMLElementFloat(&oXML, "Z", &(pCorner1->z));
        oXML.OutOfElem(); // Corner1
    }
    if (pCorner2)
    {
        oXML.AddElem("Corner2");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(pCorner2->x));
        AddXMLElementFloat(&oXML, "Y", &(pCorner2->y));
        AddXMLElementFloat(&oXML, "Z", &(pCorner2->z));
        oXML.OutOfElem(); // Corner2
    }
    if (pCorner3)
    {
        oXML.AddElem("Corner3");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(pCorner3->x));
        AddXMLElementFloat(&oXML, "Y", &(pCorner3->y));
        AddXMLElementFloat(&oXML, "Z", &(pCorner3->z));
        oXML.OutOfElem(); // Corner3
    }
    if (pCorner4)
    {
        oXML.AddElem("Corner4");
        oXML.IntoElem();
        AddXMLElementFloat(&oXML, "X", &(pCorner4->x));
        AddXMLElementFloat(&oXML, "Y", &(pCorner4->y));
        AddXMLElementFloat(&oXML, "Z", &(pCorner4->z));
        oXML.OutOfElem(); // Corner4
    }
    oXML.OutOfElem(); // Plate

    oXML.OutOfElem(); // Force
    oXML.OutOfElem(); // QTM_Settings

    return oXML.GetDoc();
}

std::string CMarkupSerializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("The_6D");
    oXML.IntoElem();

    for (auto& body : pSettings6Dofs)
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
            oXML.AddAttrib("X", std::to_string(body.mesh.position.x).c_str());
            oXML.AddAttrib("Y", std::to_string(body.mesh.position.y).c_str());
            oXML.AddAttrib("Z", std::to_string(body.mesh.position.z).c_str());
            oXML.AddElem("Rotation");
            oXML.AddAttrib("X", std::to_string(body.mesh.rotation.x).c_str());
            oXML.AddAttrib("Y", std::to_string(body.mesh.rotation.y).c_str());
            oXML.AddAttrib("Z", std::to_string(body.mesh.rotation.z).c_str());
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
                oXML.AddAttrib("X", std::to_string(point.x).c_str());
                oXML.AddAttrib("Y", std::to_string(point.y).c_str());
                oXML.AddAttrib("Z", std::to_string(point.z).c_str());
                oXML.AddAttrib("Virtual", point.virtual_ ? "1" : "0");
                oXML.AddAttrib("PhysicalId", std::to_string(point.physicalId).c_str());
                oXML.AddAttrib("Name", point.name.c_str());
            }
            oXML.OutOfElem(); // Points
        }
        oXML.AddElem("Data_origin", std::to_string(body.origin.type).c_str());
        oXML.AddAttrib("X", std::to_string(body.origin.position.x).c_str());
        oXML.AddAttrib("Y", std::to_string(body.origin.position.y).c_str());
        oXML.AddAttrib("Z", std::to_string(body.origin.position.z).c_str());
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

