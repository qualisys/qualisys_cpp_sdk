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

    struct Deserializer
    {
    private:
        std::shared_ptr<tinyxml2::XMLDocument> mDocument;
        Deserializer(std::shared_ptr<tinyxml2::XMLDocument>, tinyxml2::XMLElement*);

    public:
        tinyxml2::XMLElement* mPtr;
        Deserializer(const char* data);
        Deserializer FirstChildElement(const char* elementName) const;
        Deserializer NextSiblingElement(const char* elementName) const;
        double DoubleAttribute(const char* attributeName, double defaultValue = 0) const;
        std::uint32_t UnsignedAttribute(const char* attributeName, std::uint32_t defaultValue = 0) const;
        std::int32_t IntAttribute(const char* attributeName, std::int32_t defaultValue = 0) const;
        bool BoolAttribute(const char* attributeName, bool defaultValue = 0) const;
        bool operator==(const Deserializer& other) const;
        bool operator!=(const Deserializer& other) const;
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
        Deserializer& parent;
        TElementNameGenerator elementNameGenerator;

    public:
        ChildElementRange() = delete;
        ChildElementRange(Deserializer& parent, const char* elementName);
        ChildElementRange(Deserializer& parent,
                          TElementNameGenerator generator);

        struct Iterator
        {
            char buffer[buffSize];
            Deserializer current;
            const ChildElementRange& range;
            std::size_t index;
            explicit Iterator(const ChildElementRange& range);
            Iterator(const ChildElementRange& range, std::size_t index);
            Deserializer operator*() const;
            Iterator& operator++();
            bool operator!=(const Iterator& other) const;
        };

        Iterator begin() const;
        Iterator end() const;
    };

    std::string ToLowerXmlString(std::string str);
    bool TryReadElementDouble(Deserializer& element, const char* elementName, double& output);
    bool TryReadElementFloat(Deserializer& element, const char* elementName, float& output);
    bool TryReadElementUnsignedInt32(Deserializer& element, const char* elementName, std::uint32_t& output);
    bool TryReadElementString(Deserializer& element, const char* elementName, std::string& output);
    bool ReadXmlBool(Deserializer xml, const std::string& element, bool& value);
    SPosition ReadSPosition(Deserializer& parentElem, const std::string& element);
    SRotation ReadSRotation(Deserializer& parentElem, const std::string& element);
}
