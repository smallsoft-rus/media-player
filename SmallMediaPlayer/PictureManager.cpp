﻿/* Small Media Player 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "PictureManager.h"
#include <shlwapi.h>

#define COUNT_COVER_MASKS (sizeof(arrCoverMasks)/sizeof(arrCoverMasks[0]))
#define COUNT_PATTERN_MASKS (sizeof(arrPatternMasks)/sizeof(arrPatternMasks[0]))

Bitmap* CurrentCover;
Bitmap* CurrentPattern;
IStream* pCoverStream=nullptr;
bool fCoverLoaded=false;
bool fPatternLoaded=false;
WCHAR CurrentCoverPath[MAX_PATH]=L"";

TCHAR* arrCoverMasks[]={
	L"cover.jpg",
	L"cover.jpeg",
	L"*front*.jpg",
	L"*front*.jpeg",
	L"AlbumArt*Large.jpg",
	L"AlbumArt*Large.jpeg",
	L"folder.jpg",
	L"folder.jpeg",
	L"*.jpg",
	L"*.jpeg",
	L"*.bmp",
	L"*.png"};

TCHAR* arrPatternMasks[]={
	L"*.jpg",
	L"*.jpeg",
	L"*.png"
};


BOOL FindPicture(TCHAR* dir,TCHAR* mask,TCHAR* out){

HANDLE hSearch;
WIN32_FIND_DATA fd={0};
TCHAR find[MAX_PATH];

lstrcpy(find,dir);

lstrcat(find,mask);
hSearch=FindFirstFile(find,&fd);
if(hSearch==INVALID_HANDLE_VALUE)return FALSE;
lstrcpy(out,dir);

lstrcat(out,fd.cFileName);
return TRUE;

}

void LoadCover(WCHAR* file){

    UnloadCover();

    if(lstrcmp(file,L"")==0){return;}
	
    CurrentCover=new Bitmap(file);
    fCoverLoaded=true;
    StringCchCopy(CurrentCoverPath,MAX_PATH,file);
}

void LoadCoverFromMemory(const BYTE* pImageData, UINT size){

    UnloadCover();

    pCoverStream = SHCreateMemStream(pImageData, size);
    CurrentCover=new Bitmap(pCoverStream);
    fCoverLoaded=true;
}

bool DrawCover(HDC hDC,RECT* rc){
	Graphics* g;
	int w,h;
	int wd,hd;
	float factor;
if(fCoverLoaded==false)return false;
g=new Graphics(hDC);
w=CurrentCover->GetWidth();
h=CurrentCover->GetHeight();

wd=rc->right;
factor=wd/(float)w;
hd=h*factor;
if(hd>(rc->bottom)){
hd=rc->bottom;
factor=hd/(float)h;
wd=w*factor;
}
	g->DrawImage(CurrentCover,0,0,wd,hd);

delete g;
return true;

}

void UnloadCover(){

    if(fCoverLoaded==false) return;

    if(pCoverStream != nullptr){
        pCoverStream->Release();
        pCoverStream = nullptr;
    }

    delete CurrentCover;
    StringCchCopy(CurrentCoverPath,MAX_PATH,L"");
    fCoverLoaded=false;
}

void LoadPattern(TCHAR* file){

	UnloadPattern();
	
	if(lstrcmp(file,L"")==0){return;}
	
	CurrentPattern=Bitmap::FromFile(file);
	
	if(CurrentPattern->GetHeight()==0||CurrentPattern->GetWidth()==0)return;
	
	fPatternLoaded=true;
	
}

void UnloadPattern(){
if(fPatternLoaded==false)return;
delete CurrentPattern;
fPatternLoaded=false;
}

void DrawPattern(HDC hDC,RECT* rc){	

Graphics* g;
	int w,h;
	int wd,hd;
	float factor;
if(fPatternLoaded==false)return;
g=new Graphics(hDC);
w=CurrentPattern->GetWidth();
h=CurrentPattern->GetHeight();

wd=rc->right;
factor=wd/(float)w;
hd=h*factor;
if(hd>(rc->bottom)){
hd=rc->bottom;
factor=hd/(float)h;
wd=w*factor;
}
	g->DrawImage(CurrentPattern,0,0,wd,hd);

delete g;
}

void GetFileCover(TCHAR* file,TCHAR* out){
TCHAR dir[MAX_PATH];
TCHAR buf[MAX_PATH];
TCHAR srcdir[MAX_PATH];
BOOL res;
int i;

GetFileDirectory(file,srcdir);
lstrcpy(dir,srcdir);

for(i=0;i<COUNT_COVER_MASKS;i++){
res=FindPicture(dir,arrCoverMasks[i],buf);
if(res==TRUE)goto end;

}
lstrcat(dir,L"covers\\");
for(i=0;i<COUNT_COVER_MASKS;i++){
res=FindPicture(dir,arrCoverMasks[i],buf);
if(res==TRUE)goto end;

}
lstrcpy(dir,srcdir);
lstrcat(dir,L"art\\");
for(i=0;i<COUNT_COVER_MASKS;i++){
res=FindPicture(dir,arrCoverMasks[i],buf);
if(res==TRUE)goto end;}

lstrcpy(dir,srcdir);
lstrcat(dir,L"..\\art\\");
for(i=0;i<COUNT_COVER_MASKS;i++){
res=FindPicture(dir,arrCoverMasks[i],buf);
if(res==TRUE)goto end;}

lstrcpy(dir,srcdir);
lstrcat(dir,L"..\\covers\\");
for(i=0;i<COUNT_COVER_MASKS;i++){
res=FindPicture(dir,arrCoverMasks[i],buf);
if(res==TRUE)goto end;}

if(res==FALSE)return;
end:
lstrcpy(out,buf);
}

void GetPattern(TCHAR* out){
    TCHAR path[MAX_PATH]=L"";
    TCHAR windir[MAX_PATH]=L"";
    TCHAR dir[MAX_PATH]=L"";
    BOOL res;
	int i;

    GetWindowsDirectory(windir,MAX_PATH);
    lstrcat(windir,L"\\");

    lstrcpy(dir,windir);
    lstrcat(dir,L"Web\\Wallpaper\\Windows\\");

    for(i=0;i<COUNT_PATTERN_MASKS;i++){
        res=FindPicture(dir,arrPatternMasks[i],path);
        if(res==TRUE)goto end;
	}
	
    if(res==FALSE)return;

end:lstrcpy(out,path);
}

const WCHAR* PictureManager_GetCurrentCoverPath(){
    return CurrentCoverPath;
}
