/* Small Media Player 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef TAGS_H
#define TAGS_H

#include <Windows.h>

typedef enum {TAG_NO=0,TAG_ID3V1,TAG_ID3V2,TAG_APE,TAG_FLAC}TAG_TYPE;

typedef struct{
    BYTE* pData;
    size_t size;
    BYTE pic_type;
    char mime_type[20];
}IMAGE_DATA;

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
    IMAGE_DATA cover;
}TAGS_GENERIC;

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

// Exported functions
BOOL ReadTagsV1(TCHAR* file,TAGS_GENERIC* out);
BOOL ReadTagsV1A(char* file,TAGS_GENERIC* out);
BOOL ReadTagsV2(WCHAR* fname,TAGS_GENERIC* out,BOOL readCover);
BOOL ReadTagsV2A(char* file,TAGS_GENERIC* out);
BOOL ReadFlacTags(WCHAR* file,TAGS_GENERIC* out);
BOOL ReadFlacTagsA(char* file,TAGS_GENERIC* out);
BOOL ReadApeTagsA(char* file,TAGS_GENERIC* out);
BOOL ReadApeTags(WCHAR* file,TAGS_GENERIC* out);

// *** Inline functions ***

//Free memory associated with TAGS_GENERIC struct.
//TAGS_GENERIC should be freed after use when it contains cover image (loaded by ReadTagsV2 function with readCover parameter set to TRUE).
//Otherwise it is not required.
void inline TagsFree(TAGS_GENERIC* pTags){
    if(pTags->cover.pData != nullptr){
        HeapFree(GetProcessHeap(),0,pTags->cover.pData);
    }

    ZeroMemory(pTags, sizeof(TAGS_GENERIC));
}

#endif
