; Moly Launcher installer for Inno Setup 7.1.1.
; Build after packaging with: ISCC.exe packaging\moly.iss

#ifndef AppSource
  #define AppSource "..\dist\MolyLauncher-0.8.5"
#endif
[Setup]
AppId={{168DBEC3-1C53-44B5-ADB7-587233624632}
AppName=Moly Launcher
AppVersion=0.8.5
AppPublisher=Moly
AppCopyright=Copyright (C) 2026 Moly
AppPublisherURL=https://github.com/MArkushark111/Moly-Launcher
AppSupportURL=https://github.com/MArkushark111/Moly-Launcher/issues
AppUpdatesURL=https://github.com/MArkushark111/Moly-Launcher/releases
VersionInfoVersion=0.8.5.0
VersionInfoCompany=Moly
VersionInfoDescription=Moly Launcher Setup
VersionInfoTextVersion=0.8.5
VersionInfoCopyright=Copyright (C) 2026 Moly
VersionInfoProductName=Moly Launcher
VersionInfoProductVersion=0.8.5
VersionInfoProductTextVersion=0.8.5
; {autopf} resolves to Program Files for an all-users install and to
; %LocalAppData%\Programs for a single-user install.
DefaultDirName={autopf}\Moly Launcher
DefaultGroupName=Moly Launcher
DisableProgramGroupPage=yes
; Install for the current user by default, without elevation. The install mode
; page lets the user pick "Install for all users", which requests administrator
; rights, or keep "Install for me only", which needs no administrator.
PrivilegesRequired=none
PrivilegesRequiredOverridesAllowed=dialog commandline
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir=..\dist
OutputBaseFilename=MolyLauncher-0.8.5-Windows-x64-Setup
SetupIconFile=..\assets\moly.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern dynamic
WizardImageFile=wizard-image.bmp
WizardSmallImageFile=wizard-small.bmp
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
[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  StartupCommand: string;
  ExpectedCommand: string;
begin
  if CurUninstallStep <> usUninstall then
    Exit;
  ExpectedCommand := '"' + ExpandConstant('{app}\MolyLauncher.exe') + '"';
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run',
    'MolyLauncher', StartupCommand) then
    if CompareText(StartupCommand, ExpectedCommand) = 0 then
      RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'MolyLauncher');
end;
