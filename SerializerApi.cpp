#include "SerializerApi.h"

#include <tinyxml2.h>
#include <functional>

using namespace qualisys_cpp_sdk;

SerializerApi::SerializerApi(std::uint32_t majorVersion, std::uint32_t minorVersion)
    : mMajorVersion(majorVersion), mMinorVersion(minorVersion), mDocument(new tinyxml2::XMLDocument)
{
     mCurrentElement = mDocument->RootElement();
}

void SerializerApi::AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* elementName, const bool* value, tinyxml2::XMLDocument& document, const char* trueText, const char* falseText)
{
    if (value)
    {
        tinyxml2::XMLElement* elem = document.NewElement(elementName);
        elem->SetText(*value ? trueText : falseText);
        parentElem.InsertEndChild(elem);
    }
}

void SerializerApi::AddXMLElementBool(tinyxml2::XMLElement& parentElem, const char* elementName, const bool value, tinyxml2::XMLDocument& document, const char* trueText, const char* falseText)
{
    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetText(value ? trueText : falseText);
    parentElem.InsertEndChild(elem);
}

void SerializerApi::AddXMLElementInt(tinyxml2::XMLElement& parentElem, const char* elementName, const int* value, tinyxml2::XMLDocument& document)
{
    if (value)
    {
        tinyxml2::XMLElement* elem = document.NewElement(elementName);
        elem->SetText(*value);
        parentElem.InsertEndChild(elem);
    }
}

void SerializerApi::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* elementName, const unsigned int value, tinyxml2::XMLDocument& document)
{
    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetText(value);
    parentElem.InsertEndChild(elem);
}

void SerializerApi::AddXMLElementUnsignedInt(tinyxml2::XMLElement& parentElem, const char* elementName, const unsigned int* value, tinyxml2::XMLDocument& document)
{
    if (value)
    {
        AddXMLElementUnsignedInt(parentElem, elementName, *value, document);
    }
}

void SerializerApi::AddXMLElementFloat(tinyxml2::XMLElement& parentElem, const char* elementName, const float* value, unsigned int decimals, tinyxml2::XMLDocument& document)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, *value);

    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetText(formattedValue);
    parentElem.InsertEndChild(elem);
}

void SerializerApi::AddXMLElementFloatWithTextAttribute(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const char* elementName, const char* attributeName, const float& value, unsigned int decimals)
{
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%.*f", decimals, value);

    tinyxml2::XMLElement* elem = document.NewElement(elementName);
    elem->SetAttribute(attributeName, formattedValue);
    parentElem.InsertEndChild(elem);
}

void SerializerApi::AddXMLElementTransform(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SPosition& position, const SRotation& rotation)
{
    tinyxml2::XMLElement* transformElem = document.NewElement(name.c_str());
    parentElem.InsertEndChild(transformElem);

    tinyxml2::XMLElement* positionElem = document.NewElement("Position");
    positionElem->SetAttribute("X", std::to_string(position.x).c_str());
    positionElem->SetAttribute("Y", std::to_string(position.y).c_str());
    positionElem->SetAttribute("Z", std::to_string(position.z).c_str());
    transformElem->InsertEndChild(positionElem);

    tinyxml2::XMLElement* rotationElem = document.NewElement("Rotation");
    rotationElem->SetAttribute("X", std::to_string(rotation.x).c_str());
    rotationElem->SetAttribute("Y", std::to_string(rotation.y).c_str());
    rotationElem->SetAttribute("Z", std::to_string(rotation.z).c_str());
    rotationElem->SetAttribute("W", std::to_string(rotation.w).c_str());
    transformElem->InsertEndChild(rotationElem);
}

void SerializerApi::AddXMLElementDOF(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& parentElem, const std::string& name, const SDegreeOfFreedom& degreesOfFreedom)
{
    tinyxml2::XMLElement* dofElem = document.NewElement(name.c_str());
    parentElem.InsertEndChild(dofElem);

    if (!std::isnan(degreesOfFreedom.lowerBound) && !std::isnan(degreesOfFreedom.upperBound))
    {
        if (mMajorVersion > 1 || mMinorVersion > 21)
        {
            tinyxml2::XMLElement* constraintElem = document.NewElement("Constraint");
            constraintElem->SetAttribute("LowerBound", std::to_string(degreesOfFreedom.lowerBound).c_str());
            constraintElem->SetAttribute("UpperBound", std::to_string(degreesOfFreedom.upperBound).c_str());
            dofElem->InsertEndChild(constraintElem);
        }
        else
        {
            // If not in a 'Constraint' block, add 'LowerBound' & 'UpperBound' directly to dofElem
            dofElem->SetAttribute("LowerBound", std::to_string(degreesOfFreedom.lowerBound).c_str());
            dofElem->SetAttribute("UpperBound", std::to_string(degreesOfFreedom.upperBound).c_str());
        }
    }

    if (!degreesOfFreedom.couplings.empty())
    {
        tinyxml2::XMLElement* couplingsElem = document.NewElement("Couplings");
        dofElem->InsertEndChild(couplingsElem);

        for (const auto& coupling : degreesOfFreedom.couplings)
        {
            tinyxml2::XMLElement* couplingElem = document.NewElement("Coupling");
            couplingElem->SetAttribute("Segment", coupling.segment.c_str());
            couplingElem->SetAttribute("DegreeOfFreedom", SkeletonDofToStringSettings(coupling.degreeOfFreedom));
            couplingElem->SetAttribute("Coefficient", std::to_string(coupling.coefficient).c_str());
            couplingsElem->InsertEndChild(couplingElem);
        }
    }

    if (!std::isnan(degreesOfFreedom.goalValue) && !std::isnan(degreesOfFreedom.goalWeight))
    {
        tinyxml2::XMLElement* goalElem = document.NewElement("Goal");
        goalElem->SetAttribute("Value", std::to_string(degreesOfFreedom.goalValue).c_str());
        goalElem->SetAttribute("Weight", std::to_string(degreesOfFreedom.goalWeight).c_str());
        dofElem->InsertEndChild(goalElem);
    }
}
