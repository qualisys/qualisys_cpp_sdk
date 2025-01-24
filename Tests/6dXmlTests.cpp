#include "Data/6d.h"
#include "XmlTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

namespace
{
    bool Settings6DMeshEqualityOperator(const CRTProtocol::SSettings6DMesh& lhs, const CRTProtocol::SSettings6DMesh& rhs)
    {
        if (lhs.name != rhs.name ||
            lhs.opacity != rhs.opacity ||
            lhs.position.fX != rhs.position.fX ||
            lhs.position.fY != rhs.position.fY ||
            lhs.position.fZ != rhs.position.fZ ||
            lhs.rotation.fX != rhs.rotation.fX ||
            lhs.rotation.fY != rhs.rotation.fY ||
            lhs.rotation.fZ != rhs.rotation.fZ ||
            lhs.scale != rhs.scale)
        {
            return false;
        }
        return true;
    }

    bool OriginEqualityOperator(const CRTProtocol::SOrigin& lhs, const CRTProtocol::SOrigin& rhs)
    {
        if (lhs.position.fX != rhs.position.fX ||
            lhs.position.fY != rhs.position.fY ||
            lhs.position.fZ != rhs.position.fZ ||
            lhs.relativeBody != rhs.relativeBody ||
            lhs.rotation[0] != rhs.rotation[0] ||
            lhs.rotation[1] != rhs.rotation[1] ||
            lhs.rotation[2] != rhs.rotation[2] ||
            lhs.rotation[3] != rhs.rotation[3] ||
            lhs.rotation[4] != rhs.rotation[4] ||
            lhs.rotation[5] != rhs.rotation[5] ||
            lhs.rotation[6] != rhs.rotation[6] ||
            lhs.rotation[7] != rhs.rotation[7] ||
            lhs.rotation[8] != rhs.rotation[8] ||
            lhs.type != rhs.type)
        {
            return false;
        }

        return true;
    }

    bool BodyPointsVectorEqualityOperator(const std::vector<CRTProtocol::SBodyPoint>& lhs, const std::vector<CRTProtocol::SBodyPoint>& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (int i = 0; i < lhs.size(); i++)
        {
            if (lhs[i].name != rhs[i].name ||
                lhs[i].fX != rhs[i].fX ||
                lhs[i].fX != rhs[i].fX ||
                lhs[i].fX != rhs[i].fX ||
                lhs[i].virtual_ != rhs[i].virtual_ ||
                lhs[i].physicalId != rhs[i].physicalId)
            {
                return false;
            }
        }


        return true;
    }

