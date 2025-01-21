#include <queue>
#include <stack>

#include "../Network.h"
#include "../RTProtocol.h"
#include "../RTPacket.h"
#include "Data/Skeleton.h"
#include "TestUtils.h"

#include <doctest/doctest.h>

namespace
{
    struct TestContext
    {
        std::unique_ptr<CRTProtocol> mRTProtocol;
        qualisys_cpp_sdk::test_utils::DummyXmlNetwork* mNetwork;
    };

    TestContext CreateTestContext()
    {
        auto networkDummy = new qualisys_cpp_sdk::test_utils::DummyXmlNetwork{};

        auto protocol = std::make_unique<CRTProtocol>();

        protocol->OverrideNetwork(networkDummy);

        if (!protocol->Connect(""))
        {
            FAIL(protocol->GetErrorString());
        }

        return {std::move(protocol), networkDummy};
    }

    std::vector<CRTProtocol::SSettingsSkeletonHierarchical> CreateDummySkeletons()
    {
        auto markers = std::vector<CRTProtocol::SMarker>{
            {"marker1", {1.0, 2.0, 3.0}, 3.0},
            {"marker2", {4.0, 5.0, 6.0}, 7.0}
        };

        auto couplings = std::vector<CRTProtocol::SCoupling>{
            {"segment1", CRTProtocol::RotationX, 5.5}
        };

        const auto dofs = std::vector<CRTProtocol::SDegreeOfFreedom>{
            {CRTProtocol::RotationX, 2.3, 3.2, couplings, 4.1, 1.4}
        };

        auto rootSegment1 = CRTProtocol::SSettingsSkeletonSegmentHierarchical{
            "segment1",
            1,
            "Global Optimization",
            {0.0, 1.0, 2.0},
            {0.0, 0.0, 0.0, 1.0},
            {0.0, 1.0, 2.0},
            {0.707, -0.707, 0.0, 0.0},
            dofs,
            {0.0, 1.0, 2.0},
            markers,
            std::vector<CRTProtocol::SBody>{
                {
                    "body1", {1.0, 2.0, 3.0},
                    {1.0, 2.0, 3.0, 4.0}, 3.0
                },
                {
                    "body2", {4.0, 5.0, 6.0},
                    {5.0, 6.0, 7.0, 8.0}, 7.0
                }
            },
            std::vector<CRTProtocol::SSettingsSkeletonSegmentHierarchical>{
                {
                    "segment3",
                    3,
                    "Global Optimization",
                    {10.0, 11.0, 12.0},
                    {0.0, 0.0, 0.0, 1.0},
                    {3.0, 4.0, 5.0},
                    {0.707, 0.707, 0.0, 0.0},
                    dofs,
                    {0.0, 1.0, 2.0},
                    markers,
                    std::vector<CRTProtocol::SBody>{},
                    std::vector<CRTProtocol::SSettingsSkeletonSegmentHierarchical>{}
                }
            }
        };

        auto rootSegment2 = CRTProtocol::SSettingsSkeletonSegmentHierarchical{
            "segment2",
            2,
            "Global Optimization",
            {0.0, 1.0, 2.0},
            {0.0, 0.0, 0.0, 1.0},
            {0.0, 1.0, 2.0},
            {0.707, 0.0, 0.707, 0.0},
            dofs, {0.0, 1.0, 2.0},
            markers,
            std::vector<CRTProtocol::SBody>{},
            std::vector<CRTProtocol::SSettingsSkeletonSegmentHierarchical>{}
        };


        return std::vector<CRTProtocol::SSettingsSkeletonHierarchical>{
            {"skeleton1", 1.1, rootSegment1},
            {"skeleton2", 1.0, rootSegment2}
        };
    }
}

TEST_CASE("SetSkeletonSettings")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    if (!protocol->SetSkeletonSettings(CreateDummySkeletons()))
    {
        FAIL(protocol->GetErrorString());
    }

    auto result = network->ReadSentData();
    
    using namespace qualisys_cpp_sdk::tests;

    CHECK(qualisys_cpp_sdk::test_utils::CompareXmlIgnoreWhitespace(data::SkeletonSettingsSet(), result.data()));
}


