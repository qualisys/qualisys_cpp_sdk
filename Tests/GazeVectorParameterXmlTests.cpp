#include "TestUtils.h"
#include "../RTProtocol.h"
#include "Data/EyeTracker.h"

#include "Data/Gaze.h"
#include "doctest/doctest.h"
#include <memory>

using namespace qualisys_cpp_sdk::tests;

TEST_CASE("GetSettingsGazeVectorTest")
{
    auto [protocol, network] = qualisys_cpp_sdk::test_utils::CreateTestContext();

    network->PrepareResponse("GetParameters GazeVector", data::GetGazeVectorSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadGazeVectorSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SGazeVector> gazeVectorSettings;
    protocol->GetGazeVectorSettings(gazeVectorSettings);

    CHECK_EQ("Gaze vector 1 (L)", gazeVectorSettings[0].name);
    CHECK_EQ(240.0f, gazeVectorSettings[0].frequency);
    CHECK_EQ(true, gazeVectorSettings[0].hwSync);
    CHECK_EQ(false, gazeVectorSettings[0].filter);
    CHECK_EQ("Gaze vector 1 (R)", gazeVectorSettings[1].name);
    CHECK_EQ(240.0f, gazeVectorSettings[1].frequency);
    CHECK_EQ(true, gazeVectorSettings[1].hwSync);
    CHECK_EQ(false, gazeVectorSettings[1].filter);
}

TEST_CASE("GetSettingsEyeTrackerTest")
{
    auto [protocol, network] = qualisys_cpp_sdk::test_utils::CreateTestContext();

    network->PrepareResponse("GetParameters EyeTracker", data::GetEyeTrackerSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadEyeTrackerSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SEyeTracker> eyeTrackerSettings;
    protocol->GetEyeTrackerSettings(eyeTrackerSettings);

    CHECK_EQ(1, eyeTrackerSettings.size());
    CHECK_EQ("EyeTrackerDevice", eyeTrackerSettings[0].name);
    CHECK_EQ(240.0f, eyeTrackerSettings[0].frequency);
    CHECK_EQ(true, eyeTrackerSettings[0].hwSync);
}
