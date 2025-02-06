#pragma once

#include "Settings.h"

#include <tinyxml2.h>

namespace qualisys_cpp_sdk
{
    struct DLL_EXPORT CTinyxml2Deserializer : ISettingsDeserializer
    {
        CTinyxml2Deserializer(const char* data, std::uint32_t majorVersion, std::uint32_t minorVersion);
        bool DeserializeGeneralSettings(SSettingsGeneral& generalSettings) override;
        bool Deserialize3DSettings(SSettings3D& settings3D, bool& dataAvailable) override;
        bool DeserializeAnalogSettings(std::vector<SAnalogDevice>& analogDeviceSettings, bool& dataAvailable) override;
        bool DeserializeForceSettings(SSettingsForce& forceSettings, bool& dataAvailable) override;
        bool DeserializeImageSettings(std::vector<SImageCamera>& imageSettings, bool& dataAvailable) override;
        bool Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& settings6Dof, SSettingsGeneral&, bool& dataAvailable)  override;
        bool DeserializeGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings, bool& dataAvailable) override;
        bool DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings, bool& dataAvailable)  override;
        bool DeserializeSkeletonSettings(bool skeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>&, std::vector<SSettingsSkeleton>&, bool& dataAvailable) override;
        bool DeserializeCalibrationSettings(SCalibration& calibrationSettings) override;

    private:
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;
        tinyxml2::XMLDocument mDocument;
    };
}
