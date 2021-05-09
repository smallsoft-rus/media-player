/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef TAGS_H
#define TAGS_H

#include <Windows.h>

typedef enum {TAG_NO=0,TAG_ID3V1,TAG_ID3V2,TAG_APE,TAG_FLAC}TAG_TYPE;

typedef struct{
    WCHAR title[512];
    WCHAR album[512];
    WCHAR artist[512];
    WCHAR year[10];
    WCHAR comments[1024];
    WCHAR composer[512];
    WCHAR URL[512];
    DWORD length;
    TAG_TYPE type;
}TAGS_GENERIC;

// Exported functions
BOOL ReadTagsV1(TCHAR* file,TAGS_GENERIC* out);
BOOL ReadTagsV1A(char* file,TAGS_GENERIC* out);

#endif
