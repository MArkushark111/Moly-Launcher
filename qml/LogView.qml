import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: view
    property var shell
    property bool popped: false
    property string frozen: ""
    property bool paused: false
    property string shown: paused ? frozen : launcher.logText
    spacing:12
    RowLayout{Layout.fillWidth:true;spacing:10
        MField{id:query;Layout.fillWidth:true;placeholderText:"Search the live log…"}
        MComboBox {id:severity;model:["All messages","Warnings","Errors"];Layout.preferredWidth:140}
        MButton{text:paused?"Resume":"Pause";onClicked:{if(!paused)frozen=launcher.logText;paused=!paused}}
        MButton{text:"Save";onClicked:shell.chooseFile("log")}
        MButton{visible:!popped;text:"Pop out  ↗";onClicked:{var component=Qt.createComponent("LogWindow.qml");if(component.status===Component.Ready){var w=component.createObject(null,{shell:view.shell});w.show()}}}
    }
    Rectangle{Layout.fillWidth:true;Layout.fillHeight:true;color:Theme.app;radius:12;border.color:Theme.surface2
        ScrollView{id:scroller;anchors.fill:parent;anchors.margins:14;clip:true
            TextArea{id:logArea;readOnly:true;selectByMouse:true;wrapMode:TextEdit.WrapAnywhere;color:Theme.textSecondary;font.family:"Consolas";font.pixelSize:11;selectionColor:Theme.textMuted;background:null
                text:view.shown.split("\n").filter(function(line){return line.toLowerCase().indexOf(query.text.toLowerCase())>=0&&(severity.currentIndex===0||severity.currentIndex===1&&/warn|error|fatal/i.test(line)||severity.currentIndex===2&&/error|fatal/i.test(line))}).join("\n")||"Your Minecraft log will appear here when the game starts."
                onTextChanged:if(!view.paused)cursorPosition=length
            }
        }
    }
    Text{text:"Live output · last 2,500 lines · account tokens hidden · full session log saved up to 32 MB";color:Theme.textSecondary;font.pixelSize:10}
}
