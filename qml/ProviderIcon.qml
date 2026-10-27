import QtQuick

Image {
    property string provider: ""
    source: provider === "Modrinth" ? "../assets/modrinth.svg" : provider === "CurseForge" ? "../assets/curseforge.svg" : provider === "Prism" ? "../assets/prism.svg" : provider === "MultiMC" ? "../assets/multimc.svg" : provider === "ATLauncher" ? "../assets/atlauncher.png" : provider === "Lunar Client" ? "../assets/lunar.ico" : provider === "Moly" ? "../assets/moly.svg" : "../assets/minecraft-folder.svg"
    sourceSize.width: 40; sourceSize.height: 40
    fillMode: Image.PreserveAspectFit
    width: 18; height: 18
}
