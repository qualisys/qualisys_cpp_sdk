#pragma once
namespace qualisys_cpp_sdk::tests::data
{
    inline const char* SkeletonSettingsSet = R"XMLDATA(
<QTM_Settings>
    <Skeletons>
        <Skeleton Name="skeleton1">
            <Scale>1.100000</Scale>
            <Segments>
                <Segment Name="segment1">
                    <Solver>Global Optimization</Solver>
                    <Transform>
                        <Position X="0.000000" Y="1.000000" Z="2.000000"/>
                        <Rotation X="0.000000" Y="0.000000" Z="0.000000" W="1.000000"/>
                    </Transform>
                    <DefaultTransform>
                        <Position X="0.000000" Y="1.000000" Z="2.000000"/>
                        <Rotation X="0.707000" Y="-0.707000" Z="0.000000" W="0.000000"/>
                    </DefaultTransform>
                    <DegreesOfFreedom>
                        <RotationX>
                            <Constraint LowerBound="2.300000" UpperBound="3.200000"/>
                            <Couplings>
                                <Coupling Segment="segment1" DegreeOfFreedom="RotationX" Coefficient="5.500000"/>
                            </Couplings>
                            <Goal Value="4.100000" Weight="1.400000"/>
                        </RotationX>
                    </DegreesOfFreedom>
                    <Endpoint X="0.000000" Y="1.000000" Z="2.000000"/>
                    <Markers>
                        <Marker Name="marker1">
                            <Position X="1.000000" Y="2.000000" Z="3.000000"/>
                            <Weight>3.000000</Weight>
                        </Marker>
                        <Marker Name="marker2">
                            <Position X="4.000000" Y="5.000000" Z="6.000000"/>
                            <Weight>7.000000</Weight>
                        </Marker>
                    </Markers>
                    <RigidBodies>
                        <RigidBody Name="body1">
                            <Transform>
                                <Position X="1.000000" Y="2.000000" Z="3.000000"/>
                                <Rotation X="1.000000" Y="2.000000" Z="3.000000" W="4.000000"/>
                            </Transform>
                            <Weight>3.000000</Weight>
                        </RigidBody>
                        <RigidBody Name="body2">
                            <Transform>
                                <Position X="4.000000" Y="5.000000" Z="6.000000"/>
                                <Rotation X="5.000000" Y="6.000000" Z="7.000000" W="8.000000"/>
                            </Transform>
                            <Weight>7.000000</Weight>
                        </RigidBody>
                    </RigidBodies>
                    <Segment Name="segment3">
                        <Solver>Global Optimization</Solver>
                        <Transform>
                            <Position X="10.000000" Y="11.000000" Z="12.000000"/>
                            <Rotation X="0.000000" Y="0.000000" Z="0.000000" W="1.000000"/>
                        </Transform>
                        <DefaultTransform>
                            <Position X="3.000000" Y="4.000000" Z="5.000000"/>
                            <Rotation X="0.707000" Y="0.707000" Z="0.000000" W="0.000000"/>
                        </DefaultTransform>
                        <DegreesOfFreedom>
                            <RotationX>
                                <Constraint LowerBound="2.300000" UpperBound="3.200000"/>
                                <Couplings>
                                    <Coupling Segment="segment1" DegreeOfFreedom="RotationX" Coefficient="5.500000"/>
                                </Couplings>
                                <Goal Value="4.100000" Weight="1.400000"/>
                            </RotationX>
                        </DegreesOfFreedom>
                        <Endpoint X="0.000000" Y="1.000000" Z="2.000000"/>
                        <Markers>
                            <Marker Name="marker1">
                                <Position X="1.000000" Y="2.000000" Z="3.000000"/>
                                <Weight>3.000000</Weight>
                            </Marker>
                            <Marker Name="marker2">
                                <Position X="4.000000" Y="5.000000" Z="6.000000"/>
                                <Weight>7.000000</Weight>
                            </Marker>
                        </Markers>
                        <RigidBodies/>
                    </Segment>
                </Segment>
            </Segments>
        </Skeleton>
        <Skeleton Name="skeleton2">
            <Scale>1.000000</Scale>
            <Segments>
                <Segment Name="segment2">
                    <Solver>Global Optimization</Solver>
                    <Transform>
                        <Position X="0.000000" Y="1.000000" Z="2.000000"/>
                        <Rotation X="0.000000" Y="0.000000" Z="0.000000" W="1.000000"/>
                    </Transform>
                    <DefaultTransform>
                        <Position X="0.000000" Y="1.000000" Z="2.000000"/>
                        <Rotation X="0.707000" Y="0.000000" Z="0.707000" W="0.000000"/>
                    </DefaultTransform>
                    <DegreesOfFreedom>
                        <RotationX>
                            <Constraint LowerBound="2.300000" UpperBound="3.200000"/>
                            <Couplings>
                                <Coupling Segment="segment1" DegreeOfFreedom="RotationX" Coefficient="5.500000"/>
                            </Couplings>
                            <Goal Value="4.100000" Weight="1.400000"/>
                        </RotationX>
                    </DegreesOfFreedom>
                    <Endpoint X="0.000000" Y="1.000000" Z="2.000000"/>
                    <Markers>
                        <Marker Name="marker1">
                            <Position X="1.000000" Y="2.000000" Z="3.000000"/>
                            <Weight>3.000000</Weight>
                        </Marker>
                        <Marker Name="marker2">
                            <Position X="4.000000" Y="5.000000" Z="6.000000"/>
                            <Weight>7.000000</Weight>
                        </Marker>
                    </Markers>
                    <RigidBodies/>
                </Segment>
            </Segments>
        </Skeleton>
    </Skeletons>
</QTM_Settings>
)XMLDATA";

}
