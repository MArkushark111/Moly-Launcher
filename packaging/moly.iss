#ifndef AppSource
  #define AppSource "..\dist\MolyLauncher-0.8.3"
#endif
[Setup]
AppId={{168DBEC3-1C53-44B5-ADB7-587233624632}
AppName=Moly Launcher
AppVersion=0.8.3
AppPublisher=Moly
DefaultDirName={localappdata}\Programs\Moly Launcher
DefaultGroupName=Moly Launcher
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir=..\dist
OutputBaseFilename=MolyLauncher-0.8.3-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\MolyLauncher.exe
CloseApplications=yes
RestartApplications=no
ChangesAssociations=yes

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "{#AppSource}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Moly Launcher"; Filename: "{app}\MolyLauncher.exe"
Name: "{autodesktop}\Moly Launcher"; Filename: "{app}\MolyLauncher.exe"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.mlypack"; ValueType: string; ValueName: ""; ValueData: "MolyLauncher.Pack"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\MolyLauncher.Pack"; ValueType: string; ValueName: ""; ValueData: "Moly setup"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\MolyLauncher.Pack\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\MolyLauncher.exe"",0"
Root: HKA; Subkey: "Software\Classes\MolyLauncher.Pack\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\MolyLauncher.exe"" --import-pack ""%1"""

[Run]
Filename: "{app}\MolyLauncher.exe"; Description: "Open Moly Launcher"; Flags: nowait postinstall skipifsilent

; Instance folders and account credentials are user data. Uninstall never deletes them.
