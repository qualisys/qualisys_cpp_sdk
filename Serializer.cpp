#include "Serializer.h"

#include <functional>

using namespace qualisys_cpp_sdk;

Serializer::Serializer(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion)
{
}

void Serializer::AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* elementName, const bool* value, tinyxml2::XMLDocument& document, const char* trueText, const char* falseText)
{
    if (value)
    {
        tinyxml2::XMLElement* elem = document.NewElement(elementName);
        elem->SetText(*value ? trueText : falseText);
        parentElem.InsertEndChild(elem);
    }
}

void Serializer::AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* elementName, const bool value, tinyxml2::XMLDocument& document, const char* trueText, const char* falseText)
{
    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetText(value ? trueText : falseText);
    parentElem.InsertEndChild(elem);
}

void Serializer::AddXMLElementInt(tinyxml2::XMLElement& parentElem, const char* elementName, const int* value, tinyxml2::XMLDocument& document)
{
    if (value)
    {
        tinyxml2::XMLElement* elem = document.NewElement(elementName);
        elem->SetText(*value);
        parentElem.InsertEndChild(elem);
    }
}

void Serializer::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* elementName, const unsigned int value, tinyxml2::XMLDocument& document)
{
    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetText(value);
    parentElem.InsertEndChild(elem);
}

void Serializer::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* elementName, const unsigned int* value, tinyxml2::XMLDocument& document)
{
    if (value)
    {
        AddXMLElementUnsignedInt(parentElem, elementName, *value, document);
    }
}

void Serializer::AddXMLElementFloat(tinyxml2::XMLElement& parentElem, const char* elementName, const float* value, unsigned int decimals, tinyxml2::XMLDocument& document)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, *value);

    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetText(formattedValue);
    parentElem.InsertEndChild(elem);
}

void Serializer::AddXMLElementFloatWithTextAttribute(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const char* elementName, const char* attributeName, const float& value, unsigned int decimals)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);

    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetAttribute(attributeName, formattedValue);
    parentElem.InsertEndChild(elem);
}

void Serializer::AddXMLElementTransform(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SPosition& position, const SRotation& rotation)
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

