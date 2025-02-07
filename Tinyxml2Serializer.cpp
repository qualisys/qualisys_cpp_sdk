#define _CRT_SECURE_NO_WARNINGS

#include "Tinyxml2Serializer.h"

#include "Serializer.h"
#include "Settings.h"

#include <functional>
#include <stdexcept>

using namespace qualisys_cpp_sdk;

CTinyxml2Serializer::CTinyxml2Serializer(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mSerializer(majorVersion, minorVersion)
{
}

std::string CTinyxml2Serializer::SetGeneralSettings(const unsigned int* captureFrequency,
    const float* captureTime, const bool* startOnExtTrig,
    const bool* startOnTrigNO, const bool* startOnTrigNC,
    const bool* startOnTrigSoftware, const EProcessingActions* processingActions,
    const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    return mSerializer.SetGeneralSettings(captureFrequency, captureTime, startOnExtTrig, startOnTrigNO, startOnTrigNC, startOnTrigSoftware, processingActions, rtProcessingActions, reprocessingActions);
}

std::string CTinyxml2Serializer::SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource,
    const bool* signalModePeriodic, const unsigned int* freqMultiplier, const unsigned int* freqDivisor,
    const unsigned int* freqTolerance, const float* nominalFrequency, const bool* negativeEdge,
    const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout)
{
    return mSerializer.SetExtTimeBaseSettings(enabled, signalSource, signalModePeriodic, freqMultiplier, freqDivisor, freqTolerance, nominalFrequency, negativeEdge, signalShutterDelay, nonPeriodicTimeout);
}


std::string CTinyxml2Serializer::SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings)
{
    return mSerializer.SetExtTimestampSettings(timestampSettings);
}

std::string CTinyxml2Serializer::SetCameraSettings(
    const unsigned int cameraId, const ECameraMode* mode,
    const float* markerExposure, const float* markerThreshold,
    const int* orientation)
{
    return mSerializer.SetCameraSettings(cameraId, mode, markerExposure, markerThreshold, orientation);
}

std::string CTinyxml2Serializer::SetCameraVideoSettings(const unsigned int cameraId,
    const EVideoResolution* videoResolution, const EVideoAspectRatio* videoAspectRatio,
    const unsigned int* videoFrequency, const float* videoExposure, const float* videoFlashTime)
{
    return mSerializer.SetCameraVideoSettings(cameraId, videoResolution, videoAspectRatio, videoFrequency, videoExposure, videoFlashTime);
}

std::string CTinyxml2Serializer::SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* syncOutMode, const unsigned int* syncOutValue, const float* syncOutDutyCycle, const bool* syncOutNegativePolarity)
{
    return mSerializer.SetCameraSyncOutSettings(cameraId, portNumber, syncOutMode, syncOutValue, syncOutDutyCycle, syncOutNegativePolarity);
}

std::string CTinyxml2Serializer::SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture)
{
    return mSerializer.SetCameraLensControlSettings(cameraId, focus, aperture);
}

std::string CTinyxml2Serializer::SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation)
{
    return mSerializer.SetCameraAutoExposureSettings(cameraId, autoExposure, compensation);
}

std::string CTinyxml2Serializer::SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable)
{
    return mSerializer.SetCameraAutoWhiteBalance(cameraId, enable);
}

std::string CTinyxml2Serializer::SetImageSettings(const unsigned int cameraId, const bool* enable,
    const CRTPacket::EImageFormat* format, const unsigned int* width, const unsigned int* height,
    const float* leftCrop, const float* topCrop, const float* rightCrop, const float* bottomCrop)
{
    return mSerializer.SetImageSettings(cameraId, enable, format, width, height, leftCrop, topCrop, rightCrop, bottomCrop);
}

std::string CTinyxml2Serializer::SetForceSettings(const unsigned int plateId, const SPoint* corner1,
    const SPoint* corner2, const SPoint* corner3, const SPoint* corner4)
{
    return mSerializer.SetForceSettings(plateId, corner1, corner2, corner3, corner4);
}

std::string CTinyxml2Serializer::Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings6Dofs)
{
    return mSerializer.Set6DOFBodySettings(settings6Dofs);
}

std::string CTinyxml2Serializer::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons)
{
    return mSerializer.SetSkeletonSettings(settingsSkeletons);
}
