#pragma once

#include <memory>

#include "Settings.h"

namespace qualisys_cpp_sdk
{
    struct Deserializer;

    struct SettingsDeserializer
    {
        SettingsDeserializer(const char* data, std::uint32_t majorVersion, std::uint32_t minorVersion);

        bool DeserializeGeneralSettings(SSettingsGeneral& generalSettings);
        bool Deserialize3DSettings(SSettings3D& settings3D, bool& dataAvailable);
        bool DeserializeAnalogSettings(std::vector<SAnalogDevice>& analogDeviceSettings, bool& dataAvailable);
        bool DeserializeForceSettings(SSettingsForce& forceSettings, bool& dataAvailable);
        bool DeserializeImageSettings(std::vector<SImageCamera>& imageSettings, bool& dataAvailable);
        bool Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& settings6Dof, SSettingsGeneral& generalSettings,
                                     bool& dataAvailable);
        bool DeserializeGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings, bool& dataAvailable);
        bool DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings, bool& dataAvailable);
        bool DeserializeSkeletonSettings(bool skeletonGlobalData,
                                         std::vector<SSettingsSkeletonHierarchical>& skeletonSettingsHierarchical,
                                         std::vector<SSettingsSkeleton>& skeletonSettings, bool& dataAvailable);
        bool DeserializeCalibrationSettings(SCalibration& calibrationSettings);

    private:
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;
        std::shared_ptr<Deserializer> mDeserializer;
    };
}
