#pragma once
namespace qualisys_cpp_sdk::tests::data
{
    inline const char* GetImageSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
<Image>
    <Camera>
        <ID>1</ID>
        <Enabled>true</Enabled>
        <Format>JPG</Format>
        <Width>1920</Width>
        <Height>1088</Height>
        <Left_Crop>0.000000</Left_Crop>
        <Top_Crop>0.000000</Top_Crop>
        <Right_Crop>1.000000</Right_Crop>
        <Bottom_Crop>1.000000</Bottom_Crop>
    </Camera>
</Image>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

    inline const char* SetImageSettingsTest =  R"XMLDATA(
<QTM_Settings>
<Image>
    <Camera>
        <ID>1</ID>
        <Enabled>true</Enabled>
        <Format>RAWGrayscale</Format>
        <Width>99</Width>
        <Height>98</Height>
        <Left_Crop>97.000000</Left_Crop>
        <Top_Crop>96.000000</Top_Crop>
        <Right_Crop>95.000000</Right_Crop>
        <Bottom_Crop>94.000000</Bottom_Crop>
    </Camera>
</Image>
</QTM_Settings>
)XMLDATA";

}
