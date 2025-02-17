#pragma once

#include "Settings.h"

namespace qualisys_cpp_sdk
{
    struct SettingsSerializer
    {
        SettingsSerializer(std::uint32_t majorVersion, std::uint32_t minorVersion);

        std::string SetGeneralSettings(const unsigned int* captureFrequency, const float* captureTime,
                                       const bool* startOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC,
                                       const bool* startOnTrigSoftware,
                                       const EProcessingActions* processingActions,
                                       const EProcessingActions* rtProcessingActions,
                                       const EProcessingActions* reprocessingActions);

        std::string SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource,
                                           const bool* signalModePeriodic, const unsigned int* freqMultiplier,
                                           const unsigned int* freqDivisor, const unsigned int* freqTolerance,
                                           const float* nominalFrequency, const bool* negativeEdge,
                                           const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout);

        std::string SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings);

        std::string SetCameraSettings(const unsigned int cameraId, const ECameraMode* mode,
                                      const float* markerExposure, const float* markerThreshold,
                                      const int* orientation);

        std::string SetCameraVideoSettings(const unsigned int cameraId, const EVideoResolution* videoResolution,
                                           const EVideoAspectRatio* videoAspectRatio,
                                           const unsigned int* videoFrequency,
                                           const float* videoExposure, const float* videoFlashTime);

        std::string SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber,
                                             const ESyncOutFreqMode* syncOutMode,
                                             const unsigned int* syncOutValue, const float* syncOutDutyCycle,
                                             const bool* syncOutNegativePolarity);

        std::string SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture);

        std::string SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure,
                                                  const float compensation);

        std::string SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable);

        std::string SetImageSettings(const unsigned int cameraId, const bool* enable,
                                     const CRTPacket::EImageFormat* format,
                                     const unsigned int* width, const unsigned int* height, const float* leftCrop,
                                     const float* topCrop, const float* rightCrop, const float* bottomCrop);

        std::string SetForceSettings(const unsigned int plateId, const SPoint* corner1, const SPoint* corner2,
                                     const SPoint* corner3, const SPoint* corner4);

        std::string Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings6Dofs);

        std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons);

    private:
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;
    };
}
