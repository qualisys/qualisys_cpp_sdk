#define _CRT_SECURE_NO_WARNINGS

#include "Tinyxml2Serializer.h"
#include <tinyxml2.h>

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
