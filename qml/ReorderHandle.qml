import QtQuick
import QtQuick.Controls

Item {
    id: handle
    property string instanceId: ""
    property bool available: true
    property bool targetHandle: false
    signal moved(string id, string beforeId)
    implicitWidth: 24;implicitHeight: 30
    visible:available
    activeFocusOnTab:available
    Accessible.role:Accessible.Button
    Accessible.name:"Reorder instance. Hold Alt and press Up or Down."
    Keys.onPressed:function(event){
        if(!(event.modifiers&Qt.AltModifier))return
        const list=handle.targetHandle?playTargets.items:launcher.instances.filter(function(i){return !i._pending})
        const index=list.findIndex(function(i){return i.id===handle.instanceId})
        if(event.key===Qt.Key_Up&&index>0){handle.targetHandle?playTargets.moveTarget(handle.instanceId,list[index-1].id):launcher.moveInstance(handle.instanceId,list[index-1].id);event.accepted=true}
        if(event.key===Qt.Key_Down&&index>=0&&index<list.length-1){handle.targetHandle?playTargets.moveTarget(handle.instanceId,list[index+1].id,true):launcher.moveInstance(handle.instanceId,list[index+1].id,true);event.accepted=true}
    }
    objectName:"reorder-"+instanceId
    Rectangle {anchors.fill:parent;radius:7;color:mouse.containsMouse?Theme.surface2:Theme.transparent;border.color:handle.activeFocus?Theme.accent:Theme.transparent}
    Canvas {
        anchors.centerIn:parent;width:12;height:18
        onPaint:{let c=getContext("2d");c.reset();c.fillStyle=Theme.textMuted;for(let x=0;x<2;x++)for(let y=0;y<3;y++){c.beginPath();c.arc(3+x*6,3+y*6,1.2,0,Math.PI*2);c.fill()}}
    }
    Item {
        id:proxy;width:24;height:30
        property string instanceId:handle.instanceId
        Drag.active:mouse.drag.active
        Drag.source:proxy
        Drag.keys:[handle.targetHandle?"moly-target":"moly-instance"]
        Drag.hotSpot.x:12;Drag.hotSpot.y:15
        Drag.supportedActions:Qt.MoveAction
        Rectangle {visible:mouse.drag.active;anchors.fill:parent;radius:8;color:Theme.accent;opacity:0.8}
    }
    MouseArea {
        id:mouse;anchors.fill:parent;hoverEnabled:true;cursorShape:pressed?Qt.ClosedHandCursor:Qt.OpenHandCursor
        drag.target:proxy;drag.threshold:8
        onReleased:{proxy.Drag.drop();proxy.x=0;proxy.y=0}
        onCanceled:{proxy.Drag.cancel();proxy.x=0;proxy.y=0}
    }
}
