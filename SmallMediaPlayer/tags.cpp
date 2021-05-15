/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "tags.h"
#include <strsafe.h>

typedef struct {
    char sig[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    BYTE genre;
} MP3TAG_V1;

typedef union {
	WORD w;
	char b[2];
}WORD_UNION;

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

#define FLAC_SIGNATURE "fLaC"
#define FLAC_LASTBLOCK 0x80
#define FLAC_VORBISCOMMENT 0x04
#define FLAC_VORBISCOMMENT2 0x84

typedef struct {
	BYTE type;
	BYTE length[3];
}FLAC_BLOCK_HEADER;

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

//ID3v1 tags reading implementation

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

void ReverseWCHAR(WCHAR* c){
	WORD_UNION extractor={0};
	WORD_UNION packer={0};
	extractor.w=(WORD)*c;
	packer.b[0]=extractor.b[1];
	packer.b[1]=extractor.b[0];
	*c=(WCHAR)packer.w;
}

//ID3v2 tags reading implementation

//read ID3v2 tags (UTF8 file path)
BOOL ReadTagsv2A(char* file,TAGS_GENERIC* out){
    TCHAR fname[MAX_PATH]=L"";
    if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0){
        return ReadTagsv2(fname,out);
    }
    else {
        return FALSE;
    }
}

//read ID3v2 tags (UTF16 file path)
BOOL ReadTagsv2(WCHAR* fname,TAGS_GENERIC* out){
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

return TRUE;
}

//FLAC Vorbis comment reading implementation

//Read FLAC Vorbis comment tags (utf8 file path)
BOOL ReadFlacTagsA(char* file,TAGS_GENERIC* out){
    TCHAR fname[MAX_PATH]=L"";
    if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0){
        return ReadFlacTags(fname,out);
    }
    else return FALSE;
}

//Read FLAC Vorbis comment tags (utf16 file path)
BOOL ReadFlacTags(WCHAR* file,TAGS_GENERIC* out){
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

//APE tags reading implementation

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

//Read APE tags (utf8 file path)
BOOL ReadApeTagsA(char* file,TAGS_GENERIC* out){
    TCHAR fname[MAX_PATH]=L"";
    if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0){
        return ReadApeTags(fname,out);
    }
    else return FALSE;
}

//Read APE tags (utf16 file path)
BOOL ReadApeTags(WCHAR* file,TAGS_GENERIC* out){
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

