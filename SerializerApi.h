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
    struct SerializerApi
    {
    private:
        std::shared_ptr<tinyxml2::XMLDocument> mDocument;
        tinyxml2::XMLElement& mCurrentElement;
        
        std::uint32_t mMajorVersion;
        std::uint32_t mMinorVersion;

        SerializerApi(const SerializerApi& src, tinyxml2::XMLElement& element);

    public:
        SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion, const char* rootElementName);

        SerializerApi Element(const char* name);
        SerializerApi ElementBool(const char* name, bool value);
        SerializerApi ElementInt(const char* name, std::int32_t value);
        SerializerApi ElementFloat(const char* name, float value);
        SerializerApi ElementDouble(const char* name, double value);
        SerializerApi ElementUnsignedInt(const char* name, std::uint32_t value);
        SerializerApi ElementString(const char* name, const char* value);

        SerializerApi AttributeBool(const char* name, bool value);
        SerializerApi AttributeInt(const char* name, std::int32_t value);
        SerializerApi AttributeFloat(const char* name, float value);
        SerializerApi AttributeDouble(const char* name, double value);
        SerializerApi AttributeUnsignedInt(const char* name, std::uint32_t value);
        SerializerApi AttributeString(const char* name, const char* value);

        std::string ToString() const;
    };
}
