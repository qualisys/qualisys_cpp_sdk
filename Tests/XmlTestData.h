#ifndef  XMLTESTDATA_H
#define XMLTESTDATA_H
namespace qualisys_cpp_sdk 
{
    namespace xml_test_data 
    {
        const char* SetExtTimeBaseSettingsTest =
R"XMLDATA(<QTM_Settings>
    <General>
        <External_Time_Base>
            <Enabled>True</Enabled>
            <Signal_Source>Video sync</Signal_Source>
            <Signal_Mode>Periodic</Signal_Mode>
            <Frequency_Multiplier>999</Frequency_Multiplier>
            <Frequency_Divisor>998</Frequency_Divisor>
            <Frequency_Tolerance>997</Frequency_Tolerance>
            <Nominal_Frequency>996.000</Nominal_Frequency>
            <Signal_Edge>Negative</Signal_Edge>
            <Signal_Shutter_Delay>995</Signal_Shutter_Delay>
            <Non_Periodic_Timeout>994.000</Non_Periodic_Timeout>
        </External_Time_Base>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetExtTimestampSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <External_Timestamp>
            <Enabled>True</Enabled>
            <Type>IRIG</Type>
            <Frequency>999</Frequency>
        </External_Timestamp>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetCameraSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <Camera>
            <ID>1</ID>
            <Mode>Marker Intensity</Mode>
            <Marker_Exposure>999.000000</Marker_Exposure>
            <Marker_Threshold>998.000000</Marker_Threshold>
            <Orientation>1</Orientation>
        </Camera>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetCameraAutoExposureSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <Camera>
            <ID>1</ID>
            <LensControl>
                <AutoExposure Enabled="true" Compensation="1.234500"/>
            </LensControl>
        </Camera>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetCameraVideoSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <Camera>
            <ID>1</ID>
            <Video_Resolution>1080p</Video_Resolution>
            <Video_Aspect_Ratio>4x3</Video_Aspect_Ratio>
            <Video_Frequency>23</Video_Frequency>
            <Video_Exposure>0.123000</Video_Exposure>
            <Video_Flash_Time>0.456000</Video_Flash_Time>
        </Camera>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetCameraSyncOutSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <Camera>
            <ID>7</ID>
            <Sync_Out>
                <Mode>System live time</Mode>
                <Signal_Polarity>Negative</Signal_Polarity>
            </Sync_Out>
        </Camera>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetCameraLensControlSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <Camera>
            <ID>1</ID>
            <LensControl>
                <Focus Value="99.000000"/>
                <Aperture Value="98.000000"/>
            </LensControl>
        </Camera>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetCameraAutoWhiteBalanceTest =
            R"XMLDATA(<QTM_Settings>
    <General>
        <Camera>
            <ID>1</ID>
            <AutoWhiteBalance>true</AutoWhiteBalance>
        </Camera>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* SetImageSettingsTest =
            R"XMLDATA(<QTM_Settings>
    <Image>
        <Camera>
            <ID>1</ID>
            <Enabled>True</Enabled>
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