    bool VerifySettings6DOF(const std::vector<CRTProtocol::SSettings6DOFBody>& settings6DOF)
    {
        std::vector<std::string> expectedNames = {
            "refined", "Table", "Screen2",
            "Cup", "Phone", "Screen1"
        };
        std::vector<std::uint32_t> expectedColors = { 65280, 16711680, 16711935, 65535, 16776960, 127 };
        std::vector<float> expectedMaxResiduals = { 20.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
        std::vector<float> expectedBoneLengthTolerances = { 20.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
        CRTProtocol::SSettings6DMesh arqusMesh{ "Arqus.obj", {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.00999999978f, 1.0f };
        CRTProtocol::SSettings6DMesh emptyMesh{ "", {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f };
        std::vector<CRTProtocol::SSettings6DMesh> expectedMeshes = { arqusMesh, emptyMesh, emptyMesh, emptyMesh, emptyMesh, emptyMesh };
        CRTProtocol::SOrigin defaultOrigin{
            CRTProtocol::EOriginType::GlobalOrigin, 1, CRTProtocol::SPoint{ 0.0f, 0.0f, 0.0f },
            { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f }
        };
        std::vector<std::vector<CRTProtocol::SBodyPoint>> expectedPoints = {
            {
            CRTProtocol::SBodyPoint{ "Layout 3 - 1", -67.9271545f, 29.8100796f, -1.59297097f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 2", -95.0230560f, 1.74239099f, -19.5413551f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 3", -90.0171738f, 19.8888149f, -57.4946556f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 4", 68.3161087f, 29.1735229f, -1.82902002f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 5", 95.0272446f, 1.47444904f, -19.1718693f, false, 0 },
            CRTProtocol::SBodyPoint{ "Layout 3 - 6", 89.6240387f, 20.6907444f, -57.4501305f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Table - 1", 801.421997f, 29.8100796f, 2.14241600f, false, 0 },
            CRTProtocol::SBodyPoint{ "Table - 2", 777.315125f, -413.341278f, 4.12720299f, false, 0 },
            CRTProtocol::SBodyPoint{ "Table - 3", -778.145569f, 412.610626f, -2.27826405f, false, 0 },
            CRTProtocol::SBodyPoint{ "Table - 4", -800.591614f, -368.158539f, -3.99135494f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Screen2 - 1", 177.895599f, -40.2035255f, -276.061218f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen2 - 2", -176.929245f, 41.0591507f, 276.110107f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen2 - 3", -111.494820f, 129.602188f, -283.761566f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen2 - 4", 110.528465f, -130.457809f, 283.712677f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Cup - 1", -16.4094849f, 43.2384529f, -7.37500811f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 2", 41.2375832f, 9.39587307f, -14.9948711f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 3", -6.34368896f, -32.7491074f, -24.3911552f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 4", 27.2115021f, -35.5109825f, 31.2080975f, false, 0 },
            CRTProtocol::SBodyPoint{ "Cup - 5", -45.6959114f, 15.6257639f, 15.5529385f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Phone - 1", -39.9740448f, -68.1382675f, -0.101278998f, false, 0 },
            CRTProtocol::SBodyPoint{ "Phone - 2", 43.3482971f, 64.0306702f, -2.40792704f, false, 0 },
            CRTProtocol::SBodyPoint{ "Phone - 3", -31.9067173f, 75.0699539f, 1.21390605f, false, 0 },
            CRTProtocol::SBodyPoint{ "Phone - 4", 28.5324669f, -70.9623566f, 1.29529905f, false, 0 }
            },
            {
            CRTProtocol::SBodyPoint{ "Screen - 1", -395.500610f, 5.13963413f, -195.481262f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen - 2", 392.106415f, -2.13840294f, 196.007095f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen - 3", 402.905762f, 26.1865349f, -178.268143f, false, 0 },
            CRTProtocol::SBodyPoint{ "Screen - 4", -399.511597f, -29.1877670f, 177.742310f, false, 0 }
            }
        };

        CHECK_EQ(settings6DOF.size(), 6);

        for (int i = 0; i < settings6DOF.size(); i++)
        {
            CHECK_EQ(settings6DOF[i].name, expectedNames[i]);
            CHECK_EQ(settings6DOF[i].enabled, true);
            CHECK_EQ(settings6DOF[i].color, expectedColors[i]);
            CHECK_EQ(settings6DOF[i].filterPreset, "No filter");
            CHECK_EQ(settings6DOF[i].maxResidual, expectedMaxResiduals[i]);
            CHECK_EQ(settings6DOF[i].minMarkersInBody, 3);
            CHECK_EQ(settings6DOF[i].boneLengthTolerance, expectedBoneLengthTolerances[i]);
            CHECK_EQ(Settings6DMeshEqualityOperator(settings6DOF[i].mesh, expectedMeshes[i]), true);
            CHECK_EQ(OriginEqualityOperator(settings6DOF[i].origin, defaultOrigin), true);
            CHECK_EQ(BodyPointsVectorEqualityOperator(settings6DOF[i].points, expectedPoints[i]), true);
        }

        return true;
    }
}

TEST_CASE("SetSettings6DOFTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    CRTProtocol::SBodyPoint bodyPoint{ "bodyName", 41.0f, 42.0f, 43.0f, false, 65 };

    CRTProtocol::SSettings6DOFBody settings = {
        "test", true, 123, "false", 999.0f, 321, 888.0f,
        CRTProtocol::SSettings6DMesh {
            "test2",
            CRTProtocol::SPoint { 1.0f, 2.0f, 3.0f },
            CRTProtocol::SPoint { 4.0f, 5.0f, 6.0f },
            22.0f, 11.0f },
        CRTProtocol::SOrigin {
            CRTProtocol::EOriginType::GlobalOrigin, 7, CRTProtocol::SPoint { 7.0f, 8.0f, 9.0f },
            { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f },
        }, { bodyPoint }
    };

    if (!protocol->Set6DOFBodySettings({ settings }))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(true, utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::Set6DSettingsTest, network->ReadSentData().data()));
}

TEST_CASE("GetSettings6DOFTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters 6D", qualisys_cpp_sdk::tests::data::Get6DSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->Read6DOFSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    std::vector<CRTProtocol::SSettings6DOFBody> settings6DOF;

    protocol->Get6DOFBodySettings(settings6DOF);

    CHECK_EQ(true, VerifySettings6DOF(settings6DOF));
}
