import QtQuick

Image {
    property string provider: ""
    source: provider === "Modrinth" ? "../assets/modrinth.svg" : provider === "CurseForge" ? "../assets/curseforge.svg" : ""
    sourceSize.width: 40; sourceSize.height: 40
    fillMode: Image.PreserveAspectFit
    width: 18; height: 18
}
