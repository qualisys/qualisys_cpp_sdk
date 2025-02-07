#pragma once

#include <memory>
#include <string>
#include "Settings.h"
#include "tinyxml2.h"

namespace tinyxml2
{
    class XMLDocument;
    class XMLElement;
}

namespace qualisys_cpp_sdk
{
    struct SPosition;
    struct SRotation;
    struct SerializerApi;



    struct SerializerApi
    {
    private:
        tinyxml2::XMLDocument* mDocument;
        tinyxml2::XMLElement* mCurrentElement = nullptr;
        
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;

        SerializerApi(const SerializerApi& src, tinyxml2::XMLElement* element)
            : mDocument(src.mDocument), mCurrentElement(element), mMajorVersion(src.mMajorVersion), mMinorVersion(src.mMinorVersion)
        {
        }

        void AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* elementName, const bool* value, tinyxml2::XMLDocument& document, const char* trueText = "True", const char* falseText = "False");
        void AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* elementName, const bool value, tinyxml2::XMLDocument& document, const char* trueText = "True", const char* falseText = "False");
        void AddXMLElementInt(tinyxml2::XMLElement& parentElem, const char* elementName, const int* value, tinyxml2::XMLDocument& document);
        void AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* elementName, const unsigned int value, tinyxml2::XMLDocument& document);
        void AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* elementName, const unsigned int* value, tinyxml2::XMLDocument& document);
        void AddXMLElementFloat(tinyxml2::XMLElement& parentElem, const char* elementName, const float* value, unsigned int decimals, tinyxml2::XMLDocument& document);
        void AddXMLElementFloatWithTextAttribute(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const char* elementName, const char* attributeName, const float& value, unsigned int decimals);
        void AddXMLElementTransform(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SPosition& position, const SRotation& rotation);
        void AddXMLElementDOF(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SDegreeOfFreedom& degreesOfFreedom);

    public:

        SerializerApi Element(const char* name)
        {
            if (mCurrentElement)
            {
                return SerializerApi{ *this, mCurrentElement->InsertNewChildElement(name) };
            }

            auto newElement = mDocument->NewElement(name);
            mDocument->InsertFirstChild(newElement);
            return SerializerApi{ *this, newElement};
        }

        SerializerApi ElementBool(const char* name, bool value)
        {
            auto newElement = Element(name);
            newElement.mCurrentElement->SetText(value);
            return SerializerApi{ *this, newElement.mCurrentElement };
        }

        SerializerApi ElementInt(const char* name, std::int32_t value)
        {
            auto newElement = Element(name);
            newElement.mCurrentElement->SetText(value);
            return SerializerApi{ *this, newElement.mCurrentElement };
        }

        SerializerApi ElementFloat(const char* name, float value, int decimals)
        {
            char formattedValue[32];
            (void)snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);
            auto newElement = Element(name);
            newElement.mCurrentElement->SetText(formattedValue);
            return SerializerApi{ *this, newElement.mCurrentElement };
        }

        SerializerApi ElementDouble(const char* name, float value)
        {
            auto newElement = Element(name);
            newElement.mCurrentElement->SetText(value);
            return SerializerApi{ *this, newElement.mCurrentElement };
        }

        SerializerApi ElementUnsignedInt(const char* name, std::uint32_t value)
        {
            auto newElement = Element(name);
            newElement.mCurrentElement->SetText(value);
            return SerializerApi{ *this, newElement.mCurrentElement };
        }

        SerializerApi ElementString(const char* name, const char* value)
        {
            auto newElement = Element(name);
            newElement.mCurrentElement->SetText(value);
            return SerializerApi{ *this, newElement.mCurrentElement };
        }

        std::string ToString() const
        {
            tinyxml2::XMLPrinter printer{};
            mDocument->Print(&printer);
            return printer.CStr();
        }

        SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion);

        std::string SetGeneralSettings(const unsigned int* captureFrequency, const float* captureTime,
            const bool* startOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC, const bool* startOnTrigSoftware,
            const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions);

        std::string SetExtTimeBaseSettings(const bool* enabled, const ESignalSource* signalSource, const bool* signalModePeriodic,
            const unsigned int* freqMultiplier, const unsigned int* freqDivisor, const unsigned int* freqTolerance,
            const float* nominalFrequency, const bool* negativeEdge, const unsigned int* signalShutterDelay, const float* nonPeriodicTimeout);

        std::string SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings);

        std::string SetCameraSettings(const unsigned int cameraId, const ECameraMode* mode, const float* markerExposure,
            const float* markerThreshold, const int* orientation);

        std::string SetCameraVideoSettings(const unsigned int cameraId, const EVideoResolution* videoResolution,
            const EVideoAspectRatio* videoAspectRatio, const unsigned int* videoFrequency, const float* videoExposure, const float* videoFlashTime);

        std::string SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber, const ESyncOutFreqMode* syncOutMode,
            const unsigned int* syncOutValue, const float* syncOutDutyCycle, const bool* syncOutNegativePolarity);

        std::string SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture);

        std::string SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation);

        std::string SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable);

        std::string SetImageSettings(const unsigned int  cameraId, const bool* enable, const CRTPacket::EImageFormat* format,
            const unsigned int* width, const unsigned int* height, const float* leftCrop,
            const float* topCrop, const float* rightCrop, const float* bottomCrop);

        std::string SetForceSettings(const unsigned int plateId, const SPoint* corner1, const SPoint* corner2,
            const SPoint* corner3, const SPoint* corner4);

        std::string Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& settings6Dofs);

        std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons);
    };
}