TEST_CASE("GetSkeletonSettings")
{
    auto [protocol, network] = CreateTestContext();

    network->PrepareResponse("GetParameters Skeleton", qualisys_cpp_sdk::tests::data::SkeletonSettingsSet(), CRTPacket::PacketXML);

    using namespace qualisys_cpp_sdk::tests;

    auto dataAvailable = false;
    if (!protocol->ReadSkeletonSettings(dataAvailable, false))
    {
        FAIL(protocol->GetErrorString());
    }
    CHECK(dataAvailable);

    std::vector<CRTProtocol::SSettingsSkeletonHierarchical> actualSkeletons;
    protocol->GetSkeletons(actualSkeletons);

    auto expectedSkeletons = CreateDummySkeletons();
    CHECK_EQ(expectedSkeletons.size(), actualSkeletons.size());

    auto testSegment = [](const CRTProtocol::SSettingsSkeletonSegmentHierarchical& expectedSegment, const CRTProtocol::SSettingsSkeletonSegmentHierarchical& actualSegment)
    {
        CHECK_EQ(expectedSegment.name, actualSegment.name);

        CHECK_EQ(expectedSegment.segments.size(), actualSegment.segments.size());

        CHECK_EQ(expectedSegment.position.x, actualSegment.position.x);
        CHECK_EQ(expectedSegment.position.y, actualSegment.position.y);
        CHECK_EQ(expectedSegment.position.z, actualSegment.position.z);

        CHECK_EQ(expectedSegment.rotation.x, actualSegment.rotation.x);
        CHECK_EQ(expectedSegment.rotation.y, actualSegment.rotation.y);
        CHECK_EQ(expectedSegment.rotation.z, actualSegment.rotation.z);
        CHECK_EQ(expectedSegment.rotation.w, actualSegment.rotation.w);

        CHECK_EQ(expectedSegment.defaultPosition.x, actualSegment.defaultPosition.x);
        CHECK_EQ(expectedSegment.defaultPosition.y, actualSegment.defaultPosition.y);
        CHECK_EQ(expectedSegment.defaultPosition.z, actualSegment.defaultPosition.z);

        CHECK_EQ(expectedSegment.defaultRotation.x, actualSegment.defaultRotation.x);
        CHECK_EQ(expectedSegment.defaultRotation.y, actualSegment.defaultRotation.y);
        CHECK_EQ(expectedSegment.defaultRotation.z, actualSegment.defaultRotation.z);
        CHECK_EQ(expectedSegment.defaultRotation.w, actualSegment.defaultRotation.w);

        CHECK_EQ(expectedSegment.endpoint.x, actualSegment.endpoint.x);
        CHECK_EQ(expectedSegment.endpoint.y, actualSegment.endpoint.y);
        CHECK_EQ(expectedSegment.endpoint.z, actualSegment.endpoint.z);

        CHECK_EQ(expectedSegment.solver, actualSegment.solver);

        CHECK_EQ(expectedSegment.degreesOfFreedom.size(), actualSegment.degreesOfFreedom.size());

        for (size_t i = 0; i < expectedSegment.degreesOfFreedom.size(); ++i)
        {
            const auto& expectedDof = expectedSegment.degreesOfFreedom[i];
            const auto& actualDof = actualSegment.degreesOfFreedom[i];

            CHECK_EQ(expectedDof.goalWeight, actualDof.goalWeight);
            CHECK_EQ(expectedDof.lowerBound, actualDof.lowerBound);
            CHECK_EQ(expectedDof.upperBound, actualDof.upperBound);
            CHECK_EQ(expectedDof.type, actualDof.type);

            CHECK_EQ(expectedDof.couplings.size(), actualDof.couplings.size());

            for (size_t j = 0; j < expectedDof.couplings.size(); ++j)
            {
                const auto& expectedCoupling = expectedDof.couplings[j];
                const auto& actualCoupling = actualDof.couplings[j];

                CHECK_EQ(expectedCoupling.coefficient, actualCoupling.coefficient);
                CHECK_EQ(expectedCoupling.degreeOfFreedom, actualCoupling.degreeOfFreedom);
                CHECK_EQ(expectedCoupling.segment, actualCoupling.segment);
            }
        }
    };

    struct StackEntry
    {
        const CRTProtocol::SSettingsSkeletonSegmentHierarchical& expected;
        const CRTProtocol::SSettingsSkeletonSegmentHierarchical& actual;
    };

    auto stack = std::stack<StackEntry>();

    for (size_t i = 0; i < expectedSkeletons.size(); ++i)
    {
        const auto& expectedSkeleton = expectedSkeletons[i];
        const auto& skeleton = actualSkeletons[i];

        CHECK_EQ(expectedSkeleton.name, skeleton.name);
        CHECK_EQ(expectedSkeleton.scale, skeleton.scale);

        stack.push({ expectedSkeleton.rootSegment, skeleton.rootSegment });
    }

    while (!stack.empty())
    {
        auto entry = stack.top();
        stack.pop();

        testSegment(entry.expected, entry.actual);

        for (size_t i = 0; i < entry.expected.segments.size(); ++i)
        {
            stack.push({entry.expected.segments[i], entry.actual.segments[i] });
        }
    }
}