        const char* GetGeneralSettingsTest =
R"XMLDATA(<QTM_Parameters_Ver_1.25>
    <General>
        <Frequency>100</Frequency>
        <Capture_Time>10.000000</Capture_Time>
        <Start_On_External_Trigger>false</Start_On_External_Trigger>
        <Start_On_Trigger_NO>false</Start_On_Trigger_NO>
        <Start_On_Trigger_NC>false</Start_On_Trigger_NC>
        <Start_On_Trigger_Software>false</Start_On_Trigger_Software>
        <External_Time_Base>
            <Enabled>false</Enabled>
            <Signal_Source>Control port</Signal_Source>
            <Signal_Mode>Periodic</Signal_Mode>
            <Frequency_Multiplier>1</Frequency_Multiplier>
            <Frequency_Divisor>1</Frequency_Divisor>
            <Frequency_Tolerance>1000</Frequency_Tolerance>
            <Nominal_Frequency>None</Nominal_Frequency>
            <Signal_Edge>Negative</Signal_Edge>
            <Signal_Shutter_Delay>0</Signal_Shutter_Delay>
            <Non_Periodic_Timeout>10.000000</Non_Periodic_Timeout>
        </External_Time_Base>
        <External_Timestamp>
            <Enabled>false</Enabled>
            <Type>SMPTE</Type>
            <Frequency>30</Frequency>
        </External_Timestamp>
        <Processing_Actions>
            <PreProcessing2D>false</PreProcessing2D>
            <Tracking>3D</Tracking>
            <TwinSystemMerge>false</TwinSystemMerge>
            <SplineFill>true</SplineFill>
            <AIM>false</AIM>
            <Track6DOF>false</Track6DOF>
            <ForceData>false</ForceData>
            <GazeVector>false</GazeVector>
            <ExportTSV>false</ExportTSV>
            <ExportC3D>false</ExportC3D>
            <ExportMatlabFile>false</ExportMatlabFile>
            <ExportAviFile>false</ExportAviFile>
            <ExportFbx>false</ExportFbx>
            <ExportJson>false</ExportJson>
            <StartProgram>false</StartProgram>
            <SkeletonSolve>false</SkeletonSolve>
        </Processing_Actions>
        <RealTime_Processing_Actions>
            <PreProcessing2D>false</PreProcessing2D>
            <Tracking>3D</Tracking>
            <AIM>false</AIM>
            <Track6DOF>false</Track6DOF>
            <ForceData>false</ForceData>
            <GazeVector>false</GazeVector>
            <SkeletonSolve>false</SkeletonSolve>
        </RealTime_Processing_Actions>
        <Reprocessing_Actions>
            <PreProcessing2D>false</PreProcessing2D>
            <Tracking>3D</Tracking>
            <TwinSystemMerge>false</TwinSystemMerge>
            <SplineFill>true</SplineFill>
            <AIM>false</AIM>
            <Track6DOF>false</Track6DOF>
            <ForceData>false</ForceData>
            <GazeVector>false</GazeVector>
            <ExportTSV>false</ExportTSV>
            <ExportC3D>false</ExportC3D>
            <ExportMatlabFile>false</ExportMatlabFile>
            <ExportAviFile>false</ExportAviFile>
            <ExportFbx>false</ExportFbx>
            <ExportJson>false</ExportJson>
            <StartProgram>false</StartProgram>
            <SkeletonSolve>false</SkeletonSolve>
        </Reprocessing_Actions>
        <EulerAngles First="Roll" Second="Pitch" Third="Yaw"/>
        <Camera>
            <ID>1</ID>
            <Model>Miqus Hybrid</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>false</Supports_HW_Sync>
            <Serial>21310</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Resolution>1080p</Video_Resolution>
            <Video_Aspect_Ratio>16x9</Video_Aspect_Ratio>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>122880</Width>
                <Height>69632</Height>
            </Marker_Res>
            <Video_Res>
                <Width>1920</Width>
                <Height>1088</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>1919</Right>
                <Bottom>1087</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>1919</Right>
                <Bottom>1087</Bottom>
            </Video_FOV>
            <LensControl>
                <Focus Value="99.000000"/>
                <Aperture Value="98.000000"/>
            </LensControl>
            <AutoExposure Enabled="false" Compensation="0.000000"/>
            <AutoWhiteBalance>true</AutoWhiteBalance>
        </Camera>
        <Camera>
            <ID>2</ID>
            <Model>Miqus M5</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>false</Supports_HW_Sync>
            <Serial>22261</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>131072</Width>
                <Height>131072</Height>
            </Marker_Res>
            <Video_Res>
                <Width>2048</Width>
                <Height>2048</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Video_FOV>
        </Camera>
        <Camera>
            <ID>3</ID>
            <Model>Miqus M5</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>false</Supports_HW_Sync>
            <Serial>22254</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>131072</Width>
                <Height>131072</Height>
            </Marker_Res>
            <Video_Res>
                <Width>2048</Width>
                <Height>2048</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Video_FOV>
        </Camera>
        <Camera>
            <ID>4</ID>
            <Model>Miqus M5</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>false</Supports_HW_Sync>
            <Serial>22257</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>131072</Width>
                <Height>131072</Height>
            </Marker_Res>
            <Video_Res>
                <Width>2048</Width>
                <Height>2048</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Video_FOV>
        </Camera>
        <Camera>
            <ID>5</ID>
            <Model>Miqus M5</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>false</Supports_HW_Sync>
            <Serial>22252</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>131072</Width>
                <Height>131072</Height>
            </Marker_Res>
)XMLDATA" // Text broken off to avoid string literal size limit
R"XMLDATA(
            <Video_Res>
                <Width>2048</Width>
                <Height>2048</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Video_FOV>
        </Camera>
        <Camera>
            <ID>6</ID>
            <Model>Miqus M5</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>false</Supports_HW_Sync>
            <Serial>22256</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>131072</Width>
                <Height>131072</Height>
            </Marker_Res>
            <Video_Res>
                <Width>2048</Width>
                <Height>2048</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>2047</Right>
                <Bottom>2047</Bottom>
            </Video_FOV>
        </Camera>
        <Camera>
            <ID>7</ID>
            <Model>Oqus 200 C</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>true</Supports_HW_Sync>
            <Serial>13090</Serial>
            <Mode>Video</Mode>
            <Video_Frequency>24</Video_Frequency>
            <Video_Resolution>1080p</Video_Resolution>
            <Video_Aspect_Ratio>16x9</Video_Aspect_Ratio>
            <Video_Exposure>
                <Current>5000</Current>
                <Min>5</Min>
                <Max>41633</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>0</Current>
                <Min>0</Min>
                <Max>2000</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>122880</Width>
                <Height>69632</Height>
            </Marker_Res>
            <Video_Res>
                <Width>1920</Width>
                <Height>1088</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>64</Left>
                <Top>0</Top>
                <Right>1983</Right>
                <Bottom>1087</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>64</Left>
                <Top>0</Top>
                <Right>1983</Right>
                <Bottom>1087</Bottom>
            </Video_FOV>
            <Sync_Out>
                <Mode>Shutter out</Mode>
                <Signal_Polarity>Negative</Signal_Polarity>
            </Sync_Out>
            <AutoExposure Enabled="false" Compensation="0.000000"/>
        </Camera>
        <Camera>
            <ID>8</ID>
            <Model>Miqus Sync Unit</Model>
            <Underwater>false</Underwater>
            <Supports_HW_Sync>true</Supports_HW_Sync>
            <Serial>29758</Serial>
            <Mode>Marker</Mode>
            <Video_Frequency>25</Video_Frequency>
            <Video_Exposure>
                <Current>500</Current>
                <Min>5</Min>
                <Max>39940</Max>
            </Video_Exposure>
            <Video_Flash_Time>
                <Current>500</Current>
                <Min>0</Min>
                <Max>500</Max>
            </Video_Flash_Time>
            <Marker_Exposure>
                <Current>300</Current>
                <Min>5</Min>
                <Max>1000</Max>
            </Marker_Exposure>
            <Marker_Threshold>
                <Current>200</Current>
                <Min>100</Min>
                <Max>900</Max>
            </Marker_Threshold>
            <Position>
                <X>0.000000</X>
                <Y>0.000000</Y>
                <Z>0.000000</Z>
                <Rot_1_1>0.000000</Rot_1_1>
                <Rot_2_1>0.000000</Rot_2_1>
                <Rot_3_1>0.000000</Rot_3_1>
                <Rot_1_2>0.000000</Rot_1_2>
                <Rot_2_2>0.000000</Rot_2_2>
                <Rot_3_2>0.000000</Rot_3_2>
                <Rot_1_3>0.000000</Rot_1_3>
                <Rot_2_3>0.000000</Rot_2_3>
                <Rot_3_3>0.000000</Rot_3_3>
            </Position>
            <Orientation>0</Orientation>
            <Marker_Res>
                <Width>65536</Width>
                <Height>65536</Height>
            </Marker_Res>
            <Video_Res>
                <Width>1024</Width>
                <Height>1024</Height>
            </Video_Res>
            <Marker_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>1023</Right>
                <Bottom>1023</Bottom>
            </Marker_FOV>
            <Video_FOV>
                <Left>0</Left>
                <Top>0</Top>
                <Right>1023</Right>
                <Bottom>1023</Bottom>
            </Video_FOV>
            <Sync_Out>
                <Mode>Multiplier</Mode>
                <Value>1</Value>
                <Duty_Cycle>50.000</Duty_Cycle>
                <Signal_Polarity>Negative</Signal_Polarity>
            </Sync_Out>
            <Sync_Out2>
                <Mode>Multiplier</Mode>
                <Value>1</Value>
                <Duty_Cycle>50.000</Duty_Cycle>
                <Signal_Polarity>Negative</Signal_Polarity>
            </Sync_Out2>
            <Sync_Out_MT>
                <Signal_Polarity>Negative</Signal_Polarity>
            </Sync_Out_MT>
        </Camera>
        <Sync_Out>
            <Mode>Multiplier</Mode>
            <Value>1</Value>
            <Duty_Cycle>50.000</Duty_Cycle>
            <Signal_Polarity>Negative</Signal_Polarity>
        </Sync_Out>
    </General>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

