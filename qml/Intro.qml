import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id:intro
    objectName:"firstLaunchIntro"
    property var shell
    property int step:0
    readonly property var stepTags: ["MEET YOUR LAUNCHER", "ROOM TO EXPERIMENT", "CUSTOMIZE & COMFORT", "READY TO PLAY"]
    readonly property var titles:[
        "A little less setup.\nA lot more Minecraft.",
        "Every world,\nits own space.",
        "Make yourself\nat home.",
        "Your next world\nis waiting."
    ]
    readonly property var descriptions:[
        "Welcome to Moly. Build an instance, explore curated modpacks, manage game versions, and jump straight into playing.",
        "Instances keep worlds, mods, shaders, and configs separate. Try new updates and modpacks without risking your other setups.",
        "Tailor Moly to how you play. Select your favorite accent color, choose smooth or reduced motion, and configure your defaults.",
        "Pick a quick offline nickname or sign in with the Microsoft account that owns Minecraft Java Edition."
    ]
    parent:Overlay.overlay;anchors.centerIn:parent;width:Math.min(1080,parent.width-36);height:Math.min(680,parent.height-36)
    modal:true;focus:true;padding:0;closePolicy:Popup.NoAutoClose
    function replay(){step=0;open()}
    function finish(){launcher.setUiPreferences({introVersion:1,accountSetupDone:true});close()}
    function next(){if(step<3){step++;reveal.restart()}}
    enter:Transition{ParallelAnimation{NumberAnimation{property:"opacity";from:0;to:1;duration:Theme.motion?250:0}NumberAnimation{property:"scale";from:Theme.motion?0.94:1;to:1;duration:Theme.motion?420:0;easing.type:Easing.OutQuint}}}
    exit:Transition{NumberAnimation{property:"opacity";to:0;duration:Theme.motion?180:0}}
    background:Rectangle{radius:28;color:Theme.app;border.color:Theme.border}
    contentItem:Item {
        RowLayout {
            anchors.fill:parent;anchors.margins:32;spacing:32
            ColumnLayout {
                Layout.fillWidth:true;Layout.preferredWidth:1;Layout.fillHeight:true;spacing:16
                RowLayout{
                    spacing:10
                    Image{source:"../assets/moly.svg";Layout.preferredWidth:34;Layout.preferredHeight:34}
                    Text{text:"Moly";font.pixelSize:22;font.weight:Font.Bold;color:Theme.text}
                    Rectangle{width:5;height:5;radius:3;color:Theme.border}
                    Text{text:"WELCOME EXPERIENCE";font.pixelSize:10;font.letterSpacing:2;color:Theme.textMuted}
                }
                Item{Layout.fillHeight:true}
                ColumnLayout {
                    id:copy;Layout.fillWidth:true;spacing:16
                    transform:Translate{id:copyShift}
                    ParallelAnimation{id:reveal;NumberAnimation{target:copy;property:"opacity";from:Theme.motion?0:1;to:1;duration:Theme.motion?280:0}NumberAnimation{target:copyShift;property:"x";from:Theme.motion?20:0;to:0;duration:Theme.motion?380:0;easing.type:Easing.OutQuint}}
                    Text{text:intro.stepTags[intro.step];color:Theme.accent;font.pixelSize:11;font.letterSpacing:2;font.weight:Font.Bold}
                    Text{text:intro.titles[intro.step];color:Theme.text;font.pixelSize:intro.width<850?28:34;font.weight:Font.Bold;lineHeight:1.12;Layout.fillWidth:true;wrapMode:Text.Wrap}
                    Text{text:intro.descriptions[intro.step];color:Theme.textSecondary;font.pixelSize:13;lineHeight:1.35;Layout.fillWidth:true;wrapMode:Text.Wrap}

                    // Step 0: Highlights
                    ColumnLayout {
                        visible:intro.step===0;Layout.fillWidth:true;spacing:10
                        RowLayout{spacing:10;Icon{name:"check";size:16;color:Theme.success}Text{text:"One-click loader setup (Fabric, NeoForge, Forge, Quilt)";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}}
                        RowLayout{spacing:10;Icon{name:"check";size:16;color:Theme.success}Text{text:"Integrated Modrinth & CurseForge browser";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}}
                        RowLayout{spacing:10;Icon{name:"check";size:16;color:Theme.success}Text{text:"Automatic Java management and secure token storage";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}}
                    }

                    // Step 1: Instance capabilities
                    ColumnLayout {
                        visible:intro.step===1;Layout.fillWidth:true;spacing:10
                        RowLayout{spacing:10;Icon{name:"box";size:16;color:Theme.accent}Text{text:"Separate game directories protect your worlds from mod conflicts";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}}
                        RowLayout{spacing:10;Icon{name:"folder";size:16;color:Theme.accent}Text{text:"Export & share setups via .mrpack, .mlypack or portable ZIPs";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}}
                        RowLayout{spacing:10;Icon{name:"refresh";size:16;color:Theme.accent}Text{text:"Built-in content backups and world snapshot protection";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}}
                    }

                    // Step 2: Customization & Appearance
                    ColumnLayout {
                        visible:intro.step===2;Layout.fillWidth:true;spacing:14
                        Text{text:"Accent color:";color:Theme.text;font.pixelSize:12;font.weight:Font.DemiBold}
                        RowLayout {
                            spacing:8
                            Repeater{
                                model:["#8b5cf6","#3b82f6","#22c55e","#f97316","#ef4444","#ec4899","#06b6d4","#a1a1aa"]
                                delegate:AbstractButton{
                                    required property string modelData
                                    implicitWidth:30;implicitHeight:30
                                    onClicked:launcher.setAccentColor(modelData)
                                    Accessible.name:"Accent "+modelData
                                    background:Rectangle{
                                        radius:9;color:modelData
                                        Rectangle{anchors.fill:parent;anchors.margins:-3;radius:12;color:Theme.transparent;border.color:Theme.textSecondary;border.width:2;visible:Qt.colorEqual(Theme.accent,modelData)||parent.parent.activeFocus}
                                    }
                                }
                            }
                        }
                        RowLayout{
                            Layout.fillWidth:true
                            Text{text:"Reduce motion (disable slide & float animations)";color:Theme.text;font.pixelSize:12;Layout.fillWidth:true}
                            MSwitch{checked:launcher.settings.reducedMotion===true;onClicked:launcher.setReducedMotion(checked)}
                        }
                        RowLayout{
                            Layout.fillWidth:true
                            Text{text:"Show instance sidebar rail";color:Theme.text;font.pixelSize:12;Layout.fillWidth:true}
                            MSwitch{checked:launcher.settings.showInstanceRail!==false;onToggled:launcher.setUiPreferences({showInstanceRail:checked})}
                        }
                    }

                    // Step 3: Account Setup
                    ColumnLayout {
                        visible:intro.step===3;Layout.fillWidth:true;spacing:12
                        MButton{
                            text:"Sign in with Microsoft";iconName:"microsoft";kind:"primary";Layout.fillWidth:true;enabled:!launcher.busy
                            onClicked:{intro.finish();shell.navigate("accounts");launcher.signIn()}
                        }
                        Text{text:"Or play with an offline local nickname:";color:Theme.textMuted;font.pixelSize:12}
                        RowLayout{
                            Layout.fillWidth:true;spacing:8
                            MField{id:nickname;Layout.fillWidth:true;placeholderText:"Enter local nickname";maximumLength:16;onAccepted:if(/^[A-Za-z0-9_]{3,16}$/.test(text))createLocalBtn.clicked()}
                            MButton{id:createLocalBtn;text:"Create";enabled:/^[A-Za-z0-9_]{3,16}$/.test(nickname.text)&&!launcher.busy;onClicked:{launcher.addOfflineAccount(nickname.text);intro.finish()}}
                        }
                        Text{visible:!!launcher.activeAccount.name;text:"Currently active account: "+launcher.activeAccount.name;color:Theme.accent;font.pixelSize:11;font.weight:Font.Medium}
                    }
                }
                Item{Layout.fillHeight:true}
                RowLayout {
                    Layout.fillWidth:true;spacing:8
                    Repeater{
                        model:4
                        delegate:Rectangle{
                            required property int index
                            Layout.preferredWidth:intro.step===index?28:7
                            Layout.preferredHeight:7;height:7;radius:4
                            color:intro.step===index?Theme.accent:Theme.border
                            Behavior on Layout.preferredWidth{NumberAnimation{duration:Theme.motion?250:0}}
                            Behavior on color{ColorAnimation{duration:Theme.motion?220:0}}
                        }
                    }
                    Item{Layout.fillWidth:true}
                    MButton{visible:intro.step>0;text:"Back";kind:"ghost";compact:true;onClicked:{intro.step--;reveal.restart()}}
                    MButton{
                        text:intro.step===0?"Let’s begin":intro.step===3?"Finish & Launch":"Next"
                        kind:intro.step===3?"secondary":"primary"
                        onClicked:intro.step===3?intro.finish():intro.next()
                    }
                }
            }
            Rectangle {
                id:stage;Layout.fillWidth:true;Layout.preferredWidth:1;Layout.fillHeight:true;radius:24;color:Theme.surface;border.color:Theme.border;clip:true
                Rectangle{width:280;height:280;radius:140;anchors.centerIn:parent;color:Theme.accentSoft;scale:intro.step===0?1.2:intro.step===2?1.1:0.88;Behavior on scale{NumberAnimation{duration:Theme.motion?700:0;easing.type:Easing.OutQuint}}}
                Rectangle{width:340;height:340;radius:170;anchors.centerIn:parent;color:"transparent";border.color:Theme.border;rotation:20+intro.step*25;Behavior on rotation{NumberAnimation{duration:Theme.motion?800:0;easing.type:Easing.OutCubic}}}
                Item {
                    anchors.centerIn:parent;width:parent.width-46;height:320
                    transform:Translate{id:floatShift}
                    SequentialAnimation{
                        running:intro.visible&&Theme.motion
                        loops:Animation.Infinite
                        NumberAnimation{target:floatShift;property:"y";from:0;to:-8;duration:2200;easing.type:Easing.InOutSine}
                        NumberAnimation{target:floatShift;property:"y";from:-8;to:0;duration:2200;easing.type:Easing.InOutSine}
                    }
                    Rectangle{
                        width:parent.width-14;height:190;radius:22;y:105;x:7
                        rotation:intro.step===1?-6:intro.step===2?6:-2
                        color:Theme.surface2;border.color:Theme.border
                        Behavior on rotation{NumberAnimation{duration:Theme.motion?600:0;easing.type:Easing.OutQuint}}
                    }
                    Rectangle {
                        width:parent.width;height:200;y:80;radius:22;color:Theme.app;border.color:Theme.border
                        ColumnLayout{
                            anchors.fill:parent;anchors.margins:20;spacing:12
                            RowLayout{
                                spacing:12
                                Rectangle{
                                    width:48;height:48;radius:14;color:Theme.accentSoft
                                    Icon{anchors.centerIn:parent;name:intro.step===0?"sparkle":intro.step===1?"box":intro.step===2?"settings":"play";size:24;color:Theme.accent}
                                }
                                ColumnLayout{
                                    Layout.fillWidth:true;spacing:4
                                    Text{text:["First Adventure","Independent Spaces","Your Moly Theme","Ready for Minecraft"][intro.step];color:Theme.text;font.pixelSize:15;font.weight:Font.DemiBold;Layout.fillWidth:true;wrapMode:Text.Wrap}
                                    Text{text:intro.step===0?"Pure C++20 & Qt Quick performance":intro.step===1?"Clean isolation for saves and mods":intro.step===2?"Fully personal interface & motion settings":"Start local or sign into your Mojang account";color:Theme.textMuted;font.pixelSize:11}
                                }
                            }
                            Rectangle{Layout.fillWidth:true;height:1;color:Theme.border}
                            RowLayout{
                                Layout.fillWidth:true;spacing:8
                                Icon{name:"check";size:15;color:Theme.success}
                                Text{text:["Java 8, 17, 21+ automatic detection","Preserve game saves across versions","Colors, shadows, and sidebar scaling","Skin wardrobe and live server ping"][intro.step];color:Theme.textSecondary;font.pixelSize:11;Layout.fillWidth:true;wrapMode:Text.Wrap}
                            }
                            Rectangle{
                                Layout.fillWidth:true;height:5;radius:3;color:Theme.surface2
                                Rectangle{height:5;radius:3;width:parent.width*(intro.step+1)/4;color:Theme.accent;Behavior on width{NumberAnimation{duration:Theme.motion?450:0;easing.type:Easing.OutQuint}}}
                            }
                        }
                    }
                    Image{
                        source:"../assets/moly.svg";width:74;height:74;anchors.horizontalCenter:parent.horizontalCenter;y:-10
                        rotation:intro.step===2?15:intro.step===3?-10:0
                        Behavior on rotation{NumberAnimation{duration:Theme.motion?500:0;easing.type:Easing.OutBack}}
                    }
                }
                Text{anchors.bottom:parent.bottom;anchors.bottomMargin:24;anchors.horizontalCenter:parent.horizontalCenter;text:"MOLY LAUNCHER · MINECRAFT JAVA EDITION";color:Theme.textMuted;font.pixelSize:9;font.letterSpacing:1.6}
                MButton{anchors.right:parent.right;anchors.top:parent.top;anchors.margins:12;text:"Skip intro";kind:"ghost";compact:true;onClicked:intro.finish()}
            }
        }
    }
}
