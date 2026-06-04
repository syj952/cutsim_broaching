<?xml version="1.0"?>
<VcMachine Version="9.0" Unit="millimeter">
  <Information>
    <InfoUserName>Lenovo</InfoUserName>
    <InfoDateTime>Mon Mar 10 09:33:02 2025</InfoDateTime>
    <InfoVericutVersion>9.0.1</InfoVericutVersion>
    <InfoBuildDate>March 02, 2020 </InfoBuildDate>
    <InfoLibraryDate>03/02/20</InfoLibraryDate>
    <InfoCustomerName>LND-7330757468@CS</InfoCustomerName>
    <InfoCustomerID>090070</InfoCustomerID>
    <InfoServerID>001</InfoServerID>
    <InfoHostID>90c7f6f9154b</InfoHostID>
  </Information>
  <PriorityType>single</PriorityType>
  <Table Name="Initial Machine Location">
    <Row Index="1" SubIndex="1" Auto="manual">
      <System>1</System>
      <Value>0</Value>
      <Value>332</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
      <Value>0</Value>
    </Row>
  </Table>
  <Collision>
    <Between Sub1="off" Sub2="off" Tol="2.5">
      <Component>A</Component>
      <Component>Spindle</Component>
    </Between>
    <Between Sub1="off" Sub2="off" Tol="2.5">
      <Component>A</Component>
      <Component>Tool</Component>
    </Between>
    <Between Sub1="off" Sub2="off" Tol="2.5">
      <Component>C</Component>
      <Component>Spindle</Component>
    </Between>
    <Between Sub1="off" Sub2="off" Tol="2.5">
      <Component>C</Component>
      <Component>Tool</Component>
    </Between>
    <Between Sub1="off" Sub2="off" Tol="2.5">
      <Component>Z</Component>
      <Component>A</Component>
    </Between>
    <Between Sub1="off" Sub2="off" Tol="2.5">
      <Component>Z</Component>
      <Component>C</Component>
    </Between>
  </Collision>
  <Component Name="Base"
   Subsystem="1"
   Type="base" Visible="machine" Draw="shaded" XRGB="0x00FF0000">
    <Attach></Attach>
    <Position X="0" Y="0" Z="0"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00464646">
      <File>./UCP800DURO/BED.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00464646"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
    <STL Unit="millimeter" Normal="outward" Visible="off" XRGB="0x00FFE4B5">
      <File>./UCP800DURO/Cover.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FFE4B5"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
    <STL Unit="millimeter" Normal="outward" Visible="off" XRGB="0x00FF9B00">
      <File>./UCP800DURO/DOOR.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF9B00"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00FFE4B5">
      <File>./UCP800DURO/A_TABLE_SURPORT.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FFE4B5"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
    <STL Unit="millimeter" Normal="outward" Visible="off" XRGB="0x00FFFFFF">
      <File>./UCP800DURO/SURF.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FFFFFF"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
  </Component>
  <Component Name="X"
   Subsystem="1"
   Type="linear" Visible="machine" Draw="shaded" XRGB="0x00FF0000">
    <Attach>Base</Attach>
    <Position X="0" Y="0" Z="0"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Link Axis="X" PartSide="auto" Register="X"
     RapidRate="5000" Accel="125" Decel="125" MaxLinearVelocity="5000" MaxCornerVelocity="600">
      <AxisPriority PriorityGroup="0" PriorityPos="1" InterpolatedPos="on" PriorityNeg="1" InterpolatedNeg="on"/>
      <TlRecord TlGroup="0" TlComp="X" TlMin="-400.555" TlMax="399.545" TlCompCond="" TlMinCond="0" TlMaxCond="0" TlIgnore="off"/>
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00969696">
      <File>./UCP800DURO/X_AXIS.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00969696"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
  </Component>
  <Component Name="Y"
   Subsystem="1"
   Type="linear" Visible="machine" Draw="shaded" XRGB="0x00FF0000">
    <Attach>X</Attach>
    <Position X="0" Y="0" Z="0"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Link Axis="Y" PartSide="auto" Register="Y"
     RapidRate="5000" Accel="125" Decel="125" MaxLinearVelocity="5000" MaxCornerVelocity="600">
      <AxisPriority PriorityGroup="0" PriorityPos="1" InterpolatedPos="on" PriorityNeg="1" InterpolatedNeg="on"/>
      <TlRecord TlGroup="0" TlComp="Y" TlMin="-317.483" TlMax="332.617" TlCompCond="" TlMinCond="0" TlMaxCond="0" TlIgnore="off"/>
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00C8C8C8">
      <File>./UCP800DURO/Y_Axis.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00C8C8C8"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
  </Component>
  <Component Name="Z"
   Subsystem="1"
   Type="linear" Visible="machine" Draw="shaded" XRGB="0x00FF0000">
    <Attach>Y</Attach>
    <Position X="0" Y="0" Z="0"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Link Axis="Z" PartSide="auto" Register="Z"
     RapidRate="5000" Accel="125" Decel="125" MaxLinearVelocity="5000" MaxCornerVelocity="600">
      <AxisPriority PriorityGroup="0" PriorityPos="1" InterpolatedPos="on" PriorityNeg="1" InterpolatedNeg="on"/>
      <TlRecord TlGroup="0" TlComp="Z" TlMin="-560" TlMax="0" TlCompCond="" TlMinCond="0" TlMaxCond="0" TlIgnore="off"/>
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00969696">
      <File>./UCP800DURO/Z_Axis.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00969696"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
  </Component>

  <Component Name="Spindle"
   Subsystem="1"
   Type="spindle" Visible="machine" Draw="shaded" XRGB="0x00FF0000">
    <Attach>Z</Attach>
    <Position X="0" Y="0" Z="0"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Link Axis="Z" PartSide="auto" Register="Spindle">
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00C8C8C8">
      <File>./UCP800DURO/Spindle.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00C8C8C8"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
  </Component>

   <Component Name="ToolSpindle_ForceMeasure"
   Subsystem="1"
   Type="tool" Visible="all" Draw="translucent" XRGB="0x00FF0000">
    <Attach>Spindle</Attach>
    <Position X="0" Y="0" Z="600"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Tool TurretIndex="1"/>
    <Link Axis="Z" PartSide="auto" Register="Tool">
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="A9E3D9">
      <File>./UCP800DURO/spindle222.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    </STL>
  </Component>

   <Component Name="Tool"
   Subsystem="1"
   Type="tool" Visible="all" Draw="translucent" XRGB="A9E3D9">
    <Attach>ToolSpindle_ForceMeasure</Attach>
    <Position X="0" Y="0" Z="600"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="A9E3D9"/>
    <Tool TurretIndex="1"/>
    <Link Axis="Z" PartSide="auto" Register="Tool">
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="A9E3D9">
      <File>./UCP800DURO/tool222.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="A9E3D9"/>
    </STL>
  </Component>

  <Component Name="A"
   Subsystem="1"
   Type="rotary" Visible="machine" Draw="shaded" XRGB="0x00FF0000" RotarySpin="off">
    <Attach>Base</Attach>
    <Position X="0" Y="-49.985" Z="120.042"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Link Axis="X" PartSide="auto" Register="A"
     RapidRate="5000" Accel="125" Decel="125" MaxLinearVelocity="5000" MaxCornerVelocity="600">
      <AxisPriority PriorityGroup="0" PriorityPos="1" InterpolatedPos="on" PriorityNeg="1" InterpolatedNeg="on"/>
      <TlRecord TlGroup="0" TlComp="A" TlMin="-100.0501" TlMax="120.0502" TlCompCond="" TlMinCond="0" TlMaxCond="0" TlIgnore="off"/>
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00FFE4B5">
      <File>./UCP800DURO/A_TABLE.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FFE4B5"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="49.985" Z="-120.042"/>
      </ModelMatrix>
    </STL>
  </Component>
  <Component Name="C"
   Subsystem="1"
   Type="rotary" Visible="machine" Draw="shaded" XRGB="0x00FF0000" RotarySpin="off">
    <Attach>A</Attach>
    <Position X="0" Y="49.985" Z="-120.042"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
    <Link Axis="Z" PartSide="auto" Register="C"
     RapidRate="5000" Accel="125" Decel="125" MaxLinearVelocity="5000" MaxCornerVelocity="600">
      <AxisPriority PriorityGroup="0" PriorityPos="1" InterpolatedPos="on" PriorityNeg="1" InterpolatedNeg="on"/>
      <TlRecord TlGroup="0" TlComp="C" TlMin="-360" TlMax="360" TlCompCond="" TlMinCond="0" TlMaxCond="0" TlIgnore="off"/>
    </Link>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="0x00FFFFFF">
      <File>./UCP800DURO/C_TABLE.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FFFFFF"/>
      <EdgeAngle>30</EdgeAngle>
      <ModelMatrix>
        <MatrixXAxis X="1" Y="0" Z="0"/>
        <MatrixYAxis X="0" Y="1" Z="0"/>
        <MatrixZAxis X="0" Y="0" Z="1"/>
        <MatrixOrigin X="0" Y="0" Z="0"/>
      </ModelMatrix>
    </STL>
  </Component>

 
<Component Name="Workpiece"
   Subsystem="1"
   Type="rotary" Visible="machine" Draw="shaded" XRGB="FEB2B4" RotarySpin="off">
    <Attach>C</Attach>
    <Position X="0" Y="49.985" Z="-120.042"/>
    <Rotation I="0" J="0" K="0"/>
    <STL Unit="millimeter" Normal="outward" Visible="on" XRGB="FEB2B4">
      <File>./UCP800DURO/test_lc.STL</File>
      <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="FEB2B4"/>
    </STL>
  </Component>

   <Component Name="Attach"
   Subsystem="1"
   Type="attach" Visible="machine" Draw="shaded" XRGB="0x00FF0000">
    <Attach>C</Attach>
    <Position X="0" Y="0" Z="0"/>
    <Rotation I="0" J="0" K="0"/>
    <Appearance Name="Default" Ambient="0.2" Diffuse="0.7" Reflect="0.2" Specular="1" Shininess="6" Translucency="1" XRGB="0x00FF0000"/>
  </Component>
 
</VcMachine>
