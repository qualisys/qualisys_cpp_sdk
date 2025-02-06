#pragma once

#include "Settings.h"

#include <tinyxml2.h>

namespace qualisys_cpp_sdk
{
    struct DLL_EXPORT CTinyxml2Deserializer : ISettingsDeserializer
    {
        CTinyxml2Deserializer(const char* pData, std::uint32_t pMajorVersion, std::uint32_t pMinorVersion);
        bool DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings) override;
        bool Deserialize3DSettings(SSettings3D& p3dSettings, bool& pDataAvailable) override;
        bool DeserializeAnalogSettings(std::vector<SAnalogDevice>& pAnalogDeviceSettings, bool& pDataAvailable) override;
        bool DeserializeForceSettings(SSettingsForce& pForceSettings, bool& pDataAvailable) override;
        bool DeserializeImageSettings(std::vector<SImageCamera>& pImageSettings, bool& pDataAvailable) override;
        bool Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& p6DOFSettings, SSettingsGeneral&, bool& pDataAvailable)  override;
        bool DeserializeGazeVectorSettings(std::vector<SGazeVector>& pGazeVectorSettings, bool& pDataAvailable) override;
        bool DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& pEyeTrackerSettings, bool& pDataAvailable)  override;
        bool DeserializeSkeletonSettings(bool pSkeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>&, std::vector<SSettingsSkeleton>&, bool& pDataAvailable) override;
        bool DeserializeCalibrationSettings(SCalibration& pCalibrationSettings) override;

    private:
        std::uint32_t mnMajorVersion;
        std::uint32_t mnMinorVersion;
        tinyxml2::XMLDocument oXML;
    };
}
