#include "Deserializer.h"

#include <algorithm>
#include <tinyxml2.h>
#include <Settings.h>


/// <summary>
/// Deserializer
/// </summary>
qualisys_cpp_sdk::Deserializer::Deserializer(std::shared_ptr<tinyxml2::XMLDocument> mDocument,
                                             tinyxml2::XMLElement* ptr)
    : mDocument(mDocument), mPtr(ptr)
{
}

qualisys_cpp_sdk::Deserializer::Deserializer(const char* data)
{
    mDocument = std::make_unique<tinyxml2::XMLDocument>();
    mDocument->Parse(data);
    mPtr = mDocument->RootElement();
}

qualisys_cpp_sdk::Deserializer qualisys_cpp_sdk::Deserializer::FirstChildElement(const char* elementName) const
{
    return {mDocument, mPtr->FirstChildElement(elementName)};
}

qualisys_cpp_sdk::Deserializer qualisys_cpp_sdk::Deserializer::NextSiblingElement(const char* elementName) const
{
    return {mDocument, mPtr->NextSiblingElement(elementName)};
}

double qualisys_cpp_sdk::Deserializer::DoubleAttribute(const char* attributeName, double defaultValue) const
{
    return mPtr->DoubleAttribute(attributeName, defaultValue);
}

std::uint32_t qualisys_cpp_sdk::Deserializer::UnsignedAttribute(const char* attributeName,
                                                                std::uint32_t defaultValue) const
{
    return mPtr->UnsignedAttribute(attributeName, defaultValue);
}

std::int32_t qualisys_cpp_sdk::Deserializer::IntAttribute(const char* attributeName, std::int32_t defaultValue) const
{
    return mPtr->IntAttribute(attributeName, defaultValue);
}

bool qualisys_cpp_sdk::Deserializer::BoolAttribute(const char* attributeName, bool defaultValue) const
{
    return mPtr->BoolAttribute(attributeName, defaultValue);
}

bool qualisys_cpp_sdk::Deserializer::operator==(const Deserializer& other) const
{
    return mPtr == other.mPtr;
}

bool qualisys_cpp_sdk::Deserializer::operator!=(const Deserializer& other) const
{
    return mPtr != other.mPtr;
}

qualisys_cpp_sdk::Deserializer::operator bool() const
{
    return mPtr != nullptr;
}

int qualisys_cpp_sdk::Deserializer::IntText(std::int32_t defaultValue) const
{
    return mPtr->IntText(defaultValue);
}

unsigned int qualisys_cpp_sdk::Deserializer::UnsignedText(std::int32_t defaultValue) const
{
    return mPtr->UnsignedText(defaultValue);
}

float qualisys_cpp_sdk::Deserializer::FloatText(float defaultValue) const
{
    return mPtr->FloatText(defaultValue);
}

std::string qualisys_cpp_sdk::Deserializer::Attribute(const char* name) const
{
    if (auto charPtr = mPtr->Attribute(name))
    {
        return {charPtr};
    }

    return {};
}

float qualisys_cpp_sdk::Deserializer::FloatAttribute(const char* name, float defaultValue) const
{
    return mPtr->FloatAttribute(name, defaultValue);
}

std::string qualisys_cpp_sdk::Deserializer::GetText() const
{
    if (auto charPtr = mPtr->GetText())
    {
        return {charPtr};
    }

    return {};
}

/// <summary>
/// ChildElementRange
/// </summary>
qualisys_cpp_sdk::ChildElementRange::ChildElementRange(Deserializer& parent, const char* elementName) : parent(parent),
    elementNameGenerator([elementName](auto& buff, std::size_t, std::size_t) { return elementName; })
{
}

qualisys_cpp_sdk::ChildElementRange::ChildElementRange(Deserializer& parent, TElementNameGenerator generator) :
    parent(parent), elementNameGenerator(std::move(generator))
{
}

qualisys_cpp_sdk::ChildElementRange::Iterator::Iterator(const ChildElementRange& range) : buffer{}, current(nullptr),
    range(range), index(std::numeric_limits<std::size_t>::max())
{
}

qualisys_cpp_sdk::ChildElementRange::Iterator::Iterator(const ChildElementRange& range, std::size_t index) :
    buffer{}, current(nullptr), range(range), index(index)
{
    current = range.parent.FirstChildElement(range.elementNameGenerator(buffer, buffSize, index++));
}

