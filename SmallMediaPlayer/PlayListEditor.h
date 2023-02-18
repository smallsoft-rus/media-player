/* Small Media Player 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef PLAYLISTEDITOR_H
#define PLAYLISTEDITOR_H
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "player.h"
#include <commctrl.h>
#define STRSAFE_NO_DEPRECATE
#include <Strsafe.h>
#include "tags.h"
#include "common.h"

#define COUNT_EXTENSIONS 18
#define COUNT_IMAGE_EXTENSIONS (sizeof(ImageExtensions)/sizeof(ImageExtensions[0]))

typedef struct{
	TCHAR ext[10];
	int iIcon;
	HICON hIcon;
}TYPE_ICON_INFO;

#define COUNT_TYPE_ICONS 16

//THIS MODULE DECL.
extern HWND PlayList;
extern UINT CountTracks;
extern UINT CurrentTrack;
extern PLAYBACK_MODE CurrMode;
extern bool fIncludeImages;
extern HICON hDirIcon;
extern HICON hMusicIcon;
extern int indexDirIcon;
extern int indexMusicIcon;
extern HIMAGELIST ImageList;

BOOL GetPlaylistElement(int n,TCHAR* out);
BOOL GetPlaylistElementA(int n,char* out);
void AddPlaylistElement(TCHAR* fname);
void ClearPlaylist();
void DeletePlaylistElement(int n);
void PlayTrackByNumber(int n);
void PlayNextTrack();
void GetCurrTrackShortName(TCHAR* str);
void AddDirectory(wchar_t* dir,wchar_t* ext);
void DoDirectoryAdd(TCHAR* dir,HWND hlbt);
void Playlist_AddDirectory(TCHAR* dir);
int GetPlaylistSelectedElement();
BOOL IsPlaylistItemSelected(int n);
void SetPlaylistHighlight(int i);
void SetPlaylistSelectedElement(int i);
void LoadTextPlaylist(TCHAR* file);
void LoadTextPlaylistA(char* file);
void SaveTextPlaylist(TCHAR* file);
void ReadPlaylistTags(char* str);
int FindTrack(TCHAR* fname);
void InsertPlaylistElement(WCHAR* fname,UINT pos);
void Playlist_Paste();void Playlist_Cut();void Playlist_Copy();
void Playlist_SelectAll();
void Playlist_SetSortReverseFlag(bool sortReverse);
void Playlist_SetEventCallback(SMP_EVENT evt, SMP_ACTION callback);

void InitImageList();
int GetTypeIcon(TCHAR* ext);
#endif