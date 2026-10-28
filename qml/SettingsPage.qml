import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id:page
    property int section: 0
    onSectionChanged: settingsSlide.restart()
    property var shell
    property var initialSettings: launcher.settings
    Component.onCompleted: initialSettings=launcher.settings
    ColumnLayout {
        anchors.fill:parent;anchors.margins:24;spacing:16
        RowLayout{Layout.fillWidth:true
            ColumnLayout{spacing:5;Layout.fillWidth:true
                Text{text:"Settings";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:26;font.weight:Font.Bold;font.letterSpacing:-0.5}
                Text{text:"Appearance, game defaults, and connections.";color:Theme.textSecondary;font.pixelSize:13}
            }
            Item{Layout.fillWidth:true}
            MButton{text:"Save settings";kind:"primary";enabled:!launcher.busy;onClicked:launcher.saveSettings({memoryMb:Math.round(ram.value),width:resWidth.value,height:resHeight.value,fullscreen:fullscreen.checked,reducedMotion:motion.checked,keepVisible:keepVisible.checked,launcherFullscreen:launcherFullscreen.checked,javaPath:javaPath.text.trim(),jvmArgs:jvmArgs.text.trim()})}
        }
        TabStrip { tabs:[{label:"Appearance",icon:"file"},{label:"Game",icon:"play"},{label:"Connections",icon:"accounts"},{label:"Advanced",icon:"settings"}];currentIndex:page.section;onActivated:function(index){page.section=index} }
        ScrollView{id:scroll;Layout.fillWidth:true;Layout.fillHeight:true;clip:true;contentWidth:availableWidth
            ColumnLayout{id:settingsBody;width:scroll.availableWidth;spacing:16
                transform: Translate { id: settingsOffset }
                ParallelAnimation { id:settingsSlide; NumberAnimation { target:settingsBody;property:"opacity";from:Theme.motion?0.3:1;to:1;duration:Theme.motion?180:0 } NumberAnimation { target:settingsOffset;property:"x";from:Theme.motion?14:0;to:0;duration:Theme.motion?220:0;easing.type:Easing.OutCubic } }
                Panel{visible:page.section===0;Layout.fillWidth:true
                    ColumnLayout{anchors.fill:parent;spacing:14
                        Text{text:"Appearance";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                        RowLayout{Layout.fillWidth:true;Text{text:"Show server descriptions";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MSwitch{checked:launcher.settings.showServerMotd===true;onClicked:launcher.setUiPreferences({showServerMotd:checked})}}
                        RowLayout{Layout.fillWidth:true;Text{text:"Show server addresses";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MSwitch{checked:launcher.settings.showServerAddress!==false;onClicked:launcher.setUiPreferences({showServerAddress:checked})}}

                        Text{text:"Accent color";color:Theme.textSecondary;font.pixelSize:13}
                        RowLayout{Layout.fillWidth:true;spacing:10
                            Repeater{model:["#8b5cf6","#3b82f6","#22c55e","#f97316","#ef4444","#ec4899","#06b6d4","#a1a1aa"]
                                delegate:AbstractButton{required property string modelData;width:29;height:29;implicitWidth:29;implicitHeight:29;Accessible.name:"Accent "+modelData;onClicked:launcher.setAccentColor(modelData)
                                    background:Rectangle{radius:8;color:modelData;Rectangle{anchors.fill:parent;anchors.margins:-3;radius:11;color:Theme.transparent;border.color:Theme.textSecondary;border.width:2;visible:Qt.colorEqual(Theme.accent,modelData)||parent.parent.activeFocus}}
                                }
                            }
                            Item{Layout.fillWidth:true}
                            MField{id:customAccent;Layout.preferredWidth:105;implicitHeight:34;text:Theme.accent.toString();placeholderText:"#8b5cf6";maximumLength:7;validator:RegularExpressionValidator{regularExpression:/#[0-9a-fA-F]{6}/} onAccepted:if(acceptableInput)launcher.setAccentColor(text)}
                            MButton{text:"Apply";compact:true;enabled:customAccent.acceptableInput;onClicked:launcher.setAccentColor(customAccent.text)}
                        }
                        RowLayout{Layout.fillWidth:true;spacing:16
                            Rectangle{Layout.fillWidth:true;implicitHeight:74;radius:7;color:Theme.app;border.color:Theme.border
                                RowLayout{anchors.fill:parent;anchors.margins:12;spacing:14
                                    MButton{text:"Primary button";kind:"primary";compact:true}
                                    Rectangle{Layout.fillWidth:true;height:30;radius:5;color:Theme.accentSoft;RowLayout{anchors.fill:parent;anchors.leftMargin:9;anchors.rightMargin:9;Icon{name:"check";size:14;color:Theme.accent}Text{text:"Selected item";color:Theme.text;font.pixelSize:12;Layout.fillWidth:true}}}
                                    ColumnLayout{spacing:8;Text{text:"Active tab";color:Theme.text;font.pixelSize:12}Rectangle{width:54;height:2;color:Theme.accent}}
                                }
                            }
                            MButton{text:"Reset to default";compact:true;kind:"ghost";onClicked:launcher.setAccentColor("#8b5cf6")}
                        }
                        RowLayout{Layout.fillWidth:true;spacing:14
                            RowLayout{spacing:9
                                Text{text:"Reduce motion";color:Theme.text;font.pixelSize:12}
                                MSwitch{id:motion;objectName:"reduceMotionSwitch";checked:launcher.settings.reducedMotion===true;Accessible.name:"Reduce motion";onToggled:launcher.setReducedMotion(checked)}
                            }
                            RowLayout{spacing:9
                                Text{text:"Open launcher fullscreen";color:Theme.text;font.pixelSize:12}
                                MSwitch{id:launcherFullscreen;objectName:"motionPreviewSwitch";checked:page.initialSettings.launcherFullscreen||false;Accessible.name:"Open launcher fullscreen"}
                            }
                        }
                        Text{text:"Reduce motion saves immediately and turns off sliding, scaling, and fading effects.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                    }
                }
                Panel {visible:page.section===0;
                    Layout.fillWidth: true
                    contentItem: ColumnLayout {
                        spacing: 12
                        Text { text: "Personalization & activity"; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
                        Text { text: "These options apply and save immediately."; color: Theme.textMuted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap }
                        GridLayout {
                            Layout.fillWidth: true
                            columns: page.width > 740 ? 2 : 1
                            columnSpacing: 24
                            rowSpacing: 8
                            Repeater {
                                model: [
                                    { key: "animateArtwork", label: "Animate GIF artwork" },
                                    { key: "skinIdle", label: "Idle player movement" },
                                    { key: "quickAccess", label: "Show worlds and servers in Library" },
                                    { key: "serverPing", label: "Check saved server status" },
                                    { key: "showInstanceRail", label: "Show instance sidebar" },
                                    { key: "showModCounts", label: "Show mod counts in Library" },
                                    { key: "showCompletedTasks", label: "Show completed tasks" },
                                    { key: "showQuickAccessRefresh", label: "Show worlds and servers refresh" },
                                    { key: "openLogsOnLaunch", label: "Show logs when Minecraft starts" }
                                ]
                                delegate: RowLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 10
                                    Text { text: modelData.label; color: Theme.text; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap }
                                    MSwitch {
                                        checked: launcher.settings[modelData.key] !== false
                                        Accessible.name: modelData.label
                                        onToggled: {
                                            const change = {}
                                            change[modelData.key] = checked
                                            launcher.setUiPreferences(change)
                                        }
                                    }
                                }
                            }
                        }
                        Text { text: "Reduce motion also pauses artwork and player movement. Server checks show player counts and connection time."; color: Theme.textMuted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }
                        Text { text: "Server signal color"; color: Theme.textSecondary; font.pixelSize: 12 }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 9
                            Row {
                                spacing: 3
                                Layout.preferredWidth: 29
                                Layout.preferredHeight: 22
                                Repeater { model: 4; Rectangle { required property int index; width: 4; height: 5 + index * 4; y: 21 - height; radius: 1; color: Theme.serverAccent } }
                            }
                            Repeater {
                                model: ["#8b5cf6", "#3b82f6", "#22c55e", "#f97316", "#ec4899"]
                                delegate: AbstractButton {
                                    required property string modelData
                                    implicitWidth: 25
                                    implicitHeight: 25
                                    Accessible.name: "Signal color " + modelData
                                    onClicked: launcher.setUiPreferences({ pingColor: modelData })
                                    background: Rectangle { radius: 7; color: modelData; border.width: parent.activeFocus || Qt.colorEqual(Theme.serverAccent, modelData) ? 2 : 0; border.color: Theme.textSecondary }
                                }
                            }
                            Item { Layout.fillWidth: true }
                            MField {
                                id: signalColor
                                Layout.preferredWidth: 105
                                implicitHeight: 34
                                text: Theme.serverAccent.toString()
                                placeholderText: "#8b5cf6"
                                maximumLength: 7
                                validator: RegularExpressionValidator { regularExpression: /#[0-9a-fA-F]{6}/ }
                                Accessible.name: "Custom server signal color"
                                onAccepted: if(acceptableInput) launcher.setUiPreferences({ pingColor: text })
                            }
                            MButton { text: "Apply"; compact: true; enabled: signalColor.acceptableInput; onClicked: launcher.setUiPreferences({ pingColor: signalColor.text }) }
                            MButton { text: "Follow accent"; compact: true; kind: "ghost"; onClicked: launcher.setUiPreferences({ pingColor: "" }) }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Tile background shade"; color: Theme.textSecondary; font.pixelSize: 12; Layout.fillWidth: true }
                            Text { text: Math.round(backgroundShade.value * 100) + "%"; color: Theme.textMuted; font.pixelSize: 12 }
                        }
                        MSlider {
                            id: backgroundShade
                            Layout.fillWidth: true
                            from: 0.2
                            to: 0.95
                            stepSize: 0.01
                            value: launcher.settings.backgroundDim === undefined ? 0.5 : launcher.settings.backgroundDim
                            property bool ready: false
                            Component.onCompleted: ready = true
                            Accessible.name: "Instance background shade"
                            onPressedChanged: if(ready && !pressed) launcher.setUiPreferences({ backgroundDim: value })
                            onMoved: if(!pressed) launcher.setUiPreferences({ backgroundDim: value })
                        }
                        Text { text: "Higher values darken instance backgrounds to keep text readable."; color: Theme.textMuted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    }
                }
                Panel {visible:page.section===0;Layout.fillWidth:true
                    contentItem:ColumnLayout {spacing:14
                        Text {text:"Make room for your setup";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                        RowLayout {Layout.fillWidth:true;Text {text:"Instance sidebar width";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}Text {text:Math.round(railWidth.value)+" px";color:Theme.textMuted;font.pixelSize:12}}
                        MSlider {id:railWidth;Layout.fillWidth:true;from:160;to:260;stepSize:10;value:launcher.settings.sidebarWidth||200;onMoved:launcher.setUiPreferences({sidebarWidth:Math.round(value)})}
                        RowLayout {Layout.fillWidth:true;Text {text:"Worlds and servers shown in Library";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}MSpinBox {from:1;to:8;value:launcher.settings.quickAccessRows||3;onValueModified:launcher.setUiPreferences({quickAccessRows:value})}}
                        RowLayout {Layout.fillWidth:true;Text {text:"Open Moly on";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}MComboBox {Layout.preferredWidth:180;model:["Library","Discover","Wardrobe"];currentIndex:Math.max(0,["library","discover","skins"].indexOf(launcher.settings.startupPage||"library"));onActivated:launcher.setUiPreferences({startupPage:["library","discover","skins"][currentIndex]})}}
                        RowLayout {Layout.fillWidth:true;Text {text:"Notification duration";color:Theme.textSecondary;font.pixelSize:12;Layout.fillWidth:true}MSpinBox {from:2;to:15;value:launcher.settings.toastSeconds||6;onValueModified:launcher.setUiPreferences({toastSeconds:value})}Text {text:"seconds";color:Theme.textMuted;font.pixelSize:12}}
                    }
                }
                Panel{visible:page.section===1;Layout.fillWidth:true
                    ColumnLayout{anchors.fill:parent;spacing:12
                        Text{text:"Minecraft defaults";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                        RowLayout{Layout.fillWidth:true;Text{text:"Maximum memory";color:Theme.textSecondary;font.pixelSize:13;Layout.fillWidth:true}Text{text:(ram.value/1024).toFixed(1)+" GB / "+(launcher.memoryTotal/1024).toFixed(1)+" GB available";color:Theme.textSecondary;font.pixelSize:12}}
                        MSlider {id:ram;objectName:"memoryDraftSlider";Layout.fillWidth:true;from:512;to:Math.max(1024,launcher.memoryTotal-1024);stepSize:256;value:page.initialSettings.memoryMb}
                        Text{visible:ram.value>launcher.memoryTotal*0.65;text:"This leaves little memory for Windows and other apps.";color:Theme.danger;font.pixelSize:12}
                        RowLayout{Layout.fillWidth:true;spacing:12
                            Text{text:"Game resolution";color:Theme.textSecondary;font.pixelSize:13}
                            MSpinBox {id:resWidth;from:640;to:7680;stepSize:80;value:page.initialSettings.width;editable:true;Layout.preferredWidth:125}
                            Text{text:"×";color:Theme.textSecondary}
                            MSpinBox {id:resHeight;from:360;to:4320;stepSize:40;value:page.initialSettings.height;editable:true;Layout.preferredWidth:125}
                            Item{Layout.fillWidth:true}
                            RowLayout{spacing:9
                                Text{text:"Fullscreen";color:Theme.text;font.pixelSize:12}
                                MSwitch{id:fullscreen;checked:page.initialSettings.fullscreen||false;Accessible.name:"Fullscreen"}
                            }
                        }
                        RowLayout{Layout.fillWidth:true;spacing:9
                            Text{text:"Keep launcher visible while playing";color:Theme.text;font.pixelSize:12;Layout.fillWidth:true}
                            MSwitch{id:keepVisible;checked:page.initialSettings.keepVisible!==false;Accessible.name:"Keep launcher visible while playing"}
                        }
                    }
                }
                Panel{visible:page.section===2;Layout.fillWidth:true
                    ColumnLayout{anchors.fill:parent;spacing:12
                        Text{text:"Connections";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                        RowLayout{Layout.fillWidth:true;Text{text:"Microsoft account";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MButton{text:"Choose account type";iconName:"accounts";onClicked:shell.showAccountChooser()}}
                        Text{text:launcher.microsoftAvailable?"Browser sign-in is ready.":"Microsoft sign-in is waiting for Moly's application registration. Players do not need a client ID.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                        RowLayout{Layout.fillWidth:true;Text{text:"Show account chooser on startup";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MSwitch{checked:launcher.settings.showAccountChooser===true;onClicked:launcher.setUiPreferences({showAccountChooser:checked})}}
                        Rectangle{Layout.fillWidth:true;height:1;color:Theme.border}
                        RowLayout{Layout.fillWidth:true;Text{text:"Discord Rich Presence";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MSwitch{checked:launcher.settings.discordPresence!==false;onClicked:launcher.setUiPreferences({discordPresence:checked})}}
                        Text{text:discordPresence.status; color:Theme.textMuted;font.pixelSize:12}
                        RowLayout{Layout.fillWidth:true;Text{text:"Share public server address";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MSwitch{checked:launcher.settings.discordShareServer!==false;onClicked:launcher.setUiPreferences({discordShareServer:checked})}}
                        Text{text:"Numeric IPs, localhost and local network names appear as Private server. Presence reflects the server launched through Moly; switching servers inside Minecraft is not detected.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
                        Rectangle{Layout.fillWidth:true;height:1;color:Theme.border;Layout.topMargin:4;Layout.bottomMargin:4}
                        RowLayout{Layout.fillWidth:true;Text{text:"CurseForge";color:Theme.textSecondary;font.pixelSize:13;Layout.fillWidth:true}Badge{text:launcher.curseConnected?"Connected":"Not connected";tint:Theme.textSecondary}}
                        Text{text:"Your existing CurseForge connection is managed securely by Windows. Modrinth needs no account.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                    }
                }
                Panel{visible:page.section===3;Layout.fillWidth:true
                    ColumnLayout{anchors.fill:parent;spacing:12
                        Text{text:"Java";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                        Text{text:"Leave the executable blank for automatic runtime selection.";color:Theme.textSecondary;font.pixelSize:12}
                        MField{id:javaPath;Layout.fillWidth:true;text:page.initialSettings.javaPath||"";placeholderText:"Java executable (automatic)"}
                        MField{id:jvmArgs;Layout.fillWidth:true;text:page.initialSettings.jvmArgs||"";placeholderText:"Additional JVM arguments (optional)"}
                    }
                }
                Panel{visible:page.section===3;Layout.fillWidth:true
                    ColumnLayout{anchors.fill:parent;spacing:12
                        Text{text:"Storage";color:Theme.text;font.pixelSize:16;font.weight:Font.DemiBold}
                        RowLayout{Layout.fillWidth:true;Text{text:launcher.dataRoot;color:Theme.textSecondary;font.pixelSize:12;elide:Text.ElideMiddle;Layout.fillWidth:true}MButton{text:"Move storage";iconName:"folder";enabled:!launcher.busy;onClicked:shell.chooseFolder("storage")}}
                        RowLayout {Layout.fillWidth:true;Text {text:"Moly pack files";color:Theme.text;font.pixelSize:13;Layout.fillWidth:true}MButton {text:"Use Moly for .mlypack";iconName:"file";onClicked:launcher.registerPackFiles()}}
                        Text {text:"Adds Moly's icon in Explorer and lets you double-click a pack to import it. Applies to your Windows user.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
                        Text{text:"Copies files to a new location. Restart Moly afterwards, then repair instances to refresh their paths.";color:Theme.textMuted;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                    }
                }
                Item{implicitHeight:8}
            }
        }
    }
}
