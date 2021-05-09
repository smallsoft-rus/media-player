/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef PLAYLISTEDITOR_H
#define PLAYLISTEDITOR_H
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include "player.h"
#include "ScrollbarControl.h"
#include <commctrl.h>
#define STRSAFE_NO_DEPRECATE
#include <Strsafe.h>
#include "tags.h"

#define COUNT_EXTENSIONS 18
#define COUNT_IMAGE_EXTENSIONS (sizeof(ImageExtensions)/sizeof(ImageExtensions[0]))

typedef struct {
	BYTE type;
	char sig[6];
}VORBIS_PACKET_HEADER;

typedef struct {
	ULONG ver;
	BYTE chans;
	ULONG sample_rate;
	long bitrate_max;
	long bitrate;
	long bitrate_min;
	BYTE blocksize;
	BYTE end;
}VORBIS_ID;

#define APE_SIGNATURE ("APETAGEX")
#define APEF_HEADER 0xE0000000
#define APEF_BINARY 0x00000002
#define APEF_URL 0x00000004

typedef struct {
char sig[8];
char ver[4];
DWORD size;
DWORD item_count;
DWORD flags;
BYTE reserved[8];
}APE_HEADER;

typedef struct {
	ULONG len;
	DWORD flags;
}APE_ITEM;

#define FLAC_SIGNATURE "fLaC"
#define FLAC_LASTBLOCK 0x80
#define FLAC_VORBISCOMMENT 0x04
#define FLAC_VORBISCOMMENT2 0x84

typedef struct{
	
	UINT byte1:7;
	UINT dummy1:1;
	
	UINT byte2:7;
	UINT dummy2:1;
	
	UINT byte3:7;
	UINT dummy3:1;
	
	UINT byte4:7;
	UINT dummy4:1;
}BIT_FIELDS;

typedef struct{
	
	UINT byte1:7;	
	UINT byte2:7;	
	UINT byte3:7;	
	UINT byte4:7;
	UINT dummy:4;
	
}BIT_FIELDS_EXTRACTOR;

typedef union{
	BYTE bytes[4];
	DWORD dword;
	BIT_FIELDS bf;
	BIT_FIELDS_EXTRACTOR bfe;
}DWORD_UNION;

typedef struct {
	BYTE type;
	BYTE length[3];
}FLAC_BLOCK_HEADER;

#define ID32F_SYNC 0x80
#define ID32F_EXTHEADER 0x40
#define ID32_SIGNATURE "ID3"
#define ID32_SYNC_MASK 0xE0

typedef struct {
BYTE sig[3];
BYTE ver[2];
BYTE flags;
BYTE size[4];
}ID32_HEADER;

typedef struct {
	DWORD size;
	WORD flags;
	BYTE padding[4];
}ID32_EXTHEADER;

typedef struct {
	BYTE ID[4];
	DWORD size;
	BYTE flags[2];
}ID32_FRAME_HEADER;

#define ID32_FRAME_COMPRESSED 0x0080
#define ID32_FRAME_ENCRYPTED 0x0040
#define ID32_ENCODING_ISO 0x00
#define ID32_ENCODING_UNICODE 1
#define UNICODE_BOM_DIRECT 0xFFFE
#define UNICODE_BOM_REVERSE 0xFEFF


typedef union {
	WORD w;
	char b[2];
}WORD_UNION;



typedef struct{
	TCHAR ext[10];
	int iIcon;
	HICON hIcon;
}TYPE_ICON_INFO;

#define COUNT_TYPE_ICONS 16

//EXTERN FUNCTIONS
extern void UpdateLength();
extern void RunTimer();
extern void StopTimer();
extern void GetFileExtension(TCHAR* fname,TCHAR* ext);
extern void GetFileExtensionA(char* fname,char* ext);
extern void GetFileDirectory(wchar_t* path,wchar_t* out);
extern BOOL ReadTagsV1(TCHAR* file,TAGS_GENERIC* out);
extern BOOL ReadTagsV1A(char* file,TAGS_GENERIC* out);

//EXTERN VARS
extern ScrollbarControl Progress;
extern HWND hVolume;

//THIS MODULE DECL.
extern HWND PlayList;
extern UINT CountTracks;
extern UINT CurrentTrack;
extern TCHAR* Extensions[COUNT_EXTENSIONS];
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
BOOL ReadApeTags(TCHAR* file,TAGS_GENERIC* out);
BOOL ReadApeTagsA(char* file,TAGS_GENERIC* out);
BOOL ReadFlacTags(TCHAR* file,TAGS_GENERIC* out);
BOOL ReadFlacTagsA(char* file,TAGS_GENERIC* out);
BOOL ReadTagsv2(TCHAR* fname,TAGS_GENERIC* out);
void InsertPlaylistElement(WCHAR* fname,UINT pos);
void Playlist_Paste();void Playlist_Cut();void Playlist_Copy();
void Playlist_SelectAll();

void InitImageList();
int GetTypeIcon(TCHAR* ext);
#endif