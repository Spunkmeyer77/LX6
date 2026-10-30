; LX-6 Windows installer (Inno Setup 6)
; SPDX-License-Identifier: GPL-3.0-only
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\build\LX6_artefacts\Release"
#endif

[Setup]
AppId={{6A1C9D2E-5B7F-4E0A-9C3D-2F6B7C8D9E01}
AppName=LX-6
AppVersion={#AppVersion}
AppPublisher=Ticco Ross
AppPublisherURL=https://ticcoross.gumroad.com/
AppSupportURL=mailto:ticcoross@gmail.com
DefaultDirName={autopf}\Ticco Ross\LX-6
DefaultGroupName=LX-6
OutputBaseFilename=LX-6-{#AppVersion}-Windows
LicenseFile=resources\license.txt
InfoBeforeFile=resources\welcome.txt
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
UninstallDisplayName=LX-6

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full compact custom; Flags: fixed
Name: "app"; Description: "Standalone app"; Types: full

[Files]
Source: "{#BuildDir}\VST3\LX-6.vst3\*"; DestDir: "{commoncf64}\VST3\LX-6.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
Source: "{#BuildDir}\Standalone\LX-6.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{group}\LX-6"; Filename: "{app}\LX-6.exe"; Components: app
Name: "{group}\Uninstall LX-6"; Filename: "{uninstallexe}"