qualisys_cpp_sdk::Deserializer qualisys_cpp_sdk::ChildElementRange::Iterator::operator*() const
{
    return current;
}

qualisys_cpp_sdk::ChildElementRange::Iterator& qualisys_cpp_sdk::ChildElementRange::Iterator::operator++()
{
    current = current.NextSiblingElement(range.elementNameGenerator(buffer, buffSize, index++));
    return *this;
}

bool qualisys_cpp_sdk::ChildElementRange::Iterator::operator!=(const Iterator& other) const
{
    return current != other.current;
}

qualisys_cpp_sdk::ChildElementRange::Iterator qualisys_cpp_sdk::ChildElementRange::begin() const
{
    return Iterator(*this, 0);
}

qualisys_cpp_sdk::ChildElementRange::Iterator qualisys_cpp_sdk::ChildElementRange::end() const
{
    return Iterator(*this);
}


/// <summary>
/// Helper functions
/// </summary>
std::string qualisys_cpp_sdk::ToLowerXmlString(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
    {
        return static_cast<unsigned char>(std::tolower(c));
    });
    return str;
}

bool qualisys_cpp_sdk::TryReadElementDouble(Deserializer& element, const char* elementName, double& output)
{
    if (auto childElem = element.FirstChildElement(elementName))
    {
        return childElem.mPtr->QueryDoubleText(&output) == tinyxml2::XML_SUCCESS;
    }

    return false;
}

bool qualisys_cpp_sdk::TryReadElementFloat(Deserializer& element, const char* elementName, float& output)
{
    if (auto childElem = element.FirstChildElement(elementName))
    {
        return childElem.mPtr->QueryFloatText(&output) == tinyxml2::XML_SUCCESS;
    }

    return false;
}

bool qualisys_cpp_sdk::TryReadElementUnsignedInt32(Deserializer& element, const char* elementName,
                                                   std::uint32_t& output)
{
    if (auto childElem = element.FirstChildElement(elementName))
    {
        return childElem.mPtr->QueryUnsignedText(&output) == tinyxml2::XML_SUCCESS;
    }

    return false;
}

bool qualisys_cpp_sdk::TryReadElementString(Deserializer& element, const char* elementName, std::string& output)
{
    output.clear();

    if (auto childElem = element.FirstChildElement(elementName))
    {
        if (auto charPtr = childElem.mPtr->GetText())
        {
            output = charPtr;
        }
        return true;
    }

    return false;
}

namespace
{
    void RemoveInvalidChars(std::string& str)
    {
        auto isInvalidChar = [](int c) -> int
        {
            // iscntrl: control codes(NUL, etc.), '\t', '\n', '\v', '\f', '\r', backspace (DEL)
            // isspace: some common checks but also 0x20 (SPACE)
            // return != 0 --> invalid char
            return std::iscntrl(c) + std::isspace(c);
        };
        str.erase(std::remove_if(str.begin(), str.end(), isInvalidChar), str.end());
    }
}

bool qualisys_cpp_sdk::ReadXmlBool(Deserializer xml, const std::string& element, bool& value)
{
    auto xmlElem = xml.FirstChildElement(element.c_str());
    if (!xmlElem)
    {
        return false;
    }

    auto str = std::string(xmlElem.mPtr->GetText());
    RemoveInvalidChars(str);
    str = ToLowerXmlString(str);

    if (str == "true")
    {
        value = true;
    }
    else if (str == "false")
    {
        value = false;
    }
    else
    {
        // Don't change value, just report error.
        return false;
    }

    return true;
}

qualisys_cpp_sdk::SPosition qualisys_cpp_sdk::ReadSPosition(Deserializer& parentElem, const std::string& element)
{
    auto positionElem = parentElem.FirstChildElement(element.data());
    if (positionElem)
    {
        return {
            positionElem.DoubleAttribute("X"),
            positionElem.DoubleAttribute("Y"),
            positionElem.DoubleAttribute("Z"),
        };
    }

    return {};
}

qualisys_cpp_sdk::SRotation qualisys_cpp_sdk::ReadSRotation(Deserializer& parentElem, const std::string& element)
{
    auto rotationElem = parentElem.FirstChildElement(element.data());
    if (rotationElem)
    {
        return {
            rotationElem.DoubleAttribute("X"),
            rotationElem.DoubleAttribute("Y"),
            rotationElem.DoubleAttribute("Z"),
            rotationElem.DoubleAttribute("W")
        };
    }

    return {};
}
