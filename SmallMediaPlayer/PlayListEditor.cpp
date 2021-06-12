/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */

#include "PlayListEditor.h"
#include "errors.h"

/*
10.09.2017 - Выделение текущего элемента в списке воспроизведения '>'
*/

extern void ProcessMessages();
extern void GetFileDirectoryA(char* path,char* out);
extern bool pls_SortReverse;
extern HWND hPlaybackDlg;

HWND PlayList;
HWND hlstBuffer=NULL;
UINT CountTracks=0;
UINT CurrentTrack=0;
bool fIncludeImages=false;
TCHAR* Extensions[COUNT_EXTENSIONS];
PLAYBACK_MODE CurrMode=NORMAL;
TAGS_GENERIC CurrFileTags;
bool fCurrFileTags=false;
TAGS_GENERIC OpenedFileTags;
bool fOpenedFileTags=false;
HICON hDirIcon=NULL;
HICON hMusicIcon=NULL;
int indexDirIcon=0;
int indexMusicIcon=0;
HIMAGELIST ImageList=NULL;

TCHAR* ImageExtensions[]={
	L"jpg",	L"jpeg",L"bmp",L"png",L"gif"};

	TYPE_ICON_INFO arrTypeIcons[COUNT_TYPE_ICONS]={
		{L"mp3",0,0},{L"flac",0,0},{L"wv",0,0},{L"wma"},
		{L"mpg",0,0},{L"mpeg",0,0},{L"wmv"},
		{L"avi",0,0},{L"mkv",0,0},{L"flv",0,0},
		{L"cda",0,0},
		{L"bmp",0,0},{L"jpg",0,0},{L"jpeg",0,0},{L"gif",0,0},{L"png",0,0}
	};

    const char strPlaylistHeader[]="#~Small Media Player playlist (https://github.com/smallsoft-rus/media-player)\r\n";
    const char strEncoding[]="#~ENCODING: UTF8\r\n";



BOOL GetPlaylistElement(int n,TCHAR* out){
BOOL res;
LVITEM item={0};
if(n>=(int)CountTracks||n<0)return FALSE;

item.iItem=n;
item.iSubItem=4;
item.mask=LVIF_TEXT;
item.pszText=out;
item.cchTextMax=MAX_PATH;
//EnterCriticalSection(&csPlaylistAccess);
res=ListView_GetItem(    PlayList,    &item);
//LeaveCriticalSection(&csPlaylistAccess);
return (res);
}

BOOL GetPlaylistElementA(int n,char* out){
BOOL res;
TCHAR buf[MAX_PATH]=L"";
res=GetPlaylistElement(n,buf);
if(res==FALSE)return FALSE;
res=WideCharToMultiByte(CP_UTF8,0,buf,MAX_PATH,out,MAX_PATH,NULL,NULL);
if(res==FALSE)return FALSE;
return TRUE;
}

void AddPlaylistElement(WCHAR* fname){
	if(CountTracks>=1000000)return;
InsertPlaylistElement(fname,CountTracks);
}



