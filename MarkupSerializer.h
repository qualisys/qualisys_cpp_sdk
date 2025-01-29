#pragma once

#include "Settings.h"
#include "Markup.h"

namespace CRTProtocolNs {

    struct DLL_EXPORT CMarkupDeserializer : ISettingsDeserializer {
        CMarkupDeserializer(const char* pData, std::uint32_t pMajorVersion, std::uint32_t pMinorVersion );
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
        char maErrorStr[1024];
        CMarkup oXML;
        bool CompareNoCase(std::string tStr1, const char* tStr2) const;
        static std::string ToLower(std::string str);
        static bool ParseString(const std::string& str, std::uint32_t& value);
        static bool ParseString(const std::string& str, std::int32_t& value);
        static bool ParseString(const std::string& str, float& value);
        static bool ParseString(const std::string& str, double& value);
        static bool ParseString(const std::string& str, bool& value);
        bool ReadXmlBool(CMarkup* xml, const std::string& element, bool& value) const;
        SPosition ReadXMLPosition(CMarkup& xml, const std::string& element);
        SRotation ReadXMLRotation(CMarkup& xml, const std::string& element);
        bool ReadXMLDegreesOfFreedom(CMarkup& xml, const std::string& element, std::vector<SDegreeOfFreedom>& degreesOfFreedom);
        
        SPosition DeserializeXMLPosition(CMarkup& xml, const std::string& element);
        SRotation DeserializeXMLRotation(CMarkup& xml, const std::string& element);
    };

    struct DLL_EXPORT CMarkupSerializer : public ISettingsSerializer {
        CMarkupSerializer(std::uint32_t pMajorVersion, std::uint32_t pMinorVersion);
        CMarkup oXML;
        std::string SetGeneralSettings(const unsigned int* pnCaptureFrequency, const float* pfCaptureTime,
            const bool* pbStartOnExtTrig, const bool* pStartOnTrigNO, const bool* pStartOnTrigNC, const bool* pStartOnTrigSoftware,
            const EProcessingActions* peProcessingActions, const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions) override;

        std::string SetExtTimeBaseSettings(
            const bool* pbEnabled, const ESignalSource* peSignalSource,
            const bool* pbSignalModePeriodic, const unsigned int* pnFreqMultiplier,
            const unsigned int* pnFreqDivisor, const unsigned int* pnFreqTolerance,
            const float* pfNominalFrequency, const bool* pbNegativeEdge,
            const unsigned int* pnSignalShutterDelay, const float* pfNonPeriodicTimeout) override;

        std::string SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings) override;

        std::string SetCameraSettings(
            const unsigned int pCameraId, const ECameraMode* peMode,
            const float* pfMarkerExposure, const float* pfMarkerThreshold,
            const int* pnOrientation) override;

        std::string SetCameraVideoSettings(
            const unsigned int pCameraId, const EVideoResolution* eVideoResolution,
            const EVideoAspectRatio* eVideoAspectRatio, const unsigned int* pnVideoFrequency,
            const float* pfVideoExposure, const float* pfVideoFlashTime) override;

        std::string SetCameraSyncOutSettings(
            const unsigned int  pCameraId, const unsigned int portNumber, const ESyncOutFreqMode* peSyncOutMode,
            const unsigned int* pnSyncOutValue, const float* pfSyncOutDutyCycle,
            const bool* pbSyncOutNegativePolarity) override;

        std::string SetCameraLensControlSettings(const unsigned int pCameraId, const float focus, const float aperture) override;
        std::string SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool autoExposure, const float compensation) override;
        std::string SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool enable) override;
        std::string SetImageSettings(
            const unsigned int  pCameraId, const bool* pbEnable, const CRTPacket::EImageFormat* peFormat,
            const unsigned int* pnWidth, const unsigned int* pnHeight, const float* pfLeftCrop,
            const float* pfTopCrop, const float* pfRightCrop, const float* pfBottomCrop) override;

        std::string SetForceSettings(
            const unsigned int pPlateId, const SPoint* pCorner1, const SPoint* pCorner2,
            const SPoint* pCorner3, const SPoint* pCorner4) override;

        std::string Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs) override;
        std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& pSettingsSkeletons) override;

    private:
        std::uint32_t mnMajorVersion;
        std::uint32_t mnMinorVersion;
        void AddXMLElementBool(CMarkup* oXML, const char* tTag, const bool* pbValue, const char* tTrue = "True", const char* tFalse = "False");
        void AddXMLElementBool(CMarkup* oXML, const char* tTag, const bool bValue, const char* tTrue = "True", const char* tFalse = "False");
        void AddXMLElementInt(CMarkup* oXML, const char* tTag, const int* pnValue);
        void AddXMLElementUnsignedInt(CMarkup* oXML, const char* tTag, const unsigned int value);
        void AddXMLElementUnsignedInt(CMarkup* oXML, const char* tTag, const unsigned int* pnValue);
        void AddXMLElementFloat(CMarkup* oXML, const char* tTag, const float* pfValue, unsigned int pnDecimals = 6);
        void AddXMLElementTransform(CMarkup& xml, const std::string& name, const SPosition& position, const SRotation& rotation);
        void AddXMLElementDOF(CMarkup& xml, const std::string& name, SDegreeOfFreedom degreeOfFreedom);
    };
}



