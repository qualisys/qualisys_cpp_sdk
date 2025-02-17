#include "Deserializer.h"

#include <algorithm>
#include <tinyxml2.h>

qualisys_cpp_sdk::Deserializer::Deserializer(std::shared_ptr<tinyxml2::XMLDocument> document,
                                                   tinyxml2::XMLElement* element)
    : mDocument(document), mPtr(element)
{
}

qualisys_cpp_sdk::Deserializer::Deserializer(const char* data)
{
    mDocument = std::make_shared<tinyxml2::XMLDocument>();
    mDocument->Parse(data);
    mPtr = mDocument->RootElement();
}

qualisys_cpp_sdk::Deserializer qualisys_cpp_sdk::Deserializer::FindChild(const char* elementName) const
{
    return {mDocument, mPtr->FirstChildElement(elementName)};
}

qualisys_cpp_sdk::Deserializer qualisys_cpp_sdk::Deserializer::FindNextSibling(const char* elementName) const
{
    return {mDocument, mPtr->NextSiblingElement(elementName)};
}

double qualisys_cpp_sdk::Deserializer::ReadAttributeDouble(const char* attributeName, double defaultValue) const
{
    return mPtr->DoubleAttribute(attributeName, defaultValue);
}

std::uint32_t qualisys_cpp_sdk::Deserializer::ReadAttributeUnsignedInt(const char* attributeName,
                                                                          std::uint32_t defaultValue) const
{
    return mPtr->UnsignedAttribute(attributeName, defaultValue);
}

std::int32_t qualisys_cpp_sdk::Deserializer::ReadAttributeInt(const char* attributeName,
                                                                 std::int32_t defaultValue) const
{
    return mPtr->IntAttribute(attributeName, defaultValue);
}

bool qualisys_cpp_sdk::Deserializer::ReadAttributeBool(const char* attributeName, bool defaultValue) const
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

int qualisys_cpp_sdk::Deserializer::ReadInt(std::int32_t defaultValue) const
{
    return mPtr->IntText(defaultValue);
}

unsigned int qualisys_cpp_sdk::Deserializer::ReadUnsignedInt(std::int32_t defaultValue) const
{
    return mPtr->UnsignedText(defaultValue);
}

float qualisys_cpp_sdk::Deserializer::ReadFloat(float defaultValue) const
{
    return mPtr->FloatText(defaultValue);
}

std::string qualisys_cpp_sdk::Deserializer::ReadAttributeString(const char* name) const
{
    if (auto charPtr = mPtr->Attribute(name))
    {
        return {charPtr};
    }

    return {};
}

float qualisys_cpp_sdk::Deserializer::ReadAttributeFloat(const char* name, float defaultValue) const
{
    return mPtr->FloatAttribute(name, defaultValue);
}

std::string qualisys_cpp_sdk::Deserializer::ReadString() const
{
    if (auto charPtr = mPtr->GetText())
    {
        return {charPtr};
    }

    return {};
}

qualisys_cpp_sdk::ChildElementRange::ChildElementRange(Deserializer& parent, const char* elementName)
    : mParent(parent), mElementName(elementName)
{
}

qualisys_cpp_sdk::ChildElementRange::Iterator::Iterator(const ChildElementRange& range)
    : mCurrent(nullptr), mChildElementRange(range)
{
}

qualisys_cpp_sdk::ChildElementRange::Iterator::Iterator(const ChildElementRange& range, std::size_t index)
    : mCurrent(nullptr), mChildElementRange(range)
{
    mCurrent = range.mParent.FindChild(mChildElementRange.mElementName);
}

qualisys_cpp_sdk::Deserializer qualisys_cpp_sdk::ChildElementRange::Iterator::operator*() const
{
    return mCurrent;
}

qualisys_cpp_sdk::ChildElementRange::Iterator& qualisys_cpp_sdk::ChildElementRange::Iterator::operator++()
{
    mCurrent = mCurrent.FindNextSibling(mChildElementRange.mElementName);
    return *this;
}

bool qualisys_cpp_sdk::ChildElementRange::Iterator::operator!=(const Iterator& other) const
{
    return mCurrent != other.mCurrent;
}

qualisys_cpp_sdk::ChildElementRange::Iterator qualisys_cpp_sdk::ChildElementRange::begin() const
{
    return Iterator(*this, 0);
}

qualisys_cpp_sdk::ChildElementRange::Iterator qualisys_cpp_sdk::ChildElementRange::end() const
{
    return Iterator(*this);
}

bool qualisys_cpp_sdk::Deserializer::TryReadElementDouble(const char* elementName, double& output) const
{
    if (auto childElem = FindChild(elementName))
    {
        return childElem.mPtr->QueryDoubleText(&output) == tinyxml2::XML_SUCCESS;
    }

    return false;
}

bool qualisys_cpp_sdk::Deserializer::TryReadElementFloat(const char* elementName, float& output) const
{
    if (auto childElem = FindChild(elementName))
    {
        return childElem.mPtr->QueryFloatText(&output) == tinyxml2::XML_SUCCESS;
    }

    return false;
}

bool qualisys_cpp_sdk::Deserializer::TryReadElementUnsignedInt32(const char* elementName,
                                                                    std::uint32_t& output) const
{
    if (auto childElem = FindChild(elementName))
    {
        return childElem.mPtr->QueryUnsignedText(&output) == tinyxml2::XML_SUCCESS;
    }

    return false;
}

bool qualisys_cpp_sdk::Deserializer::TryReadElementString(const char* elementName, std::string& output) const
{
    output.clear();

    if (auto childElem = FindChild(elementName))
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
            return std::iscntrl(c) != 0 || std::isspace(c) != 0;
        };
        str.erase(std::remove_if(str.begin(), str.end(), isInvalidChar), str.end());
    }
}

bool qualisys_cpp_sdk::Deserializer::TryReadElementBool(const std::string& element, bool& value) const
{
    auto xmlElem = FindChild(element.c_str());
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

std::string qualisys_cpp_sdk::ToLowerXmlString(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
    {
        return static_cast<unsigned char>(std::tolower(c));
    });
    return str;
}
