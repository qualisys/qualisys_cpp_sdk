#pragma once
namespace qualisys_cpp_sdk::tests::data
{
    inline const char* GetEyeTrackerSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <Eye_Tracker>
        <Device>
            <Name>EyeTrackerDevice</Name>
            <Frequency>240.000000</Frequency>
            <Hardware_Sync>True</Hardware_Sync>
        </Device>
    </Eye_Tracker>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

}