#include "Data/3d.h"
#include "XmlTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

namespace
{
    bool Verify3DLabels(const std::vector<CRTProtocol::SSettings3DLabel>& labels3D)
    {
        std::vector<std::string> expectedLabels = {
            "VF_LeftShoulder", "VF_Spine", "VF_RightShoulder", "VF_HeadL", "VF_HeadTop", "VF_HeadR", "VF_HeadFront",
            "VF_LShoulderTop", "VF_LShoulderBack", "VF_LArm", "VF_LElbowOut", "VF_LWristOut", "VF_LWristIn", "VF_LHandOut",
            "VF_RShoulderTop", "VF_RShoulderBack", "VF_RArm", "VF_RElbowOut", "VF_RWristOut", "VF_RWristIn", "VF_RHandOut",
            "VF_Chest", "VF_SpineTop", "VF_BackL", "VF_BackR", "VF_WaistLFront", "VF_WaistLBack", "VF_WaistRBack",
            "VF_WaistRFront", "VF_LThigh", "VF_LKneeOut", "VF_LShin", "VF_LAnkleOut", "VF_LHeelBack", "VF_LForefootOut",
            "VF_LToeTip", "VF_LForefootIn", "VF_RThigh", "VF_RKneeOut", "VF_RShin", "VF_RAnkleOut", "VF_RHeelBack",
            "VF_RForefootOut", "VF_RToeTip", "VF_RForefootIn", "Eye Tracker Layout 3 - 1", "Eye Tracker Layout 3 - 2",
            "Eye Tracker Layout 3 - 3", "Eye Tracker Layout 3 - 4", "Eye Tracker Layout 3 - 5", "Eye Tracker Layout 3 - 6",
            "Table - 1", "Table - 2", "Table - 3", "Table - 4", "Screen2 - 1", "Screen2 - 2", "Screen2 - 3", "Screen2 - 4",
            "Cup - 1", "Cup - 2", "Cup - 3", "Cup - 4", "Cup - 5", "Phone - 1", "Phone - 2", "Phone - 3", "Phone - 4",
            "Screen - 1","Screen - 2","Screen - 3", "Screen - 4"
        };

        if (expectedLabels.size() != labels3D.size())
        {
            return false;
        }

        for (int i = 0; i < labels3D.size(); i++)
        {
            if (labels3D[i].oName != expectedLabels[i])
            {
                return false;
            }
        }

        return true;
    }

    bool Verify3DBones(const std::vector<CRTProtocol::SSettingsBone>& bones)
    {
        std::vector<std::tuple<std::string, std::string>> expectedFromAndToBoneNames = {
            {"VF_WaistLBack", "VF_WaistRBack"}, {"VF_LWristOut", "VF_LWristIn"}, {"VF_RWristOut", "VF_RWristIn"},
            {"VF_LWristOut", "VF_LHandOut"}, {"VF_LToeTip", "VF_LForefootIn"}, {"VF_RToeTip", "VF_RForefootIn"},
            {"VF_RForefootOut", "VF_RToeTip"}, {"VF_RAnkleOut", "VF_RHeelBack"}, {"VF_RWristOut", "VF_RHandOut"},
            {"VF_LForefootOut", "VF_LToeTip"}, {"VF_LAnkleOut", "VF_LHeelBack"}, {"VF_RArm", "VF_RElbowOut"},
            {"VF_RKneeOut", "VF_RShin"}, {"VF_LKneeOut", "VF_LShin"}, {"VF_LShoulderTop", "VF_LShoulderBack"},
            {"VF_HeadL", "VF_HeadTop"}, {"VF_RShoulderTop", "VF_RShoulderBack"}, {"VF_HeadTop", "VF_HeadR"},
            {"VF_LArm", "VF_LElbowOut"}, {"VF_HeadTop", "VF_HeadFront"}, {"VF_RThigh", "VF_RKneeOut"},
            {"VF_LThigh", "VF_LKneeOut"}, {"VF_WaistLFront", "VF_WaistRFront"}, {"VF_WaistLFront", "VF_WaistLBack"},
            {"VF_LElbowOut", "VF_LWristOut"}, {"VF_RElbowOut", "VF_RWristOut"}, {"VF_RShin", "VF_RAnkleOut"},
            {"VF_LShin", "VF_LAnkleOut"}, {"VF_WaistRFront", "VF_RThigh"}, {"VF_WaistLFront", "VF_LThigh"},
            {"VF_WaistRBack", "VF_WaistRFront"}, {"VF_BackL", "VF_BackR"}, {"VF_RShoulderBack", "VF_RArm"},
            {"VF_Chest", "VF_SpineTop"}, {"VF_LShoulderBack", "VF_LArm"}, {"VF_SpineTop", "VF_BackL"},
            {"VF_RHeelBack", "VF_RForefootIn"}, {"VF_SpineTop", "VF_BackR"}, {"VF_WaistLBack", "VF_LKneeOut"},
            {"VF_WaistRBack", "VF_RKneeOut"}, {"VF_Chest", "VF_BackL"}, {"VF_Chest", "VF_BackR"},
            {"VF_HeadR", "VF_HeadFront"}, {"VF_LShoulderTop", "VF_LElbowOut"}, {"VF_RShoulderTop", "VF_RElbowOut"},
            {"VF_LKneeOut", "VF_LAnkleOut"}, {"VF_LHeelBack", "VF_LForefootOut"}, {"VF_LHeelBack", "VF_LForefootIn"},
            {"VF_RKneeOut", "VF_RAnkleOut"}, {"VF_RHeelBack", "VF_RForefootOut"}, {"VF_HeadL", "VF_HeadFront"}
        };

        if (expectedFromAndToBoneNames.size() != bones.size())
        {
            return false;
        }

        for (int i = 0; i < bones.size(); i++)
        {
            if (bones[i].fromName != std::get<0>(expectedFromAndToBoneNames[i]) ||
                bones[i].toName != std::get<1>(expectedFromAndToBoneNames[i]))
            {
                return false;
            }
        }

        return true;
    }
}

TEST_CASE("GetSettings3DTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters 3D", data::Get3DSettingsTest, CRTPacket::PacketXML);

    bool bDataAvailable = true;
    if (!protocol->Read3DSettings(bDataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK(bDataAvailable);

    CRTProtocol::EAxis axisUpwards = CRTProtocol::EAxis::ZNeg;
    std::string calibrationTime = "";
    std::vector<CRTProtocol::SSettings3DLabel> labels3D;
    std::vector<CRTProtocol::SSettingsBone> bones;

    protocol->Get3DSettings(axisUpwards, calibrationTime, labels3D, bones);

    CHECK_EQ(CRTProtocol::EAxis::ZPos, axisUpwards);
    CHECK_EQ("2019-09-17 16:00:43", calibrationTime);
    CHECK_EQ(true, Verify3DLabels(labels3D));
    CHECK_EQ(true, Verify3DBones(bones));
}