void Serializer::AddXMLElementDOF(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SDegreeOfFreedom& degreesOfFreedom)
{
    tinyxml2::XMLElement* dofElem = document.NewElement(name.c_str());
    parentElem.InsertEndChild(dofElem);

    if (!std::isnan(degreesOfFreedom.lowerBound) && !std::isnan(degreesOfFreedom.upperBound))
    {
        if (mMajorVersion > 1 || mMinorVersion > 21)
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

std::string Serializer::SetGeneralSettings(const unsigned int* captureFrequency,
    const float* captureTime, const bool* startOnExtTrig,
    const bool* startOnTrigNO, const bool* startOnTrigNC,
    const bool* startOnTrigSoftware, const EProcessingActions* processingActions,
    const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    if (captureFrequency)
    {
        AddXMLElementUnsignedInt(*generalElem, "Frequency", captureFrequency, document);
    }

    if (captureTime)
    {
        AddXMLElementFloat(*generalElem, "Capture_Time", captureTime, 3, document);
    }

    // External Trigger and additional triggers
    if (startOnExtTrig)
    {
        AddXMLElementBool(*generalElem, "Start_On_External_Trigger", startOnExtTrig, document);

        if (mMajorVersion > 1 || mMinorVersion > 14)
        {
            AddXMLElementBool(*generalElem, "Start_On_Trigger_NO", startOnTrigNO, document);
            AddXMLElementBool(*generalElem, "Start_On_Trigger_NC", startOnTrigNC, document);
            AddXMLElementBool(*generalElem, "Start_On_Trigger_Software", startOnTrigSoftware, document);
        }
    }

    const char* processingActionTags[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActionSets[3] = { processingActions, rtProcessingActions, reprocessingActions };

    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActionSets[i])
        {
            tinyxml2::XMLElement* processing = document.NewElement(processingActionTags[i]);
            generalElem->InsertEndChild(processing);

            if (mMajorVersion > 1 || mMinorVersion > 13)
            {
                AddXMLElementBool(*processing, "PreProcessing2D", (*processingActionSets[i] & ProcessingPreProcess2D) != 0, document);
            }

            if (*processingActionSets[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                tinyxml2::XMLElement* trackingElem = document.NewElement("Tracking");
                trackingElem->SetText("2D");
                processing->InsertEndChild(trackingElem);
            }
            else if (*processingActionSets[i] & ProcessingTracking3D)
            {
                tinyxml2::XMLElement* trackingElem = document.NewElement("Tracking");
                trackingElem->SetText("3D");
                processing->InsertEndChild(trackingElem);
            }
            else
            {
                tinyxml2::XMLElement* trackingElem = document.NewElement("Tracking");
                trackingElem->SetText("False");
                processing->InsertEndChild(trackingElem);
            }

            if (i != 1) // Not RtProcessingSettings
            {
                AddXMLElementBool(*processing, "TwinSystemMerge", (*processingActionSets[i] & ProcessingTwinSystemMerge) != 0, document);
                AddXMLElementBool(*processing, "SplineFill", (*processingActionSets[i] & ProcessingSplineFill) != 0, document);
            }

            AddXMLElementBool(*processing, "AIM", (*processingActionSets[i] & ProcessingAIM) != 0, document);
            AddXMLElementBool(*processing, "Track6DOF", (*processingActionSets[i] & Processing6DOFTracking) != 0, document);
            AddXMLElementBool(*processing, "ForceData", (*processingActionSets[i] & ProcessingForceData) != 0, document);
            AddXMLElementBool(*processing, "GazeVector", (*processingActionSets[i] & ProcessingGazeVector) != 0, document);

            if (i != 1) // Not RtProcessingSettings
            {
                AddXMLElementBool(*processing, "ExportTSV", (*processingActionSets[i] & ProcessingExportTSV) != 0, document);
                AddXMLElementBool(*processing, "ExportC3D", (*processingActionSets[i] & ProcessingExportC3D) != 0, document);
                AddXMLElementBool(*processing, "ExportMatlabFile", (*processingActionSets[i] & ProcessingExportMatlabFile) != 0, document);
                AddXMLElementBool(*processing, "ExportAviFile", (*processingActionSets[i] & ProcessingExportAviFile) != 0, document);
            }
        }
    }

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource, const bool* signalModePeriodic,
    const unsigned int* freqMultiplier, const unsigned int* freqDivisor, const unsigned int* freqTolerance,
    const float* nominalFrequency, const bool* negativeEdge, const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* timeBaseElem = document.NewElement("External_Time_Base");
    generalElem->InsertEndChild(timeBaseElem);

    AddXMLElementBool(*timeBaseElem, "Enabled", enabled, document);

    if (signalSource)
    {
        tinyxml2::XMLElement* signalSourceElem = document.NewElement("Signal_Source");
        switch (*signalSource)
        {
        case SourceControlPort:
            signalSourceElem->SetText("Control port");
            break;
        case SourceIRReceiver:
            signalSourceElem->SetText("IR receiver");
            break;
        case SourceSMPTE:
            signalSourceElem->SetText("SMPTE");
            break;
        case SourceVideoSync:
            signalSourceElem->SetText("Video sync");
            break;
        case SourceIRIG:
            signalSourceElem->SetText("IRIG");
            break;
        }
        timeBaseElem->InsertEndChild(signalSourceElem);
    }

    AddXMLElementBool(*timeBaseElem, "Signal_Mode", signalModePeriodic, document, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(*timeBaseElem, "Frequency_Multiplier", freqMultiplier, document);
    AddXMLElementUnsignedInt(*timeBaseElem, "Frequency_Divisor", freqDivisor, document);
    AddXMLElementUnsignedInt(*timeBaseElem, "Frequency_Tolerance", freqTolerance, document);

    if (nominalFrequency)
    {
        if (*nominalFrequency < 0)
        {
            tinyxml2::XMLElement* nominalFreqElem = document.NewElement("Nominal_Frequency");
            nominalFreqElem->SetText("None");
            timeBaseElem->InsertEndChild(nominalFreqElem);
        }
        else
        {
            AddXMLElementFloat(*timeBaseElem, "Nominal_Frequency", nominalFrequency, 3, document);
        }
    }

    AddXMLElementBool(*timeBaseElem, "Signal_Edge", negativeEdge, document, "Negative", "Positive");
    AddXMLElementUnsignedInt(*timeBaseElem, "Signal_Shutter_Delay", signalShutterDelay, document);
    AddXMLElementFloat(*timeBaseElem, "Non_Periodic_Timeout", nonPeriodicTimeout, 3, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* timeStampElem = document.NewElement("External_Timestamp");
    generalElem->InsertEndChild(timeStampElem);

    AddXMLElementBool(*timeStampElem, "Enabled", timestampSettings.bEnabled, document);

    tinyxml2::XMLElement* typeElem = document.NewElement("Type");
    switch (timestampSettings.nType)
    {
    case ETimestampType::Timestamp_SMPTE:
        typeElem->SetText("SMPTE");
        break;
    case ETimestampType::Timestamp_IRIG:
        typeElem->SetText("IRIG");
        break;
    case ETimestampType::Timestamp_CameraTime:
        typeElem->SetText("CameraTime");
        break;
    default:
        break;
    }
    timeStampElem->InsertEndChild(typeElem);

    AddXMLElementUnsignedInt(*timeStampElem, "Frequency", timestampSettings.nFrequency, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetCameraSettings(
    const unsigned int cameraId, const ECameraMode* mode,
    const float* markerExposure, const float* markerThreshold,
    const int* orientation)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    if (mode)
    {
        tinyxml2::XMLElement* modeElem = document.NewElement("Mode");
        switch (*mode)
        {
        case ModeMarker:
            modeElem->SetText("Marker");
            break;
        case ModeMarkerIntensity:
            modeElem->SetText("Marker Intensity");
            break;
        case ModeVideo:
            modeElem->SetText("Video");
            break;
        }
        cameraElem->InsertEndChild(modeElem);
    }

    AddXMLElementFloat(*cameraElem, "Marker_Exposure", markerExposure, 6, document);
    AddXMLElementFloat(*cameraElem, "Marker_Threshold", markerThreshold, 6, document);
    AddXMLElementInt(*cameraElem, "Orientation", orientation, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetCameraVideoSettings(const unsigned int cameraId, const EVideoResolution* videoResolution,
    const EVideoAspectRatio* videoAspectRatio, const unsigned int* videoFrequency, const float* videoExposure, const float* videoFlashTime)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    if (videoResolution)
    {
        tinyxml2::XMLElement* resolutionElem = document.NewElement("Video_Resolution");
        switch (*videoResolution)
        {
        case VideoResolution1440p:
            resolutionElem->SetText("1440p");
            break;
        case VideoResolution1080p:
            resolutionElem->SetText("1080p");
            break;
        case VideoResolution720p:
            resolutionElem->SetText("720p");
            break;
        case VideoResolution540p:
            resolutionElem->SetText("540p");
            break;
        case VideoResolution480p:
            resolutionElem->SetText("480p");
            break;
        case VideoResolutionNone:
            break;
        }
        cameraElem->InsertEndChild(resolutionElem);
    }

    if (videoAspectRatio)
    {
        tinyxml2::XMLElement* videoAspectRatioElem = document.NewElement("Video_Aspect_Ratio");
        switch (*videoAspectRatio)
        {
        case VideoAspectRatio16x9:
            videoAspectRatioElem->SetText("16x9");
            break;
        case VideoAspectRatio4x3:
            videoAspectRatioElem->SetText("4x3");
            break;
        case VideoAspectRatio1x1:
            videoAspectRatioElem->SetText("1x1");
            break;
        case VideoAspectRatioNone:
            break;
        }
        cameraElem->InsertEndChild(videoAspectRatioElem);
    }

    AddXMLElementUnsignedInt(*cameraElem, "Video_Frequency", videoFrequency, document);
    AddXMLElementFloat(*cameraElem, "Video_Exposure", videoExposure, 6, document);
    AddXMLElementFloat(*cameraElem, "Video_Flash_Time", videoFlashTime, 6, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* syncOutMode, const unsigned int* syncOutValue, const float* syncOutDutyCycle,
    const bool* syncOutNegativePolarity)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    int port = portNumber - 1;
    if (((port == 0 || port == 1) && syncOutMode) || (port == 2))
    {
        tinyxml2::XMLElement* syncOutElem = nullptr;
        if (port == 0)
            syncOutElem = document.NewElement("Sync_Out");
        else if (port == 1)
            syncOutElem = document.NewElement("Sync_Out2");
        else
            syncOutElem = document.NewElement("Sync_Out_MT");

        cameraElem->InsertEndChild(syncOutElem);

        // Add Sync Out Mode
        if (port == 0 || port == 1)
        {
            tinyxml2::XMLElement* modeElem = document.NewElement("Mode");
            switch (*syncOutMode)
            {
            case ModeShutterOut:
                modeElem->SetText("Shutter out");
                break;
            case ModeMultiplier:
                modeElem->SetText("Multiplier");
                break;
            case ModeDivisor:
                modeElem->SetText("Divisor");
                break;
            case ModeIndependentFreq:
                modeElem->SetText("Camera independent");
                break;
            case ModeMeasurementTime:
                modeElem->SetText("Measurement time");
                break;
            case ModeFixed100Hz:
                modeElem->SetText("Continuous 100Hz");
                break;
            case ModeSystemLiveTime:
                modeElem->SetText("System live time");
                break;
            default:
                return "";
            }
            syncOutElem->InsertEndChild(modeElem);

            if (*syncOutMode == ModeMultiplier ||
                *syncOutMode == ModeDivisor ||
                *syncOutMode == ModeIndependentFreq)
            {
                AddXMLElementUnsignedInt(*syncOutElem, "Value", syncOutValue, document);
                AddXMLElementFloat(*syncOutElem, "Duty_Cycle", syncOutDutyCycle, 3, document);
            }
        }

        if (syncOutNegativePolarity && (port == 2 ||
            (syncOutMode && *syncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(*syncOutElem, "Signal_Polarity", syncOutNegativePolarity, document, "Negative", "Positive");
        }
    }

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    tinyxml2::XMLElement* lensControlElem = document.NewElement("LensControl");
    cameraElem->InsertEndChild(lensControlElem);

    AddXMLElementFloatWithTextAttribute(document, *lensControlElem, "Focus", "Value", focus, 6);
    AddXMLElementFloatWithTextAttribute(document, *lensControlElem, "Aperture", "Value", aperture, 6);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    tinyxml2::XMLElement* lensControlElem = document.NewElement("LensControl");
    cameraElem->InsertEndChild(lensControlElem);

    tinyxml2::XMLElement* autoExposureElem = document.NewElement("AutoExposure");
    autoExposureElem->SetAttribute("Enabled", autoExposure ? "true" : "false");

    char compensationStr[32];
    snprintf(compensationStr, sizeof(compensationStr), "%.6f", compensation);
    autoExposureElem->SetAttribute("Compensation", compensationStr);

    lensControlElem->InsertEndChild(autoExposureElem);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    tinyxml2::XMLElement* autoWhiteBalanceElem = document.NewElement("AutoWhiteBalance");
    autoWhiteBalanceElem->SetText(enable ? "true" : "false");
    cameraElem->InsertEndChild(autoWhiteBalanceElem);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetImageSettings(const unsigned int  cameraId, const bool* enable, const CRTPacket::EImageFormat* format,
    const unsigned int* width, const unsigned int* height, const float* leftCrop,
    const float* topCrop, const float* rightCrop, const float* bottomCrop)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* imageElem = document.NewElement("Image");
    rootElem->InsertEndChild(imageElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    imageElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", cameraId, document);

    AddXMLElementBool(*cameraElem, "Enabled", enable, document);

    if (format)
    {
        const char* formatStr = nullptr;
        switch (*format)
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
            tinyxml2::XMLElement* formatElem = document.NewElement("Format");
            formatElem->SetText(formatStr);
            cameraElem->InsertEndChild(formatElem);
        }
    }

    AddXMLElementUnsignedInt(*cameraElem, "Width", width, document);
    AddXMLElementUnsignedInt(*cameraElem, "Height", height, document);
    AddXMLElementFloat(*cameraElem, "Left_Crop", leftCrop, 6, document);
    AddXMLElementFloat(*cameraElem, "Top_Crop", topCrop, 6, document);
    AddXMLElementFloat(*cameraElem, "Right_Crop", rightCrop, 6, document);
    AddXMLElementFloat(*cameraElem, "Bottom_Crop", bottomCrop, 6, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetForceSettings(const unsigned int plateId, const SPoint* corner1, const SPoint* corner2,
    const SPoint* corner3, const SPoint* corner4)
{
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* forceElem = document.NewElement("Force");
    rootElem->InsertEndChild(forceElem);

    tinyxml2::XMLElement* plateElem = document.NewElement("Plate");
    forceElem->InsertEndChild(plateElem);

    if (mMajorVersion > 1 || mMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(*plateElem, "Plate_ID", &plateId, document);
    }
    else
    {
        AddXMLElementUnsignedInt(*plateElem, "Force_Plate_Index", &plateId, document);
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

    addCorner("Corner1", corner1);
    addCorner("Corner2", corner2);
    addCorner("Corner3", corner3);
    addCorner("Corner4", corner4);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings6Dofs)
{
    tinyxml2::XMLDocument document;
    auto* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    auto* the6D = document.NewElement("The_6D");
    rootElem->InsertEndChild(the6D);

    for (const auto& body : settings6Dofs)
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
            sprintf_s(tmpStr, sizeof(tmpStr), "R%u%u", (i / 3) + 1, (i % 3) + 1);
            dataOrientationElem->SetAttribute(tmpStr, std::to_string(body.origin.rotation[i]).c_str());
        }
        dataOrientationElem->SetAttribute("Relative_body", body.origin.relativeBody);
        bodyElem->InsertEndChild(dataOrientationElem);
    }

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string Serializer::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons)
{
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* skeletonsElem = document.NewElement("Skeletons");
    rootElem->InsertEndChild(skeletonsElem);

    for (const auto& skeleton : settingsSkeletons)
    {
        tinyxml2::XMLElement* skeletonElem = document.NewElement("Skeleton");
        skeletonElem->SetAttribute("Name", skeleton.name.c_str());
        skeletonsElem->InsertEndChild(skeletonElem);

        if (mMajorVersion == 1 && mMinorVersion < 22)
        {
            tinyxml2::XMLElement* solverElem = document.NewElement("Solver");
            solverElem->SetText(skeleton.rootSegment.solver.c_str());
            skeletonElem->InsertEndChild(solverElem);
        }

        tinyxml2::XMLElement* scaleElem = document.NewElement("Scale");
        scaleElem->SetText(std::to_string(skeleton.scale).c_str());
        skeletonElem->InsertEndChild(scaleElem);

        tinyxml2::XMLElement* segmentsElem = document.NewElement("Segments");
        skeletonElem->InsertEndChild(segmentsElem);

        std::function<void(const SSettingsSkeletonSegmentHierarchical&, tinyxml2::XMLElement*)> recurseSegments;
        recurseSegments = [&](const SSettingsSkeletonSegmentHierarchical& segment, tinyxml2::XMLElement* parentElem)
            {
                tinyxml2::XMLElement* segmentElem = document.NewElement("Segment");
                segmentElem->SetAttribute("Name", segment.name.c_str());
                parentElem->InsertEndChild(segmentElem);

                if (mMajorVersion > 1 || mMinorVersion > 21)
                {
                    tinyxml2::XMLElement* solverElem = document.NewElement("Solver");
                    solverElem->SetText(segment.solver.c_str());
                    segmentElem->InsertEndChild(solverElem);
                }

                if (!std::isnan(segment.position.x))
                {
                    AddXMLElementTransform(document, *segmentElem, "Transform", segment.position, segment.rotation);
                }

                if (!std::isnan(segment.defaultPosition.x))
                {
                    AddXMLElementTransform(document, *segmentElem, "DefaultTransform", segment.defaultPosition, segment.defaultRotation);
                }

                tinyxml2::XMLElement* dofElem = document.NewElement("DegreesOfFreedom");
                segmentElem->InsertEndChild(dofElem);
                for (const auto& dof : segment.degreesOfFreedom)
                {
                    AddXMLElementDOF(document, *dofElem, SkeletonDofToStringSettings(dof.type), dof);
                }

                tinyxml2::XMLElement* endpointElem = document.NewElement("Endpoint");
                if (!std::isnan(segment.endpoint.x) && !std::isnan(segment.endpoint.y) && !std::isnan(segment.endpoint.z))
                {
                    endpointElem->SetAttribute("X", std::to_string(segment.endpoint.x).c_str());
                    endpointElem->SetAttribute("Y", std::to_string(segment.endpoint.y).c_str());
                    endpointElem->SetAttribute("Z", std::to_string(segment.endpoint.z).c_str());
                }
                segmentElem->InsertEndChild(endpointElem);

                tinyxml2::XMLElement* markersElem = document.NewElement("Markers");
                segmentElem->InsertEndChild(markersElem);
                for (const auto& marker : segment.markers)
                {
                    tinyxml2::XMLElement* markerElem = document.NewElement("Marker");
                    markerElem->SetAttribute("Name", marker.name.c_str());
                    markersElem->InsertEndChild(markerElem);

                    tinyxml2::XMLElement* positionElem = document.NewElement("Position");
                    positionElem->SetAttribute("X", std::to_string(marker.position.x).c_str());
                    positionElem->SetAttribute("Y", std::to_string(marker.position.y).c_str());
                    positionElem->SetAttribute("Z", std::to_string(marker.position.z).c_str());
                    markerElem->InsertEndChild(positionElem);

                    tinyxml2::XMLElement* weightElem = document.NewElement("Weight");
                    weightElem->SetText(std::to_string(marker.weight).c_str());
                    markerElem->InsertEndChild(weightElem);
                }

                tinyxml2::XMLElement* rigidBodiesElem = document.NewElement("RigidBodies");
                segmentElem->InsertEndChild(rigidBodiesElem);
                for (const auto& rigidBody : segment.bodies)
                {
                    tinyxml2::XMLElement* rigidBodyElem = document.NewElement("RigidBody");
                    rigidBodyElem->SetAttribute("Name", rigidBody.name.c_str());
                    rigidBodiesElem->InsertEndChild(rigidBodyElem);

                    tinyxml2::XMLElement* transformElem = document.NewElement("Transform");
                    rigidBodyElem->InsertEndChild(transformElem);

                    tinyxml2::XMLElement* positionElem = document.NewElement("Position");
                    positionElem->SetAttribute("X", std::to_string(rigidBody.position.x).c_str());
                    positionElem->SetAttribute("Y", std::to_string(rigidBody.position.y).c_str());
                    positionElem->SetAttribute("Z", std::to_string(rigidBody.position.z).c_str());
                    transformElem->InsertEndChild(positionElem);

                    tinyxml2::XMLElement* rotationElem = document.NewElement("Rotation");
                    rotationElem->SetAttribute("X", std::to_string(rigidBody.rotation.x).c_str());
                    rotationElem->SetAttribute("Y", std::to_string(rigidBody.rotation.y).c_str());
                    rotationElem->SetAttribute("Z", std::to_string(rigidBody.rotation.z).c_str());
                    rotationElem->SetAttribute("W", std::to_string(rigidBody.rotation.w).c_str());
                    transformElem->InsertEndChild(rotationElem);

                    tinyxml2::XMLElement* weightElem = document.NewElement("Weight");
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
    document.Print(&printer);
    return printer.CStr();
}
