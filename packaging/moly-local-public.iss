; Public installer. The staging directory contains the launcher and its runtime only.
#ifndef AppSource
  #define AppSource "..\dist\public\Local"
#endif
[Setup]
AppId={{F91602B5-3157-468A-A121-2D6EDBC3A487}
AppName=Moly Local Launcher
AppVersion=0.8.5
AppPublisher=Moly
VersionInfoDescription=Moly Local Launcher Setup
VersionInfoProductName=Moly Local Launcher
VersionInfoVersion=0.8.5.0
DefaultDirName={localappdata}\Programs\Moly Local Launcher Public
DefaultGroupName=Moly Local Launcher
DisableProgramGroupPage=yes
PrivilegesRequired=none
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir=..\dist
OutputBaseFilename=MolyLocalLauncher-0.8.5-Windows-x64-Setup
SetupIconFile=..\assets\moly.ico
UninstallDisplayIcon={app}\MolyLocalLauncher.exe
Compression=lzma2/fast
SolidCompression=no
WizardStyle=modern dynamic
WizardImageFile=wizard-image.bmp
WizardSmallImageFile=wizard-small.bmp
CloseApplications=yes
RestartApplications=no
[Tasks]
Name: "desktopicon"; Description: "Create a Moly Local Launcher desktop shortcut"
[Files]
Source: "{#AppSource}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\Moly Local Launcher"; Filename: "{app}\MolyLocalLauncher.exe"
Name: "{autodesktop}\Moly Local Launcher"; Filename: "{app}\MolyLocalLauncher.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\MolyLocalLauncher.exe"; Description: "Open Moly Local Launcher"; Flags: nowait postinstall skipifsilent
; Existing launcher data, accounts, worlds and mods are never removed or migrated.
