#pragma once

#include <functional>
#include <memory>
#include <string>

namespace tinyxml2
{
    class XMLDocument;
    class XMLElement;
}

namespace qualisys_cpp_sdk
{
    struct SPosition;
    struct SRotation;

    struct DeserializerApi
    {
    private:
        std::shared_ptr<tinyxml2::XMLDocument> mDocument;
        DeserializerApi(std::shared_ptr<tinyxml2::XMLDocument> document, tinyxml2::XMLElement* element);

    public:
        tinyxml2::XMLElement* mPtr;
        DeserializerApi(const char* data);
        DeserializerApi FirstChildElement(const char* elementName) const;
        DeserializerApi NextSiblingElement(const char* elementName) const;
        double DoubleAttribute(const char* attributeName, double defaultValue = 0) const;
        std::uint32_t UnsignedAttribute(const char* attributeName, std::uint32_t defaultValue = 0) const;
        std::int32_t IntAttribute(const char* attributeName, std::int32_t defaultValue = 0) const;
        bool BoolAttribute(const char* attributeName, bool defaultValue = 0) const;
        bool operator==(const DeserializerApi& other) const;
        bool operator!=(const DeserializerApi& other) const;
        explicit operator bool() const;
        int IntText(std::int32_t defaultValue = 0) const;
        unsigned int UnsignedText(std::int32_t defaultValue = 0) const;
        float FloatText(float defaultValue = .0f) const;
        std::string Attribute(const char* name) const;
        float FloatAttribute(const char* name, float defaultValue = .0f) const;
        std::string GetText() const;
    };

    struct ChildElementRange
    {
        static constexpr std::size_t buffSize = 128;
        using TElementNameGenerator = std::function<const char*(char (&buff)[buffSize], std::size_t bufferSize,
                                                                std::size_t index)>;

    private:
        DeserializerApi& parent;
        TElementNameGenerator elementNameGenerator;

    public:
        ChildElementRange() = delete;
        ChildElementRange(DeserializerApi& parent, const char* elementName);
        ChildElementRange(DeserializerApi& parent,
                          TElementNameGenerator generator);

        struct Iterator
        {
            char buffer[buffSize];
            DeserializerApi current;
            const ChildElementRange& range;
            std::size_t index;
            explicit Iterator(const ChildElementRange& range);
            Iterator(const ChildElementRange& range, std::size_t index);
            DeserializerApi operator*() const;
            Iterator& operator++();
            bool operator!=(const Iterator& other) const;
        };

        Iterator begin() const;
        Iterator end() const;
    };

    std::string ToLowerXmlString(std::string str);
    bool TryReadElementDouble(DeserializerApi& element, const char* elementName, double& output);
    bool TryReadElementFloat(DeserializerApi& element, const char* elementName, float& output);
    bool TryReadElementUnsignedInt32(DeserializerApi& element, const char* elementName, std::uint32_t& output);
    bool TryReadElementString(DeserializerApi& element, const char* elementName, std::string& output);
    bool ReadXmlBool(DeserializerApi xml, const std::string& element, bool& value);
    SPosition ReadSPosition(DeserializerApi& parentElem, const std::string& element);
    SRotation ReadSRotation(DeserializerApi& parentElem, const std::string& element);
}
