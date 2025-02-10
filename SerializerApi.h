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

        SerializerApi ElementDouble(const char* name, double value)
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

        SerializerApi AttributeBool(const char* name, bool value)
        {
            mCurrentElement->SetAttribute(name, value);
            return *this;
        }

        SerializerApi AttributeInt(const char* name, std::int32_t value)
        {
            mCurrentElement->SetAttribute(name, value);
            return *this;
        }

        SerializerApi AttributeFloat(const char* name, float value, int decimals)
        {
            char formattedValue[32];
            (void)snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);
            mCurrentElement->SetAttribute(name, formattedValue);
            return *this;
        }

        SerializerApi AttributeDouble(const char* name, double value)
        {
            mCurrentElement->SetAttribute(name, value);
            return *this;
        }

        SerializerApi AttributeUnsignedInt(const char* name, std::uint32_t value)
        {
            mCurrentElement->SetAttribute(name, value);
            return *this;
        }

        SerializerApi AttributeString(const char* name, const char* value)
        {
            mCurrentElement->SetAttribute(name, value);
            return *this;
        }

        std::string ToString() const
        {
            tinyxml2::XMLPrinter printer{};
            mDocument->Print(&printer);
            return printer.CStr();
        }

        SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion);

        std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons);
    };
}
