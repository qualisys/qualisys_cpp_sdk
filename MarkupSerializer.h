#pragma once

#include "Settings.h"
#include "Markup.h"

namespace qualisys_cpp_sdk {

    struct DLL_EXPORT CMarkupDeserializer : ISettingsDeserializer {
        CMarkupDeserializer(const char* data, std::uint32_t majorVersion, std::uint32_t minorVersion );
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
        char mErrorStr[1024];
        CMarkup xmlDocument;
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
        CMarkupSerializer(std::uint32_t majorVersion, std::uint32_t minorVersion);
        CMarkup xmlDocument;
        std::string SetGeneralSettings(const unsigned int* captureFrequency, const float* captureTime,
            const bool* startOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC, const bool* startOnTrigSoftware,
            const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions) override;

        std::string SetExtTimeBaseSettings(
            const bool* enabled, const ESignalSource* signalSource,
            const bool* signalModePeriodic, const unsigned int* freqMultiplier,
            const unsigned int* freqDivisor, const unsigned int* freqTolerance,
            const float* nominalFrequency, const bool* negativeEdge,
            const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout) override;

        std::string SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings) override;

        std::string SetCameraSettings(
            const unsigned int cameraId, const ECameraMode* mode,
            const float* markerExposure, const float* markerThreshold,
            const int* orientation) override;

        std::string SetCameraVideoSettings(
            const unsigned int cameraId, const EVideoResolution* videoResolution,
            const EVideoAspectRatio* videoAspectRatio, const unsigned int* videoFrequency,
            const float* videoExposure, const float* videoFlashTime) override;

        std::string SetCameraSyncOutSettings(
            const unsigned int  cameraId, const unsigned int portNumber, const ESyncOutFreqMode* syncOutMode,
            const unsigned int* syncOutValue, const float* syncOutDutyCycle,
            const bool* syncOutNegativePolarity) override;

        std::string SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture) override;
        std::string SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation) override;
        std::string SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable) override;
        std::string SetImageSettings(
            const unsigned int  cameraId, const bool* enable, const CRTPacket::EImageFormat* format,
            const unsigned int* width, const unsigned int* height, const float* leftCrop,
            const float* topCrop, const float* rightCrop, const float* bottomCrop) override;

        std::string SetForceSettings(
            const unsigned int plateId, const SPoint* corner1, const SPoint* corner2,
            const SPoint* corner3, const SPoint* corner4) override;

        std::string Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings6Dofs) override;
        std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons) override;

    private:
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;
        void AddXMLElementBool(CMarkup* xmlDocument, const char* tTag, const bool* pbValue, const char* tTrue = "True", const char* tFalse = "False");
        void AddXMLElementBool(CMarkup* xmlDocument, const char* tTag, const bool bValue, const char* tTrue = "True", const char* tFalse = "False");
        void AddXMLElementInt(CMarkup* xmlDocument, const char* tTag, const int* pnValue);
        void AddXMLElementUnsignedInt(CMarkup* xmlDocument, const char* tTag, const unsigned int value);
        void AddXMLElementUnsignedInt(CMarkup* xmlDocument, const char* tTag, const unsigned int* pnValue);
        void AddXMLElementFloat(CMarkup* xmlDocument, const char* tTag, const float* pfValue, unsigned int pnDecimals = 6);
        void AddXMLElementTransform(CMarkup& xml, const std::string& name, const SPosition& position, const SRotation& rotation);
        void AddXMLElementDOF(CMarkup& xml, const std::string& name, SDegreeOfFreedom degreeOfFreedom);
    };
}



