import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id:intro
    objectName:"firstLaunchIntro"
    property var shell
    property int step:0
    readonly property var titles:["A little less setup.\nA lot more Minecraft.","Every world,\nits own space.","Make yourself\nat home.","Your next world\nis waiting."]
    readonly property var descriptions:["Welcome to Moly. Build a setup, bring your favorites, and get straight to playing.","Instances keep worlds, mods and settings together. Try something new without changing your other setups.","Pick an accent and the movement that feels right. You can change everything later in Settings.","Choose a local nickname or sign in with the Microsoft account that owns Minecraft Java Edition."]
    parent:Overlay.overlay;anchors.centerIn:parent;width:Math.min(1020,parent.width-40);height:Math.min(650,parent.height-40)
    modal:true;focus:true;padding:0;closePolicy:Popup.NoAutoClose
    function replay(){step=0;open()}
    function finish(){launcher.setUiPreferences({introVersion:1,accountSetupDone:true});close()}
    function next(){if(step<3){step++;reveal.restart()}}
    enter:Transition{ParallelAnimation{NumberAnimation{property:"opacity";from:0;to:1;duration:Theme.motion?250:0}NumberAnimation{property:"scale";from:Theme.motion?0.94:1;to:1;duration:Theme.motion?420:0;easing.type:Easing.OutQuint}}}
    exit:Transition{NumberAnimation{property:"opacity";to:0;duration:Theme.motion?180:0}}
    background:Rectangle{radius:28;color:Theme.app;border.color:Theme.border}
    contentItem:Item {
        RowLayout {
            anchors.fill:parent;anchors.margins:34;spacing:36
            ColumnLayout {
                Layout.fillWidth:true;Layout.preferredWidth:1;Layout.fillHeight:true;spacing:18
                RowLayout{spacing:10;Image{source:"../assets/moly.svg";Layout.preferredWidth:32;Layout.preferredHeight:32}Text{text:"Moly";font.pixelSize:20;font.weight:Font.Bold;color:Theme.text}Text{text:" /  WELCOME";font.pixelSize:10;font.letterSpacing:2;color:Theme.textMuted}}
                Item{Layout.fillHeight:true}
                ColumnLayout {
                    id:copy;Layout.fillWidth:true;spacing:18
                    transform:Translate{id:copyShift}
                    ParallelAnimation{id:reveal;NumberAnimation{target:copy;property:"opacity";from:Theme.motion?0:1;to:1;duration:Theme.motion?280:0}NumberAnimation{target:copyShift;property:"x";from:Theme.motion?20:0;to:0;duration:Theme.motion?380:0;easing.type:Easing.OutQuint}}
                    Text{text:["MEET YOUR LAUNCHER","ROOM TO EXPERIMENT","YOUR MOLY","LET’S PLAY"][intro.step];color:Theme.accent;font.pixelSize:10;font.letterSpacing:2;font.weight:Font.DemiBold}
                    Text{text:intro.titles[intro.step];color:Theme.text;font.pixelSize:intro.width<850?29:36;font.weight:Font.Bold;lineHeight:1.1;Layout.fillWidth:true;wrapMode:Text.Wrap}
                    Text{text:intro.descriptions[intro.step];color:Theme.textSecondary;font.pixelSize:14;lineHeight:1.3;Layout.fillWidth:true;wrapMode:Text.Wrap}
                    ColumnLayout {
                        visible:intro.step===2;Layout.fillWidth:true;spacing:18
                        RowLayout {spacing:10;Repeater{model:["#8b5cf6","#3b82f6","#22c55e","#f97316","#ec4899"];delegate:AbstractButton{required property string modelData;implicitWidth:34;implicitHeight:34;onClicked:launcher.setAccentColor(modelData);Accessible.name:"Accent "+modelData;background:Rectangle{radius:11;color:modelData;border.width:Qt.colorEqual(Theme.accent,modelData)?3:0;border.color:Theme.text}}}}
                        RowLayout{Layout.fillWidth:true;Text{text:"Reduce motion";color:Theme.text;Layout.fillWidth:true}MSwitch{checked:launcher.settings.reducedMotion===true;onClicked:launcher.setReducedMotion(checked)}}
                    }
                    ColumnLayout {
                        visible:intro.step===3;Layout.fillWidth:true;spacing:10
                        MButton{text:"Sign in with Microsoft";iconName:"microsoft";kind:"primary";Layout.fillWidth:true;enabled:!launcher.busy;onClicked:{intro.finish();shell.navigate("accounts");launcher.signIn()}}
                        RowLayout{Layout.fillWidth:true;MField{id:nickname;Layout.fillWidth:true;placeholderText:"Local nickname";maximumLength:16}MButton{text:"Create";enabled:/^[A-Za-z0-9_]{3,16}$/.test(nickname.text)&&!launcher.busy;onClicked:{launcher.addOfflineAccount(nickname.text);intro.finish()}}}
                        Text{visible:!!launcher.activeAccount.name;text:"Already playing as "+launcher.activeAccount.name;color:Theme.textMuted;font.pixelSize:11}
                    }
                }
                Item{Layout.fillHeight:true}
                RowLayout {
                    Layout.fillWidth:true;spacing:8
                    Repeater{model:4;delegate:Rectangle{required property int index;Layout.preferredWidth:intro.step===index?28:7;Layout.preferredHeight:7;height:7;radius:4;color:intro.step===index?Theme.accent:Theme.border;Behavior on Layout.preferredWidth{NumberAnimation{duration:Theme.motion?250:0}}Behavior on color{ColorAnimation{duration:Theme.motion?220:0}}}}
                    Item{Layout.fillWidth:true}
                    MButton{visible:intro.step>0;text:"Back";kind:"ghost";compact:true;onClicked:{intro.step--;reveal.restart()}}
                    MButton{text:intro.step===0?"Let’s begin":intro.step===3?"Explore Moly":"Next";kind:intro.step===3?"secondary":"primary";onClicked:intro.step===3?intro.finish():intro.next()}
                }
            }
            Rectangle {
                id:stage;Layout.fillWidth:true;Layout.preferredWidth:1;Layout.fillHeight:true;radius:24;color:Theme.surface;border.color:Theme.border;clip:true
                Rectangle{width:280;height:280;radius:140;anchors.centerIn:parent;color:Theme.accentSoft;scale:intro.step===0?1.2:0.85;Behavior on scale{NumberAnimation{duration:Theme.motion?700:0;easing.type:Easing.OutQuint}}}
                Rectangle{width:330;height:330;radius:165;anchors.centerIn:parent;color:"transparent";border.color:Theme.border;rotation:20}
                Item {
                    anchors.centerIn:parent;width:parent.width-46;height:300
                    transform:Translate{id:floatShift}
                    SequentialAnimation{running:intro.visible&&Theme.motion;loops:Animation.Infinite;NumberAnimation{target:floatShift;property:"y";from:0;to:-9;duration:2300;easing.type:Easing.InOutSine}NumberAnimation{target:floatShift;property:"y";from:-9;to:0;duration:2300;easing.type:Easing.InOutSine}}
                    Rectangle{width:parent.width-14;height:180;radius:22;y:110;x:7;rotation:intro.step===1?-7:4;color:Theme.surface2;border.color:Theme.border;Behavior on rotation{NumberAnimation{duration:Theme.motion?600:0;easing.type:Easing.OutQuint}}}
                    Rectangle {
                        width:parent.width;height:190;y:80;radius:22;color:Theme.app;border.color:Theme.border
                        ColumnLayout{anchors.fill:parent;anchors.margins:22;spacing:14
                            RowLayout{spacing:13;Rectangle{width:48;height:48;radius:14;color:Theme.accentSoft;Icon{anchors.centerIn:parent;name:intro.step===3?"play":"box";size:26;color:Theme.accent}}ColumnLayout{Layout.fillWidth:true;spacing:5;Text{text:["Your first adventure","Creative corner","A setup that feels like you","Ready when you are"][intro.step];color:Theme.text;font.pixelSize:15;font.weight:Font.DemiBold;Layout.fillWidth:true;wrapMode:Text.Wrap}Text{text:intro.step===1?"A separate space for every idea":"Minecraft Java Edition";color:Theme.textMuted;font.pixelSize:11}}}
                            Rectangle{Layout.fillWidth:true;height:1;color:Theme.border}
                            RowLayout{Layout.fillWidth:true;Icon{name:"check";size:16;color:Theme.success}Text{text:["Game files and Java, handled","Worlds stay with their instance","Your colors. Your pace.","Mods, packs, and possibilities"][intro.step];color:Theme.textSecondary;font.pixelSize:11;Layout.fillWidth:true;wrapMode:Text.Wrap}}
                            Rectangle{Layout.fillWidth:true;height:5;radius:3;color:Theme.surface2;Rectangle{height:5;radius:3;width:parent.width*(intro.step+1)/4;color:Theme.accent;Behavior on Layout.preferredWidth{NumberAnimation{duration:Theme.motion?450:0;easing.type:Easing.OutQuint}}}}
                        }
                    }
                    Image{source:"../assets/moly.svg";width:76;height:76;anchors.horizontalCenter:parent.horizontalCenter;y:-12;rotation:intro.step===2?12:0;Behavior on rotation{NumberAnimation{duration:Theme.motion?500:0;easing.type:Easing.OutBack}}}
                }
                Text{anchors.bottom:parent.bottom;anchors.bottomMargin:30;anchors.horizontalCenter:parent.horizontalCenter;text:"A LITTLE DROP OF POSSIBILITY";color:Theme.textMuted;font.pixelSize:9;font.letterSpacing:1.6}
                MButton{anchors.right:parent.right;anchors.top:parent.top;anchors.margins:12;text:"Skip intro";kind:"ghost";compact:true;onClicked:intro.finish()}
            }
        }
    }
}