        const char* SetGeneralSettingsTest = R"XMLDATA(
<QTM_Settings>
    <General>
        <Frequency>1</Frequency>
        <Capture_Time>999.000</Capture_Time>
        <Start_On_External_Trigger>True</Start_On_External_Trigger>
        <Start_On_Trigger_NO>True</Start_On_Trigger_NO>
        <Start_On_Trigger_NC>True</Start_On_Trigger_NC>
        <Start_On_Trigger_Software>True</Start_On_Trigger_Software>
        <Processing_Actions>
            <PreProcessing2D>False</PreProcessing2D>
            <Tracking>False</Tracking>
            <TwinSystemMerge>False</TwinSystemMerge>
            <SplineFill>False</SplineFill>
            <AIM>False</AIM>
            <Track6DOF>False</Track6DOF>
            <ForceData>False</ForceData>
            <GazeVector>True</GazeVector>
            <ExportTSV>False</ExportTSV>
            <ExportC3D>False</ExportC3D>
            <ExportMatlabFile>False</ExportMatlabFile>
            <ExportAviFile>False</ExportAviFile>
        </Processing_Actions>
        <RealTime_Processing_Actions>
            <PreProcessing2D>False</PreProcessing2D>
            <Tracking>False</Tracking>
            <AIM>False</AIM>
            <Track6DOF>False</Track6DOF>
            <ForceData>False</ForceData>
            <GazeVector>False</GazeVector>
        </RealTime_Processing_Actions>
        <Reprocessing_Actions>
            <PreProcessing2D>False</PreProcessing2D>
            <Tracking>False</Tracking>
            <TwinSystemMerge>True</TwinSystemMerge>
            <SplineFill>False</SplineFill>
            <AIM>False</AIM>
            <Track6DOF>False</Track6DOF>
            <ForceData>False</ForceData>
            <GazeVector>False</GazeVector>
            <ExportTSV>False</ExportTSV>
            <ExportC3D>False</ExportC3D>
            <ExportMatlabFile>False</ExportMatlabFile>
            <ExportAviFile>False</ExportAviFile>
        </Reprocessing_Actions>
    </General>
</QTM_Settings>
)XMLDATA";

        const char* GetImageSettingsTest = R"XMLDATA(
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

        const char* GetForceSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <Force>
        <Unit_Length>mm</Unit_Length>
        <Unit_Force>N</Unit_Force>
    </Force>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

        const char* GetAnalogSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <Analog>
        <Device>
            <Device_ID>1</Device_ID>
            <Device_Name>Force plate 1</Device_Name>
            <Channels>10</Channels>
            <Frequency>100</Frequency>
            <Range>
                <Min>0.000000</Min>
                <Max>0.000000</Max>
            </Range>
            <Channel>
                <Label>fx</Label>
                <Unit>newtons</Unit>
            </Channel>
            <Channel>
                <Label>fy</Label>
                <Unit>newtons</Unit>
            </Channel>
            <Channel>
                <Label>fz</Label>
                <Unit>newtons</Unit>
            </Channel>
            <Channel>
                <Label>mx</Label>
                <Unit>newtonmetre</Unit>
            </Channel>
            <Channel>
                <Label>my</Label>
                <Unit>newtonmetre</Unit>
            </Channel>
            <Channel>
                <Label>mz</Label>
                <Unit>newtonmetre</Unit>
            </Channel>
            <Channel>
                <Label>trigger</Label>
                <Unit/>
            </Channel>
            <Channel>
                <Label>aux</Label>
                <Unit/>
            </Channel>
            <Channel>
                <Label>zero</Label>
                <Unit/>
            </Channel>
            <Channel>
                <Label>sync</Label>
                <Unit/>
            </Channel>
        </Device>
    </Analog>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

        const char* GetGazeVectorSettingsTest = R"XMLDATA(
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

        const char* GetEyeTrackerSettingsTest = R"XMLDATA(
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
        const char* Get6DSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <The_6D>
        <Body>
            <Name>refined</Name>
            <Enabled>true</Enabled>
            <Color R="0" G="255" B="0"/>
            <MaximumResidual>20.000000</MaximumResidual>
            <MinimumMarkersInBody>3</MinimumMarkersInBody>
            <BoneLengthTolerance>20.000000</BoneLengthTolerance>
            <Filter Preset="No filter"/>
            <Mesh>
                <Name>Arqus.obj</Name>
                <Position X="0.000000" Y="0.000000" Z="0.000000"/>
                <Rotation X="0.000000" Y="0.000000" Z="0.000000"/>
                <Scale>0.010000</Scale>
                <Opacity>1.000000</Opacity>
            </Mesh>
            <Points>
                <Point X="-67.927157" Y="29.810079" Z="-1.592971" Virtual="0" PhysicalId="0" Name="Layout 3 - 1"/>
                <Point X="-95.023059" Y="1.742391" Z="-19.541355" Virtual="0" PhysicalId="0" Name="Layout 3 - 2"/>
                <Point X="-90.017171" Y="19.888814" Z="-57.494654" Virtual="0" PhysicalId="0" Name="Layout 3 - 3"/>
                <Point X="68.316106" Y="29.173523" Z="-1.829020" Virtual="0" PhysicalId="0" Name="Layout 3 - 4"/>
                <Point X="95.027245" Y="1.474449" Z="-19.171870" Virtual="0" PhysicalId="0" Name="Layout 3 - 5"/>
                <Point X="89.624036" Y="20.690744" Z="-57.450130" Virtual="0" PhysicalId="0" Name="Layout 3 - 6"/>
            </Points>
            <Data_origin X="0.000000" Y="0.000000" Z="0.000000" Relative_body="1">0</Data_origin>
            <Data_orientation R11="1.000000" R12="0.000000" R13="0.000000" R21="0.000000" R22="1.000000" R23="0.000000" R31="0.000000" R32="0.000000" R33="1.000000" Relative_body="1">0</Data_orientation>
        </Body>
        <Body>
            <Name>Table</Name>
            <Enabled>true</Enabled>
            <Color R="0" G="0" B="255"/>
            <MaximumResidual>10.000000</MaximumResidual>
            <MinimumMarkersInBody>3</MinimumMarkersInBody>
            <BoneLengthTolerance>10.000000</BoneLengthTolerance>
            <Filter Preset="No filter"/>
            <Mesh>
                <Name/>
                <Position X="0.000000" Y="0.000000" Z="0.000000"/>
                <Rotation X="0.000000" Y="0.000000" Z="0.000000"/>
                <Scale>1.000000</Scale>
                <Opacity>1.000000</Opacity>
            </Mesh>
            <Points>
                <Point X="801.422010" Y="368.889193" Z="2.142416" Virtual="0" PhysicalId="0" Name="Table - 1"/>
                <Point X="777.315122" Y="-413.341275" Z="4.127203" Virtual="0" PhysicalId="0" Name="Table - 2"/>
                <Point X="-778.145542" Y="412.610612" Z="-2.278264" Virtual="0" PhysicalId="0" Name="Table - 3"/>
                <Point X="-800.591590" Y="-368.158531" Z="-3.991355" Virtual="0" PhysicalId="0" Name="Table - 4"/>
            </Points>
            <Data_origin X="0.000000" Y="0.000000" Z="0.000000" Relative_body="1">0</Data_origin>
            <Data_orientation R11="1.000000" R12="0.000000" R13="0.000000" R21="0.000000" R22="1.000000" R23="0.000000" R31="0.000000" R32="0.000000" R33="1.000000" Relative_body="1">0</Data_orientation>
        </Body>
        <Body>
            <Name>Screen2</Name>
            <Enabled>true</Enabled>
            <Color R="255" G="0" B="255"/>
            <MaximumResidual>10.000000</MaximumResidual>
            <MinimumMarkersInBody>3</MinimumMarkersInBody>
            <BoneLengthTolerance>10.000000</BoneLengthTolerance>
            <Filter Preset="No filter"/>
            <Mesh>
                <Name/>
                <Position X="0.000000" Y="0.000000" Z="0.000000"/>
                <Rotation X="0.000000" Y="0.000000" Z="0.000000"/>
                <Scale>1.000000</Scale>
                <Opacity>1.000000</Opacity>
            </Mesh>
            <Points>
                <Point X="177.895605" Y="-40.203524" Z="-276.061206" Virtual="0" PhysicalId="0" Name="Screen2 - 1"/>
                <Point X="-176.929252" Y="41.059151" Z="276.110102" Virtual="0" PhysicalId="0" Name="Screen2 - 2"/>
                <Point X="-111.494821" Y="129.602182" Z="-283.761580" Virtual="0" PhysicalId="0" Name="Screen2 - 3"/>
                <Point X="110.528467" Y="-130.457810" Z="283.712683" Virtual="0" PhysicalId="0" Name="Screen2 - 4"/>
            </Points>
            <Data_origin X="0.000000" Y="0.000000" Z="0.000000" Relative_body="1">0</Data_origin>
            <Data_orientation R11="1.000000" R12="0.000000" R13="0.000000" R21="0.000000" R22="1.000000" R23="0.000000" R31="0.000000" R32="0.000000" R33="1.000000" Relative_body="1">0</Data_orientation>
        </Body>
        <Body>
            <Name>Cup</Name>
            <Enabled>true</Enabled>
            <Color R="255" G="255" B="0"/>
            <MaximumResidual>10.000000</MaximumResidual>
            <MinimumMarkersInBody>3</MinimumMarkersInBody>
            <BoneLengthTolerance>10.000000</BoneLengthTolerance>
            <Filter Preset="No filter"/>
            <Mesh>
                <Name/>
                <Position X="0.000000" Y="0.000000" Z="0.000000"/>
                <Rotation X="0.000000" Y="0.000000" Z="0.000000"/>
                <Scale>1.000000</Scale>
                <Opacity>1.000000</Opacity>
            </Mesh>
            <Points>
                <Point X="-16.409485" Y="43.238453" Z="-7.375008" Virtual="0" PhysicalId="0" Name="Cup - 1"/>
                <Point X="41.237583" Y="9.395873" Z="-14.994871" Virtual="0" PhysicalId="0" Name="Cup - 2"/>
                <Point X="-6.343689" Y="-32.749109" Z="-24.391156" Virtual="0" PhysicalId="0" Name="Cup - 3"/>
                <Point X="27.211503" Y="-35.510982" Z="31.208097" Virtual="0" PhysicalId="0" Name="Cup - 4"/>
                <Point X="-45.695912" Y="15.625764" Z="15.552938" Virtual="0" PhysicalId="0" Name="Cup - 5"/>
            </Points>
            <Data_origin X="0.000000" Y="0.000000" Z="0.000000" Relative_body="1">0</Data_origin>
            <Data_orientation R11="1.000000" R12="0.000000" R13="0.000000" R21="0.000000" R22="1.000000" R23="0.000000" R31="0.000000" R32="0.000000" R33="1.000000" Relative_body="1">0</Data_orientation>
        </Body>
        <Body>
            <Name>Phone</Name>
            <Enabled>true</Enabled>
            <Color R="0" G="255" B="255"/>
            <MaximumResidual>10.000000</MaximumResidual>
            <MinimumMarkersInBody>3</MinimumMarkersInBody>
            <BoneLengthTolerance>10.000000</BoneLengthTolerance>
            <Filter Preset="No filter"/>
            <Mesh>
                <Name/>
                <Position X="0.000000" Y="0.000000" Z="0.000000"/>
                <Rotation X="0.000000" Y="0.000000" Z="0.000000"/>
                <Scale>1.000000</Scale>
                <Opacity>1.000000</Opacity>
            </Mesh>
            <Points>
                <Point X="-39.974045" Y="-68.138270" Z="-0.101279" Virtual="0" PhysicalId="0" Name="Phone - 1"/>
                <Point X="43.348296" Y="64.030668" Z="-2.407927" Virtual="0" PhysicalId="0" Name="Phone - 2"/>
                <Point X="-31.906718" Y="75.069956" Z="1.213906" Virtual="0" PhysicalId="0" Name="Phone - 3"/>
                <Point X="28.532467" Y="-70.962354" Z="1.295299" Virtual="0" PhysicalId="0" Name="Phone - 4"/>
            </Points>
            <Data_origin X="0.000000" Y="0.000000" Z="0.000000" Relative_body="1">0</Data_origin>
            <Data_orientation R11="1.000000" R12="0.000000" R13="0.000000" R21="0.000000" R22="1.000000" R23="0.000000" R31="0.000000" R32="0.000000" R33="1.000000" Relative_body="1">0</Data_orientation>
        </Body>
        <Body>
            <Name>Screen1</Name>
            <Enabled>true</Enabled>
            <Color R="127" G="0" B="0"/>
            <MaximumResidual>10.000000</MaximumResidual>
            <MinimumMarkersInBody>3</MinimumMarkersInBody>
            <BoneLengthTolerance>10.000000</BoneLengthTolerance>
            <Filter Preset="No filter"/>
            <Mesh>
                <Name/>
                <Position X="0.000000" Y="0.000000" Z="0.000000"/>
                <Rotation X="0.000000" Y="0.000000" Z="0.000000"/>
                <Scale>1.000000</Scale>
                <Opacity>1.000000</Opacity>
            </Mesh>
            <Points>
                <Point X="-395.500599" Y="5.139634" Z="-195.481255" Virtual="0" PhysicalId="0" Name="Screen - 1"/>
                <Point X="392.106423" Y="-2.138403" Z="196.007090" Virtual="0" PhysicalId="0" Name="Screen - 2"/>
                <Point X="402.905766" Y="26.186535" Z="-178.268136" Virtual="0" PhysicalId="0" Name="Screen - 3"/>
                <Point X="-399.511589" Y="-29.187767" Z="177.742302" Virtual="0" PhysicalId="0" Name="Screen - 4"/>
            </Points>
            <Data_origin X="0.000000" Y="0.000000" Z="0.000000" Relative_body="1">0</Data_origin>
            <Data_orientation R11="1.000000" R12="0.000000" R13="0.000000" R21="0.000000" R22="1.000000" R23="0.000000" R31="0.000000" R32="0.000000" R33="1.000000" Relative_body="1">0</Data_orientation>
        </Body>
    </The_6D>
