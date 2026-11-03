#ifndef MyAppVersion
  #define MyAppVersion "2.0.0"
#endif

#ifndef MyAppSource
  #define MyAppSource "dist\\bin"
#endif

#define Arch "x64"
#define MyArchSpec "x64compatible"

#define MyAppName "Clip Cut 2.0"
#define MyAppId "C11PC072-0000-4892-ACEE-EC56B595E89B"
#define MyOutputBase "ClipCut-2.0-Setup-" + Arch
#define MyAppPublisher "Clip Cut Team"
#define MyAppExeName "ClipCut2.exe"

[Setup]
AppId={{{#MyAppId}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
ArchitecturesAllowed={#MyArchSpec}
ArchitecturesInstallIn64BitMode={#MyArchSpec}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\..\resources\windows\drift.ico
LicenseFile=..\..\LICENSE
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\{#MyAppExeName}
ChangesAssociations=yes
OutputDir=output
OutputBaseFilename={#MyOutputBase}

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#MyAppSource}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; File association: .clipcut
Root: HKA; Subkey: "Software\Classes\.clipcut"; ValueType: string; ValueName: ""; ValueData: "ClipCut.Project"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.clipcut\OpenWithProgids"; ValueType: string; ValueName: "ClipCut.Project"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\ClipCut.Project"; ValueType: string; ValueName: ""; ValueData: "Clip Cut 2.0 Project"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\ClipCut.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKA; Subkey: "Software\Classes\ClipCut.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

; File association: .drift
Root: HKA; Subkey: "Software\Classes\.drift"; ValueType: string; ValueName: ""; ValueData: "ClipCut.Project"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.drift\OpenWithProgids"; ValueType: string; ValueName: "ClipCut.Project"; ValueData: ""; Flags: uninsdeletevalue

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
