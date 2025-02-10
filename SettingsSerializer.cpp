#include "SettingsSerializer.h"
#include "SerializerApi.h"
#include "Settings.h"

#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

SettingsSerializer::SettingsSerializer(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mSerializer(new SerializerApi(majorVersion, minorVersion))
{
}

SettingsSerializer::~SettingsSerializer()
{
    delete mSerializer;
}

std::string SettingsSerializer::SetGeneralSettings(const unsigned int* captureFrequency,
                                           const float* captureTime, const bool* startOnExtTrig,
                                           const bool* startOnTrigNO, const bool* startOnTrigNC,
                                           const bool* startOnTrigSoftware, const EProcessingActions* processingActions,
                                           const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    auto theGeneral = mSerializer
        ->Element("QTM_Settings")
        .Element("General");

    if (captureFrequency)
    {
        theGeneral.ElementUnsignedInt("Frequency", *captureFrequency);
    }

    if (captureTime)
    {
        theGeneral.ElementFloat("Capture_Time", *captureTime, 3);
    }

    // External Trigger and additional triggers
    if (startOnExtTrig)
    {
        theGeneral.ElementBool("Start_On_External_Trigger", startOnExtTrig);

        if (mMajorVersion > 1 || mMinorVersion > 14)
        {
            theGeneral.ElementBool("Start_On_Trigger_NO", startOnTrigNO);
            theGeneral.ElementBool("Start_On_Trigger_NC", startOnTrigNC);
            theGeneral.ElementBool("Start_On_Trigger_Software", startOnTrigSoftware);
        }
    }

    const char* processingActionTags[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActionSets[3] = { processingActions, rtProcessingActions, reprocessingActions };

    auto actionsCount = (mMajorVersion > 1 || mMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActionSets[i])
        {
            auto processingActionsElem = theGeneral.Element(processingActionTags[i]);

            if (mMajorVersion > 1 || mMinorVersion > 13)
            {
                processingActionsElem.ElementBool("PreProcessing2D", (*processingActionSets[i] & ProcessingPreProcess2D) != 0);
            }

            if (*processingActionSets[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                processingActionsElem.ElementString("Tracking", "2D");
            }
            else if (*processingActionSets[i] & ProcessingTracking3D)
            {
                processingActionsElem.ElementString("Tracking", "3D");
            }
            else
            {
                processingActionsElem.ElementString("Tracking", "false");
            }

            if (i != 1) // Not RtProcessingSettings
            {
                processingActionsElem.ElementBool("TwinSystemMerge", (*processingActionSets[i] & ProcessingTwinSystemMerge) != 0);
                processingActionsElem.ElementBool("SplineFill", (*processingActionSets[i] & ProcessingSplineFill) != 0);
            }

            processingActionsElem.ElementBool("AIM", (*processingActionSets[i] & ProcessingAIM) != 0);
            processingActionsElem.ElementBool("Track6DOF", (*processingActionSets[i] & Processing6DOFTracking) != 0);
            processingActionsElem.ElementBool("ForceData", (*processingActionSets[i] & ProcessingForceData) != 0);
            processingActionsElem.ElementBool("GazeVector", (*processingActionSets[i] & ProcessingGazeVector) != 0);

            if (i != 1) // Not RtProcessingSettings
            {
                processingActionsElem.ElementBool("ExportTSV", (*processingActionSets[i] & ProcessingExportTSV) != 0);
                processingActionsElem.ElementBool("ExportC3D", (*processingActionSets[i] & ProcessingExportC3D) != 0);
                processingActionsElem.ElementBool("ExportMatlabFile", (*processingActionSets[i] & ProcessingExportMatlabFile) != 0);
                processingActionsElem.ElementBool("ExportAviFile", (*processingActionSets[i] & ProcessingExportAviFile) != 0);
            }
        }
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource,
    const bool* signalModePeriodic, const unsigned int* freqMultiplier, const unsigned int* freqDivisor,
    const unsigned int* freqTolerance, const float* nominalFrequency, const bool* negativeEdge,
    const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout)
{
    auto timeBaseElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("External_Time_Base");

    timeBaseElem.ElementBool("Enabled", enabled);

    if (signalSource)
    {
        switch (*signalSource)
        {
        case SourceControlPort:
            timeBaseElem.ElementString("Signal_Source", "Control_port");
            break;
        case SourceIRReceiver:
            timeBaseElem.ElementString("Signal_Source", "IR receiver");
            break;
        case SourceSMPTE:
            timeBaseElem.ElementString("Signal_Source", "SMPTE");
            break;
        case SourceVideoSync:
            timeBaseElem.ElementString("Signal_Source", "Video sync");
            break;
        case SourceIRIG:
            timeBaseElem.ElementString("Signal_Source", "IRIG");
            break;
        }
    }

    timeBaseElem.ElementString("Signal_Mode", (*signalModePeriodic ? "Periodic" : "Non-periodic"));

    if (freqMultiplier)
    {
        timeBaseElem.ElementUnsignedInt("Frequency_Multiplier", *freqMultiplier);
    }
    if (freqDivisor)
    {
        timeBaseElem.ElementUnsignedInt("Frequency_Divisor", *freqDivisor);
    }
    if (freqTolerance)
    {
        timeBaseElem.ElementUnsignedInt("Frequency_Tolerance", *freqTolerance);
    }

    if (nominalFrequency)
    {
        if (*nominalFrequency < 0)
        {
            timeBaseElem.ElementString("Nominal_Frequency", "None");
        }
        else
        {
            timeBaseElem.ElementFloat("Nominal_Frequency", *nominalFrequency, 3);
        }
    }

    timeBaseElem.ElementString("Signal_Edge", (*negativeEdge ? "Negative" : "Positive"));
    timeBaseElem.ElementUnsignedInt("Signal_Shutter_Delay", *signalShutterDelay);
    timeBaseElem.ElementFloat("Non_Periodic_Timeout", *nonPeriodicTimeout, 3);

    return timeBaseElem.ToString();
}

std::string SettingsSerializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    auto timeStampElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("External_Timestamp");

    timeStampElem.ElementBool("Enabled", timestampSettings.bEnabled);

    switch (timestampSettings.nType)
    {
    case ETimestampType::Timestamp_SMPTE:
        timeStampElem.ElementString("Type", "SMPTE");
        break;
    case ETimestampType::Timestamp_IRIG:
        timeStampElem.ElementString("Type", "IRIG");
        break;
    case ETimestampType::Timestamp_CameraTime:
        timeStampElem.ElementString("Type", "CameraTime");
        break;
    default:
        break;
    }

    timeStampElem.ElementUnsignedInt("Frequency", timestampSettings.nFrequency);

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetCameraSettings(
    const unsigned int cameraId, const ECameraMode* mode,
    const float* markerExposure, const float* markerThreshold,
    const int* orientation)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);

    if (mode)
    {
        switch (*mode)
        {
        case ModeMarker:
            cameraElem.ElementString("Mode", "Marker");
            break;
        case ModeMarkerIntensity:
            cameraElem.ElementString("Mode", "Marker Intensity");
            break;
        case ModeVideo:
            cameraElem.ElementString("Mode", "Video");
            break;
        }
    }

    if (markerExposure)
    {
        cameraElem.ElementFloat("Marker_Exposure", *markerExposure, 6);
    }
    if (markerThreshold)
    {
        cameraElem.ElementFloat("Marker_Threshold", *markerThreshold, 6);
    }
    if (orientation)
    {
        cameraElem.ElementInt("Orientation", *orientation);
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetCameraVideoSettings(const unsigned int cameraId,
    const EVideoResolution* videoResolution, const EVideoAspectRatio* videoAspectRatio,
    const unsigned int* videoFrequency, const float* videoExposure, const float* videoFlashTime)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);

    if (videoResolution)
    {
        switch (*videoResolution)
        {
        case VideoResolution1440p:
            cameraElem.ElementString("Video_Resolution", "1440p");
            break;
        case VideoResolution1080p:
            cameraElem.ElementString("Video_Resolution", "1080p");
            break;
        case VideoResolution720p:
            cameraElem.ElementString("Video_Resolution", "720p");
            break;
        case VideoResolution540p:
            cameraElem.ElementString("Video_Resolution", "540p");
            break;
        case VideoResolution480p:
            cameraElem.ElementString("Video_Resolution", "480p");
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
            cameraElem.ElementString("Video_Aspect_Ratio", "16x9");
            break;
        case VideoAspectRatio4x3:
            cameraElem.ElementString("Video_Aspect_Ratio", "4x3");
            break;
        case VideoAspectRatio1x1:
            cameraElem.ElementString("Video_Aspect_Ratio", "1x1");
            break;
        case VideoAspectRatioNone:
            break;
        }
    }

    if (videoFrequency)
    {
        cameraElem.ElementUnsignedInt("Video_Frequency", *videoFrequency);
    }
    if (videoExposure)
    {
        cameraElem.ElementFloat("Video_Exposure", *videoExposure, 6);
    }
    if (videoFlashTime)
    {
        cameraElem.ElementFloat("Video_Flash_Time", *videoFlashTime, 6);
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* syncOutMode, const unsigned int* syncOutValue, const float* syncOutDutyCycle, const bool* syncOutNegativePolarity)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);

    int port = static_cast<int>(portNumber) - 1;
    if (((port == 0 || port == 1) && syncOutMode) || (port == 2))
    {
        auto syncOutElem = [&port, &cameraElem](){
        if (port == 0)
            return cameraElem.Element("Sync_Out");
        else if (port == 1)
            return cameraElem.Element("Sync_Out2");
        else
            return cameraElem.Element("Sync_Out_MT");
            }();

        // Add Sync Out Mode
        if (port == 0 || port == 1)
        {
            switch (*syncOutMode)
            {
            case ModeShutterOut:
                syncOutElem.ElementString("Mode", "Shutter out");
                break;
            case ModeMultiplier:
                syncOutElem.ElementString("Mode", "Multiplier");
                break;
            case ModeDivisor:
                syncOutElem.ElementString("Mode", "Divisor");
                break;
            case ModeIndependentFreq:
                syncOutElem.ElementString("Mode", "Camera independent");
                break;
            case ModeMeasurementTime:
                syncOutElem.ElementString("Mode", "Measurement time");
                break;
            case ModeFixed100Hz:
                syncOutElem.ElementString("Mode", "Continuous 100Hz");
                break;
            case ModeSystemLiveTime:
                syncOutElem.ElementString("Mode", "System live time");
                break;
            default:
                return "";
            }

            if (*syncOutMode == ModeMultiplier ||
                *syncOutMode == ModeDivisor ||
                *syncOutMode == ModeIndependentFreq)
            {
                if (syncOutValue)
                {
                    syncOutElem.ElementUnsignedInt("Value", *syncOutValue);
                }
                if (syncOutDutyCycle)
                {
                    syncOutElem.ElementFloat("Duty_Cycle", *syncOutDutyCycle, 3);
                }
            }
        }

        if (syncOutNegativePolarity && (port == 2 ||
            (syncOutMode && *syncOutMode != ModeFixed100Hz)))
        {
            syncOutElem.ElementString("Signal_Polarity", (syncOutNegativePolarity ? "Negative" : "Positive"));
        }
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);

    auto lensControlElem = cameraElem.Element("LensControl");

    lensControlElem.Element("Focus")
        .AttributeFloat("Value", focus, 6);

    lensControlElem.Element("Aperture")
        .AttributeFloat("Value", aperture, 6);

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);

    auto lensControlElem = cameraElem.Element("LensControl");

    char compensationStr[32];
    (void)snprintf(compensationStr, sizeof(compensationStr), "%.6f", compensation);

    lensControlElem.Element("AutoExposure")
        .AttributeString("Enabled", (autoExposure ? "true" : "false"))
        .AttributeString("Compensation", compensationStr);

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("General")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);
    cameraElem.ElementString("AutoWhiteBalance", (enable ? "true" : "false"));

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetImageSettings(const unsigned int cameraId, const bool* enable,
    const CRTPacket::EImageFormat* format, const unsigned int* width, const unsigned int* height,
    const float* leftCrop, const float* topCrop, const float* rightCrop, const float* bottomCrop)
{
    auto cameraElem = mSerializer
        ->Element("QTM_Settings")
        .Element("Image")
        .Element("Camera");

    cameraElem.ElementUnsignedInt("ID", cameraId);
    if (enable)
    {
        cameraElem.ElementBool("Enabled", *enable);
    }

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
            cameraElem.ElementString("Format", formatStr);
        }
    }
    
    cameraElem.ElementUnsignedInt("Width", *width);
    cameraElem.ElementUnsignedInt("Height", *height);
    if (leftCrop)
    {
        cameraElem.ElementFloat("Left_Crop", *leftCrop, 6);
    }
    if (topCrop)
    {
        cameraElem.ElementFloat("Top_Crop", *topCrop, 6);
    }
    if (rightCrop)
    {
        cameraElem.ElementFloat("Right_Crop", *rightCrop, 6);
    }
    if (bottomCrop)
    {
        cameraElem.ElementFloat("Bottom_Crop", *bottomCrop, 6);
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetForceSettings(const unsigned int plateId, const SPoint* corner1,
    const SPoint* corner2, const SPoint* corner3, const SPoint* corner4)
{
    auto plateElem = mSerializer
        ->Element("QTM_Settings")
        .Element("Force")
        .Element("Plate");

    if (mMajorVersion > 1 || mMinorVersion > 7)
    {
        plateElem.ElementUnsignedInt("Plate_ID", plateId);
    }
    else
    {
        plateElem.ElementUnsignedInt("Force_Plate_Index", plateId);
    }

    auto addCorner = [&](const char* name, const SPoint pCorner, SerializerApi elem)
        {
            auto cornerElem = elem.Element(name);
            cornerElem.ElementFloat("X", pCorner.fX, 6);
            cornerElem.ElementFloat("Y", pCorner.fY, 6);
            cornerElem.ElementFloat("Z", pCorner.fZ, 6);
        };

    if (corner1)
    {
        addCorner("Corner1", *corner1, plateElem);
    }

    if (corner2)
    {
        addCorner("Corner2", *corner2, plateElem);
    }

    if (corner3)
    {
        addCorner("Corner3", *corner3, plateElem);
    }

    if (corner4)
    {
        addCorner("Corner4", *corner4, plateElem);
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings6Dofs)
{
    auto the6D = mSerializer
        ->Element("QTM_Settings")
        .Element("The_6D");

    for (const auto& body : settings6Dofs)
    {
        auto bodyElem = the6D.Element("Body");

        bodyElem.ElementString("Name", body.name.c_str());
        bodyElem.ElementBool("Enabled", body.enabled);

        auto colorElem = bodyElem.Element("Color");
        colorElem.AttributeUnsignedInt("R", body.color & 0xff);
        colorElem.AttributeUnsignedInt("G", (body.color >> 8) & 0xff);
        colorElem.AttributeUnsignedInt("B", (body.color >> 16) & 0xff);

        bodyElem.ElementFloat("MaximumResidual", body.maxResidual, 6);
        bodyElem.ElementUnsignedInt("MinimumMarkersInBody", body.minMarkersInBody);
        bodyElem.ElementFloat("BoneLengthTolerance", body.boneLengthTolerance, 6);
        bodyElem.Element("Filter").AttributeString("Preset", body.filterPreset.c_str());

        if (!body.mesh.name.empty())
        {
            auto meshElem = bodyElem.Element("Mesh");
            meshElem.ElementString("Name", body.mesh.name.c_str());

            meshElem.Element("Position")
                .AttributeFloat("X",body.mesh.position.fX,6)
                .AttributeFloat("Y", body.mesh.position.fY, 6)
                .AttributeFloat("Z", body.mesh.position.fZ, 6);

            meshElem.Element("Rotation")
                .AttributeFloat("X", body.mesh.rotation.fX, 6)
                .AttributeFloat("Y", body.mesh.rotation.fY, 6)
                .AttributeFloat("Z", body.mesh.rotation.fZ, 6);

            meshElem.ElementFloat("Scale", body.mesh.scale, 6);
            meshElem.ElementFloat("Opacity", body.mesh.opacity, 6);
        }

        if (!body.points.empty())
        {
            auto pointsElem = bodyElem.Element("Points");
            for (const auto& point : body.points)
            {
                pointsElem.Element("Point")
                    .AttributeFloat("X", point.fX, 6)
                    .AttributeFloat("Y", point.fY, 6)
                    .AttributeFloat("Z", point.fZ, 6)
                    .AttributeUnsignedInt("Virtual", point.virtual_ ? 1 : 0)
                    .AttributeUnsignedInt("PhysicalId", point.physicalId)
                    .AttributeString("Name", point.name.c_str());
            }
        }

        bodyElem.ElementUnsignedInt("Data_origin", body.origin.type)
            .AttributeFloat("X", body.origin.position.fX, 6)
            .AttributeFloat("Y", body.origin.position.fY, 6)
            .AttributeFloat("Z", body.origin.position.fZ, 6)
            .AttributeUnsignedInt("Relative_body", body.origin.relativeBody);

        auto orientationElem = bodyElem.ElementUnsignedInt("Data_orientation", body.origin.type);
        for (std::uint32_t i = 0; i < 9; i++)
        {
            char tmpStr[16];
            (void)sprintf_s(tmpStr, sizeof(tmpStr), "R%u%u", (i / 3) + 1, (i % 3) + 1);
            orientationElem.AttributeFloat(tmpStr, body.origin.rotation[i], 6);
        }
        orientationElem.AttributeUnsignedInt("Relative_body", body.origin.relativeBody);
    }

    return mSerializer->ToString();
}

std::string SettingsSerializer::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons)
{
    return mSerializer->SetSkeletonSettings(settingsSkeletons);
}
