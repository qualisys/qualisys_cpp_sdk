#define _CRT_SECURE_NO_WARNINGS

#include "Tinyxml2Serializer.h"
#include <tinyxml2.h>

#include "Settings.h"

#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

void CTinyxml2Serializer::AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* tag, const bool* value, tinyxml2::XMLDocument& document, const char* trueText, const char* falseText)
{
    if (value)
    {
        tinyxml2::XMLElement* pElement = document.NewElement(tag);
        pElement->SetText(*value ? trueText : falseText);
        parentElem.InsertEndChild(pElement);
    }
}

void CTinyxml2Serializer::AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* tag, const bool value, tinyxml2::XMLDocument& document, const char* trueText, const char* falseText)
{
    tinyxml2::XMLElement* pElement = document.NewElement(tag);
    pElement->SetText(value ? trueText : falseText);
    parentElem.InsertEndChild(pElement);
}

void CTinyxml2Serializer::AddXMLElementInt(tinyxml2::XMLElement& parentElem, const char* tag, const int* value, tinyxml2::XMLDocument& document)
{
    if (value)
    {
        tinyxml2::XMLElement* elem = document.NewElement(tag);
        elem->SetText(*value);
        parentElem.InsertEndChild(elem);
    }
}

void CTinyxml2Serializer::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* tag, const unsigned int value, tinyxml2::XMLDocument& document)
{
    tinyxml2::XMLElement* elem = document.NewElement(tag);
    elem->SetText(value);
    parentElem.InsertEndChild(elem);
}

void CTinyxml2Serializer::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* tag, const unsigned int* value, tinyxml2::XMLDocument& document)
{
    if (value)
    {
        AddXMLElementUnsignedInt(parentElem, tag, *value, document);
    }
}

void CTinyxml2Serializer::AddXMLElementFloat(tinyxml2::XMLElement& parentElem, const char* tag, const float* value, unsigned int decimals, tinyxml2::XMLDocument& document)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, *value);

    tinyxml2::XMLElement* elem = document.NewElement(tag);
    elem->SetText(formattedValue);
    parentElem.InsertEndChild(elem);
}

void CTinyxml2Serializer::AddXMLElementFloatWithTextAttribute(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const char* elementName, const char* attributeName, const float& value, unsigned int decimals)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);

    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetAttribute(attributeName, formattedValue);
    parentElem.InsertEndChild(elem);
}


void CTinyxml2Serializer::AddXMLElementTransform(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SPosition& position, const SRotation& rotation)
{
    tinyxml2::XMLElement* transformElem = document.NewElement(name.c_str());
    parentElem.InsertEndChild(transformElem);

    tinyxml2::XMLElement* positionElem = document.NewElement("Position");
    positionElem->SetAttribute("X", std::to_string(position.x).c_str());
    positionElem->SetAttribute("Y", std::to_string(position.y).c_str());
    positionElem->SetAttribute("Z", std::to_string(position.z).c_str());
    transformElem->InsertEndChild(positionElem);

    tinyxml2::XMLElement* rotationElem = document.NewElement("Rotation");
    rotationElem->SetAttribute("X", std::to_string(rotation.x).c_str());
    rotationElem->SetAttribute("Y", std::to_string(rotation.y).c_str());
    rotationElem->SetAttribute("Z", std::to_string(rotation.z).c_str());
    rotationElem->SetAttribute("W", std::to_string(rotation.w).c_str());
    transformElem->InsertEndChild(rotationElem);
}

