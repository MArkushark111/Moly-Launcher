import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: panel
    property var instance: ({})
    property var shell
    property string loadedId: ""
    property string profile: "custom"
    property var shaderEntries: []
    spacing: 14
    enabled: !launcher.busy && !instance.running
    function load() {
        if(instance.engine!=="moly-rust")return
        loadedId=instance.id
        const values=launcher.rustVisualSettings()
        managed.checked=values.managed===true
        near.value=values.nearChunks||12
        far.value=values.distantChunks===undefined?256:values.distantChunks
        scale.value=values.resolutionScale||100
        fps.value=values.fps||144
        aa.checked=values.fxaa===true
        sync.checked=values.vsync!==false
        api.currentIndex=Math.max(0,["auto","vulkan","dx12","opengl"].indexOf(values.graphicsApi||"auto"))
        shaderEntries=launcher.rustShaderPacks()
        shader.currentIndex=Math.max(0,shaderEntries.findIndex(function(pack){return pack.file===(values.shaderPack||"")}))
        profile=values.profile||"custom"
    }
    function preset(name) {
        managed.checked=true
        if(name==="swift") {near.value=8;far.value=64;scale.value=75;fps.value=120;aa.checked=false}
        if(name==="balanced") {near.value=12;far.value=256;scale.value=100;fps.value=144;aa.checked=true}
        if(name==="vista") {near.value=20;far.value=1024;scale.value=100;fps.value=144;aa.checked=true}
        profile=name
    }
    function save() {
        launcher.saveRustVisualSettings({managed:managed.checked,profile:profile,nearChunks:Math.round(near.value),distantChunks:Math.round(far.value),resolutionScale:Math.round(scale.value),fps:fps.value,fxaa:aa.checked,vsync:sync.checked,graphicsApi:["auto","vulkan","dx12","opengl"][api.currentIndex],shaderPack:shader.currentValue||""})
    }
    onInstanceChanged: if(instance.id!==loadedId)Qt.callLater(load)
    Component.onCompleted: Qt.callLater(load)
    Panel {
        Layout.fillWidth: true
        contentItem: ColumnLayout {
            spacing: 14
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    Text {text:"MOLY / RUST EDITION";color:Theme.accent;font.pixelSize:10;font.letterSpacing:2}
                    Text {text:"Your view, your way.";color:Theme.text;font.pixelSize:25;font.weight:Font.DemiBold}
                    Text {text:"Choose how much detail to keep nearby and how far the landscape stretches.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
                }
                Rectangle {
                    Layout.preferredWidth:120;Layout.preferredHeight:86;radius:16;color:Theme.accentSoft
                    Rectangle {anchors.centerIn:parent;width:76;height:76;radius:38;color:"transparent";border.color:Theme.accent;opacity:0.35}
                    Rectangle {anchors.centerIn:parent;width:48;height:48;radius:24;color:"transparent";border.color:Theme.accent;opacity:0.7}
                    Icon {anchors.centerIn:parent;name:"world";size:23;color:Theme.accent}
                }
            }
            RowLayout {Layout.fillWidth:true
                Text {text:"Manage graphics from Moly";color:Theme.text;Layout.fillWidth:true;font.pixelSize:13}
                MSwitch {id:managed;Accessible.name:"Manage Rust graphics from Moly"}
            }
            Text {text:managed.checked?"Saved here and applied before your next launch.":"The game keeps control of its graphics settings. Enable this to use Moly's presets.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
            RowLayout {
                Layout.fillWidth:true;spacing:10
                Repeater {
                    model:[{id:"swift",name:"Swift",hint:"Light & responsive"},{id:"balanced",name:"Everyday",hint:"Detail meets distance"},{id:"vista",name:"Vista",hint:"1024-chunk skyline"}]
                    delegate:ItemDelegate {
                        required property var modelData
                        Layout.fillWidth:true;implicitHeight:68;padding:12
                        onClicked:panel.preset(modelData.id)
                        background:Rectangle {radius:13;color:panel.profile===modelData.id?Theme.accentSoft:parent.hovered?Theme.surface2:Theme.surface;border.color:panel.profile===modelData.id?Theme.accent:Theme.border}
                        contentItem:ColumnLayout {spacing:5;Text{text:modelData.name;color:Theme.text;font.pixelSize:14;font.weight:Font.DemiBold}Text{text:modelData.hint;color:Theme.textSecondary;font.pixelSize:10}}
                    }
                }
            }
        }
    }
    Panel {
        Layout.fillWidth:true
        contentItem:ColumnLayout {
            spacing:12;enabled:managed.checked
            Text {text:"Landscape";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold}
            RowLayout {Layout.fillWidth:true;Text{text:"Full-detail chunks";color:Theme.text;Layout.fillWidth:true;font.pixelSize:12}Text{text:Math.round(near.value)+" chunks";color:Theme.accent;font.pixelSize:13}}
            MSlider {id:near;Layout.fillWidth:true;from:2;to:64;stepSize:1;value:12;Accessible.name:"Full-detail render distance";onMoved:panel.profile="custom"}
            RowLayout {Layout.fillWidth:true;Text{text:"Distant terrain";color:Theme.text;Layout.fillWidth:true;font.pixelSize:12}Text{text:far.value===0?"Off":Math.round(far.value)+" chunks";color:Theme.accent;font.pixelSize:13}}
            MSlider {id:far;Layout.fillWidth:true;from:0;to:1024;stepSize:32;value:256;Accessible.name:"Distant terrain render distance up to 1024 chunks";onMoved:panel.profile="custom"}
            Text {text:"Distant terrain uses simplified geometry. 1024 chunks is not full detail at that distance; some integrated GPUs cap it at 128.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
            Text {visible:near.value>32;text:"Large full-detail distances can use substantial memory and slow chunk loading.";color:Theme.danger;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
        }
    }
    Panel {
        Layout.fillWidth:true
        contentItem:ColumnLayout {
            spacing:12;enabled:managed.checked
            Text {text:"Colour & shaders";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold}
            Text {text:"Moly Colour adds a subtle colour grade. Native .hshader packs and compatible shaders/ ZIPs can also be selected here.";color:Theme.textSecondary;font.pixelSize:12;wrapMode:Text.Wrap;Layout.fillWidth:true}
            RowLayout {Layout.fillWidth:true
                MComboBox {id:shader;Layout.fillWidth:true;model:panel.shaderEntries;textRole:"name";valueRole:"file";Accessible.name:"Rust shader pack"}
                MButton {text:"Add pack";iconName:"plus";onClicked:shell.chooseFile("local","shader")}
                MButton {text:"Refresh";kind:"ghost";onClicked:{const selected=shader.currentValue;panel.shaderEntries=launcher.rustShaderPacks();shader.currentIndex=Math.max(0,panel.shaderEntries.findIndex(function(p){return p.file===selected}))}}
            }
            Text {text:"Pack compatibility varies. The game reports shader errors and keeps its normal renderer available; pack options are in the in-game Shader Packs screen.";color:Theme.textMuted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true}
        }
    }
    Panel {
        Layout.fillWidth:true
        contentItem:ColumnLayout {
            spacing:12;enabled:managed.checked
            Text {text:"Frame comfort";color:Theme.text;font.pixelSize:17;font.weight:Font.DemiBold}
            RowLayout {Layout.fillWidth:true;Text{text:"Render scale";color:Theme.text;Layout.fillWidth:true;font.pixelSize:12}Text{text:Math.round(scale.value)+"%";color:Theme.accent;font.pixelSize:13}}
            MSlider {id:scale;Layout.fillWidth:true;from:50;to:150;stepSize:5;value:100;Accessible.name:"Rust render scale";onMoved:panel.profile="custom"}
            RowLayout {Layout.fillWidth:true;spacing:12
                Text {text:"FPS cap";color:Theme.text;font.pixelSize:12}
                MSpinBox {id:fps;from:30;to:1000;stepSize:10;editable:true;value:144;Layout.preferredWidth:130}
                Item {Layout.fillWidth:true}
                MCheckBox {id:aa;text:"FXAA"}
                MCheckBox {id:sync;text:"VSync";checked:true}
            }
            Text {text:"Graphics API";color:Theme.textSecondary;font.pixelSize:12}
            MComboBox {id:api;Layout.fillWidth:true;model:["Automatic","Vulkan","DirectX 12","OpenGL"];Accessible.name:"Rust graphics API"}
        }
    }
    RowLayout {
        Layout.fillWidth:true
        MButton {text:"Reload settings";kind:"ghost";onClicked:panel.load()}
        Item {Layout.fillWidth:true}
        MButton {text:"Save Rust visuals";kind:"primary";onClicked:panel.save()}
    }
}
