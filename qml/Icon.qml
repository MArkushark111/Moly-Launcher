import QtQuick

Canvas {
    id: icon
    property string name: "box"
    property int size: 20
    property color color: Theme.textSecondary
    implicitWidth: size
    implicitHeight: size
    width: size
    height: size
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c = getContext("2d")
        c.reset(); c.scale(width / 24, height / 24)
        c.strokeStyle = color; c.lineWidth = 1.7; c.lineCap = "round"; c.lineJoin = "round"
        function line(a) { c.beginPath(); c.moveTo(a[0],a[1]); for(let i=2;i<a.length;i+=2)c.lineTo(a[i],a[i+1]); c.stroke() }
        function circle(x,y,r) { c.beginPath();c.arc(x,y,r,0,Math.PI*2);c.stroke() }
        function rect(x,y,w,h) { c.strokeRect(x,y,w,h) }
        switch(name) {
        case "library": case "grid": rect(3,3,7,7);rect(14,3,7,7);rect(3,14,7,7);rect(14,14,7,7);break
        case "list": line([8,5,21,5]);line([8,12,21,12]);line([8,19,21,19]);line([3,5,3.1,5]);line([3,12,3.1,12]);line([3,19,3.1,19]);break
        case "discover": circle(12,12,9);line([15.5,8.5,13.5,13.5,8.5,15.5,10.5,10.5,15.5,8.5]);break
        case "download": line([12,3,12,15]);line([7,10,12,15,17,10]);line([4,16,4,21,20,21,20,16]);break
        case "accounts": case "user": circle(12,7,4);c.beginPath();c.arc(12,21,8,Math.PI,0);c.stroke();break
        case "skin": line([8,3,5,5,2,9,6,12,7,10,7,21,17,21,17,10,18,12,22,9,19,5,16,3]);c.beginPath();c.arc(12,3,4,0,Math.PI);c.stroke();break
        case "settings": circle(12,12,4);for(let i=0;i<8;i++){let a=i*Math.PI/4;line([12+7*Math.cos(a),12+7*Math.sin(a),12+10*Math.cos(a),12+10*Math.sin(a)])}circle(12,12,8);break
        case "search": circle(10,10,6);line([15,15,21,21]);break
        case "plus": line([12,5,12,19]);line([5,12,19,12]);break
        case "play": line([7,4,20,12,7,20,7,4]);break
        case "stop": rect(5,5,14,14);break
        case "folder": line([3,7,3,20,21,20,21,7,12,7,10,4,3,4,3,7]);break
        case "file": line([6,3,15,3,20,8,20,21,6,21,6,3]);line([15,3,15,8,20,8]);break
        case "world": line([3,17,8,9,12,14,16,6,21,17,3,17]);line([3,21,21,21]);circle(6,5,1.5);break
        case "logs": rect(3,4,18,16);line([6,8,9,11,6,14]);line([12,15,18,15]);break
        case "share": line([12,16,12,3]);line([7,8,12,3,17,8]);line([4,12,4,21,20,21,20,12]);break
        case "more": circle(5,12,0.8);circle(12,12,0.8);circle(19,12,0.8);break
        case "chevron": line([8,5,15,12,8,19]);break
        case "down": line([6,9,12,15,18,9]);break
        case "back": line([15,5,8,12,15,19]);break
        case "check": line([5,12,10,17,20,6]);break
        case "close": line([6,6,18,18]);line([18,6,6,18]);break
        case "trash": line([3,6,21,6]);line([9,6,9,3,15,3,15,6]);line([5,6,6,21,18,21,19,6]);line([10,10,10,17]);line([14,10,14,17]);break
        case "refresh": c.beginPath();c.arc(12,12,8,0.5,5.5);c.stroke();line([20,3,20,9,14,9]);break
        case "external": line([14,3,21,3,21,10]);line([11,13,21,3]);line([10,3,3,3,3,21,21,21,21,14]);break
        case "clock": circle(12,12,9);line([12,6,12,12,16,14]);break
        default: line([3,7,12,2,21,7,21,17,12,22,3,17,3,7,12,12,21,7]);line([12,12,12,22]);break
        }
    }
}
