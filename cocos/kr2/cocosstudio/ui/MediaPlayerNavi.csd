<GameFile>
  <PropertyGroup Name="MediaPlayerNavi" Type="Layer" ID="1579a466-7977-464e-b402-ea4d21282e25" Version="3.10.0.0" />
  <Content ctype="GameProjectContent">
    <Content>
      <Animation Duration="0" Speed="1.0000" />
      <ObjectData Name="Layer" ctype="GameLayerObjectData">
        <Size X="960.0000" Y="640.0000" />
        <Children>
          <AbstractNodeData Name="NaviBar" ActionTag="210411868" Tag="10" IconVisible="False" HorizontalEdge="BothEdge" VerticalEdge="TopEdge" BottomMargin="490.0000" TouchEnable="True" StretchWidthEnable="True" ClipAble="False" ComboBoxIndex="1" ColorAngle="90.0000" ctype="PanelObjectData">
            <Size X="960.0000" Y="150.0000" />
            <Children>
              <AbstractNodeData Name="Title" ActionTag="-2067758255" Tag="11" IconVisible="False" HorizontalEdge="BothEdge" VerticalEdge="TopEdge" LeftMargin="80.0000" RightMargin="80.0000" TopMargin="20.0000" BottomMargin="58.0000" StretchWidthEnable="True" IsCustomSize="True" FontSize="40" LabelText="Text Label" HorizontalAlignmentType="HT_Center" ShadowOffsetX="2.0000" ShadowOffsetY="-2.0000" ctype="TextObjectData">
                <Size X="800.0000" Y="52.0000" />
                <AnchorPoint ScaleX="0.5000" ScaleY="1.0000" />
                <Position X="480.0000" Y="130.0000" />
                <Scale ScaleX="1.0000" ScaleY="1.0000" />
                <CColor A="255" R="255" G="255" B="255" />
                <PrePosition X="0.5000" Y="1.0000" />
                <PreSize X="0.1521" Y="0.4200" />
                <FontResource Type="Normal" Path="DroidSansFallback.ttf" Plist="" />
                <OutlineColor A="255" R="255" G="0" B="0" />
                <ShadowColor A="255" R="110" G="110" B="110" />
              </AbstractNodeData>
              <AbstractNodeData Name="Timeline" ActionTag="-1226225042" Tag="12" IconVisible="False" HorizontalEdge="BothEdge" VerticalEdge="BottomEdge" LeftMargin="20.0000" RightMargin="20.0000" TopMargin="93.0000" BottomMargin="45.0000" TouchEnable="True" StretchWidthEnable="True" PercentInfo="50" ctype="SliderObjectData">
                <Size X="920.0000" Y="20.0000" />
                <AnchorPoint ScaleX="0.5000" ScaleY="0.5000" />
                <Position X="480.0000" Y="55.0000" />
                <Scale ScaleX="1.0000" ScaleY="1.0000" />
                <CColor A="255" R="255" G="255" B="255" />
                <PrePosition X="0.1250" />
                <PreSize X="0.9583" Y="0.1538" />
                <BackGroundData Type="Default" Path="Default/Slider_Back.png" Plist="" />
                <ProgressBarData Type="Default" Path="Default/Slider_PressBar.png" Plist="" />
                <BallNormalData Type="Default" Path="Default/SliderNode_Normal.png" Plist="" />
                <BallPressedData Type="Default" Path="Default/SliderNode_Press.png" Plist="" />
                <BallDisabledData Type="Default" Path="Default/SliderNode_Disable.png" Plist="" />
              </AbstractNodeData>
              <AbstractNodeData Name="Back" ActionTag="259485318" Tag="37" IconVisible="False" HorizontalEdge="LeftEdge" VerticalEdge="TopEdge" LeftMargin="20.0000" RightMargin="620.0000" BottomMargin="20.0000" TouchEnable="True" FontSize="18" Scale9Width="64" Scale9Height="64" OutlineSize="0" ShadowOffsetX="0.0000" ShadowOffsetY="0.0000" ctype="ButtonObjectData">
                <Size X="80.0000" Y="80.0000" />
                <AnchorPoint ScaleY="0.5000" />
                <Position X="20.0000" Y="110.0000" />
                <Scale ScaleX="1.0000" ScaleY="1.0000" />
                <CColor A="255" R="255" G="255" B="255" />
                <PrePosition X="0.0278" Y="0.5000" />
                <PreSize X="0.1111" Y="0.8333" />
                <TextColor A="255" R="199" G="199" B="199" />
                <DisabledFileData Type="Normal" Path="img/back_btn_on.png" Plist="" />
                <PressedFileData Type="Normal" Path="img/back_btn_on.png" Plist="" />
                <NormalFileData Type="Normal" Path="img/back_btn_off.png" Plist="" />
                <OutlineColor A="255" R="255" G="0" B="0" />
                <ShadowColor A="255" R="255" G="127" B="80" />
              </AbstractNodeData>
              <AbstractNodeData Name="PlayTime" ActionTag="1409097770" Tag="39" IconVisible="False" HorizontalEdge="LeftEdge" VerticalEdge="BottomEdge" LeftMargin="20.0000" RightMargin="794.0000" TopMargin="103.0000" BottomMargin="5.0000" FontSize="32" LabelText="Text Label" ShadowOffsetX="2.0000" ShadowOffsetY="-2.0000" ctype="TextObjectData">
                <Size X="146.0000" Y="42.0000" />
                <AnchorPoint />
                <Position X="20.0000" Y="5.0000" />
                <Scale ScaleX="1.0000" ScaleY="1.0000" />
                <CColor A="255" R="255" G="255" B="255" />
                <PrePosition X="0.0208" Y="0.0333" />
                <PreSize X="0.1521" Y="0.2800" />
                <FontResource Type="Normal" Path="DroidSansFallback.ttf" Plist="" />
                <OutlineColor A="255" R="255" G="0" B="0" />
                <ShadowColor A="255" R="110" G="110" B="110" />
              </AbstractNodeData>
              <AbstractNodeData Name="RemainTime" ActionTag="-1091607380" Tag="40" IconVisible="False" HorizontalEdge="RightEdge" VerticalEdge="BottomEdge" LeftMargin="794.0000" RightMargin="20.0000" TopMargin="98.0000" BottomMargin="5.0000" FontSize="32" LabelText="Text Label" ShadowOffsetX="2.0000" ShadowOffsetY="-2.0000" ctype="TextObjectData">
                <Size X="146.0000" Y="42.0000" />
                <AnchorPoint ScaleX="1.0000" />
                <Position X="940.0000" Y="5.0000" />
                <Scale ScaleX="1.0000" ScaleY="1.0000" />
                <CColor A="255" R="255" G="255" B="255" />
                <PrePosition X="0.0521" Y="-0.0667" />
                <PreSize X="0.1521" Y="0.2800" />
                <FontResource Type="Normal" Path="DroidSansFallback.ttf" Plist="" />
                <OutlineColor A="255" R="255" G="0" B="0" />
                <ShadowColor A="255" R="110" G="110" B="110" />
              </AbstractNodeData>
            </Children>
            <AnchorPoint />
            <Position Y="490.0000" />
            <Scale ScaleX="1.0000" ScaleY="1.0000" />
            <CColor A="255" R="255" G="255" B="255" />
            <PrePosition Y="0.7656" />
            <PreSize X="1.0000" Y="0.2344" />
            <SingleColor A="255" R="42" G="42" B="42" />
            <FirstColor A="255" R="150" G="200" B="255" />
            <EndColor A="255" R="255" G="255" B="255" />
            <ColorVector ScaleY="1.0000" />
          </AbstractNodeData>
        </Children>
      </ObjectData>
    </Content>
  </Content>
</GameFile>