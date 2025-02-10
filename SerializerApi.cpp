#include "SerializerApi.h"

#include <tinyxml2.h>
#include <functional>

using namespace qualisys_cpp_sdk;

SerializerApi::SerializerApi(const SerializerApi& src, tinyxml2::XMLElement* element): mDocument(src.mDocument), mCurrentElement(element), mMajorVersion(src.mMajorVersion), mMinorVersion(src.mMinorVersion)
{
}

SerializerApi SerializerApi::Element(const char* name)
{
    if (mCurrentElement)
    {
        return SerializerApi{ *this, mCurrentElement->InsertNewChildElement(name) };
    }

    auto newElement = mDocument->NewElement(name);
    mDocument->InsertFirstChild(newElement);
    return SerializerApi{ *this, newElement};
}

SerializerApi SerializerApi::ElementBool(const char* name, bool value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement->SetText(value);
    return SerializerApi{ *this, newElement.mCurrentElement };
}

SerializerApi SerializerApi::ElementInt(const char* name, std::int32_t value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement->SetText(value);
    return SerializerApi{ *this, newElement.mCurrentElement };
}

SerializerApi SerializerApi::ElementFloat(const char* name, float value, int decimals)
{
    char formattedValue[32];
    (void)snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);
    auto newElement = Element(name);
    newElement.mCurrentElement->SetText(formattedValue);
    return SerializerApi{ *this, newElement.mCurrentElement };
}

SerializerApi SerializerApi::ElementDouble(const char* name, double value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement->SetText(value);
    return SerializerApi{ *this, newElement.mCurrentElement };
}

SerializerApi SerializerApi::ElementUnsignedInt(const char* name, std::uint32_t value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement->SetText(value);
    return SerializerApi{ *this, newElement.mCurrentElement };
}

SerializerApi SerializerApi::ElementString(const char* name, const char* value)
{
    auto newElement = Element(name);
    newElement.mCurrentElement->SetText(value);
    return SerializerApi{ *this, newElement.mCurrentElement };
}

SerializerApi SerializerApi::AttributeBool(const char* name, bool value)
{
    mCurrentElement->SetAttribute(name, value);
    return *this;
}

SerializerApi SerializerApi::AttributeInt(const char* name, std::int32_t value)
{
    mCurrentElement->SetAttribute(name, value);
    return *this;
}

SerializerApi SerializerApi::AttributeFloat(const char* name, float value, int decimals)
{
    char formattedValue[32];
    (void)snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);
    mCurrentElement->SetAttribute(name, formattedValue);
    return *this;
}

SerializerApi SerializerApi::AttributeDouble(const char* name, double value, int decimals)
{
    char formattedValue[32];
    (void)snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);
    mCurrentElement->SetAttribute(name, formattedValue);
    return *this;
}

SerializerApi SerializerApi::AttributeUnsignedInt(const char* name, std::uint32_t value)
{
    mCurrentElement->SetAttribute(name, value);
    return *this;
}

SerializerApi SerializerApi::AttributeString(const char* name, const char* value)
{
    mCurrentElement->SetAttribute(name, value);
    return *this;
}

std::string SerializerApi::ToString() const
{
    tinyxml2::XMLPrinter printer{};
    mDocument->Print(&printer);
    return printer.CStr();
}

SerializerApi::SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mDocument(new tinyxml2::XMLDocument)
{
     mCurrentElement = mDocument->RootElement();
}
