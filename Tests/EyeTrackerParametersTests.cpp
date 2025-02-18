#include "Data/EyeTracker.h"
#include "ParametersTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

TEST_CASE("GetSettingsEyeTrackerTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters EyeTracker", data::GetEyeTrackerSettingsTest, CRTPacket::PacketXML);

    bool dataAvailable = true;
    if (!protocol->ReadEyeTrackerSettings(dataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(dataAvailable);

    std::vector<CRTProtocol::SEyeTracker> eyeTrackerSettings;
    protocol->GetEyeTrackerSettings(eyeTrackerSettings);

    CHECK_EQ(1, eyeTrackerSettings.size());
    CHECK_EQ("EyeTrackerDevice", eyeTrackerSettings[0].name);
    CHECK_EQ(240.0f, eyeTrackerSettings[0].frequency);
    CHECK_EQ(true, eyeTrackerSettings[0].hwSync);
}