void CTinyxml2Serializer::AddXMLElementDOF(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SDegreeOfFreedom& degreesOfFreedom)
{
    tinyxml2::XMLElement* dofElem = document.NewElement(name.c_str());
    parentElem.InsertEndChild(dofElem);

    if (!std::isnan(degreesOfFreedom.lowerBound) && !std::isnan(degreesOfFreedom.upperBound))
    {
        if (mnMajorVersion > 1 || mnMinorVersion > 21)
        {
            tinyxml2::XMLElement* constraintElem = document.NewElement("Constraint");
            constraintElem->SetAttribute("LowerBound", std::to_string(degreesOfFreedom.lowerBound).c_str());
            constraintElem->SetAttribute("UpperBound", std::to_string(degreesOfFreedom.upperBound).c_str());
            dofElem->InsertEndChild(constraintElem);
        }
        else
        {
            // If not in a 'Constraint' block, add 'LowerBound' & 'UpperBound' directly to dofElem
            dofElem->SetAttribute("LowerBound", std::to_string(degreesOfFreedom.lowerBound).c_str());
            dofElem->SetAttribute("UpperBound", std::to_string(degreesOfFreedom.upperBound).c_str());
        }
    }

    if (!degreesOfFreedom.couplings.empty())
    {
        tinyxml2::XMLElement* couplingsElem = document.NewElement("Couplings");
        dofElem->InsertEndChild(couplingsElem);

        for (const auto& coupling : degreesOfFreedom.couplings)
        {
            tinyxml2::XMLElement* couplingElem = document.NewElement("Coupling");
            couplingElem->SetAttribute("Segment", coupling.segment.c_str());
            couplingElem->SetAttribute("DegreeOfFreedom", SkeletonDofToStringSettings(coupling.degreeOfFreedom));
            couplingElem->SetAttribute("Coefficient", std::to_string(coupling.coefficient).c_str());
            couplingsElem->InsertEndChild(couplingElem);
        }
    }

    if (!std::isnan(degreesOfFreedom.goalValue) && !std::isnan(degreesOfFreedom.goalWeight))
    {
        tinyxml2::XMLElement* goalElem = document.NewElement("Goal");
        goalElem->SetAttribute("Value", std::to_string(degreesOfFreedom.goalValue).c_str());
        goalElem->SetAttribute("Weight", std::to_string(degreesOfFreedom.goalWeight).c_str());
        dofElem->InsertEndChild(goalElem);
    }
}