void InsertPlaylistElement(WCHAR* fname,UINT pos){

	TAGS_GENERIC info={0};
	LVITEM item={0};
	int iIcon;
	BOOL res=FALSE;
	WCHAR buf[1000]=L"";
	TCHAR ext[10];
	LONG lRes;
	
	if(pos>CountTracks)pos=CountTracks;
	if(pos<0)pos=0;
	if(pos<=CurrentTrack)CurrentTrack++;
	GetFileExtension(fname,ext);
if(lstrcmp(ext,L"M3U")==0||lstrcmp(ext,L"m3u")==0||lstrcmp(ext,L"lst")==0||lstrcmp(ext,L"LST")==0){
LoadTextPlaylist(fname);
return;
}


iIcon=GetTypeIcon(ext);

if(fCurrFileTags==true){
info=CurrFileTags;fCurrFileTags=false;
goto Use_tags;
}
		res=ReadTagsv2(fname,&info);
		if(lstrcmpi(ext,L"flac")==0&&(res==FALSE)){
		res=ReadFlacTags(fname,&info);
		}
		
		if(res==FALSE)res=ReadApeTags(fname,&info);	
		if(res==FALSE){ res=ReadTagsV1(fname,&info);}



if(res==FALSE){//if there's no data...
item.iItem=pos;
item.mask=LVIF_TEXT|LVIF_IMAGE;
item.iImage=iIcon;
item.pszText=GetShortName(fname);
if(lstrcmp(item.pszText,L"")==0)item.pszText=fname;

lRes=ListView_InsertItem(PlayList,&item);
if(lRes==-1){return;}

	}

else{//read tags
Use_tags:
	StringCchCopyN(buf,1000,info.title,512);
	if(lstrcmp(buf,L"")==0){
	item.iItem=pos;
item.mask=LVIF_TEXT|LVIF_IMAGE;
item.iImage=iIcon;
item.pszText=GetShortName(fname);
if(lstrcmp(item.pszText,L"")==0)item.pszText=fname;
lRes=ListView_InsertItem(PlayList,&item);
if(lRes==-1){return;}
	}
	else{
item.iItem=pos;
item.mask=LVIF_TEXT|LVIF_IMAGE;
item.iImage=iIcon;
item.pszText=buf;
lRes=SendMessage(PlayList,LVM_INSERTITEM,0,(LPARAM)&item);
if(lRes==-1){return;}
	}
StringCchCopyN(buf,1000,info.artist,512);
item.iItem=pos;
item.iSubItem=1;
item.mask=LVIF_TEXT;
item.pszText=buf;
res=SendMessage(PlayList,LVM_SETITEM,0,(LPARAM)&item);
StringCchCopyN(buf,1000,info.album,512);
item.iItem=pos;
item.iSubItem=2;
item.mask=LVIF_TEXT;
item.pszText=buf;
res=SendMessage(PlayList,LVM_SETITEM,0,(LPARAM)&item);
StringCchCopyN(buf,1000,info.year,10);
item.iItem=pos;
item.iSubItem=3;
item.mask=LVIF_TEXT;
item.pszText=buf;
res=SendMessage(PlayList,LVM_SETITEM,0,(LPARAM)&item);
}
	

//insert full path
item.iItem=pos;
item.iSubItem=4;
item.mask=LVIF_TEXT;
item.pszText=fname;
res=ListView_SetItem(PlayList,&item);
if(res==FALSE){return;}
CountTracks++;

}

void AddPlaylistElementA(char* fname){
    TCHAR filename[MAX_PATH]=L"";
    MultiByteToWideChar(CP_UTF8,0,fname,MAX_PATH,filename,MAX_PATH);
    AddPlaylistElement(filename);
}

int GetPlaylistSelectedElement(){
	int res;
	//EnterCriticalSection(&csPlaylistAccess);
res=ListView_GetSelectionMark(PlayList);
//LeaveCriticalSection(&csPlaylistAccess);
return res;
}

BOOL IsPlaylistItemSelected(int n){
UINT res;
if(n>=(int)CountTracks||n<0)return FALSE;

res=ListView_GetItemState(PlayList,n,LVIS_SELECTED);

if(res&LVIS_SELECTED)return TRUE;
else return FALSE;

}
void SetPlaylistHighlight(int i){
	if(i>=(int)CountTracks||i<0)return;
int j;

ListView_SetItemState(PlayList,i,LVIS_SELECTED,LVIS_SELECTED);
for(j=0;j<(int)CountTracks;j++){
if(j==i)continue;
ListView_SetItemState(PlayList,j,0,LVIS_SELECTED);
}
}
void SetPlaylistSelectedElement(int i){
	if(i>=(int)CountTracks||i<0)return;
ListView_SetItemState(PlayList,i,LVIS_SELECTED,LVIS_SELECTED);
}

