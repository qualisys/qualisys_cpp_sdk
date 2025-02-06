#pragma once

#include "Settings.h"

#include <tinyxml2.h>

namespace qualisys_cpp_sdk {



    struct DLL_EXPORT CTinyxml2Serializer : public ISettingsSerializer {
        CTinyxml2Serializer(std::uint32_t pMajorVersion, std::uint32_t pMinorVersion);
        tinyxml2::XMLDocument oXML;
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

        std::string SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus, const float pAperture) override;
        std::string SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure, const float pCompensation) override;
        std::string SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable) override;
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
        void AddXMLElementBool(tinyxml2::XMLElement& parent, const char* tTag, const bool* pbValue, tinyxml2::XMLDocument& oXML, const char* tTrue = "True", const char* tFalse = "False");
        void AddXMLElementBool(tinyxml2::XMLElement& parent, const char* tTag, const bool pbValue, tinyxml2::XMLDocument& oXML, const char* tTrue = "True", const char* tFalse = "False");
        void AddXMLElementInt(tinyxml2::XMLElement& parent, const char* tTag, const int* pnValue, tinyxml2::XMLDocument& oXML);
        void AddXMLElementUnsignedInt(tinyxml2::XMLElement& parent, const char* tTag, const unsigned int nValue, tinyxml2::XMLDocument& oXML);
        void AddXMLElementUnsignedInt(tinyxml2::XMLElement& parent, const char* tTag, const unsigned int* pnValue, tinyxml2::XMLDocument& oXML);
        void AddXMLElementFloat(tinyxml2::XMLElement& parent, const char* tTag, const float* pfValue, unsigned int pnDecimals, tinyxml2::XMLDocument& oXML);
        void AddXMLElementFloatWithTextAttribute(tinyxml2::XMLDocument& oXML, tinyxml2::XMLElement& parent, const char* elementName, const char* attributeName, const float& value, unsigned int decimals);
        void AddXMLElementTransform(tinyxml2::XMLDocument& oXML, tinyxml2::XMLElement& parentElem, const std::string& name, const SPosition& position, const SRotation& rotation);
        void AddXMLElementDOF(tinyxml2::XMLDocument& oXML, tinyxml2::XMLElement& parentElem, const std::string& name, const SDegreeOfFreedom& degreeOfFreedoms);
    };
}