CTinyxml2Serializer::CTinyxml2Serializer(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mnMajorVersion(majorVersion), mnMinorVersion(minorVersion)
{
}

std::string CTinyxml2Serializer::SetGeneralSettings(const unsigned int* captureFrequency,
    const float* captureTime, const bool* startOnExtTrig,
    const bool* startOnTrigNO, const bool* startOnTrigNC,
    const bool* startOnTrigSoftware, const EProcessingActions* processingActions,
    const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Capture Frequency
    if (captureFrequency)
    {
        AddXMLElementUnsignedInt(*general, "Frequency", captureFrequency, document);
    }

    // Capture Time
    if (captureTime)
    {
        AddXMLElementFloat(*general, "Capture_Time", captureTime, 3, document);
    }

    // External Trigger and additional triggers
    if (startOnExtTrig)
    {
        AddXMLElementBool(*general, "Start_On_External_Trigger", startOnExtTrig, document);

        if (mnMajorVersion > 1 || mnMinorVersion > 14)
        {
            AddXMLElementBool(*general, "Start_On_Trigger_NO", startOnTrigNO, document);
            AddXMLElementBool(*general, "Start_On_Trigger_NC", startOnTrigNC, document);
            AddXMLElementBool(*general, "Start_On_Trigger_Software", startOnTrigSoftware, document);
        }
    }

    // Processing Actions
    const char* processingActionTags[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActionSets[3] = { processingActions, rtProcessingActions, reprocessingActions };

    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActionSets[i])
        {
            tinyxml2::XMLElement* processing = document.NewElement(processingActionTags[i]);
            general->InsertEndChild(processing);

            if (mnMajorVersion > 1 || mnMinorVersion > 13)
            {
                AddXMLElementBool(*processing, "PreProcessing2D", (*processingActionSets[i] & ProcessingPreProcess2D) != 0, document);
            }

            if (*processingActionSets[i] & ProcessingTracking2D && i != 1) // i != 1 => Not Not RtProcessingSettings
            {
                tinyxml2::XMLElement* pTracking = document.NewElement("Tracking");
                pTracking->SetText("2D");
                processing->InsertEndChild(pTracking);
            }
            else if (*processingActionSets[i] & ProcessingTracking3D)
            {
                tinyxml2::XMLElement* pTracking = document.NewElement("Tracking");
                pTracking->SetText("3D");
                processing->InsertEndChild(pTracking);
            }
            else
            {
                tinyxml2::XMLElement* pTracking = document.NewElement("Tracking");
                pTracking->SetText("False");
                processing->InsertEndChild(pTracking);
            }

            if (i != 1) // Not RtprocessingActionTagsettings
            {
                AddXMLElementBool(*processing, "TwinSystemMerge", (*processingActionSets[i] & ProcessingTwinSystemMerge) != 0, document);
                AddXMLElementBool(*processing, "SplineFill", (*processingActionSets[i] & ProcessingSplineFill) != 0, document);
            }

            AddXMLElementBool(*processing, "AIM", (*processingActionSets[i] & ProcessingAIM) != 0, document);
            AddXMLElementBool(*processing, "Track6DOF", (*processingActionSets[i] & Processing6DOFTracking) != 0, document);
            AddXMLElementBool(*processing, "ForceData", (*processingActionSets[i] & ProcessingForceData) != 0, document);
            AddXMLElementBool(*processing, "GazeVector", (*processingActionSets[i] & ProcessingGazeVector) != 0, document);

            if (i != 1) // Not RtprocessingActionTagsettings
            {
                AddXMLElementBool(*processing, "ExportTSV", (*processingActionSets[i] & ProcessingExportTSV) != 0, document);
                AddXMLElementBool(*processing, "ExportC3D", (*processingActionSets[i] & ProcessingExportC3D) != 0, document);
                AddXMLElementBool(*processing, "ExportMatlabFile", (*processingActionSets[i] & ProcessingExportMatlabFile) != 0, document);
                AddXMLElementBool(*processing, "ExportAviFile", (*processingActionSets[i] & ProcessingExportAviFile) != 0, document);
            }
        }
    }

    // Convert to string
    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource,
    const bool* signalModePeriodic, const unsigned int* freqMultiplier, const unsigned int* freqDivisor,
    const unsigned int* freqTolerance, const float* nominalFrequency, const bool* negativeEdge,
    const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // External Time Base element
    tinyxml2::XMLElement* pTimeBase = document.NewElement("External_Time_Base");
    general->InsertEndChild(pTimeBase);

    // Add Enabled element
    AddXMLElementBool(*pTimeBase, "Enabled", enabled, document);

    // Add Signal Source if available
    if (signalSource)
    {
        tinyxml2::XMLElement* pSignalSource = document.NewElement("Signal_Source");
        switch (*signalSource)
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
    AddXMLElementBool(*pTimeBase, "Signal_Mode", signalModePeriodic, document, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(*pTimeBase, "Frequency_Multiplier", freqMultiplier, document);
    AddXMLElementUnsignedInt(*pTimeBase, "Frequency_Divisor", freqDivisor, document);
    AddXMLElementUnsignedInt(*pTimeBase, "Frequency_Tolerance", freqTolerance, document);

    // Add Nominal Frequency element
    if (nominalFrequency)
    {
        if (*nominalFrequency < 0)
        {
            tinyxml2::XMLElement* pNominalFreq = document.NewElement("Nominal_Frequency");
            pNominalFreq->SetText("None");
            pTimeBase->InsertEndChild(pNominalFreq);
        }
        else
        {
            AddXMLElementFloat(*pTimeBase, "Nominal_Frequency", nominalFrequency, 3, document);
        }
    }

    AddXMLElementBool(*pTimeBase, "Signal_Edge", negativeEdge, document, "Negative", "Positive");
    AddXMLElementUnsignedInt(*pTimeBase, "Signal_Shutter_Delay", signalShutterDelay, document);
    AddXMLElementFloat(*pTimeBase, "Non_Periodic_Timeout", nonPeriodicTimeout, 3, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // External Timestamp element
    tinyxml2::XMLElement* pTimestamp = document.NewElement("External_Timestamp");
    general->InsertEndChild(pTimestamp);

    // Add Enabled element
    AddXMLElementBool(*pTimestamp, "Enabled", timestampSettings.bEnabled, document);

    // Add Type element
    tinyxml2::XMLElement* pType = document.NewElement("Type");
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
    AddXMLElementUnsignedInt(*pTimestamp, "Frequency", timestampSettings.nFrequency, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetCameraSettings(
    const unsigned int pCameraId, const ECameraMode* peMode,
    const float* pfMarkerExposure, const float* pfMarkerThreshold,
    const int* pnOrientation)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    general->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, document);

    // Add Mode
    if (peMode)
    {
        tinyxml2::XMLElement* pMode = document.NewElement("Mode");
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
    AddXMLElementFloat(*pCamera, "Marker_Exposure", pfMarkerExposure, 6, document);
    AddXMLElementFloat(*pCamera, "Marker_Threshold", pfMarkerThreshold, 6, document);
    AddXMLElementInt(*pCamera, "Orientation", pnOrientation, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraVideoSettings(const unsigned int pCameraId,
    const EVideoResolution* eVideoResolution, const EVideoAspectRatio* eVideoAspectRatio,
    const unsigned int* pnVideoFrequency, const float* pfVideoExposure, const float* pfVideoFlashTime)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    general->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, document);

    // Add Video Resolution
    if (eVideoResolution)
    {
        tinyxml2::XMLElement* pResolution = document.NewElement("Video_Resolution");
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
        tinyxml2::XMLElement* pAspectRatio = document.NewElement("Video_Aspect_Ratio");
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
    AddXMLElementUnsignedInt(*pCamera, "Video_Frequency", pnVideoFrequency, document);
    AddXMLElementFloat(*pCamera, "Video_Exposure", pfVideoExposure, 6, document);
    AddXMLElementFloat(*pCamera, "Video_Flash_Time", pfVideoFlashTime, 6, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraSyncOutSettings(const unsigned int pCameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* peSyncOutMode, const unsigned int* pnSyncOutValue, const float* syncOutDutyCycle,
    const bool* pbSyncOutNegativePolarity)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    general->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, document);

    // Determine port name
    int port = portNumber - 1;
    if (((port == 0 || port == 1) && peSyncOutMode) || (port == 2))
    {
        tinyxml2::XMLElement* pSyncOut = nullptr;
        if (port == 0)
            pSyncOut = document.NewElement("Sync_Out");
        else if (port == 1)
            pSyncOut = document.NewElement("Sync_Out2");
        else
            pSyncOut = document.NewElement("Sync_Out_MT");

        pCamera->InsertEndChild(pSyncOut);

        // Add Sync Out Mode
        if (port == 0 || port == 1)
        {
            tinyxml2::XMLElement* pMode = document.NewElement("Mode");
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
                AddXMLElementUnsignedInt(*pSyncOut, "Value", pnSyncOutValue, document);
                AddXMLElementFloat(*pSyncOut, "Duty_Cycle", syncOutDutyCycle, 3, document);
            }
        }

        // Add Signal Polarity
        if (pbSyncOutNegativePolarity && (port == 2 ||
            (peSyncOutMode && *peSyncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(*pSyncOut, "Signal_Polarity", pbSyncOutNegativePolarity, document, "Negative", "Positive");
        }
    }

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus,
    const float pAperture)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    general->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, document);

    // LensControl element
    tinyxml2::XMLElement* pLensControl = document.NewElement("LensControl");
    pCamera->InsertEndChild(pLensControl);

    // Add Focus and Aperture as float attributes
    AddXMLElementFloatWithTextAttribute(document, *pLensControl, "Focus", "Value", pFocus, 6);
    AddXMLElementFloatWithTextAttribute(document, *pLensControl, "Aperture", "Value", pAperture, 6);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string CTinyxml2Serializer::SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure,
    const float pCompensation)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    general->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, document);

    // LensControl element
    tinyxml2::XMLElement* pLensControl = document.NewElement("LensControl");
    pCamera->InsertEndChild(pLensControl);

    // AutoExposure element with attributes
    tinyxml2::XMLElement* pAutoExposureElem = document.NewElement("AutoExposure");
    pAutoExposureElem->SetAttribute("Enabled", pAutoExposure ? "true" : "false");

    // Format Compensation float value
    char compensationStr[32];
    snprintf(compensationStr, sizeof(compensationStr), "%.6f", pCompensation);
    pAutoExposureElem->SetAttribute("Compensation", compensationStr);

    pLensControl->InsertEndChild(pAutoExposureElem);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // General element
    tinyxml2::XMLElement* general = document.NewElement("General");
    root->InsertEndChild(general);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    general->InsertEndChild(pCamera);

    // Add Camera ID
    AddXMLElementUnsignedInt(*pCamera, "ID", &pCameraId, document);

    // AutoWhiteBalance element
    tinyxml2::XMLElement* pAWB = document.NewElement("AutoWhiteBalance");
    pAWB->SetText(pEnable ? "true" : "false");
    pCamera->InsertEndChild(pAWB);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}


std::string CTinyxml2Serializer::SetImageSettings(const unsigned int pCameraId, const bool* pbEnable,
    const CRTPacket::EImageFormat* peFormat, const unsigned int* pnWidth, const unsigned int* pnHeight,
    const float* pfLeftCrop, const float* pfTopCrop, const float* pfRightCrop, const float* pfBottomCrop)
{
    tinyxml2::XMLDocument document;

    // Root element
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    // Image element
    tinyxml2::XMLElement* pImage = document.NewElement("Image");
    root->InsertEndChild(pImage);

    // Camera element
    tinyxml2::XMLElement* pCamera = document.NewElement("Camera");
    pImage->InsertEndChild(pCamera);

    // ID
    AddXMLElementUnsignedInt(*pCamera, "ID", pCameraId, document);

    // Enabled
    AddXMLElementBool(*pCamera, "Enabled", pbEnable, document);

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
            tinyxml2::XMLElement* pFormat = document.NewElement("Format");
            pFormat->SetText(formatStr);
            pCamera->InsertEndChild(pFormat);
        }
    }

    // Other settings
    AddXMLElementUnsignedInt(*pCamera, "Width", pnWidth, document);
    AddXMLElementUnsignedInt(*pCamera, "Height", pnHeight, document);
    AddXMLElementFloat(*pCamera, "Left_Crop", pfLeftCrop, 6, document);
    AddXMLElementFloat(*pCamera, "Top_Crop", pfTopCrop, 6, document);
    AddXMLElementFloat(*pCamera, "Right_Crop", pfRightCrop, 6, document);
    AddXMLElementFloat(*pCamera, "Bottom_Crop", pfBottomCrop, 6, document);

    // Convert to string
    tinyxml2::XMLPrinter printer;
    document.Print(&printer);

    return printer.CStr();
}

