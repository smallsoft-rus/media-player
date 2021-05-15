/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "SMPSettings.h"
#include "resource.h"

SMPSETTINGS Settings =
{
	sizeof(SMPSETTINGS),
	SMPVER_MAJOR,
	SMPVER_MINOR,
	true,//show pls
	true,//show cover
	true,//load pls
	false,//remember pos
	true,//load wallpaper
	true,//load def wallpaper
	L"",//wallpaper file name
	5000,//image delay
	CW_USEDEFAULT,
	0,
	CW_USEDEFAULT,
	0,
	false,//is window maximized
	FILE_NOT_LOADED,//state
	NORMAL,//playback mode
	0,
	0,
	100 //volume
};

void GetDataDir(WCHAR* dir,int maxcount){ //export
	TCHAR fname[MAX_PATH]=L"";

	//get documents dir
	HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, fname);
	if(result!=S_OK)GetFileDirectory(ProgramFileName,fname);
	else StringCchCat(fname,MAX_PATH,L"\\Small Media Player\\");
	CreateDirectory(fname,NULL);
	StringCchCopyW(dir,maxcount,fname);
}

void LoadSettings()
{
    HANDLE hFile;
    DWORD dwCount;
    SMPSETTINGS t;
    TCHAR fname[MAX_PATH]=L"";
    BOOL res;

	GetDataDir(fname,MAX_PATH);
	StringCchCat(fname,MAX_PATH,L"smpdata.bin");

    hFile=CreateFile(fname,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile==INVALID_HANDLE_VALUE)return;
    res=ReadFile(hFile,&t,sizeof(SMPSETTINGS),&dwCount,NULL);
    if(res==FALSE){CloseHandle(hFile);return;}
    if(dwCount!=sizeof(SMPSETTINGS)){CloseHandle(hFile);return;}
    Settings=t;
    CloseHandle(hFile);
}

void WriteSettings()
{
    HANDLE hFile;
    DWORD dwCount;

    TCHAR fname[MAX_PATH]=L"";
    BOOL res;

	GetDataDir(fname,MAX_PATH);
    StringCchCat(fname,MAX_PATH,L"smpdata.bin");

    hFile=CreateFile(fname,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile==INVALID_HANDLE_VALUE)return;
    res=WriteFile(hFile,&Settings,sizeof(SMPSETTINGS),&dwCount,NULL);
    CloseHandle(hFile);
}

void RestoreLastPosition()
{		
	CurrMode=Settings.LastPlaybackMode;
	CurrVolume=Settings.LastVolume;
	UpdateVolume();

	if(Settings.LastPlayerState==FILE_NOT_LOADED)return;
	PlayTrackByNumber(Settings.LastFileIndex);
	switch(Settings.LastPlayerState)
	{
		case STOPPED:Stop();break;
		case PAUSED:
			SetPosition(Settings.LastPosition);
			Pause();break;
			case PLAYING:SetPosition(Settings.LastPosition);
				
			break;			
	}
}

void SaveLastPosition()
{
	Settings.LastFileIndex=CurrentTrack;
	Settings.LastPlayerState=PlayerState;
	Settings.LastPosition=GetPosition();
	Settings.LastPlaybackMode=CurrMode;
	Settings.LastVolume=CurrVolume;
}

BOOL CALLBACK SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam){

   wchar_t buf[256]=L"";
   HWND hw;
   OPENFILENAME ofn={0};
   BOOL success;
   UINT i;
   static SMPSETTINGS t;

switch(uMsg){
case WM_INITDIALOG:
	t=Settings;
	if(Settings.fLoadDefaultPls==true){CheckDlgButton(hDlg,IDC_REMEMBER_PLAYLIST,BST_CHECKED);}
	else{CheckDlgButton(hDlg,IDC_REMEMBER_PLAYLIST,BST_UNCHECKED);}
	if(Settings.fLoadDefaultPls==false){
		hw=GetDlgItem(hDlg,IDC_REMEMBER_POSITION);
		EnableWindow(hw,FALSE);}
	if(Settings.fRememberPosition==true){CheckDlgButton(hDlg,IDC_REMEMBER_POSITION,BST_CHECKED);}
	else{CheckDlgButton(hDlg,IDC_REMEMBER_POSITION,BST_UNCHECKED);}
	if(Settings.fLoadWallpaper==true){CheckDlgButton(hDlg,IDC_SHOW_WALLPAPER,BST_CHECKED);}
	else{CheckDlgButton(hDlg,IDC_SHOW_WALLPAPER,BST_UNCHECKED);}
	if(Settings.fLoadDefWallpaper==true){SetDlgItemText(hDlg,IDC_FILENAME,L"");}
	else{SetDlgItemText(hDlg,IDC_FILENAME,Settings.WallpaperFilePath);}
	StringCchPrintf(buf,256,L"%u",(UINT)Settings.ImageDelay);
	SetDlgItemText(hDlg,IDC_DELAY,buf);
	
	return TRUE;

case WM_COMMAND:switch(LOWORD(wParam)){
case IDC_REMEMBER_PLAYLIST:t.fLoadDefaultPls=!t.fLoadDefaultPls;
	if(t.fLoadDefaultPls==false){
	CheckDlgButton(hDlg,IDC_REMEMBER_POSITION,BST_UNCHECKED);
	t.fRememberPosition=false;
	hw=GetDlgItem(hDlg,IDC_REMEMBER_POSITION);
	EnableWindow(hw,FALSE);
	}
	else{
	hw=GetDlgItem(hDlg,IDC_REMEMBER_POSITION);
	EnableWindow(hw,TRUE);
	}
	break;
case IDC_REMEMBER_POSITION:t.fRememberPosition=!t.fRememberPosition;break;
case IDC_SHOW_WALLPAPER:t.fLoadWallpaper=!t.fLoadWallpaper;break;
case ID_OPEN:ofn.lStructSize=sizeof(OPENFILENAME);
                ofn.lpstrFile=buf;
                ofn.nMaxFile=sizeof(buf);				
                success=GetOpenFileName(&ofn);
                if(success==FALSE){break;}
				SetDlgItemText(hDlg,IDC_FILENAME,buf);
				StringCchCopy(Settings.WallpaperFilePath,MAX_PATH,buf);
				break;

case IDOK:
	Settings=t;
	GetDlgItemText(hDlg,IDC_FILENAME,Settings.WallpaperFilePath,MAX_PATH);
	if(lstrcmp(Settings.WallpaperFilePath,L"")==0){Settings.fLoadDefWallpaper=true;}
	else {Settings.fLoadDefWallpaper=false;}
	i=GetDlgItemInt(hDlg,IDC_DELAY,&success,FALSE);
	if(success!=FALSE)Settings.ImageDelay=i;
	UnloadPattern();
	if(Settings.fLoadWallpaper==true){
	if(Settings.fLoadDefWallpaper==true){
		GetPattern(buf);
		LoadPattern(buf);}
	else{LoadPattern(Settings.WallpaperFilePath);}
}
		WriteSettings();
 case IDCANCEL:EndDialog(hDlg,0);return TRUE;  							  
				
				}break;
case WM_CLOSE:EndDialog(hDlg,0);return TRUE;               
                  
    }
return FALSE;
}
