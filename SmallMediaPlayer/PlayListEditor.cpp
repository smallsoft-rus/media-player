/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */

#include "PlayListEditor.h"
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

TCHAR* GetShortName(TCHAR* fullname){
TCHAR* p;
int i;
p=fullname;
for(i=0;i<MAX_PATH-1;i++){
	if(fullname[i]==0)break;
	if(fullname[i]==L'\\'||fullname[i]==L'/'){p=&(fullname[i+1]);}
}
return p;
}

char* GetShortNameA(char* fullname){
char* p;
int i;
p=fullname;
for(i=0;i<MAX_PATH-1;i++){
	if(fullname[i]==0)break;
	if(fullname[i]=='\\'||fullname[i]=='/'){p=&(fullname[i+1]);}
}
return p;
}

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
		if(IsPlayingCDA==true)EnableWindow(hVolume,FALSE);
		else EnableWindow(hVolume,TRUE);
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
if(hlbt==NULL){MessageBox(0,L"Не удалось создать список",0,MB_OK|MB_ICONERROR);return;}
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
else return ReadTagsV1(filename,out);
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

memset(&CurrFileTags,0,sizeof(TAGS_GENERIC));
StringCchCopyA(str,512,_str);
p=strtok(str,"~");
if(p==NULL)return;
if(lstrcmpA(p,"#TAGS")!=0)return;
p=strtok(NULL,"~");
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.title,512);
fCurrFileTags=true;
p=strtok(NULL,"~");
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.artist,512);
p=strtok(NULL,"~");
if(p==NULL)return;
MultiByteToWideChar(CP_UTF8,0,p,512,CurrFileTags.album,512);
p=strtok(NULL,"~");
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


void ReadApeItems(HANDLE hFile,APE_HEADER* ph,TAGS_GENERIC* out){
DWORD dwCount;

APE_ITEM item;
char key[256];
char value[512];
int i,j;
for(i=0;i<(int)ph->item_count;i++){
	ReadFile(hFile,&item,sizeof(APE_ITEM),&dwCount,NULL);
	j=0;
	while(1){//read key
	ReadFile(hFile,&(key[j]),1,&dwCount,NULL);
	if(key[j]==0)break;
	j++;
	if(j>=256)break;
	}
	//read value
	if(item.len>=511)item.len=510;
	ReadFile(hFile,value,item.len,&dwCount,NULL);
	value[item.len]=0;
	//what we have:
	
	if(lstrcmpiA(key,"Title")==0){
		MultiByteToWideChar(CP_UTF8,0,value,512,out->title,512);
		continue;
	}
	if(lstrcmpiA(key,"Artist")==0){
		MultiByteToWideChar(CP_UTF8,0,value,512,out->artist,512);
		continue;
	}
	if(lstrcmpiA(key,"Album")==0){
		MultiByteToWideChar(CP_UTF8,0,value,512,out->album,512);
		continue;
	}
	if(lstrcmpiA(key,"Composer")==0){
		MultiByteToWideChar(CP_UTF8,0,value,512,out->composer,512);
		continue;
	}
	if(lstrcmpiA(key,"Comment")==0){
		MultiByteToWideChar(CP_UTF8,0,value,512,out->comments,512);
		continue;
	}
	
	if(lstrcmpiA(key,"Year")==0){
		MultiByteToWideChar(CP_UTF8,0,value,10,out->year,10);
		continue;
	}
	if(lstrcmpiA(key,"Related")==0){
		MultiByteToWideChar(CP_UTF8,0,value,512,out->URL,512);
		continue;
	}

}
}


BOOL ReadApeTagsA(char* file,TAGS_GENERIC* out){
	TCHAR fname[MAX_PATH]=L"";
	if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0)
	{return ReadApeTags(fname,out);}
	else return FALSE;
}


