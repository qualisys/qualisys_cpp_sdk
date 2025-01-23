

#include "doctest/doctest.h"
#include "TestUtils.h"
#include "Data/Calibration.h"
#include <cmath>

using namespace qualisys_cpp_sdk::tests;

TEST_CASE("GetCalibrationTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters Calibration", data::Calibration, CRTPacket::PacketXML);

    bool bDataAvailable = true;

    if (!protocol->ReadCalibrationSettings())
    {
        FAIL(protocol->GetErrorString());
    }

    CRTProtocol::SCalibration actualCalibration;
    protocol->GetCalibrationSettings(actualCalibration);

    CRTProtocol::SCalibrationCamera expectedCamera {
        true,
        true,
        "calibrated",
        100,
        .01,
        12345,
        "Test Camera",
        180,
        {1,2,3,4},
        {5,6,7,8},
        {9, 10, 11, 12},
        {13, 14, 15, 16},
        {.1, .2, .3, .4, .5, .6, .7, .8, .9, .10, .11, .12 },
        { .13, .14, .15, .16, .17, .18, .19, .20, .21, .22, .23, .24, .25, .26, .27 }
    };

    CRTProtocol::SCalibration expectedCalibration {
        true,
        "source.qca",
        "Calibration carried out: 2019-09-17 16:00:43",
        "2024.3 (build 14360)",
        CRTProtocol::ECalibrationType::regular,
        std::numeric_limits<float>::quiet_NaN(),
        601.1,
        1500,
        365.0,
        566.0,
        200.0,
        1.959473,
        41.064426,
        std::numeric_limits<float>::quiet_NaN(),
        0,
        {expectedCamera}
    };

    CHECK_EQ(expectedCalibration.calibrated, actualCalibration.calibrated);
    CHECK_EQ(expectedCalibration.source, actualCalibration.source);
    CHECK_EQ(expectedCalibration.created, actualCalibration.created);
    CHECK_EQ(expectedCalibration.qtm_version, actualCalibration.qtm_version);
    CHECK_EQ(expectedCalibration.type, actualCalibration.type);
    CHECK(std::isnan(actualCalibration.result_refit_residual));
    CHECK_EQ(expectedCalibration.wand_length, actualCalibration.wand_length);
    CHECK_EQ(expectedCalibration.max_frames, actualCalibration.max_frames);
    CHECK_EQ(expectedCalibration.short_arm_end, actualCalibration.short_arm_end);
    CHECK_EQ(expectedCalibration.long_arm_end, actualCalibration.long_arm_end);
    CHECK_EQ(expectedCalibration.long_arm_middle, actualCalibration.long_arm_middle);
    CHECK_EQ(expectedCalibration.result_std_dev, actualCalibration.result_std_dev);
    CHECK_EQ(expectedCalibration.result_min_max_diff, actualCalibration.result_min_max_diff);
    CHECK(std::isnan(actualCalibration.refit_residual));
    CHECK_EQ(expectedCalibration.result_consecutive, actualCalibration.result_consecutive);

    CHECK_EQ(expectedCalibration.cameras.size(), actualCalibration.cameras.size());

    const auto compareFov = [](const CRTProtocol::SCalibrationFov& l, const CRTProtocol::SCalibrationFov& r )
    {
        CHECK_EQ(l.bottom, r.bottom);
        CHECK_EQ(l.left, r.left);
        CHECK_EQ(l.right, r.right);
        CHECK_EQ(l.top, r.top);
    };

    const auto compareTransform = [](const CRTProtocol::SCalibrationTransform& l, const CRTProtocol::SCalibrationTransform& r) -> void
    {
        const auto [l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12] = l;
        const auto [r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12] = r;
        CHECK_EQ(l1, r1);
        CHECK_EQ(l2, r2);
        CHECK_EQ(l3, r3);
        CHECK_EQ(l4, r4);
        CHECK_EQ(l5, r5);
        CHECK_EQ(l6, r6);
        CHECK_EQ(l7, r7);
        CHECK_EQ(l8, r8);
        CHECK_EQ(l9, r9);
        CHECK_EQ(l10, r10);
        CHECK_EQ(l11, r11);
        CHECK_EQ(l12, r12);
    };

    const auto compareIntrinsic = [](const CRTProtocol::SCalibrationIntrinsic& l, const CRTProtocol::SCalibrationIntrinsic& r) -> void
    {
        const auto [l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15] = l;
        const auto [r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15] = r;
        CHECK_EQ(l1, r1);
        CHECK_EQ(l2, r2);
        CHECK_EQ(l3, r3);
        CHECK_EQ(l4, r4);
        CHECK_EQ(l5, r5);
        CHECK_EQ(l6, r6);
        CHECK_EQ(l7, r7);
        CHECK_EQ(l8, r8);
        CHECK_EQ(l9, r9);
        CHECK_EQ(l10, r10);
        CHECK_EQ(l11, r11);
        CHECK_EQ(l12, r12);
        CHECK_EQ(l13, r13);
        CHECK_EQ(l14, r14);
        CHECK_EQ(l15, r15);
    };

    for (std::size_t i = 0; i < expectedCalibration.cameras.size(); ++i)
    {
        const auto& exectedCamera = expectedCalibration.cameras[i];
        const auto& actualCamera = actualCalibration.cameras[i];

        compareFov(exectedCamera.fov_marker, actualCamera.fov_marker);
        compareFov(exectedCamera.fov_video, actualCamera.fov_video);
        compareFov(exectedCamera.fov_marker_max, actualCamera.fov_marker_max);
        compareFov(exectedCamera.fov_video_max, actualCamera.fov_video_max);
        compareTransform(exectedCamera.transform, actualCamera.transform);
        compareIntrinsic(expectedCamera.intrinsic, actualCamera.intrinsic);
    }
}