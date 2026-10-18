#define AppName "Prism"
#define AppVersion "0.1.0"
#define Publisher "Prism Project"

[Setup]
AppId={{8E4B0B6C-5F8B-4F4A-8D65-AC9A3E39C2E1}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf}\Prism
DefaultGroupName=Prism
OutputDir=Output
OutputBaseFilename=Prism-Setup
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayIcon={app}\PrismLauncher.exe
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
CloseApplications=no

[Files]
Source: "..\build\Release\PrismLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\PrismRuntime.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\PrismHelloMod.dll"; DestDir: "{app}\mods"; Flags: ignoreversion
Source: "..\include\prism\mod_api.h"; DestDir: "{app}\sdk\prism"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\MOD_API.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\BUILDING.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Prism"; Filename: "{app}\PrismLauncher.exe"
Name: "{autodesktop}\Prism"; Filename: "{app}\PrismLauncher.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Dirs]
Name: "{app}\mods"
Name: "{localappdata}\Prism\logs"

[UninstallDelete]
Type: filesandordirs; Name: "{app}\mods"
