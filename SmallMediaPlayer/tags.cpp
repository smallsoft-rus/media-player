/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "tags.h"

typedef struct {
    char sig[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    BYTE genre;
} MP3TAG_V1;

BOOL ReadTagsV1(TCHAR* file,TAGS_GENERIC* out){

HANDLE hFile;
DWORD dwCount;
BOOL res;
int i;
MP3TAG_V1 tags={0};

memset(&tags,0,sizeof(MP3TAG_V1));
hFile=CreateFile(file,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
if(hFile==INVALID_HANDLE_VALUE)return FALSE;

SetFilePointer(hFile,-128,NULL,FILE_END);
res=ReadFile(hFile,&tags,sizeof(MP3TAG_V1),&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
CloseHandle(hFile);

if(tags.title[0]==0)return FALSE;

if(tags.sig[0]=='T'&&tags.sig[1]=='A'&&tags.sig[2]=='G'){
    //delete spaces
    for(i=29;i>=0;i--){
        if(tags.title[i]==' '){tags.title[i]=0;continue;}
        if(tags.title[i]!=0)break;
    }
    for(i=29;i>=0;i--){
        if(tags.artist[i]==' '){tags.artist[i]=0;continue;}
        if(tags.artist[i]!=0)break;
    }
    for(i=29;i>=0;i--){
        if(tags.album[i]==' '){tags.album[i]=0;continue;}
        if(tags.album[i]!=0)break;
    }
    for(i=3;i>=0;i--){
        if(tags.year[i]==' '){tags.year[i]=0;continue;}
        if(tags.year[i]!=0)break;
    }
    MultiByteToWideChar(CP_ACP,0,tags.title,30,out->title,512);
    MultiByteToWideChar(CP_ACP,0,tags.artist,30,out->artist,512);
    MultiByteToWideChar(CP_ACP,0,tags.album,30,out->album,512);
    MultiByteToWideChar(CP_ACP,0,tags.year,4,out->year,10);
    MultiByteToWideChar(CP_ACP,0,tags.comment,30,out->comments,1024);
    out->type=TAG_ID3V1;
    return TRUE;
}
else return FALSE;

}

BOOL ReadTagsV1A(char* file,TAGS_GENERIC* out){
    WCHAR filename[MAX_PATH]=L"";

    if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,filename,MAX_PATH)==0)
        return FALSE;
    else 
        return ReadTagsV1(filename,out);
}