void ClearPlaylist(){
	
	ListView_DeleteAllItems(PlayList);
CountTracks=0;
CurrentTrack=0;
}

void DeletePlaylistElement(int n){
if(((UINT)n)>=CountTracks||n<0)return;
ListView_DeleteItem(  PlayList,    n);
if(n<=(int)CurrentTrack)CurrentTrack--;
CountTracks--;

}


void GetCurrTrackShortName(TCHAR* str){
LVITEM item={0};
TCHAR buf[256];
TCHAR * p = NULL;
StringCchCopy(str,256,L"");
if(PlayerState==FILE_NOT_LOADED)return;
if(CountTracks==0)return;
	item.mask=LVIF_TEXT;
	item.pszText=buf;
	item.cchTextMax=256;
	item.iItem=CurrentTrack;
	item.iSubItem=0;
	
	ListView_GetItem(PlayList,&item);

	//***
	if(buf[0]=='>')p=buf+1;//удалить символ текущего трека
	else p=buf;	
	StringCchCopy(str,256,p);
	//***

	item.iSubItem=1;
	StringCchCopy(buf,256,L"");
	
	if(ListView_GetItem(PlayList,&item)!=FALSE && lstrcmp(buf,L"")!=0){
		StringCchCat(str,256,L" - ");
		StringCchCat(str,256,buf);
	}
	

}

void SetCurrentTrackIndicator(BOOL add)
{
	LVITEM item={0};
TCHAR str[256];
TCHAR buf[256];
TCHAR * p = NULL;

if(CountTracks==0)return;

	//получить имя текущего трека
	item.mask=LVIF_TEXT;
	item.pszText=buf;
	item.cchTextMax=256;
	item.iItem=CurrentTrack;
	item.iSubItem=0;	
	ListView_GetItem(PlayList,&item);

	if(add==FALSE){
		//удалить знак
		if(buf[0]=='>')p=buf+1;
		else p = buf;	
		StringCchCopy(str,256,p);
		
	}else{
		//добавить знак
		StringCchCopy(str,256,L"");
		if(buf[0]!='>')StringCchCat(str,256,L">");
		StringCchCat(str,256,buf);
	}

	//установить имя текущего трека
	item.pszText=str;	
	ListView_SetItem(PlayList,&item);

}


void PlayTrackByNumber(int n){
  TCHAR str[256];
	TCHAR ext[10]=L"";
	BOOL res=FALSE;

if(n<0)return;
if(((UINT)n)>=CountTracks)return;
		if(fOpenedFileTags==true){
			fOpenedFileTags=false;
			memset(&OpenedFileTags,0,sizeof(TAGS_GENERIC));
		}

		//***
		SetCurrentTrackIndicator(FALSE);
		//***

        CurrentTrack=n;
		
		SetPlaylistHighlight(CurrentTrack);
        GetPlaylistElement(n,str);
		if(lstrcmp(str,L"")==0){PlayNextTrack();return;}
		GetFileExtension(str,ext);
		res=ReadTagsv2(str,&OpenedFileTags);
		if(lstrcmpi(ext,L"flac")==0&&res==FALSE){
		res=ReadFlacTags(str,&OpenedFileTags);
		}
		if(res==FALSE)res=ReadApeTags(str,&OpenedFileTags);	
		if(res==FALSE){ res=ReadTagsV1(str,&OpenedFileTags);}
		if(res!=FALSE){fOpenedFileTags=true;} else {fOpenedFileTags=false;}
		
		
        PlayFile(str);
		EnableWindow(hVolume,TRUE);
		UpdateLength();

		//***
		SetCurrentTrackIndicator(TRUE);
		//***

		RunTimer();
		
		Progress.SetTrackPosition(0);

}