std::string CTinyxml2Serializer::SetForceSettings(const unsigned int pPlateId, const SPoint* pCorner1,
    const SPoint* pCorner2, const SPoint* pCorner3, const SPoint* pCorner4)
{
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    tinyxml2::XMLElement* forceElem = document.NewElement("Force");
    root->InsertEndChild(forceElem);

    tinyxml2::XMLElement* plateElem = document.NewElement("Plate");
    forceElem->InsertEndChild(plateElem);

    if (mnMajorVersion > 1 || mnMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(*plateElem, "Plate_ID", &pPlateId, document);
    }
    else
    {
        AddXMLElementUnsignedInt(*plateElem, "Force_Plate_Index", &pPlateId, document);
    }

    auto addCorner = [&](const char* name, const SPoint* pCorner)
        {
            if (pCorner)
            {
                tinyxml2::XMLElement* cornerElem = document.NewElement(name);
                plateElem->InsertEndChild(cornerElem);

                AddXMLElementFloat(*cornerElem, "X", &(pCorner->fX), 6, document);
                AddXMLElementFloat(*cornerElem, "Y", &(pCorner->fY), 6, document);
                AddXMLElementFloat(*cornerElem, "Z", &(pCorner->fZ), 6, document);
            }
        };

    addCorner("Corner1", pCorner1);
    addCorner("Corner2", pCorner2);
    addCorner("Corner3", pCorner3);
    addCorner("Corner4", pCorner4);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);

    return printer.CStr();
}

