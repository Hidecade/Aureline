#ifndef MyAppVersion
  #error MyAppVersion must be supplied by build-windows-installer.ps1
#endif
#ifndef StandaloneSource
  #error StandaloneSource must be supplied by build-windows-installer.ps1
#endif
#ifndef Vst3Source
  #error Vst3Source must be supplied by build-windows-installer.ps1
#endif
#ifndef OutputDirectory
  #error OutputDirectory must be supplied by build-windows-installer.ps1
#endif

#define MyAppName "Aureline"
#define MyAppPublisher "Hidecade"
#define MyAppExeName "Aureline.exe"

[Setup]
AppId={{A482773C-2576-4DB7-9D99-BB5F88FDD696}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=
OutputDir={#OutputDirectory}
OutputBaseFilename=Aureline-{#MyAppVersion}-Windows-x64-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} installer
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone application"; Types: full custom; Flags: fixed
Name: "vst3"; Description: "VST3 instrument plug-in"; Types: full custom

[Files]
Source: "{#StandaloneSource}"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#Vst3Source}\*"; DestDir: "{commoncf64}\VST3\Aureline.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent; Components: standalone
