#define AppName "Prism"
#define AppVersion "0.1.0"
#define Publisher "Prism Project"

[Setup]
AppId={{8E4B0B6C-5F8B-4F4A-8D65-AC9A3E39C2E1}
AppName={#AppName} - Geometry Dash Mod Loader
AppVersion={#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf32}\Steam\steamapps\common\Geometry Dash
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
DirExistsWarning=no

[Files]
; Install beside GeometryDash.exe so Prism's launcher/runtime use the game folder.
; Setup copies files; it does not inject a DLL into a running process.
Source: "..\build\Release\PrismLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\PrismRuntime.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\PrismHelloMod.dll"; DestDir: "{app}\mods"; Flags: ignoreversion
Source: "..\include\prism\mod_api.h"; DestDir: "{app}\PrismSDK"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}\Prism-README.md"; Flags: ignoreversion
Source: "..\docs\MOD_API.md"; DestDir: "{app}\PrismSDK"; Flags: ignoreversion
Source: "..\docs\BUILDING.md"; DestDir: "{app}\PrismSDK"; Flags: ignoreversion
Source: "..\assets\prism-icon.svg"; DestDir: "{app}\PrismAssets"; Flags: ignoreversion

[Icons]
Name: "{group}\Prism Mod Loader"; Filename: "{app}\PrismLauncher.exe"; IconFilename: "{app}\PrismLauncher.exe"
Name: "{autodesktop}\Prism Mod Loader"; Filename: "{app}\PrismLauncher.exe"; IconFilename: "{app}\PrismLauncher.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Dirs]
Name: "{app}\mods"
Name: "{localappdata}\Prism\logs"

; Installed files are removed on uninstall. Only remove directories if empty, preserving user mods.
[UninstallDelete]
Type: dirifempty; Name: "{app}\mods"
Type: dirifempty; Name: "{app}\PrismSDK"
Type: dirifempty; Name: "{app}\PrismAssets"

[Code]
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    if not FileExists(ExpandConstant('{app}\GeometryDash.exe')) then
    begin
      MsgBox('Choose the Geometry Dash installation folder containing GeometryDash.exe. Prism installs its files beside the game executable.', mbError, MB_OK);
      Result := False;
    end;
  end;
end;
