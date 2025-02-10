#include "SerializerApi.h"

#include <tinyxml2.h>
#include <functional>

using namespace qualisys_cpp_sdk;

SerializerApi::SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mDocument(new tinyxml2::XMLDocument)
{
     mCurrentElement = mDocument->RootElement();
}