void PlayNextTrack(){
	int size;
	RECT rc={0};
	TCHAR str[MAX_PATH]=L"";
		
	//***
	SetCurrentTrackIndicator(FALSE);
	//***
	ProcessMessages();
	
	switch(CurrMode){
case NORMAL:case REPEAT_LIST:InterlockedIncrement((volatile LONG*)&CurrentTrack);break;
case REPEAT_FILE: PlayTrackByNumber(CurrentTrack);return;
case RANDOM:CurrentTrack=rand()%CountTracks;break;
	}


 if(CurrentTrack>=CountTracks){

        CurrentTrack=0;
		if(CurrMode==REPEAT_LIST){PlayTrackByNumber(CurrentTrack);}
		else {StopTimer();
		DisableFullScreen();
		}
        return;}

	//scroll playlist
 ListView_GetItemRect(PlayList,0,&rc,LVIR_SELECTBOUNDS);
 size=rc.bottom-rc.top;
 ListView_Scroll(PlayList,0,size);

 SetPlaylistHighlight(CurrentTrack);
 PlayTrackByNumber(CurrentTrack);
 	
}


void AddDirectory(wchar_t* dir,wchar_t* ext,HWND hlbt){

   wchar_t mask[1024];
   TCHAR fname[1024];
      
        HANDLE hSearch;
		
        WIN32_FIND_DATAW fd={0};
		int c=0;
		LONG lRes;

		
         StringCchCopy(mask,1024,dir);
         StringCchCat(mask,1024,L"*.");
         StringCchCat(mask,1024,ext);
         
//find items and insert into temp list
        hSearch=FindFirstFileW(mask,&fd);
		if(hSearch==INVALID_HANDLE_VALUE){return;}
		StringCchCopy(fname,1024,dir);
		StringCchCat(fname,1024,fd.cFileName);
        
		SendMessage(hlbt,LB_ADDSTRING,0,(LONG)fname);
		
		c++;

        while(FindNextFileW(hSearch,&fd)){
          StringCchCopy(fname,1024,dir);
		StringCchCat(fname,1024,fd.cFileName);
		        
lRes=SendMessage(hlbt,LB_ADDSTRING,0,(LONG)fname);
if(lRes==LB_ERR){FindClose(hSearch);return;}
if(lRes==LB_ERRSPACE){FindClose(hSearch);return;}
c++;
        }
        FindClose(hSearch);

 }



void Playlist_AddDirectory(TCHAR* dir){
HANDLE hSearch;

WIN32_FIND_DATA fd={0};
int i,c;	
LRESULT res;
TCHAR mask[MAX_PATH]=L"";
TCHAR buf[MAX_PATH]=L"";

HWND hlbt=CreateWindowW(L"LISTBOX",L"TEMP",LBS_SORT,0,0,200,200,NULL,NULL,GetModuleHandle(NULL),0);

if(hlbt==NULL){
    HandleError(L"Не удалось создать список",SMP_ALERT_BLOCKING,L"");
    return;
}

for(i=0;i<COUNT_EXTENSIONS;i++){
	AddDirectory(dir,Extensions[i],hlbt);
	
}
if(fIncludeImages==true){
for(i=0;i<COUNT_IMAGE_EXTENSIONS;i++){
	AddDirectory(dir,ImageExtensions[i],hlbt);
	
}
}
c=SendMessage(hlbt,LB_GETCOUNT,0,0);
for(i=0;i<c;i++){
res=SendMessage(hlbt,LB_GETTEXT,i,(LPARAM)buf);
if(res==LB_ERR){continue;}
AddPlaylistElement(buf);

}
DestroyWindow(hlbt);
if(CountTracks>=1000000)return;
StringCchCopy(mask,MAX_PATH,dir);
StringCchCat(mask,MAX_PATH,L"*");
hSearch=FindFirstFile(mask,&fd);
if(hSearch==INVALID_HANDLE_VALUE)return;
while(1){
	
	if(lstrcmp(fd.cFileName,L".")!=0 && lstrcmp(fd.cFileName,L"..")!=0	){

	if(fd.dwFileAttributes & (DWORD)FILE_ATTRIBUTE_DIRECTORY){
		StringCchCopy(buf,MAX_PATH,dir);
		StringCchCat(buf,MAX_PATH,fd.cFileName);
		StringCchCat(buf,MAX_PATH,L"\\");
		
		Playlist_AddDirectory(buf);
		
	}
	}
	if(FindNextFile(hSearch,&fd)==FALSE)break;
	
}
FindClose(hSearch);

}

