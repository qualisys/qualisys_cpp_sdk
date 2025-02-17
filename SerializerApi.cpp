#include "SerializerApi.h"

#include <tinyxml2.h>

using namespace qualisys_cpp_sdk;

SerializerApi::SerializerApi(const SerializerApi& src, tinyxml2::XMLElement& element)
    : mDocument(src.mDocument), mCurrentElement(element), mMajorVersion(src.mMajorVersion),
      mMinorVersion(src.mMinorVersion)
{
}

SerializerApi::SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion, const char* rootElementName)
    : mDocument(std::make_shared<tinyxml2::XMLDocument>()), mCurrentElement(*mDocument->NewElement(rootElementName)),
      mMajorVersion(majorVersion), mMinorVersion(minorVersion)
{
    mDocument->InsertFirstChild(&mCurrentElement);
}

SerializerApi SerializerApi::Element(const char* name)
{
    return SerializerApi{*this, *mCurrentElement.InsertNewChildElement(name)};
}

SerializerApi SerializerApi::ElementBool(const char* name, bool value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return SerializerApi{*this, newElement.mCurrentElement};
}

SerializerApi SerializerApi::ElementInt(const char* name, std::int32_t value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return SerializerApi{*this, newElement.mCurrentElement};
}

SerializerApi SerializerApi::ElementFloat(const char* name, float value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(std::to_string(value).c_str());
    return SerializerApi{*this, newElement.mCurrentElement};
}

SerializerApi SerializerApi::ElementDouble(const char* name, double value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(std::to_string(value).c_str());
    return SerializerApi{*this, newElement.mCurrentElement};
}

SerializerApi SerializerApi::ElementUnsignedInt(const char* name, std::uint32_t value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return SerializerApi{*this, newElement.mCurrentElement};
}

SerializerApi SerializerApi::ElementString(const char* name, const char* value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement.SetText(value);
    return SerializerApi{*this, newElement.mCurrentElement};
}

SerializerApi SerializerApi::AttributeBool(const char* name, bool value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

SerializerApi SerializerApi::AttributeInt(const char* name, std::int32_t value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

SerializerApi SerializerApi::AttributeFloat(const char* name, float value)
{
    mCurrentElement.SetAttribute(name, std::to_string(value).c_str());
    return *this;
}

SerializerApi SerializerApi::AttributeDouble(const char* name, double value)
{
    mCurrentElement.SetAttribute(name, std::to_string(value).c_str());
    return *this;
}

SerializerApi SerializerApi::AttributeUnsignedInt(const char* name, std::uint32_t value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

SerializerApi SerializerApi::AttributeString(const char* name, const char* value)
{
    mCurrentElement.SetAttribute(name, value);
    return *this;
}

std::string SerializerApi::ToString() const
{
    tinyxml2::XMLPrinter printer{};
    mDocument->Print(&printer);
    return printer.CStr();
}
