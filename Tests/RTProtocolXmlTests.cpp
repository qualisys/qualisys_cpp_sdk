#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <tinyxml2.h>
#include "../Network.h"
#include "../RTProtocol.h"
#include "TestUtils.h"
#include "XmlTestData.h"
#include <iostream>


TEST_CASE("GetGeneralSettingsTest") {
    auto versionString = std::string{ "Version set to " + std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION) };
    auto networkDummy = new qualisys_cpp_sdk::test_utils::DummyXmlReceiverNetwork{};

    CRTProtocol protocol{};
    protocol.OverrideNetwork(networkDummy);

    if (!protocol.Connect(""))
    {
        FAIL(protocol.GetErrorString());
    }
   
    networkDummy->QueueResponse(qualisys_cpp_sdk::xml_test_data::generalSettingsXml, CRTPacket::EPacketType::PacketXML);

    if (!protocol.ReadGeneralSettings())
    {
        FAIL(protocol.GetErrorString());
    }

    unsigned int nCaptureFrequency;
    float fCaptureTime;
    bool bStartOnExtTrig;
    bool startOnTrigNO; 
    bool startOnTrigNC;
    bool startOnTrigSoftware;
    CRTProtocol::EProcessingActions eProcessingActions;
    CRTProtocol::EProcessingActions eRtProcessingActions;
    CRTProtocol::EProcessingActions eReprocessingActions;

    protocol.GetGeneralSettings(
    nCaptureFrequency,
    fCaptureTime,
    bStartOnExtTrig,
    startOnTrigNO,
    startOnTrigNC,
    startOnTrigSoftware,
    eProcessingActions,
    eRtProcessingActions,
    eReprocessingActions
    );

    CHECK_EQ(100, nCaptureFrequency);
}


//
//TEST_CASE("Test XML Parsing") {
//
//    auto* network = new DummyXmlReceiverNetwork{};
//
//    CRTProtocol protocol{};
//
//    protocol.OverrideNetwork(network);
//
//    network->SetXmlData("");
//    
//    protocol.ReadGeneralSettings();
//
//}