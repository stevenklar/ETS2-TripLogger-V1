; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "ETS2 Trip Logger"
#define MyAppVersion "0.1.2"
#define MyAppPublisher "NV1S1ON"
#define MyAppURL "http://ets2triplog.ddns.net/"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{523B0CEA-8910-46B0-AD8D-8DEB393F52D6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppendDefaultDirName=yes
DefaultDirName={pf}\ETS2TripLogger
DisableDirPage=no

DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=L:\Programming\C++ Projects\ETS2TripLogger\Install
OutputBaseFilename=Install ETS2TripLogger
Compression=lzma
SolidCompression=yes
AllowUNCPath=True
PrivilegesRequired=admin
EnableDirDoesntExistWarning=True
DirExistsWarning=no
ShowLanguageDialog=no

WizardImageFile=main.bmp
WizardSmallImageFile=small.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "L:\Programming\C++ Projects\ETS2TripLogger\Install\Files\ETS2TripLogger.dll"; DestDir: "{code:GetDataDir}\bin\win_x86\Plugins"; Flags: ignoreversion
Source: "L:\Programming\C++ Projects\ETS2TripLogger\Install\Files\libmysql.dll"; DestDir: "{sys}" ; Flags: uninsneveruninstall sharedfile onlyifdoesntexist

[Registry]
Root: HKCU; Subkey: "Software\NV1S1ON"; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: "Software\NV1S1ON\ETS2TripLogger"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\NV1S1ON\ETS2TripLogger\Settings"; ValueType: string; ValueName: "UserName"; ValueData: "{code:GetUser|UserName}"
Root: HKCU; Subkey: "Software\NV1S1ON\ETS2TripLogger\Settings"; ValueType: string; ValueName: "Password"; ValueData: "{code:GetUser|Password}"
Root: HKCU; Subkey: "Software\NV1S1ON\ETS2TripLogger\Settings"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

[Dirs]
Name: "{app}\Logs"

[UninstallDelete]
Type: files; Name: "{app}\{#MyAppName}.url"

[code]
   var
   UserAccountPage: TInputQueryWizardPage;
 DataDirPage: TInputDirWizardPage;

 procedure InitializeWizard;
begin
  { Create the pages }
  // User Account Page
  UserAccountPage := CreateInputQueryPage(wpWelcome,
    'Account Information', 'Your account login information?',
    'Please enter your Username and Password that you used to register on ETS2TripLogger');
  UserAccountPage.Add('Username:', False);
  UserAccountPage.Add('Password:', False);

  // Euro Truck Directory Select
  DataDirPage := CreateInputDirPage(wpSelectDir,
    'Euro Truck Simulator 2 Directory', 'Where is Euro Truck Simulator 2 Installed?',
    'Please select your Euro Truck Simulator 2 Main Directory',
    False, '');
  DataDirPage.Add('');
    DataDirPage.Values[0] := GetPreviousData('DataDir', '');
  end;
 
 // Get Selected Directory
function GetDataDir(Param: String): String;
begin
  { Return the selected DataDir }
  Result := DataDirPage.Values[0];
end;

// Get UserName and Password from UserAccountPage
function GetUser(Param: String): String;
begin
  { Return a user value }
  { Could also be split into separate GetUserName and GetUserCompany functions }
  if Param = 'UserName' then
    Result := UserAccountPage.Values[0]
  else if Param = 'Password' then
    Result := UserAccountPage.Values[1];
end;

  Function NextButtonClick(CurPage: Integer): Boolean;
 begin
     Result := True;
     if (CurPage = DataDirPage.ID) and not FileExists(ExpandConstant('{code:GetDataDir}\bin\win_x86\eurotrucks2.exe')) then begin
         MsgBox('Setup cannot find ETS2, Please select your Euro Truck Simulator 2 Main Installation Directory.', mbError, MB_OK);
         Result := False;
         exit;
     end;
 end;