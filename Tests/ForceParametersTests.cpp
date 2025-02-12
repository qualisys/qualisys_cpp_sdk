#include "Data/Force.h"
#include "ParametersTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

namespace
{
    bool VerifyForceSettings(const CRTProtocol::SSettingsForce& forceSettings)
    {
        std::vector<CRTProtocol::SForcePlate> expectedPlates = {
            CRTProtocol::SForcePlate {
                1, 1, "Kistler", "Force-plate 1", 1000, 600.0f, 400.0f,
                { CRTProtocol::SPoint { 4.66907978f, 3.01687002f, 1.71506000f },
                  CRTProtocol::SPoint { 3.84404993f, 394.877991f, 1.08413005f },
                  CRTProtocol::SPoint { 593.901978f, 395.980011f, -0.707184970f },
                  CRTProtocol::SPoint { 594.333008f, 2.39286995f, -0.577087998f }
                }, CRTProtocol::SPoint { 120.0f, 200.0f, 63.0f },
                { CRTProtocol::SForceChannel { 1, -263.227173f },
                  CRTProtocol::SForceChannel { 2, -262.536102f },
                  CRTProtocol::SForceChannel { 3, -262.536102f },
                  CRTProtocol::SForceChannel { 4, -262.123199f },
                  CRTProtocol::SForceChannel { 5, -513.610657f },
                  CRTProtocol::SForceChannel { 6, -515.729736f },
                  CRTProtocol::SForceChannel { 7, -512.557678f },
                  CRTProtocol::SForceChannel { 8, -512.557678f },
                }, false,
                {
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
                }, 6, 6
            },
            CRTProtocol::SForcePlate {
                2, 1, "Kistler", "Force-plate 2", 1000, 600.0f, 400.0f,
                { CRTProtocol::SPoint { 606.708984f, 2.52706003f, -0.719672978f },
                  CRTProtocol::SPoint { 608.502014f, 397.065002f, -0.878275990f },
                  CRTProtocol::SPoint { 1200.28003f, 396.040985f, -2.18829012f },
                  CRTProtocol::SPoint { 1200.19995f, 3.80520010f, -1.57172000f }
                }, CRTProtocol::SPoint { 120.0f, 200.0f, 63.0f },
                { CRTProtocol::SForceChannel { 9, -263.227173f },
                  CRTProtocol::SForceChannel { 10, -262.536102f },
                  CRTProtocol::SForceChannel { 11, -262.605042f },
                  CRTProtocol::SForceChannel { 12, -262.881195f },
                  CRTProtocol::SForceChannel { 13, -513.083618f },
                  CRTProtocol::SForceChannel { 14, -512.557678f },
                  CRTProtocol::SForceChannel { 15, -513.083618f },
                  CRTProtocol::SForceChannel { 16, -513.347046f },
                }, true,
                {
                    {1.996960f,-1.044330f,3.445170f,-2.529840f,0.122704f,-49.981499f,52.937000f,0.123107f},
                    {52.937000f,0.123107f,-1.957380f,-2.063730f,0.611111f,2.067350f,49.304401f,0.559802f},
                    {49.304401f,0.559802f,-0.592905f,49.466301f,-105.681999f,-103.581001f,-105.087997f,-105.438004f},
                    {-105.087997f,-105.438004f,-0.596136f,-0.043322f,-0.245594f,-0.506804f,19.028299f,18.703899f},
                    {19.028299f,18.703899f,-19.228001f,-19.119301f,0.000000f,0.000000f,0.000000f,0.000000f},
                    {0.000000f,0.000000f,19.713499f,-18.906700f,19.650000f,-19.800900f,0.000000f,0.000000f}
                },
                6,
                8
            }
        };

        CHECK_EQ("N", forceSettings.oUnitForce);
        CHECK_EQ("mm", forceSettings.oUnitLength);
        CHECK_EQ(2, forceSettings.vsForcePlates.size());

        for (std::size_t i = 0; i < forceSettings.vsForcePlates.size(); i++)
        {
            CHECK_EQ(expectedPlates[i].nID, forceSettings.vsForcePlates[i].nID);
            CHECK_EQ(expectedPlates[i].nAnalogDeviceID, forceSettings.vsForcePlates[i].nAnalogDeviceID);
            CHECK_EQ(expectedPlates[i].oType, forceSettings.vsForcePlates[i].oType);
            CHECK_EQ(expectedPlates[i].oName, forceSettings.vsForcePlates[i].oName);
            CHECK_EQ(expectedPlates[i].nFrequency, forceSettings.vsForcePlates[i].nFrequency);
            CHECK_EQ(expectedPlates[i].fLength, forceSettings.vsForcePlates[i].fLength);
            CHECK_EQ(expectedPlates[i].fWidth, forceSettings.vsForcePlates[i].fWidth);
            for (std::size_t j = 0; j < 4; j++)
            {
                CHECK_EQ(expectedPlates[i].asCorner[j].fX, forceSettings.vsForcePlates[i].asCorner[j].fX);
                CHECK_EQ(expectedPlates[i].asCorner[j].fY, forceSettings.vsForcePlates[i].asCorner[j].fY);
                CHECK_EQ(expectedPlates[i].asCorner[j].fZ, forceSettings.vsForcePlates[i].asCorner[j].fZ);
            }
            CHECK_EQ(expectedPlates[i].sOrigin.fX, forceSettings.vsForcePlates[i].sOrigin.fX);
            CHECK_EQ(expectedPlates[i].sOrigin.fY, forceSettings.vsForcePlates[i].sOrigin.fY);
            CHECK_EQ(expectedPlates[i].sOrigin.fZ, forceSettings.vsForcePlates[i].sOrigin.fZ);
            for (std::size_t j = 0; j < 8; j++)
            {
                CHECK_EQ(((8 * i) + (j + 1)), forceSettings.vsForcePlates[i].vChannels[j].nChannelNumber);
                CHECK_EQ(expectedPlates[i].vChannels[j].fConversionFactor, forceSettings.vsForcePlates[i].vChannels[j].fConversionFactor);
            }
            CHECK_EQ(expectedPlates[i].bValidCalibrationMatrix, forceSettings.vsForcePlates[i].bValidCalibrationMatrix);
            for (std::size_t j = 0; j < expectedPlates[i].nCalibrationMatrixRows; j++)
            {
                for (std::size_t k = 0; k < expectedPlates[i].nCalibrationMatrixColumns; k++)
                {
                    CHECK_EQ(expectedPlates[i].afCalibrationMatrix[j][k], forceSettings.vsForcePlates[i].afCalibrationMatrix[j][k]);
                }
            }
            CHECK_EQ(expectedPlates[i].nCalibrationMatrixRows, forceSettings.vsForcePlates[i].nCalibrationMatrixRows);
            CHECK_EQ(expectedPlates[i].nCalibrationMatrixColumns, forceSettings.vsForcePlates[i].nCalibrationMatrixColumns);
        }

        return true;
    }
}

TEST_CASE("SetSettingsForceTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    unsigned int nPlateID = 999;
    CRTProtocol::SPoint psCorner1 = { 1.0f, 2.0f, 3.0f };
    CRTProtocol::SPoint psCorner2 = { 4.0f, 5.0f, 6.0f };
    CRTProtocol::SPoint psCorner3 = { 7.0f, 8.0f, 9.0f };
    CRTProtocol::SPoint psCorner4 = { 10.0f, 11.0f, 12.0f };

    if (!protocol->SetForceSettings(
        nPlateID, &psCorner1, &psCorner2,
        &psCorner3, &psCorner4))
    {
        FAIL(protocol->GetErrorString());
    }

    auto testData = network->ReadSentData();
    CHECK(utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetForceSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetSettingsForceTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters Force", qualisys_cpp_sdk::tests::data::GetForceSettingsTest, CRTPacket::PacketXML);

    bool dataAvailable = true;
    if (!protocol->ReadForceSettings(dataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(dataAvailable);

    CRTProtocol::SSettingsForce forceSettings;
    protocol->GetForceSettings(forceSettings);

    CHECK(VerifyForceSettings(forceSettings));
}
