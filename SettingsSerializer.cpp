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
    return mSerializer->SetGeneralSettings(captureFrequency, captureTime, startOnExtTrig, startOnTrigNO, startOnTrigNC, startOnTrigSoftware, processingActions, rtProcessingActions, reprocessingActions);
}

std::string SettingsSerializer::SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource,
    const bool* signalModePeriodic, const unsigned int* freqMultiplier, const unsigned int* freqDivisor,
    const unsigned int* freqTolerance, const float* nominalFrequency, const bool* negativeEdge,
    const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout)
{
    return mSerializer->SetExtTimeBaseSettings(enabled, signalSource, signalModePeriodic, freqMultiplier, freqDivisor, freqTolerance, nominalFrequency, negativeEdge, signalShutterDelay, nonPeriodicTimeout);
}


std::string SettingsSerializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    return mSerializer->SetExtTimestampSettings(timestampSettings);
}

std::string SettingsSerializer::SetCameraSettings(
    const unsigned int cameraId, const ECameraMode* mode,
    const float* markerExposure, const float* markerThreshold,
    const int* orientation)
{
    return mSerializer->SetCameraSettings(cameraId, mode, markerExposure, markerThreshold, orientation);
}

std::string SettingsSerializer::SetCameraVideoSettings(const unsigned int cameraId,
    const EVideoResolution* videoResolution, const EVideoAspectRatio* videoAspectRatio,
    const unsigned int* videoFrequency, const float* videoExposure, const float* videoFlashTime)
{
    return mSerializer->SetCameraVideoSettings(cameraId, videoResolution, videoAspectRatio, videoFrequency, videoExposure, videoFlashTime);
}

std::string SettingsSerializer::SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* syncOutMode, const unsigned int* syncOutValue, const float* syncOutDutyCycle, const bool* syncOutNegativePolarity)
{
    return mSerializer->SetCameraSyncOutSettings(cameraId, portNumber, syncOutMode, syncOutValue, syncOutDutyCycle, syncOutNegativePolarity);
}

std::string SettingsSerializer::SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture)
{
    return mSerializer->SetCameraLensControlSettings(cameraId, focus, aperture);
}

std::string SettingsSerializer::SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation)
{
    return mSerializer->SetCameraAutoExposureSettings(cameraId, autoExposure, compensation);
}

std::string SettingsSerializer::SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable)
{
    return mSerializer->SetCameraAutoWhiteBalance(cameraId, enable);
}

std::string SettingsSerializer::SetImageSettings(const unsigned int cameraId, const bool* enable,
    const CRTPacket::EImageFormat* format, const unsigned int* width, const unsigned int* height,
    const float* leftCrop, const float* topCrop, const float* rightCrop, const float* bottomCrop)
{
    return mSerializer->SetImageSettings(cameraId, enable, format, width, height, leftCrop, topCrop, rightCrop, bottomCrop);
}

std::string SettingsSerializer::SetForceSettings(const unsigned int plateId, const SPoint* corner1,
    const SPoint* corner2, const SPoint* corner3, const SPoint* corner4)
{
    auto plateElem = mSerializer->Element("QTM_Settings").Element("Force").Element("Plate");

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
    auto the6D = mSerializer->Element("QTM_Settings").Element("The_6D");

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
            sprintf_s(tmpStr, sizeof(tmpStr), "R%u%u", (i / 3) + 1, (i % 3) + 1);
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
