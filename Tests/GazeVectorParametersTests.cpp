#include "Data/GazeVector.h"
#include "ParametersTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

TEST_CASE("GetSettingsGazeVectorTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters GazeVector", data::GetGazeVectorSettingsTest, CRTPacket::PacketXML);

    bool dataAvailable = true;
    if (!protocol->ReadGazeVectorSettings(dataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(dataAvailable);

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
