#pragma once

#include <string>
#include <cstdint>
#include <memory>

namespace tinyxml2
{
    class XMLDocument;
    class XMLElement;
}

namespace qualisys_cpp_sdk
{
    struct Serializer
    {
    private:
        std::shared_ptr<tinyxml2::XMLDocument> mDocument;
        tinyxml2::XMLElement& mCurrentElement;

        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;

        Serializer(const Serializer& src, tinyxml2::XMLElement& element);

    public:
        Serializer(std::uint32_t majorVersion, std::uint32_t minorVersion, const char* rootElementName);

        Serializer Element(const char* name);
        Serializer ElementBool(const char* name, bool value);
        Serializer ElementInt(const char* name, std::int32_t value);
        Serializer ElementFloat(const char* name, float value);
        Serializer ElementDouble(const char* name, double value);
        Serializer ElementUnsignedInt(const char* name, std::uint32_t value);
        Serializer ElementString(const char* name, const char* value);

        Serializer AttributeBool(const char* name, bool value);
        Serializer AttributeInt(const char* name, std::int32_t value);
        Serializer AttributeFloat(const char* name, float value);
        Serializer AttributeDouble(const char* name, double value);
        Serializer AttributeUnsignedInt(const char* name, std::uint32_t value);
        Serializer AttributeString(const char* name, const char* value);

        std::string ToString() const;
    };
}
