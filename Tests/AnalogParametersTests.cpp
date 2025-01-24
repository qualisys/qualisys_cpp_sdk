#include "Data/Analog.h"
#include "ParametersTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

namespace
{
    bool VerifySettingsAnalog(const std::vector<CRTProtocol::SAnalogDevice>& analogSettings)
    {
        std::vector<std::string> expectedLabels = { "fx", "fy", "fz", "mx", "my", "mz", "trigger", "aux", "zero", "sync" };
        std::vector<std::string> expectedUnits = { "newtons", "newtons", "newtons", "newtonmetre", "newtonmetre", "newtonmetre", "", "", "", "" };

        CHECK_EQ(1, analogSettings.size());
        CHECK_EQ(1u, analogSettings[0].nDeviceID);
        CHECK_EQ(10u, analogSettings[0].nChannels);
        CHECK_EQ("Force plate 1", analogSettings[0].oName);

        for (int i = 0; i < analogSettings[0].voLabels.size(); i++)
        {
            CHECK_EQ(expectedLabels[i], analogSettings[0].voLabels[i]);
            CHECK_EQ(expectedUnits[i], analogSettings[0].voUnits[i]);
        }

        CHECK_EQ(100u, analogSettings[0].nFrequency);
        CHECK_EQ("", analogSettings[0].oUnit);
        CHECK_EQ(0.0f, analogSettings[0].fMinRange);
        CHECK_EQ(0.0f, analogSettings[0].fMaxRange);

        return true;
    }
}


TEST_CASE("GetSettingsAnalogTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters Analog", qualisys_cpp_sdk::tests::data::GetAnalogSettingsTest, CRTPacket::PacketXML);

    bool dataAvailable = true;
    if (!protocol->ReadAnalogSettings(dataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(dataAvailable);

    std::vector<CRTProtocol::SAnalogDevice> analogSettings;
    protocol->GetAnalogSettings(analogSettings);

    CHECK(VerifySettingsAnalog(analogSettings));
}