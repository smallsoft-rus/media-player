/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

//this module
extern WCHAR ProgramFileName[256];

BOOL RegisterFolderAssociation();
void DestroyFolderAssociation();
void RegisterFileAssociation(WCHAR* ext);
void DestroyFileAssociation(TCHAR* ext);