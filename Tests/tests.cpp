#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <tinyxml2.h>
#include "../Network.h"
#include "../RTProtocol.h"
#include "TestUtils.h"
#include <iostream>

TEST_CASE("") {
    // Code that writes to std::cout
    //std::cout.rdbuf(debugOut.rdbuf());
    std::cout << "This will always show!" << std::endl;

    CRTProtocol protocol{};
    CHECK(protocol.Connect("127.0.0.1"));
    std::cout << "Connected" << std::endl;
    std::string output;
    
    CHECK(false);
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