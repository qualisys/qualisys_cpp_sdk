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

        const char* GetCameraLensControlSettingsTest =
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
    }
}
#endif // ! XMLTESTDATA_H
