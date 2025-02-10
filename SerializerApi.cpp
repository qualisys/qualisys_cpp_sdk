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

std::string SerializerApi::SetCameraSyncOutSettings(const unsigned int cameraId, const unsigned int portNumber,
    const ESyncOutFreqMode* syncOutMode, const unsigned int* syncOutValue, const float* syncOutDutyCycle,
    const bool* syncOutNegativePolarity)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    int port = portNumber - 1;
    if (((port == 0 || port == 1) && syncOutMode) || (port == 2))
    {
        tinyxml2::XMLElement* syncOutElem = nullptr;
        if (port == 0)
            syncOutElem = document.NewElement("Sync_Out");
        else if (port == 1)
            syncOutElem = document.NewElement("Sync_Out2");
        else
            syncOutElem = document.NewElement("Sync_Out_MT");

        cameraElem->InsertEndChild(syncOutElem);

        // Add Sync Out Mode
        if (port == 0 || port == 1)
        {
            tinyxml2::XMLElement* modeElem = document.NewElement("Mode");
            switch (*syncOutMode)
            {
            case ModeShutterOut:
                modeElem->SetText("Shutter out");
                break;
            case ModeMultiplier:
                modeElem->SetText("Multiplier");
                break;
            case ModeDivisor:
                modeElem->SetText("Divisor");
                break;
            case ModeIndependentFreq:
                modeElem->SetText("Camera independent");
                break;
            case ModeMeasurementTime:
                modeElem->SetText("Measurement time");
                break;
            case ModeFixed100Hz:
                modeElem->SetText("Continuous 100Hz");
                break;
            case ModeSystemLiveTime:
                modeElem->SetText("System live time");
                break;
            default:
                return "";
            }
            syncOutElem->InsertEndChild(modeElem);

            if (*syncOutMode == ModeMultiplier ||
                *syncOutMode == ModeDivisor ||
                *syncOutMode == ModeIndependentFreq)
            {
                AddXMLElementUnsignedInt(*syncOutElem, "Value", syncOutValue, document);
                AddXMLElementFloat(*syncOutElem, "Duty_Cycle", syncOutDutyCycle, 3, document);
            }
        }

        if (syncOutNegativePolarity && (port == 2 ||
            (syncOutMode && *syncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(*syncOutElem, "Signal_Polarity", syncOutNegativePolarity, document, "Negative", "Positive");
        }
    }

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string SerializerApi::SetCameraLensControlSettings(const unsigned int cameraId, const float focus, const float aperture)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    tinyxml2::XMLElement* lensControlElem = document.NewElement("LensControl");
    cameraElem->InsertEndChild(lensControlElem);

    AddXMLElementFloatWithTextAttribute(document, *lensControlElem, "Focus", "Value", focus, 6);
    AddXMLElementFloatWithTextAttribute(document, *lensControlElem, "Aperture", "Value", aperture, 6);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string SerializerApi::SetCameraAutoExposureSettings(const unsigned int cameraId, const bool autoExposure, const float compensation)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    tinyxml2::XMLElement* lensControlElem = document.NewElement("LensControl");
    cameraElem->InsertEndChild(lensControlElem);

    tinyxml2::XMLElement* autoExposureElem = document.NewElement("AutoExposure");
    autoExposureElem->SetAttribute("Enabled", autoExposure ? "true" : "false");

    char compensationStr[32];
    snprintf(compensationStr, sizeof(compensationStr), "%.6f", compensation);
    autoExposureElem->SetAttribute("Compensation", compensationStr);

    lensControlElem->InsertEndChild(autoExposureElem);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string SerializerApi::SetCameraAutoWhiteBalance(const unsigned int cameraId, const bool enable)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* generalElem = document.NewElement("General");
    rootElem->InsertEndChild(generalElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    generalElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", &cameraId, document);

    tinyxml2::XMLElement* autoWhiteBalanceElem = document.NewElement("AutoWhiteBalance");
    autoWhiteBalanceElem->SetText(enable ? "true" : "false");
    cameraElem->InsertEndChild(autoWhiteBalanceElem);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string SerializerApi::SetImageSettings(const unsigned int  cameraId, const bool* enable, const CRTPacket::EImageFormat* format,
    const unsigned int* width, const unsigned int* height, const float* leftCrop,
    const float* topCrop, const float* rightCrop, const float* bottomCrop)
{
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* imageElem = document.NewElement("Image");
    rootElem->InsertEndChild(imageElem);

    tinyxml2::XMLElement* cameraElem = document.NewElement("Camera");
    imageElem->InsertEndChild(cameraElem);

    AddXMLElementUnsignedInt(*cameraElem, "ID", cameraId, document);

    AddXMLElementBool(*cameraElem, "Enabled", enable, document);

    if (format)
    {
        const char* formatStr = nullptr;
        switch (*format)
        {
        case CRTPacket::FormatRawGrayscale:
            formatStr = "RAWGrayscale";
            break;
        case CRTPacket::FormatRawBGR:
            formatStr = "RAWBGR";
            break;
        case CRTPacket::FormatJPG:
            formatStr = "JPG";
            break;
        case CRTPacket::FormatPNG:
            formatStr = "PNG";
            break;
        }

        if (formatStr)
        {
            tinyxml2::XMLElement* formatElem = document.NewElement("Format");
            formatElem->SetText(formatStr);
            cameraElem->InsertEndChild(formatElem);
        }
    }

    AddXMLElementUnsignedInt(*cameraElem, "Width", width, document);
    AddXMLElementUnsignedInt(*cameraElem, "Height", height, document);
    AddXMLElementFloat(*cameraElem, "Left_Crop", leftCrop, 6, document);
    AddXMLElementFloat(*cameraElem, "Top_Crop", topCrop, 6, document);
    AddXMLElementFloat(*cameraElem, "Right_Crop", rightCrop, 6, document);
    AddXMLElementFloat(*cameraElem, "Bottom_Crop", bottomCrop, 6, document);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string SerializerApi::SetForceSettings(const unsigned int plateId, const SPoint* corner1, const SPoint* corner2,
    const SPoint* corner3, const SPoint* corner4)
{
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* forceElem = document.NewElement("Force");
    rootElem->InsertEndChild(forceElem);

    tinyxml2::XMLElement* plateElem = document.NewElement("Plate");
    forceElem->InsertEndChild(plateElem);

    if (mMajorVersion > 1 || mMinorVersion > 7)
    {
        AddXMLElementUnsignedInt(*plateElem, "Plate_ID", &plateId, document);
    }
    else
    {
        AddXMLElementUnsignedInt(*plateElem, "Force_Plate_Index", &plateId, document);
    }

    auto addCorner = [&](const char* name, const SPoint* pCorner)
        {
            if (pCorner)
            {
                tinyxml2::XMLElement* cornerElem = document.NewElement(name);
                plateElem->InsertEndChild(cornerElem);

                AddXMLElementFloat(*cornerElem, "X", &(pCorner->fX), 6, document);
                AddXMLElementFloat(*cornerElem, "Y", &(pCorner->fY), 6, document);
                AddXMLElementFloat(*cornerElem, "Z", &(pCorner->fZ), 6, document);
            }
        };

    addCorner("Corner1", corner1);
    addCorner("Corner2", corner2);
    addCorner("Corner3", corner3);
    addCorner("Corner4", corner4);

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

std::string SerializerApi::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& settingsSkeletons)
{
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement* rootElem = document.NewElement("QTM_Settings");
    document.InsertFirstChild(rootElem);

    tinyxml2::XMLElement* skeletonsElem = document.NewElement("Skeletons");
    rootElem->InsertEndChild(skeletonsElem);

    for (const auto& skeleton : settingsSkeletons)
    {
        tinyxml2::XMLElement* skeletonElem = document.NewElement("Skeleton");
        skeletonElem->SetAttribute("Name", skeleton.name.c_str());
        skeletonsElem->InsertEndChild(skeletonElem);

        if (mMajorVersion == 1 && mMinorVersion < 22)
        {
            tinyxml2::XMLElement* solverElem = document.NewElement("Solver");
            solverElem->SetText(skeleton.rootSegment.solver.c_str());
            skeletonElem->InsertEndChild(solverElem);
        }

        tinyxml2::XMLElement* scaleElem = document.NewElement("Scale");
        scaleElem->SetText(std::to_string(skeleton.scale).c_str());
        skeletonElem->InsertEndChild(scaleElem);

        tinyxml2::XMLElement* segmentsElem = document.NewElement("Segments");
        skeletonElem->InsertEndChild(segmentsElem);

        std::function<void(const SSettingsSkeletonSegmentHierarchical&, tinyxml2::XMLElement*)> recurseSegments;
        recurseSegments = [&](const SSettingsSkeletonSegmentHierarchical& segment, tinyxml2::XMLElement* parentElem)
            {
                tinyxml2::XMLElement* segmentElem = document.NewElement("Segment");
                segmentElem->SetAttribute("Name", segment.name.c_str());
                parentElem->InsertEndChild(segmentElem);

                if (mMajorVersion > 1 || mMinorVersion > 21)
                {
                    tinyxml2::XMLElement* solverElem = document.NewElement("Solver");
                    solverElem->SetText(segment.solver.c_str());
                    segmentElem->InsertEndChild(solverElem);
                }

                if (!std::isnan(segment.position.x))
                {
                    AddXMLElementTransform(document, *segmentElem, "Transform", segment.position, segment.rotation);
                }

                if (!std::isnan(segment.defaultPosition.x))
                {
                    AddXMLElementTransform(document, *segmentElem, "DefaultTransform", segment.defaultPosition, segment.defaultRotation);
                }

                tinyxml2::XMLElement* dofElem = document.NewElement("DegreesOfFreedom");
                segmentElem->InsertEndChild(dofElem);
                for (const auto& dof : segment.degreesOfFreedom)
                {
                    AddXMLElementDOF(document, *dofElem, SkeletonDofToStringSettings(dof.type), dof);
                }

                tinyxml2::XMLElement* endpointElem = document.NewElement("Endpoint");
                if (!std::isnan(segment.endpoint.x) && !std::isnan(segment.endpoint.y) && !std::isnan(segment.endpoint.z))
                {
                    endpointElem->SetAttribute("X", std::to_string(segment.endpoint.x).c_str());
                    endpointElem->SetAttribute("Y", std::to_string(segment.endpoint.y).c_str());
                    endpointElem->SetAttribute("Z", std::to_string(segment.endpoint.z).c_str());
                }
                segmentElem->InsertEndChild(endpointElem);

                tinyxml2::XMLElement* markersElem = document.NewElement("Markers");
                segmentElem->InsertEndChild(markersElem);
                for (const auto& marker : segment.markers)
                {
                    tinyxml2::XMLElement* markerElem = document.NewElement("Marker");
                    markerElem->SetAttribute("Name", marker.name.c_str());
                    markersElem->InsertEndChild(markerElem);

                    tinyxml2::XMLElement* positionElem = document.NewElement("Position");
                    positionElem->SetAttribute("X", std::to_string(marker.position.x).c_str());
                    positionElem->SetAttribute("Y", std::to_string(marker.position.y).c_str());
                    positionElem->SetAttribute("Z", std::to_string(marker.position.z).c_str());
                    markerElem->InsertEndChild(positionElem);

                    tinyxml2::XMLElement* weightElem = document.NewElement("Weight");
                    weightElem->SetText(std::to_string(marker.weight).c_str());
                    markerElem->InsertEndChild(weightElem);
                }

                tinyxml2::XMLElement* rigidBodiesElem = document.NewElement("RigidBodies");
                segmentElem->InsertEndChild(rigidBodiesElem);
                for (const auto& rigidBody : segment.bodies)
                {
                    tinyxml2::XMLElement* rigidBodyElem = document.NewElement("RigidBody");
                    rigidBodyElem->SetAttribute("Name", rigidBody.name.c_str());
                    rigidBodiesElem->InsertEndChild(rigidBodyElem);

                    tinyxml2::XMLElement* transformElem = document.NewElement("Transform");
                    rigidBodyElem->InsertEndChild(transformElem);

                    tinyxml2::XMLElement* positionElem = document.NewElement("Position");
                    positionElem->SetAttribute("X", std::to_string(rigidBody.position.x).c_str());
                    positionElem->SetAttribute("Y", std::to_string(rigidBody.position.y).c_str());
                    positionElem->SetAttribute("Z", std::to_string(rigidBody.position.z).c_str());
                    transformElem->InsertEndChild(positionElem);

                    tinyxml2::XMLElement* rotationElem = document.NewElement("Rotation");
                    rotationElem->SetAttribute("X", std::to_string(rigidBody.rotation.x).c_str());
                    rotationElem->SetAttribute("Y", std::to_string(rigidBody.rotation.y).c_str());
                    rotationElem->SetAttribute("Z", std::to_string(rigidBody.rotation.z).c_str());
                    rotationElem->SetAttribute("W", std::to_string(rigidBody.rotation.w).c_str());
                    transformElem->InsertEndChild(rotationElem);

                    tinyxml2::XMLElement* weightElem = document.NewElement("Weight");
                    weightElem->SetText(std::to_string(rigidBody.weight).c_str());
                    rigidBodyElem->InsertEndChild(weightElem);
                }

                for (const auto& childSegment : segment.segments)
                {
                    recurseSegments(childSegment, segmentElem);
                }
            };

        recurseSegments(skeleton.rootSegment, segmentsElem);
    }

    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}