BOOL ReadApeTags(TCHAR* file,TAGS_GENERIC* out){
HANDLE hFile;
DWORD dwCount;
BOOL res;
APE_HEADER header;

hFile=CreateFile(file,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
if(hFile==INVALID_HANDLE_VALUE)return FALSE;
SetFilePointer(hFile,-(int)sizeof(APE_HEADER),NULL,FILE_END);
res=ReadFile(hFile,&header,sizeof(APE_HEADER),&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
if(dwCount!=sizeof(APE_HEADER)){CloseHandle(hFile);return FALSE;}
if((header.sig[0]=='A'&&header.sig[1]=='P'&&header.sig[2]=='E'&&
header.sig[3]=='T'&&header.sig[4]=='A'&&header.sig[5]=='G'&&header.sig[6]=='E'&&
header.sig[7]=='X')==false){goto Read_from_begin;}
SetFilePointer(hFile,-(int)header.size,NULL,FILE_END);
ReadApeItems(hFile,&header,out);
CloseHandle(hFile);
out->type=TAG_APE;
return TRUE;
	//end read tags(end)
Read_from_begin:

SetFilePointer(hFile,0,NULL,FILE_BEGIN);
res=ReadFile(hFile,&header,sizeof(APE_HEADER),&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
if(dwCount!=sizeof(APE_HEADER)){CloseHandle(hFile);return FALSE;}
if((header.sig[0]=='A'&&header.sig[1]=='P'&&header.sig[2]=='E'&&
header.sig[3]=='T'&&header.sig[4]=='A'&&header.sig[5]=='G'&&header.sig[6]=='E'&&
header.sig[7]=='X')==false){CloseHandle(hFile);return FALSE;}
ReadApeItems(hFile,&header,out);

CloseHandle(hFile);
out->type=TAG_APE;
return TRUE;
}

BOOL ReadFlacTagsA(char* file,TAGS_GENERIC* out){
TCHAR fname[MAX_PATH]=L"";
	if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0)
	{return ReadFlacTags(fname,out);}
	else return FALSE;
}

BOOL ReadFlacTags(TCHAR* file,TAGS_GENERIC* out){
HANDLE hFile;
DWORD dwCount;
BOOL res;
char buf[1024]="";
char name[1024]="";
char value[1024]="";
FLAC_BLOCK_HEADER header={0};
char sig[5]="";
int i,j;
ULONG c,n;
DWORD_UNION length={0};

hFile=CreateFile(file,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

if(hFile==INVALID_HANDLE_VALUE){
	return FALSE;
}

i=0;
while(1){
res=ReadFile(hFile,sig,4,&dwCount,NULL);
SetFilePointer(hFile,i,NULL,FILE_BEGIN);
res=ReadFile(hFile,sig,4,&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
if(dwCount!=4){CloseHandle(hFile);return FALSE;}
if(lstrcmpA(sig,FLAC_SIGNATURE)==0)break;
i++;
if(i>=99999999){CloseHandle(hFile);return FALSE;}
}

res=ReadFile(hFile,&header,sizeof(header),&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
length.bytes[2]=header.length[0];
length.bytes[1]=header.length[1];
length.bytes[0]=header.length[2];
SetFilePointer(hFile,(LONG)length.dword,NULL,FILE_CURRENT);

while(1){
res=ReadFile(hFile,&header,sizeof(header),&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
if(dwCount!=sizeof(header)){CloseHandle(hFile);return FALSE;}
length.dword=0;
length.bytes[2]=header.length[0];
length.bytes[1]=header.length[1];
length.bytes[0]=header.length[2];

if(header.type==FLAC_VORBISCOMMENT||header.type==FLAC_VORBISCOMMENT2){break;}
SetFilePointer(hFile,length.dword,NULL,FILE_CURRENT);
}
ReadFile(hFile,&n,sizeof(n),&dwCount,NULL);
SetFilePointer(hFile,n,NULL,FILE_CURRENT);
ReadFile(hFile,&n,sizeof(n),&dwCount,NULL);

for(i=0;i<n;i++){
	ReadFile(hFile,&c,sizeof(c),&dwCount,NULL);
	ReadFile(hFile,buf,c,&dwCount,NULL);
	buf[c]=0;	
	j=0;
	StringCchCopyA(name,1024,"");
	while(1){
	if(buf[j]=='=')break;
	if(j>=1024)break;
	name[j]=buf[j];
	j++;
	}
	name[j]=0;
	
	if(j>=1024)continue;
	StringCchCopyA(value,1024,&(buf[j+1]));
	
	if(lstrcmpiA(name,"title")==0){
		MultiByteToWideChar(CP_UTF8,0,value,1024,out->title,512);
		continue;
	}
	if(lstrcmpiA(name,"artist")==0){
		MultiByteToWideChar(CP_UTF8,0,value,1024,out->artist,512);
		continue;
	}
	if(lstrcmpiA(name,"album")==0){
		MultiByteToWideChar(CP_UTF8,0,value,1024,out->album,512);
		continue;
	}
	if(lstrcmpiA(name,"date")==0){
		MultiByteToWideChar(CP_UTF8,0,value,10,out->year,10);
		continue;
	}
	if(lstrcmpiA(name,"description")==0){
		MultiByteToWideChar(CP_UTF8,0,value,1024,out->comments,1024);
		continue;
	}
	if(lstrcmpiA(name,"contact")==0){
		MultiByteToWideChar(CP_UTF8,0,value,1024,out->URL,512);
		continue;
	}

}
CloseHandle(hFile);
out->type=TAG_FLAC;
return TRUE;
}


void ReverseWCHAR(WCHAR* c){
	WORD_UNION extractor={0};
	WORD_UNION packer={0};
	extractor.w=(WORD)*c;
	packer.b[0]=extractor.b[1];
	packer.b[1]=extractor.b[0];
	*c=(WCHAR)packer.w;
}


BOOL ReadTagsv2A(char* file,TAGS_GENERIC* out){
TCHAR fname[MAX_PATH]=L"";
	if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0)
	{return ReadTagsv2(fname,out);}
	else return FALSE;
}
BOOL ReadTagsv2(TCHAR* fname,TAGS_GENERIC* out){
HANDLE hFile=0;
DWORD dwCount=0;
BOOL res=0;
BOOL unsync=FALSE;
ID32_HEADER header={0};
DWORD_UNION extractor={0};
DWORD_UNION packer={0};
ID32_EXTHEADER* pextheader=0;
ID32_FRAME_HEADER fh={0};
char* pSourceTags=NULL;DWORD SourceSize=0;
char* pOutputTags=NULL;DWORD OutputRealSize=0;
char buf[1024]="";
TCHAR wbuf[1024]=L"";
WORD BOM=0;

DWORD i=0,j=0;

memset(&extractor,0,sizeof(DWORD));memset(&packer,0,sizeof(DWORD));
memset(out,0,sizeof(TAGS_GENERIC));

hFile=CreateFile(fname,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
if(hFile==INVALID_HANDLE_VALUE)return FALSE;
res=ReadFile(hFile,&header,sizeof(ID32_HEADER),&dwCount,NULL);
if(res==FALSE){CloseHandle(hFile);return FALSE;}
if((header.sig[0]=='I'&&header.sig[1]=='D'&&header.sig[2]=='3')==FALSE)
{CloseHandle(hFile);return FALSE;}
if(header.ver[0]>0x03){CloseHandle(hFile);return FALSE;}
extractor.bytes[0]=header.size[0];
extractor.bytes[1]=header.size[1];
extractor.bytes[2]=header.size[2];
extractor.bytes[3]=header.size[3];

packer.bfe.byte1=extractor.bf.byte4;packer.bfe.byte2=extractor.bf.byte3;
packer.bfe.byte3=extractor.bf.byte2;packer.bfe.byte4=extractor.bf.byte1;
packer.bfe.dummy=0;

SourceSize=packer.dword;

pSourceTags=(char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,SourceSize);
if(pSourceTags==NULL){CloseHandle(hFile);return FALSE;}
ReadFile(hFile,pSourceTags,SourceSize,&dwCount,NULL);
pOutputTags=(char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,SourceSize);
if(pOutputTags==NULL){
	CloseHandle(hFile);
	HeapFree(GetProcessHeap(),0,pSourceTags);
		return FALSE;}

j=0;
for(i=0;i<=SourceSize-3;i++){
	if((header.flags&ID32F_SYNC)>0){
		if(pSourceTags[i]==0xFF&&pSourceTags[i+1]==0&&((pSourceTags[i+2]&ID32_SYNC_MASK)>0)){
		pOutputTags[j]=pSourceTags[i];pOutputTags[j+1]=pSourceTags[i+2];
		i+=2;j+=2;continue;
		}
		if(pSourceTags[i]==0xFF&&pSourceTags[i+1]==0&&pSourceTags[i+2]==0){
			pOutputTags[j]=(char)0xFF;pOutputTags[j+1]=0;
			i+=2;j+=2;continue;
		}

	}
	pOutputTags[j]=pSourceTags[i];j++;

}
pOutputTags[j]=pSourceTags[i];j++;i++;
pOutputTags[j]=pSourceTags[i];j++;i++;

OutputRealSize=j;

HeapFree(GetProcessHeap(),0,pSourceTags);pSourceTags=0;
i=0;
if((header.flags&ID32F_EXTHEADER)!=0){
pextheader=(ID32_EXTHEADER*)&(pOutputTags[i]);
extractor.dword=pextheader->size;
packer.bytes[0]=extractor.bytes[3];packer.bytes[1]=extractor.bytes[2];
packer.bytes[2]=extractor.bytes[1];packer.bytes[3]=extractor.bytes[0];
if(packer.dword>=20)packer.dword=10;
i+=(4+packer.dword);
}

while(1){//read frames
	
if(i>=OutputRealSize)break;
memcpy(&fh,&(pOutputTags[i]),sizeof(ID32_FRAME_HEADER));
StringCchCopyNA(buf,1024,(char*)fh.ID,4);
//fprintf(f,"Frame:%s\n",fh.ID);
if(lstrcmpA((char*)fh.ID,"")==0)break;
extractor.dword=fh.size;
packer.bytes[0]=extractor.bytes[3];
packer.bytes[1]=extractor.bytes[2];
packer.bytes[2]=extractor.bytes[1];
packer.bytes[3]=extractor.bytes[0];
if(i+packer.dword>OutputRealSize+1)break;
i+=10;


if(strncmp((char*)fh.ID,"TALB",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i++;
	MultiByteToWideChar(CP_ACP,0,&(pOutputTags[i]),packer.dword-1,out->album,512);
	i+=packer.dword-1;continue;
	}
	else{
	i++;
	memcpy(&BOM,&(pOutputTags[i]),2);i+=2;
	if(BOM==UNICODE_BOM_DIRECT){
		for(j=i;j<i+packer.dword-3;j+=2){
			ReverseWCHAR((WCHAR*)&(pOutputTags[j]));
		}
	}
	StringCbCopyN(out->album,sizeof(out->album),
		(WCHAR*)&(pOutputTags[i]),packer.dword-3);
	i+=packer.dword-3;
	continue;
	}
}

if(strncmp((char*)fh.ID,"TCOM",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i++;
	MultiByteToWideChar(CP_ACP,0,&(pOutputTags[i]),packer.dword-1,out->composer,512);
	i+=packer.dword-1;continue;
	}
	else{
	i++;
	memcpy(&BOM,&(pOutputTags[i]),2);i+=2;
	if(BOM==UNICODE_BOM_DIRECT){
		for(j=i;j<i+packer.dword-3;j+=2){
			ReverseWCHAR((WCHAR*)&(pOutputTags[j]));
		}
	}
	StringCbCopyN(out->composer,sizeof(out->composer),
		(WCHAR*)&(pOutputTags[i]),packer.dword-3);
	i+=packer.dword-3;
	continue;
	}
}
if(strncmp((char*)fh.ID,"TYER",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i++;
	MultiByteToWideChar(CP_ACP,0,&(pOutputTags[i]),packer.dword-1,out->year,10);
	i+=packer.dword-1;continue;
	}
	else{
	i++;
	memcpy(&BOM,&(pOutputTags[i]),2);i+=2;
	if(BOM==UNICODE_BOM_DIRECT){
		for(j=i;j<i+packer.dword-3;j+=2){
			ReverseWCHAR((WCHAR*)&(pOutputTags[j]));
		}
	}
	StringCbCopyN(out->year,sizeof(out->year),
		(WCHAR*)&(pOutputTags[i]),packer.dword-3);
	i+=packer.dword-3;
	continue;
	}
}
if(strncmp((char*)fh.ID,"TPE1",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i++;
	MultiByteToWideChar(CP_ACP,0,&(pOutputTags[i]),packer.dword-1,out->artist,512);
	i+=packer.dword-1;continue;
	}
	else{
	i++;
	memcpy(&BOM,&(pOutputTags[i]),2);i+=2;
	if(BOM==UNICODE_BOM_DIRECT){
		for(j=i;j<i+packer.dword-3;j+=2){
			ReverseWCHAR((WCHAR*)&(pOutputTags[j]));
		}
	}
	StringCbCopyN(out->artist,sizeof(out->artist),
		(WCHAR*)&(pOutputTags[i]),packer.dword-3);
	i+=packer.dword-3;
	continue;
	}
}
if(strncmp((char*)fh.ID,"TIT2",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i++;
	MultiByteToWideChar(CP_ACP,0,&(pOutputTags[i]),packer.dword-1,out->title,512);
	i+=packer.dword-1;continue;
	}
	else{
	i++;
	memcpy(&BOM,&(pOutputTags[i]),2);i+=2;
	if(BOM==UNICODE_BOM_DIRECT){
		for(j=i;j<i+packer.dword-3;j+=2){
			ReverseWCHAR((WCHAR*)&(pOutputTags[j]));
		}
	}
	StringCbCopyN(out->title,sizeof(out->title),
		(WCHAR*)&(pOutputTags[i]),packer.dword-3);
	i+=packer.dword-3;
	continue;
	}
}
if(strncmp((char*)fh.ID,"TLEN",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i++;
	StringCchCopyNA(buf,1024,&(pOutputTags[i]),packer.dword-1);
	out->length=0;
	sscanf(buf,"%u",&(out->length));
	i+=packer.dword-1;continue;
	}
	i+=packer.dword;continue;
}


if(strncmp((char*)fh.ID,"COMM",4)==0){
	if(pOutputTags[i]==ID32_ENCODING_ISO){		
	i+=4;
	memset(buf,0,sizeof(buf));
	if((packer.dword-4)>1023){memcpy(buf,&(pOutputTags[i]),1023);buf[1023]=0;}
	else{memcpy(buf,&(pOutputTags[i]),packer.dword-4);buf[packer.dword-4]=0;}
	for(j=0;j<1023;j++){
		if(buf[j]==0){buf[j]='\n';break;}
		}
	MultiByteToWideChar(CP_ACP,0,buf,1024,out->comments,1024);
	i+=packer.dword-4;continue;
	}
	i+=packer.dword;continue;
}

i+=packer.dword;
}

HeapFree(GetProcessHeap(),0,pOutputTags);
CloseHandle(hFile);
out->type=TAG_ID3V2;
//fwprintf(f,L"end read tags, file: %s\n",fname);
return TRUE;
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

void InitImageList(){
	SHFILEINFO si={0};
	DWORD_PTR res;
	TCHAR dir[MAX_PATH]=L"c:\\windows";
	TCHAR ext[MAX_PATH]=L"c:\\x.mp3";
	int i;
/*GetWindowsDirectory(dir,MAX_PATH);
res=SHGetFileInfo(dir,0,&si,sizeof(SHFILEINFO),SHGFI_ICON|SHGFI_SMALLICON);
if(res==FALSE)hDirIcon=LoadIcon(NULL,MAKEINTRESOURCE(IDI_APPLICATION));
else hDirIcon=si.hIcon;*/

ImageList=ImageList_Create(16, 16, ILC_MASK| ILC_COLOR32, COUNT_TYPE_ICONS+1, 30);
for(i=0;i<COUNT_TYPE_ICONS;i++){
	StringCchPrintf(ext,MAX_PATH,L"c:\\x.%s",arrTypeIcons[i].ext);
res=SHGetFileInfo(ext,FILE_ATTRIBUTE_NORMAL,&si,sizeof(SHFILEINFO),SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_SMALLICON);
if(res==FALSE)arrTypeIcons[i].hIcon=LoadIcon(NULL,MAKEINTRESOURCE(IDI_APPLICATION));
else arrTypeIcons[i].hIcon=si.hIcon;
arrTypeIcons[i].iIcon=ImageList_AddIcon(ImageList, arrTypeIcons[i].hIcon);
}
//indexDirIcon=ImageList_AddIcon(ImageList, hDirIcon); 

ListView_SetImageList(PlayList,ImageList,LVSIL_SMALL);
}

int GetTypeIcon(TCHAR* ext){
int i;
for(i=0;i<COUNT_TYPE_ICONS;i++){
if(lstrcmpi(ext,arrTypeIcons[i].ext)==0)return i;
}
return 0;
}











