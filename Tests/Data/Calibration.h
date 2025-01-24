#pragma once
namespace qualisys_cpp_sdk::tests::data {
inline const char* Calibration = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <calibration calibrated="true" source="source.qca" created="Calibration carried out: 2019-09-17 16:00:43" qtm-version="2024.3 (build 14360)" type="regular" wandLength="601.100000" maximumFrames="1500" lengthOfCalibration="90" shortArmEnd="365.000000" longArmEnd="566.000000" longArmMiddle="200.000000">
        <results std-dev="1.959473" min-max-diff="41.064426"/>
        <cameras>
            <camera active="1" calibrated="true" message="calibrated" point-count="100" avg-residual="0.01" serial="12345" model="Test Camera" viewrotation="180" video_resolution="1080p">
                <fov_marker left="1" top="2" right="3" bottom="4"/>
                <fov_marker_max left="5" top="6" right="7" bottom="8"/>
                <fov_video left="9" top="10" right="11" bottom="12"/>
                <fov_video_max left="13" top="14" right="15" bottom="16"/>
                <transform x="0.1" y="0.2" z="0.3" r11="0.4" r12="0.5" r13="0.6" r21="0.7" r22="0.8" r23="0.9" r31="0.10" r32="0.11" r33="0.12"/>
                <intrinsic focallength="0.13" sensorMinU="0.14" sensorMaxU="0.15" sensorMinV="0.16" sensorMaxV="0.17" focalLengthU="0.18" focalLengthV="0.19" centerPointU="0.20" centerPointV="0.21" skew="0.22" radialDistortion1="0.23" radialDistortion2="0.24" radialDistortion3="0.25" tangentalDistortion1="0.26" tangentalDistortion2="0.27"/>
            </camera>
        </cameras>
    </calibration>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

}