#include "Serializer.h"

#include <tinyxml2.h>

using namespace qualisys_cpp_sdk;

Serializer::Serializer(const Serializer& src, tinyxml2::XMLElement& element)
    : mDocument(src.mDocument), mCurrentElement(element), mMajorVersion(src.mMajorVersion),
      mMinorVersion(src.mMinorVersion)
{
}

Serializer::Serializer(std::uint32_t majorVersion, std::uint32_t minorVersion, const char* rootElementName)
    : mDocument(std::make_shared<tinyxml2::XMLDocument>()), mCurrentElement(*mDocument->NewElement(rootElementName)),
      mMajorVersion(majorVersion), mMinorVersion(minorVersion)
{
    mDocument->InsertFirstChild(&mCurrentElement);
}

Serializer Serializer::Element(const char* name)
{
    return Serializer{*this, *mCurrentElement.InsertNewChildElement(name)};
}

Serializer Serializer::ElementBool(const char* name, bool value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return Serializer{*this, newElement.mCurrentElement};
}

Serializer Serializer::ElementInt(const char* name, std::int32_t value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return Serializer{*this, newElement.mCurrentElement};
}

Serializer Serializer::ElementFloat(const char* name, float value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(std::to_string(value).c_str());
    return Serializer{*this, newElement.mCurrentElement};
}

Serializer Serializer::ElementDouble(const char* name, double value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(std::to_string(value).c_str());
    return Serializer{*this, newElement.mCurrentElement};
}

Serializer Serializer::ElementUnsignedInt(const char* name, std::uint32_t value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return Serializer{*this, newElement.mCurrentElement};
}

Serializer Serializer::ElementString(const char* name, const char* value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return Serializer{*this, newElement.mCurrentElement};
}

Serializer Serializer::AttributeBool(const char* name, bool value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

Serializer Serializer::AttributeInt(const char* name, std::int32_t value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

Serializer Serializer::AttributeFloat(const char* name, float value)
{
    mCurrentElement.SetAttribute(name, std::to_string(value).c_str());
    return *this;
}

Serializer Serializer::AttributeDouble(const char* name, double value)
{
    mCurrentElement.SetAttribute(name, std::to_string(value).c_str());
    return *this;
}

Serializer Serializer::AttributeUnsignedInt(const char* name, std::uint32_t value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

Serializer Serializer::AttributeString(const char* name, const char* value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

std::string Serializer::ToString() const
{
    tinyxml2::XMLPrinter printer{};
    mDocument->Print(&printer);
    return printer.CStr();
}
