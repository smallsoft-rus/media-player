/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef SMPSETTINGS_H
#define SMPSETTINGS_H

#ifndef UNICODE
#define UNICODE
#endif

#include "player.h"
#include "PlaylistEditor.h"
#include "PictureManager.h"

//extern
extern WCHAR ProgramFileName[256];
extern long CurrVolume;
extern void UpdateVolume();

//this module
extern SMPSETTINGS Settings;
void RestoreLastPosition();
void SaveLastPosition();
void WriteSettings();
void LoadSettings();
BOOL CALLBACK SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
void GetDataDir(WCHAR* dir,int maxcount);

#endif
