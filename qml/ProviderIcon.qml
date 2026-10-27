import QtQuick

Image {
    property string provider: ""
    readonly property var marks: ({"Modrinth":"modrinth.svg","CurseForge":"curseforge.svg","Prism":"prism.svg","MultiMC":"multimc.svg","ATLauncher":"atlauncher.png","Lunar Client":"lunar.ico","Moly":"moly.svg","Dawn Client":"dawn.png","Fast Client":"fastclient.svg","NoRisk Client":"norisk.png","Badlion Client":"badlion.png","LabyMod":"labymod.png","Feather":"feather.png","GDLauncher":"gdlauncher.ico"})
    source:"../assets/"+(marks[provider]||"minecraft-folder.svg")
    sourceSize.width: 40; sourceSize.height: 40
    fillMode: Image.PreserveAspectFit
    width: 18; height: 18
}