BOOL IsPathAbsolute(char* path){
int i,c;
c=lstrlenA(path);
for(i=0;i<c-1;i++){
	if(path[i]==':')return TRUE;
	if(path[i]=='\\'&&path[i+1]=='\\')return TRUE;}
return FALSE;
}



void LoadTextPlaylist(TCHAR* file){
HANDLE hFile;
DWORD dwCount;
char str[MAX_PATH]="";
char str_dir[MAX_PATH]="";
char str_path[MAX_PATH*2]="";
TCHAR dir[MAX_PATH]=L"";
BOOL res;
char c;
int i=0;
int l;
GetFileDirectory(file,dir);
l=lstrlen(dir);
if(dir[l-1]!='\\'){dir[l-1]='\\';dir[l]=0;}
WideCharToMultiByte(CP_UTF8,0,dir,lstrlen(dir),str_dir,sizeof(str_dir),NULL,NULL);

hFile=CreateFile(file,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
if(hFile==INVALID_HANDLE_VALUE)return;

while(1){
res=ReadFile(hFile,&c,1,&dwCount,NULL);
if(res==FALSE)break;
if(dwCount==0)break;
if(c=='\n'||c=='\r'||c==0){
str[i]=0;
i=0;
if(lstrcmpA(str,"")==0)continue;
if(str[0]=='#'){	
	ReadPlaylistTags(str);
	continue;}
if(IsPathAbsolute(str)==FALSE){
StringCchCopyA(str_path,520,str_dir);
StringCchCatA(str_path,520,str);
AddPlaylistElementA(str_path);
}else{
	AddPlaylistElementA(str);
}
continue;
}
if(i>=MAX_PATH-2)continue;
str[i]=c;
i++;
}

str[i]=0;
if(lstrcmpA(str,"")==0)goto end;
if(str[0]=='#')goto end;

if(IsPathAbsolute(str)==FALSE){
StringCchCopyA(str_path,520,str_dir);
StringCchCatA(str_path,520,str);
AddPlaylistElementA(str_path);
}else{
	AddPlaylistElementA(str);
}

end:
CloseHandle(hFile);
}

void LoadTextPlaylistA(char* file){
    TCHAR filename[MAX_PATH]=L"";
    MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,filename,MAX_PATH);
    LoadTextPlaylist(filename);
}


void ReadPlaylistTags(char* _str){

char* p;
char str[512]="";
char* context=NULL;

memset(&CurrFileTags,0,sizeof(TAGS_GENERIC));
StringCchCopyA(str,512,_str);
p=strtok_s(str,"~",&context);
if(p==NULL)return;
if(lstrcmpA(p,"#TAGS")!=0)return;
p=strtok_s(NULL,"~",&context);
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.title,512);
fCurrFileTags=true;
p=strtok_s(NULL,"~",&context);
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.artist,512);
p=strtok_s(NULL,"~",&context);
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.album,512);
p=strtok_s(NULL,"~",&context);
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.year,512);
}

void PrepareTagString(char * str, int maxlen){
	//replace ~ as it is used as field separator when saving
	//playlist to text
	for(int i=0;i<maxlen;i++){
		if(str[i]==0)break;
		if(str[i]=='~')str[i]=' ';
	}
}

//Get ListView subitem text as UTF-8
void LvGetItemTextA(HWND hWnd,int iItem,int iSubitem,char* str,int maxcount){
    //get UTF-16 item text
    const int bufsize=1024;
    WCHAR buf[bufsize]=L"";
    LVITEM item={0};
    item.iItem=iItem;
    item.iSubItem=iSubitem;
    item.mask=LVIF_TEXT;
    item.pszText=buf;
    item.cchTextMax=bufsize;
    SendMessage(hWnd,LVM_GETITEMTEXT,iItem,(LPARAM)&item);

    //convert UTF-16 item text to UTF-8
    WideCharToMultiByte(CP_UTF8,0,buf,bufsize,str,maxcount,NULL,NULL);
}

