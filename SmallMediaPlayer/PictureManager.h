/* Small Media Player 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#define STRSAFE_NO_DEPRECATE
#include <Strsafe.h>
#include <GdiPlus.h>
#include <shlobj.h>
using namespace Gdiplus;

//this module!!!
extern Bitmap* CurrentCover;
extern bool fCoverLoaded;

BOOL FindPicture(TCHAR* dir,TCHAR* mask,TCHAR* out);

void LoadCover(TCHAR* file);
void LoadCoverFromMemory(const BYTE* pImageData, UINT size);
bool DrawCover(HDC hDC,RECT* rc);
void UnloadCover();
void GetFileCover(TCHAR* file,TCHAR* out);
void GetPattern(TCHAR* out);
void DrawPattern(HDC hDC,RECT* rc);
void UnloadPattern();
void LoadPattern(TCHAR* file);
const WCHAR* PictureManager_GetCurrentCoverPath();
