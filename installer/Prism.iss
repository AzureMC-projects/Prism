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
UninstallDisplayIcon={app}\version.dll
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
CloseApplications=no
DirExistsWarning=no

[Files]
; Install beside GeometryDash.exe. The local version.dll proxy attempts to load PrismRuntime at game startup.
Source: "..\build\Release\PrismRuntime.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\PrismUpdater.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\version.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\xinput1_3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\xinput1_4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\PrismBootstrap.installed"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\PrismHelloMod.dll"; DestDir: "{app}\mods"; Flags: ignoreversion
Source: "..\include\prism\mod_api.h"; DestDir: "{app}\PrismSDK"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}\Prism-README.md"; Flags: ignoreversion
Source: "..\docs\MOD_API.md"; DestDir: "{app}\PrismSDK"; Flags: ignoreversion
Source: "..\docs\BUILDING.md"; DestDir: "{app}\PrismSDK"; Flags: ignoreversion
Source: "..\assets\prism-icon.svg"; DestDir: "{app}\PrismAssets"; Flags: ignoreversion

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
    end
    else if (FileExists(ExpandConstant('{app}\version.dll')) or FileExists(ExpandConstant('{app}\xinput1_3.dll')) or FileExists(ExpandConstant('{app}\xinput1_4.dll'))) and not FileExists(ExpandConstant('{app}\PrismBootstrap.installed')) then
    begin
      MsgBox('A version.dll or XInput proxy already exists in this game folder. Prism uses these proxy names for automatic startup, so installing over one could break another mod loader. Remove or back up the existing proxy first, then retry.', mbError, MB_OK);
      Result := False;
    end;
  end;
end;
