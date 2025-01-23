#pragma once
namespace qualisys_cpp_sdk::tests::data
{

    inline inline const char* GetGazeVectorSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <Gaze_Vector>
        <Vector>
            <Name>Gaze vector 1 (L)</Name>
            <Frequency>240.000000</Frequency>
            <Hardware_Sync>True</Hardware_Sync>
            <Filter>False</Filter>
        </Vector>
        <Vector>
            <Name>Gaze vector 1 (R)</Name>
            <Frequency>240.000000</Frequency>
            <Hardware_Sync>True</Hardware_Sync>
            <Filter>False</Filter>
        </Vector>
    </Gaze_Vector>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

}
