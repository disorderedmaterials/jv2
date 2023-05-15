; Script generated by the Inno Script Studio Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "jv2"
#define MyAppVersion "1.2.0"
#define MyAppPublisher "Team JournalViewer"
#define MyAppURL "https://isis.stfc.ac.uk"
#define MyAppExeName "jv2.exe"

; Locations of bin directories
#define Jv2Dir GetEnv('JV2_DIR')
#define QtDir GetEnv('Qt6_DIR')
#define BackendDir GetEnv('BACKEND_DIR')
#define FrontendDir GetEnv('FRONTEND_DIR')

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{EB65EDA8-F371-4934-962B-8F79CF9D7980}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf}\JournalViewer2
DefaultGroupName={#MyAppName}
LicenseFile=..\..\LICENSE.txt
OutputDir=..\..\
OutputBaseFilename=jv2-{#MyAppVersion}-Win64
SetupIconFile=JournalViewer2.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#FrontendDir}\jv2.exe"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "JournalViewer2.ico"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#QtDir}\Qt6Gui.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6Core.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6OpenGL.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6OpenGLWidgets.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6Svg.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6Widgets.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6Network.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6PrintSupport.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6Charts.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\Qt6Xml.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion
Source: "{#QtDir}\iconengines\qsvgicon.dll"; DestDir: "{app}\bin\jv2\iconengines"; Flags: ignoreversion
Source: "{#QtDir}\platforms\qwindows.dll"; DestDir: "{app}\bin\jv2\platforms"; Flags: ignoreversion
Source: "{#QtDir}\imageformats\*.dll"; DestDir: "{app}\bin\jv2\imageformats"; Flags: ignoreversion
Source: "{#BackendDir}\jv2Setup.dist\*"; DestDir: "{app}\bin\jv2Setup"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BackendDir}\isisInternal.dist\*"; DestDir: "{app}\bin\jv2Setup\isisInternal"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Jv2Dir}\..\..\extra\*"; DestDir: "{app}\bin\extra"; Flags: ignoreversion
; Windows 7
;Source: "C:\Windows\System32\D3DCompiler_43.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
; Windows 10
Source: "C:\Windows\System32\D3DCompiler_47.dll"; DestDir: "{app}\bin\jv2"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; IconFilename: "{app}\bin\jv2\JournalViewer2.ico"; Filename: "{app}\bin\jv2Setup\jv2Setup.exe"; WorkingDir: "{app}\bin\jv2Setup"
Name: "{commondesktop}\{#MyAppName}"; IconFilename: "{app}\bin\jv2\JournalViewer2.ico"; Filename: "{app}\bin\jv2Setup\jv2Setup.exe"; WorkingDir: "{app}\bin\jv2Setup"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\jv2Setup\jv2Setup.exe"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
