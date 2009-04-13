;
; install.iss -- Inno Setup 4 install configuration file for Embedthis Ejscript
;
; Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
;

[Setup]
AppName=!!BLD_NAME!!
AppVerName=!!BLD_NAME!! !!BLD_VERSION!!-!!BLD_NUMBER!!
DefaultDirName={sd}!!BLD_PREFIX!!
DefaultGroupName=!!BLD_NAME!!
UninstallDisplayIcon={app}/!!BLD_PRODUCT!!.exe
LicenseFile=!!BLD_PREFIX!!/LICENSE.TXT
ChangesEnvironment=yes

[Icons]
Name: "{group}\!!BLD_NAME!! shell"; Filename: "{app}/bin/!!BLD_PRODUCT!!.exe"; Parameters: ""
Name: "{group}\!!BLD_NAME!! documentation"; Filename: "{app}/doc/product/index.html"; Parameters: ""
Name: "{group}\ReadMe"; Filename: "{app}/README.TXT"

[Types]
Name: "full"; Description: "Complete Installation with Documentation and Development Libraries"; 
Name: "binary"; Description: "Binary Installation"; 
Name: "development"; Description: "Development Documentation, Headers and Libraries"; 
; Name: "source"; Description: "Full Source Code"; 

[Components]
Name: "bin"; Description: "Binary Files"; Types: binary full;
Name: "dev"; Description: "Development Headers"; Types: development full;
; Name: "src"; Description: "Source Code"; Types: source full;

[Dirs]
Name: "{app}/bin"

[UninstallDelete]

[Tasks]
Name: addpath; Description: Add !!BLD_NAME!! to the system PATH variable;

[Code]
function IsPresent(const file: String): Boolean;
begin
  file := ExpandConstant(file);
  if FileExists(file) then begin
    Result := True;
  end else begin
    Result := False;
  end
end;

//
//	Initial sample by Jared Breland
//
procedure AddPath(keyName: String; dir: String);
var
	newPath, oldPath, hive, key: String;
	paths:		                   TArrayOfString;
	i:	                         Integer;
	regHive:                        Integer;
begin

  if (IsAdminLoggedOn or IsPowerUserLoggedOn) then begin
    regHive := HKEY_LOCAL_MACHINE;
    key := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  end else begin
    regHive := HKEY_CURRENT_USER;
    key := 'Environment';
  end;

	i := 0;
	RegQueryStringValue(regHive, key, keyName, oldPath);
	oldPath := oldPath + ';';

	while (Pos(';', oldPath) > 0) do begin
		SetArrayLength(paths, i + 1);
		paths[i] := Copy(oldPath, 0, Pos(';', oldPath) - 1);
		oldPath := Copy(oldPath, Pos(';', oldPath) + 1, Length(oldPath));
		i := i + 1;

		if dir = paths[i - 1] then begin
				continue;
		end;

		if i = 1 then begin
			newPath := paths[i - 1];
		end else begin
			newPath := newPath + ';' + paths[i - 1];
		end;
	end;

	if (IsUninstaller() = false) and (dir <> '') then begin
	  if (newPath <> '') then begin
		  newPath := newPath + ';' + dir;
	  end else begin
	    newPath := dir;
	  end;
  end;
	RegWriteStringValue(regHive, key, keyName, newPath);
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
  bin: String;
begin
	if CurStep = ssPostInstall then
		if IsTaskSelected('addpath') then begin
			bin := ExpandConstant('{app}\bin');			
			// AddPath('EJSPATH', bin);
			AddPath('Path', bin);
	  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
	bin:			String;
begin
	if CurUninstallStep = usUninstall then begin
	    bin := ExpandConstant('{app}\bin');			
		// AddPath('EJSPATH', bin);
		AddPath('Path', bin);
	end;
end;


[Run]
Filename: "file:///{app}/doc/product/index.html"; Description: "View the Documentation"; Flags: skipifsilent waituntilidle shellexec postinstall; Check: IsPresent('{app}/doc/product/index.html'); Components: bin

[UninstallRun]
Filename: "{app}/bin/removeFiles.exe"; Parameters: "-r -s 5"; WorkingDir: "{app}"; Flags:

[Files]