</QTM_Parameters_Ver_1.25>
)XMLDATA";
        const char* Get3DSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <The_3D>
        <AxisUpwards>+Z</AxisUpwards>
        <CalibrationTime>2019-09-17 16:00:43</CalibrationTime>
        <Labels>72</Labels>
        <Label>
            <Name>VF_LeftShoulder</Name>
            <RGBColor>2124031</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_Spine</Name>
            <RGBColor>2124031</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RightShoulder</Name>
            <RGBColor>2124031</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_HeadL</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_HeadTop</Name>
            <RGBColor>3907071</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_HeadR</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_HeadFront</Name>
            <RGBColor>3907071</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LShoulderTop</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LShoulderBack</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LArm</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LElbowOut</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LWristOut</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LWristIn</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LHandOut</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RShoulderTop</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RShoulderBack</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RArm</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RElbowOut</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RWristOut</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RWristIn</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RHandOut</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_Chest</Name>
            <RGBColor>3907071</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_SpineTop</Name>
            <RGBColor>3907071</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_BackL</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_BackR</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_WaistLFront</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_WaistLBack</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_WaistRBack</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_WaistRFront</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LThigh</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LKneeOut</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LShin</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LAnkleOut</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LHeelBack</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LForefootOut</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LToeTip</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_LForefootIn</Name>
            <RGBColor>16763904</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RThigh</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RKneeOut</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RShin</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RAnkleOut</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RHeelBack</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RForefootOut</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RToeTip</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>VF_RForefootIn</Name>
            <RGBColor>3329330</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Eye Tracker Layout 3 - 1</Name>
            <RGBColor>65280</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Eye Tracker Layout 3 - 2</Name>
            <RGBColor>65280</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Eye Tracker Layout 3 - 3</Name>
            <RGBColor>65280</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Eye Tracker Layout 3 - 4</Name>
            <RGBColor>65280</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Eye Tracker Layout 3 - 5</Name>
            <RGBColor>65280</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Eye Tracker Layout 3 - 6</Name>
            <RGBColor>65280</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Table - 1</Name>
            <RGBColor>16711680</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Table - 2</Name>
            <RGBColor>16711680</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Table - 3</Name>
            <RGBColor>16711680</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Table - 4</Name>
            <RGBColor>16711680</RGBColor>
            <Trajectory_Type>-</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen2 - 1</Name>
            <RGBColor>16711935</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen2 - 2</Name>
            <RGBColor>16711935</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen2 - 3</Name>
            <RGBColor>16711935</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen2 - 4</Name>
            <RGBColor>16711935</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Cup - 1</Name>
            <RGBColor>65535</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Cup - 2</Name>
            <RGBColor>65535</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Cup - 3</Name>
            <RGBColor>65535</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Cup - 4</Name>
            <RGBColor>65535</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Cup - 5</Name>
            <RGBColor>65535</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Phone - 1</Name>
            <RGBColor>16776960</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Phone - 2</Name>
            <RGBColor>16776960</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Phone - 3</Name>
            <RGBColor>16776960</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Phone - 4</Name>
            <RGBColor>16776960</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen - 1</Name>
            <RGBColor>127</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen - 2</Name>
            <RGBColor>127</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen - 3</Name>
            <RGBColor>127</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Label>
            <Name>Screen - 4</Name>
            <RGBColor>127</RGBColor>
            <Trajectory_Type>Measured</Trajectory_Type>
        </Label>
        <Bones>
            <Bone From="VF_WaistLBack" To="VF_WaistRBack" Color="1732959616"/>
            <Bone From="VF_LWristOut" To="VF_LWristIn" Color="1732959616"/>
            <Bone From="VF_RWristOut" To="VF_RWristIn" Color="1732959616"/>
            <Bone From="VF_LWristOut" To="VF_LHandOut" Color="1732959616"/>
            <Bone From="VF_LToeTip" To="VF_LForefootIn" Color="1732959616"/>
            <Bone From="VF_RToeTip" To="VF_RForefootIn" Color="1732959616"/>
            <Bone From="VF_RForefootOut" To="VF_RToeTip" Color="1732959616"/>
            <Bone From="VF_RAnkleOut" To="VF_RHeelBack" Color="1732959616"/>
            <Bone From="VF_RWristOut" To="VF_RHandOut" Color="1732959616"/>
            <Bone From="VF_LForefootOut" To="VF_LToeTip" Color="1732959616"/>
            <Bone From="VF_LAnkleOut" To="VF_LHeelBack" Color="1732959616"/>
            <Bone From="VF_RArm" To="VF_RElbowOut" Color="1732959616"/>
            <Bone From="VF_RKneeOut" To="VF_RShin" Color="1732959616"/>
            <Bone From="VF_LKneeOut" To="VF_LShin" Color="1732959616"/>
            <Bone From="VF_LShoulderTop" To="VF_LShoulderBack" Color="1732959616"/>
            <Bone From="VF_HeadL" To="VF_HeadTop" Color="1732959616"/>
            <Bone From="VF_RShoulderTop" To="VF_RShoulderBack" Color="1732959616"/>
            <Bone From="VF_HeadTop" To="VF_HeadR" Color="1732959616"/>
            <Bone From="VF_LArm" To="VF_LElbowOut" Color="1732959616"/>
            <Bone From="VF_HeadTop" To="VF_HeadFront" Color="1732959616"/>
            <Bone From="VF_RThigh" To="VF_RKneeOut" Color="1732959616"/>
            <Bone From="VF_LThigh" To="VF_LKneeOut" Color="1732959616"/>
            <Bone From="VF_WaistLFront" To="VF_WaistRFront" Color="1732959616"/>
            <Bone From="VF_WaistLFront" To="VF_WaistLBack" Color="1732959616"/>
            <Bone From="VF_LElbowOut" To="VF_LWristOut" Color="1732959616"/>
            <Bone From="VF_RElbowOut" To="VF_RWristOut" Color="1732959616"/>
            <Bone From="VF_RShin" To="VF_RAnkleOut" Color="1732959616"/>
            <Bone From="VF_LShin" To="VF_LAnkleOut" Color="1732959616"/>
            <Bone From="VF_WaistRFront" To="VF_RThigh" Color="1732959616"/>
            <Bone From="VF_WaistLFront" To="VF_LThigh" Color="1732959616"/>
            <Bone From="VF_WaistRBack" To="VF_WaistRFront" Color="1732959616"/>
            <Bone From="VF_BackL" To="VF_BackR" Color="1732959616"/>
            <Bone From="VF_RShoulderBack" To="VF_RArm" Color="1732959616"/>
            <Bone From="VF_Chest" To="VF_SpineTop" Color="1732959616"/>
            <Bone From="VF_LShoulderBack" To="VF_LArm" Color="1732959616"/>
            <Bone From="VF_SpineTop" To="VF_BackL" Color="1732959616"/>
            <Bone From="VF_RHeelBack" To="VF_RForefootIn" Color="1732959616"/>
            <Bone From="VF_SpineTop" To="VF_BackR" Color="1732959616"/>
            <Bone From="VF_WaistLBack" To="VF_LKneeOut" Color="1732959616"/>
            <Bone From="VF_WaistRBack" To="VF_RKneeOut" Color="1732959616"/>
            <Bone From="VF_Chest" To="VF_BackL" Color="1732959616"/>
            <Bone From="VF_Chest" To="VF_BackR" Color="1732959616"/>
            <Bone From="VF_HeadR" To="VF_HeadFront" Color="1732959616"/>
            <Bone From="VF_LShoulderTop" To="VF_LElbowOut" Color="1732959616"/>
            <Bone From="VF_RShoulderTop" To="VF_RElbowOut" Color="1732959616"/>
            <Bone From="VF_LKneeOut" To="VF_LAnkleOut" Color="1732959616"/>
            <Bone From="VF_LHeelBack" To="VF_LForefootOut" Color="1732959616"/>
            <Bone From="VF_LHeelBack" To="VF_LForefootIn" Color="1732959616"/>
            <Bone From="VF_RKneeOut" To="VF_RAnkleOut" Color="1732959616"/>
            <Bone From="VF_RHeelBack" To="VF_RForefootOut" Color="1732959616"/>
            <Bone From="VF_HeadL" To="VF_HeadFront" Color="1732959616"/>
        </Bones>
    </The_3D>
</QTM_Parameters_Ver_1.25>
)XMLDATA";
    }
}
#endif // ! XMLTESTDATA_H