std::string CTinyxml2Serializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs)
{
    tinyxml2::XMLDocument document;
    auto* root = document.NewElement("QTM_Settings");
    document.InsertFirstChild(root);

    auto* the6D = document.NewElement("The_6D");
    root->InsertEndChild(the6D);

    for (const auto& body : pSettings6Dofs)
    {
        auto* bodyElem = document.NewElement("Body");
        the6D->InsertEndChild(bodyElem);

        auto* nameElem = document.NewElement("Name");
        nameElem->SetText(body.name.c_str());
        bodyElem->InsertEndChild(nameElem);

        auto* enabledElem = document.NewElement("Enabled");
        enabledElem->SetText(body.enabled ? "true" : "false");
        bodyElem->InsertEndChild(enabledElem);

        auto* colorElem = document.NewElement("Color");
        colorElem->SetAttribute("R", body.color & 0xff);
        colorElem->SetAttribute("G", (body.color >> 8) & 0xff);
        colorElem->SetAttribute("B", (body.color >> 16) & 0xff);
        bodyElem->InsertEndChild(colorElem);

        auto* maxResidualElem = document.NewElement("MaximumResidual");
        maxResidualElem->SetText(std::to_string(body.maxResidual).c_str());
        bodyElem->InsertEndChild(maxResidualElem);

        auto* minMarkersElem = document.NewElement("MinimumMarkersInBody");
        minMarkersElem->SetText(std::to_string(body.minMarkersInBody).c_str());
        bodyElem->InsertEndChild(minMarkersElem);

        auto* boneToleranceElem = document.NewElement("BoneLengthTolerance");
        boneToleranceElem->SetText(std::to_string(body.boneLengthTolerance).c_str());
        bodyElem->InsertEndChild(boneToleranceElem);

        auto* filterElem = document.NewElement("Filter");
        filterElem->SetAttribute("Preset", body.filterPreset.c_str());
        bodyElem->InsertEndChild(filterElem);

        if (!body.mesh.name.empty())
        {
            auto* meshElem = document.NewElement("Mesh");
            bodyElem->InsertEndChild(meshElem);

            auto* meshNameElem = document.NewElement("Name");
            meshNameElem->SetText(body.mesh.name.c_str());
            meshElem->InsertEndChild(meshNameElem);

            auto* positionElem = document.NewElement("Position");
            positionElem->SetAttribute("X", std::to_string(body.mesh.position.fX).c_str());
            positionElem->SetAttribute("Y", std::to_string(body.mesh.position.fY).c_str());
            positionElem->SetAttribute("Z", std::to_string(body.mesh.position.fZ).c_str());
            meshElem->InsertEndChild(positionElem);

            auto* rotationElem = document.NewElement("Rotation");
            rotationElem->SetAttribute("X", std::to_string(body.mesh.rotation.fX).c_str());
            rotationElem->SetAttribute("Y", std::to_string(body.mesh.rotation.fY).c_str());
            rotationElem->SetAttribute("Z", std::to_string(body.mesh.rotation.fZ).c_str());
            meshElem->InsertEndChild(rotationElem);

            auto* scaleElem = document.NewElement("Scale");
            scaleElem->SetText(std::to_string(body.mesh.scale).c_str());
            meshElem->InsertEndChild(scaleElem);

            auto* opacityElem = document.NewElement("Opacity");
            opacityElem->SetText(std::to_string(body.mesh.opacity).c_str());
            meshElem->InsertEndChild(opacityElem);
        }

        if (!body.points.empty())
        {
            auto* pointsElem = document.NewElement("Points");
            bodyElem->InsertEndChild(pointsElem);

            for (const auto& point : body.points)
            {
                auto* pointElem = document.NewElement("Point");
                pointElem->SetAttribute("X", std::to_string(point.fX).c_str());
                pointElem->SetAttribute("Y", std::to_string(point.fY).c_str());
                pointElem->SetAttribute("Z", std::to_string(point.fZ).c_str());
                pointElem->SetAttribute("Virtual", point.virtual_ ? "1" : "0");
                pointElem->SetAttribute("PhysicalId", point.physicalId);
                pointElem->SetAttribute("Name", point.name.c_str());
                pointsElem->InsertEndChild(pointElem);
            }
        }

        auto* dataOriginElem = document.NewElement("Data_origin");
        dataOriginElem->SetText(std::to_string(body.origin.type).c_str());
        dataOriginElem->SetAttribute("X", std::to_string(body.origin.position.fX).c_str());
        dataOriginElem->SetAttribute("Y", std::to_string(body.origin.position.fY).c_str());
        dataOriginElem->SetAttribute("Z", std::to_string(body.origin.position.fZ).c_str());
        dataOriginElem->SetAttribute("Relative_body", body.origin.relativeBody);
        bodyElem->InsertEndChild(dataOriginElem);

        auto* dataOrientationElem = document.NewElement("Data_orientation");
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
    document.Print(&printer);
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
