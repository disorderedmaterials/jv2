; Script generated by the Inno Script Studio Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "JournalViewer2"
#define MyAppVersion "0.1"
#define MyAppPublisher "Tristan Youngs & Ethan Devlin"
#define MyAppURL "https://www.projectaten.com/"
#define MyAppExeName "jv2.exe"

; Locations of bin directories of Dissolve, Qt, GnuWin, MinGW etc.
#define JvDir GetEnv('JV2_DIR')
#define QtDir GetEnv('Qt6_DIR')
#define MinGWLibDir GetEnv('MINGW_LIB_DIR')

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{EB65EDA8-F371-4934-962B-8F79CF9D7980}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf}\JournalViewer
DefaultGroupName={#MyAppName}
LicenseFile=..\..\LICENSE.txt
OutputDir=..\..\
OutputBaseFilename=JournalViewer2-{#MyAppVersion}-Win64
SetupIconFile=JournalViewer2.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#Jv2Dir}\jv2.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "JournalViewer.ico"; DestDir: "{app}\bin"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#MinGWLibDir}\libgcc_s_seh-1.dll"; DestDir: "{app}\bin"
Source: "{#MinGWLibDir}\libstdc++-6.dll"; DestDir: "{app}\bin"
Source: "{#MinGWLibDir}\libwinpthread-1.dll"; DestDir: "{app}\bin"
Source: "{#MinGWLibDir}\libquadmath-0.dll"; DestDir: "{app}\bin"
Source: "{#QtDir}\bin\Qt6Gui.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt6Core.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt6OpenGL.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt6Svg.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt6Widgets.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt6Network.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt6PrintSupport.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\libEGL.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\bin\libGLESv2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#QtDir}\plugins\iconengines\qsvgicon.dll"; DestDir: "{app}\bin\iconengines"; Flags: ignoreversion
Source: "{#QtDir}\plugins\platforms\qwindows.dll"; DestDir: "{app}\bin\platforms"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\*.dll"; DestDir: "{app}\bin\imageformats"; Flags: ignoreversion
; Windows 7
;Source: "C:\Windows\System32\D3DCompiler_43.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
; Windows 10
Source: "C:\Windows\System32\D3DCompiler_47.dll"; DestDir: "{app}\bin"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; IconFilename: "{app}\bin\JournalViewer.ico"; Filename: "{app}\bin\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{commondesktop}\{#MyAppName}"; IconFilename: "{app}\bin\JournalViewer.ico"; Filename: "{app}\bin\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent