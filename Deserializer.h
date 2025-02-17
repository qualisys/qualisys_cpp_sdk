#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace tinyxml2
{
    class XMLDocument;
    class XMLElement;
}

namespace qualisys_cpp_sdk
{
    struct Deserializer
    {
    private:
        std::shared_ptr<tinyxml2::XMLDocument> mDocument;
        tinyxml2::XMLElement* mPtr;

        Deserializer(std::shared_ptr<tinyxml2::XMLDocument> document, tinyxml2::XMLElement* element);

    public:
        Deserializer(const char* data);

        Deserializer FindChild(const char* elementName) const;
        Deserializer FindNextSibling(const char* elementName) const;

        bool TryReadElementDouble(const char* elementName, double& output) const;
        bool TryReadElementFloat(const char* elementName, float& output) const;
        bool TryReadElementUnsignedInt32(const char* elementName, std::uint32_t& output) const;
        bool TryReadElementString(const char* elementName, std::string& output) const;
        bool TryReadElementBool(const std::string& element, bool& value) const;

        double ReadAttributeDouble(const char* attributeName, double defaultValue = 0) const;
        std::uint32_t ReadAttributeUnsignedInt(const char* attributeName, std::uint32_t defaultValue = 0) const;
        std::int32_t ReadAttributeInt(const char* attributeName, std::int32_t defaultValue = 0) const;
        std::string ReadAttributeString(const char* name) const;
        float ReadAttributeFloat(const char* name, float defaultValue = 0.0f) const;
        bool ReadAttributeBool(const char* attributeName, bool defaultValue = 0) const;

        std::string ReadString() const;
        int ReadInt(std::int32_t defaultValue = 0) const;
        unsigned int ReadUnsignedInt(std::int32_t defaultValue = 0) const;
        float ReadFloat(float defaultValue = 0.0f) const;

        bool operator==(const Deserializer& other) const;
        bool operator!=(const Deserializer& other) const;
        explicit operator bool() const;
    };

    struct ChildElementRange
    {
    private:
        Deserializer& mParent;
        const char* mElementName;

    public:
        ChildElementRange() = delete;
        ChildElementRange(Deserializer& parent, const char* elementName);

        struct Iterator
        {
            Deserializer mCurrent;
            const ChildElementRange& mChildElementRange;
            explicit Iterator(const ChildElementRange& range);
            Iterator(const ChildElementRange& range, std::size_t index);
            Deserializer operator*() const;
            Iterator& operator++();
            bool operator!=(const Iterator& other) const;
        };

        Iterator begin() const;
        Iterator end() const;
    };

    std::string ToLowerXmlString(std::string& str);
}