// Write tags comment string of the specified trach number into playlist file. 
// This enables faster loading of tags data when playlist is opened.
void WritePlaylistTags(HANDLE hFile,int index){
char str[300]="";
char * p = NULL;
DWORD dwCount;

WriteFile(hFile,"#TAGS~",6,&dwCount,NULL);

//title
LvGetItemTextA(PlayList,index,0,str,sizeof(str));
PrepareTagString(str,sizeof(str));

if(str[0]=='>')p=str+1;//удалить символ текущего трека
else p=str;

WriteFile(hFile,p,lstrlenA(p),&dwCount,NULL);

if(lstrlenA(str)==0)WriteFile(hFile," ",1,&dwCount,NULL);
StringCchCopyA(str,300,"");
WriteFile(hFile,"~",1,&dwCount,NULL);

LvGetItemTextA(PlayList,index,1,str,sizeof(str));
PrepareTagString(str,sizeof(str));
WriteFile(hFile,str,lstrlenA(str),&dwCount,NULL);
if(lstrlenA(str)==0)WriteFile(hFile," ",1,&dwCount,NULL);
StringCchCopyA(str,300,"");
WriteFile(hFile,"~",1,&dwCount,NULL);

LvGetItemTextA(PlayList,index,2,str,sizeof(str));
PrepareTagString(str,sizeof(str));
WriteFile(hFile,str,lstrlenA(str),&dwCount,NULL);
if(lstrlenA(str)==0)WriteFile(hFile," ",1,&dwCount,NULL);
StringCchCopyA(str,300,"");
WriteFile(hFile,"~",1,&dwCount,NULL);

LvGetItemTextA(PlayList,index,3,str,sizeof(str));
PrepareTagString(str,sizeof(str));
WriteFile(hFile,str,lstrlenA(str),&dwCount,NULL);
if(lstrlenA(str)==0)WriteFile(hFile," ",1,&dwCount,NULL);
WriteFile(hFile,"\r\n",2,&dwCount,NULL);

}


void SaveTextPlaylist(TCHAR* file){
HANDLE hFile;
DWORD dwCount;
char str[MAX_PATH]="";
BOOL res;
char c;
UINT i=0;
int j=0;
hFile=CreateFile(file,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
if(hFile==INVALID_HANDLE_VALUE)return;

    //write header
    WriteFile(hFile,strPlaylistHeader,sizeof(strPlaylistHeader)-1,&dwCount,NULL);

    //write encoding marker
    WriteFile(hFile,strEncoding,sizeof(strEncoding)-1,&dwCount,NULL);

for(i=0;i<CountTracks;i++){
GetPlaylistElementA(i,str);
WritePlaylistTags(hFile,i);
j=0;
while(1){
	if(str[j]==0)break;
	c=(char)str[j];
	res=WriteFile(hFile,&c,1,&dwCount,NULL);
	if(res==FALSE)break;
	j++;
}
c='\r';
res=WriteFile(hFile,&c,1,&dwCount,NULL);
if(res==FALSE)break;
c='\n';
res=WriteFile(hFile,&c,1,&dwCount,NULL);
if(res==FALSE)break;

}
CloseHandle(hFile);
}

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort){
TCHAR str1[280]=L"";
TCHAR str2[280]=L"";
bool rev=false;

ListView_GetItemText (PlayList,lParam1,lParamSort,str1,280);
ListView_GetItemText (PlayList,lParam2,lParamSort,str2,280);
if(pls_SortReverse==true){return -lstrcmp(str1,str2);}
else return lstrcmp(str1,str2);
}

