#pragma once

#include "Settings.h"
#include "Markup.h"

namespace qualisys_cpp_sdk {

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
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;
        char mErrorStr[1024];
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
        std::string SetGeneralSettings(const unsigned int* captureFrequency, const float* captureTime,
            const bool* startOnExtTrig, const bool* pStartOnTrigNO, const bool* pStartOnTrigNC, const bool* pStartOnTrigSoftware,
            const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions) override;

        std::string SetExtTimeBaseSettings(
            const bool* enabled, const ESignalSource* signalSource,
            const bool* signalModePeriodic, const unsigned int* freqMultiplier,
            const unsigned int* freqDivisor, const unsigned int* freqTolerance,
            const float* nominalFrequency, const bool* negativeEdge,
            const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout) override;

        std::string SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings) override;

        std::string SetCameraSettings(
            const unsigned int pCameraId, const ECameraMode* mode,
            const float* markerExposure, const float* markerThreshold,
            const int* orientation) override;

        std::string SetCameraVideoSettings(
            const unsigned int pCameraId, const EVideoResolution* videoResolution,
            const EVideoAspectRatio* videoAspectRatio, const unsigned int* videoFrequency,
            const float* videoExposure, const float* videoFlashTime) override;

        std::string SetCameraSyncOutSettings(
            const unsigned int  pCameraId, const unsigned int portNumber, const ESyncOutFreqMode* syncOutMode,
            const unsigned int* syncOutValue, const float* syncOutDutyCycle,
            const bool* syncOutNegativePolarity) override;

        std::string SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus, const float pAperture) override;
        std::string SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure, const float pCompensation) override;
        std::string SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable) override;
        std::string SetImageSettings(
            const unsigned int  pCameraId, const bool* enable, const CRTPacket::EImageFormat* format,
            const unsigned int* width, const unsigned int* height, const float* leftCrop,
            const float* topCrop, const float* rightCrop, const float* bottomCrop) override;

        std::string SetForceSettings(
            const unsigned int pPlateId, const SPoint* pCorner1, const SPoint* pCorner2,
            const SPoint* pCorner3, const SPoint* pCorner4) override;

        std::string Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs) override;
        std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& pSettingsSkeletons) override;

    private:
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;
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



