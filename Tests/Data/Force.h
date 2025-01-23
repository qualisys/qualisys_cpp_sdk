#pragma once
namespace qualisys_cpp_sdk::tests::data
{
    inline const char* GetForceSettingsTest = R"XMLDATA(
<QTM_Parameters_Ver_1.25>
    <Force>
        <Unit_Length>mm</Unit_Length>
        <Unit_Force>N</Unit_Force>
        <Plate>
            <Force_Plate_Index>1</Force_Plate_Index>
            <Analog_Device_ID>1</Analog_Device_ID>
            <Frequency>1000.000000</Frequency>
            <Type>Kistler</Type>
            <Name>Force-plate 1</Name>
            <Length>600.000000</Length>
            <Width>400.000000</Width>
            <Location>
                <Corner1>
                    <X>4.669080</X>
                    <Y>3.016870</Y>
                    <Z>1.715060</Z>
                </Corner1>
                <Corner2>
                    <X>3.844050</X>
                    <Y>394.878000</Y>
                    <Z>1.084130</Z>
                </Corner2>
                <Corner3>
                    <X>593.901992</X>
                    <Y>395.980000</Y>
                    <Z>-0.707185</Z>
                </Corner3>
                <Corner4>
                    <X>594.332993</X>
                    <Y>2.392870</Y>
                    <Z>-0.577088</Z>
                </Corner4>
            </Location>
            <Origin>
                <X>119.999997</X>
                <Y>200.000003</Y>
                <Z>63.000001</Z>
            </Origin>
            <Channels>
                <Channel>
                    <Channel_No>1</Channel_No>
                    <ConversionFactor>-263.227163</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>2</Channel_No>
                    <ConversionFactor>-262.536098</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>3</Channel_No>
                    <ConversionFactor>-262.536098</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>4</Channel_No>
                    <ConversionFactor>-262.123194</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>5</Channel_No>
                    <ConversionFactor>-513.610676</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>6</Channel_No>
                    <ConversionFactor>-515.729755</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>7</Channel_No>
                    <ConversionFactor>-512.557669</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>8</Channel_No>
                    <ConversionFactor>-512.557669</ConversionFactor>
                </Channel>
            </Channels>
        </Plate>
        <Plate>
            <Force_Plate_Index>2</Force_Plate_Index>
            <Analog_Device_ID>1</Analog_Device_ID>
            <Frequency>1000.000000</Frequency>
            <Type>Kistler</Type>
            <Name>Force-plate 2</Name>
            <Length>600.000000</Length>
            <Width>400.000000</Width>
            <Location>
                <Corner1>
                    <X>606.709003</X>
                    <Y>2.527060</Y>
                    <Z>-0.719673</Z>
                </Corner1>
                <Corner2>
                    <X>608.502030</X>
                    <Y>397.065014</Y>
                    <Z>-0.878276</Z>
                </Corner2>
                <Corner3>
                    <X>1200.280070</X>
                    <Y>396.040976</Y>
                    <Z>-2.188290</Z>
                </Corner3>
                <Corner4>
                    <X>1200.199962</X>
                    <Y>3.805200</Y>
                    <Z>-1.571720</Z>
                </Corner4>
            </Location>
            <Origin>
                <X>119.999997</X>
                <Y>200.000003</Y>
                <Z>63.000001</Z>
            </Origin>
            <Channels>
                <Channel>
                    <Channel_No>9</Channel_No>
                    <ConversionFactor>-263.227163</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>10</Channel_No>
                    <ConversionFactor>-262.536098</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>11</Channel_No>
                    <ConversionFactor>-262.605036</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>12</Channel_No>
                    <ConversionFactor>-262.881185</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>13</Channel_No>
                    <ConversionFactor>-513.083632</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>14</Channel_No>
                    <ConversionFactor>-512.557669</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>15</Channel_No>
                    <ConversionFactor>-513.083632</ConversionFactor>
                </Channel>
                <Channel>
                    <Channel_No>16</Channel_No>
                    <ConversionFactor>-513.347035</ConversionFactor>
                </Channel>
            </Channels>
        </Plate>
    </Force>
</QTM_Parameters_Ver_1.25>
)XMLDATA";

}