int FindTrack(TCHAR* fname){
int i;
TCHAR buf[MAX_PATH];
for(i=0;i<(int)CountTracks;i++){
GetPlaylistElement(i,buf);
if(lstrcmp(buf,fname)==0)return i;
}
return -1;
}

void Playlist_Clearbuffer(){
int i;int c;
c=SendMessage(hlstBuffer,LB_GETCOUNT,0,0);
for(i=0;i<c;i++){
SendMessage(hlstBuffer,LB_DELETESTRING,0,0);
}
}

void Playlist_Cut(){
	int i;
	BOOL res;
	Playlist_Clearbuffer();
	TCHAR buf[MAX_PATH]=L"";
	for(i=CountTracks-1;i>=0;i--){
		if(IsPlaylistItemSelected(i)!=FALSE){
			if(CurrentTrack==i){Stop();Close();CurrentTrack=0;}
		res=GetPlaylistElement(i,buf);
		if(res==FALSE)continue;
		DeletePlaylistElement(i);
		SendMessage(hlstBuffer,LB_ADDSTRING,0,(LPARAM)buf);
		}
	}
}

void Playlist_Copy(){
	int i;
	BOOL res;
	Playlist_Clearbuffer();
	TCHAR buf[MAX_PATH]=L"";
	for(i=CountTracks-1;i>=0;i--){
		if(IsPlaylistItemSelected(i)!=FALSE){
		res=GetPlaylistElement(i,buf);
		if(res==FALSE)continue;
		SendMessage(hlstBuffer,LB_ADDSTRING,0,(LPARAM)buf);
		}
	}
}
void Playlist_Paste(){
int i;int p;int c;
TCHAR buf[MAX_PATH]=L"";
p=GetPlaylistSelectedElement();
if(p<0)p=0;
if(p>(int)CountTracks)p=CountTracks;
c=SendMessage(hlstBuffer,LB_GETCOUNT,0,0);
for(i=c-1;i>=0;i--){
SendMessage(hlstBuffer,LB_GETTEXT,i,(LONG)buf);
InsertPlaylistElement(buf,p);
p++;
}


}

void Playlist_SelectAll(){
    int i;
    for(i=0;i<(int)CountTracks;i++){
        ListView_SetItemState(PlayList,i,LVIS_SELECTED,LVIS_SELECTED);
    }
}

//warning caused by Windows SDK MAKEINTRESOURCE macro 
#pragma warning(disable:4302)
//fills image list with icons for playlist files
void InitImageList(){
    SHFILEINFO si={0};
    DWORD_PTR res;
    TCHAR dir[MAX_PATH]=L"c:\\windows";
    TCHAR ext[MAX_PATH]=L"c:\\x.mp3";
    int i;

    ImageList=ImageList_Create(16, 16, ILC_MASK| ILC_COLOR32, COUNT_TYPE_ICONS+1, 30);

    for(i=0;i<COUNT_TYPE_ICONS;i++){
        StringCchPrintf(ext,MAX_PATH,L"c:\\x.%s",arrTypeIcons[i].ext);
        res=SHGetFileInfo(
            ext,FILE_ATTRIBUTE_NORMAL,&si,sizeof(SHFILEINFO),SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_SMALLICON
            );
        if(res==FALSE)arrTypeIcons[i].hIcon=LoadIcon(NULL,MAKEINTRESOURCE(IDI_APPLICATION));
        else arrTypeIcons[i].hIcon=si.hIcon;
        arrTypeIcons[i].iIcon=ImageList_AddIcon(ImageList, arrTypeIcons[i].hIcon);
    }

    ListView_SetImageList(PlayList,ImageList,LVSIL_SMALL);
}
#pragma warning(default:4302)

int GetTypeIcon(TCHAR* ext){
    int i;
    for(i=0;i<COUNT_TYPE_ICONS;i++){
        if(lstrcmpi(ext,arrTypeIcons[i].ext)==0)return i;
    }
    return 0;
}
