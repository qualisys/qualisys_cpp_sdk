

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

    CRTProtocol::SCalibration expectedCalibration{
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
        {
            CRTProtocol::SCalibrationCamera {
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
            }
        }
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

    const auto checkFov = [](const CRTProtocol::SCalibrationFov& l, const CRTProtocol::SCalibrationFov& r) {
            CHECK_EQ(l.bottom, r.bottom);
            CHECK_EQ(l.left, r.left);
            CHECK_EQ(l.right, r.right);
            CHECK_EQ(l.top, r.top);
        };

    const auto checkTransform = [](const CRTProtocol::SCalibrationTransform& l, const CRTProtocol::SCalibrationTransform& r) -> void {
            CHECK_EQ(l.x, r.x);
            CHECK_EQ(l.y, r.y);
            CHECK_EQ(l.z, r.z);
            CHECK_EQ(l.r11, r.r11);
            CHECK_EQ(l.r12, r.r12);
            CHECK_EQ(l.r13, r.r13);
            CHECK_EQ(l.r21, r.r21);
            CHECK_EQ(l.r22, r.r22);
            CHECK_EQ(l.r23, r.r23);
            CHECK_EQ(l.r31, r.r31);
            CHECK_EQ(l.r32, r.r32);
            CHECK_EQ(l.r33, r.r33);
        };

    const auto checkIntrinsic = [](const CRTProtocol::SCalibrationIntrinsic& l, const CRTProtocol::SCalibrationIntrinsic& r) -> void {
            CHECK_EQ(l.focal_length, r.focal_length);
            CHECK_EQ(l.sensor_min_u, r.sensor_min_u);
            CHECK_EQ(l.sensor_max_u, r.sensor_max_u);
            CHECK_EQ(l.sensor_min_v, r.sensor_min_v);
            CHECK_EQ(l.sensor_max_v, r.sensor_max_v);
            CHECK_EQ(l.focal_length_u, r.focal_length_u);
            CHECK_EQ(l.focal_length_v, r.focal_length_v);
            CHECK_EQ(l.center_point_u, r.center_point_u);
            CHECK_EQ(l.center_point_v, r.center_point_v);
            CHECK_EQ(l.skew, r.skew);
            CHECK_EQ(l.radial_distortion_1, r.radial_distortion_1);
            CHECK_EQ(l.radial_distortion_2, r.radial_distortion_2);
            CHECK_EQ(l.radial_distortion_3, r.radial_distortion_3);
            CHECK_EQ(l.tangental_distortion_1, r.tangental_distortion_1);
            CHECK_EQ(l.tangental_distortion_2, r.tangental_distortion_2);
        };

    for (std::size_t i = 0; i < expectedCalibration.cameras.size(); ++i)
    {
        const auto& expectedCamera = expectedCalibration.cameras[i];
        const auto& actualCamera = actualCalibration.cameras[i];

        checkFov(expectedCamera.fov_marker, actualCamera.fov_marker);
        checkFov(expectedCamera.fov_video, actualCamera.fov_video);
        checkFov(expectedCamera.fov_marker_max, actualCamera.fov_marker_max);
        checkFov(expectedCamera.fov_video_max, actualCamera.fov_video_max);
        checkTransform(expectedCamera.transform, actualCamera.transform);
        checkIntrinsic(expectedCamera.intrinsic, actualCamera.intrinsic);
    }
}