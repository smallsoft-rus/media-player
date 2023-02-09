/* Small Media Player 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
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
#define CP_ISO88591_LATIN 28591

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
    BYTE size[4];
    BYTE flag_bytes;
}ID3V24_EXTHEADER;

typedef struct {
    BYTE ID[4];
    DWORD size;
    BYTE flags[2];
}ID32_FRAME_HEADER;

#define ID32_FRAME_COMPRESSED 0x0080
#define ID32_FRAME_ENCRYPTED 0x0040
#define ID32_ENCODING_ISO 0x00
#define ID32_ENCODING_UTF16 0x01
#define ID32_ENCODING_UTF8 0x03
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
BOOL ReadTagsV2A(char* file,TAGS_GENERIC* out){
    TCHAR fname[MAX_PATH]=L"";
    if(MultiByteToWideChar(CP_UTF8,0,file,MAX_PATH,fname,MAX_PATH)!=0){
        return ReadTagsV2(fname,out,FALSE);
    }
    else {
        return FALSE;
    }
}

typedef struct tagID3V22_FRAME_HEADER_RAW{
    char id[3];
    char size[3];
} ID3V22_FRAME_HEADER_RAW;

typedef struct tagID3V22_FRAME_HEADER{
    char id[3];
    DWORD size;
} ID3V22_FRAME_HEADER;

int ReadV22Frame(char* pInputData, DWORD sizeInputData, ID3V22_FRAME_HEADER* pOutputHeader, char* pOutputData, DWORD sizeOutputData){

    if(sizeInputData<sizeof(ID3V22_FRAME_HEADER_RAW)) return 0;
    
    ID3V22_FRAME_HEADER_RAW header={0};
    DWORD_UNION extractor={0};

    //read frame header
    memcpy(&header, pInputData, sizeof(ID3V22_FRAME_HEADER_RAW));

    if(header.id[0] == 0) return 0;

    extractor.bytes[0] = header.size[2];
    extractor.bytes[1] = header.size[1];
    extractor.bytes[2] = header.size[0];
    DWORD size = extractor.dword;

    if(size == 0 || size + sizeof(ID3V22_FRAME_HEADER_RAW) > sizeInputData) return 0;

    //read frame data
    if(size <= sizeOutputData) memcpy(pOutputData, pInputData + sizeof(ID3V22_FRAME_HEADER_RAW), size);
    else StringCchCopyA(pOutputData, sizeOutputData, "");

    pOutputHeader->id[0] = header.id[0];
    pOutputHeader->id[1] = header.id[1];
    pOutputHeader->id[2] = header.id[2];
    pOutputHeader->size = size;
    return size + sizeof(ID3V22_FRAME_HEADER_RAW);
}

//Reads ID3V2 Tags string with the specified encoding
BOOL ReadID3V2StringImpl(char* pInputData, int sizeInputData, BYTE encoding, WCHAR* pOutput, int cchOutput){

    if(sizeInputData<1) return FALSE;

    //ANSI
    if(encoding==ID32_ENCODING_ISO){
        int bytesConverted = MultiByteToWideChar(CP_ACP,0,&(pInputData[0]),sizeInputData, pOutput, cchOutput);
        
        if(bytesConverted>0) return TRUE;
        else return FALSE;
    }

    //UTF-8
    if(encoding==ID32_ENCODING_UTF8){
        int bytesConverted = MultiByteToWideChar(CP_UTF8,0,&(pInputData[0]),sizeInputData, pOutput, cchOutput);
        
        if(bytesConverted>0) return TRUE;
        else return FALSE;
    }
	
    //UTF-16
    if(sizeInputData<2) return FALSE;
    WORD BOM=0;
    memcpy(&BOM,&(pInputData[0]),2);
    
    if(BOM==UNICODE_BOM_DIRECT){
        for(int j=2;j<sizeInputData;j+=2){
            ReverseWCHAR((WCHAR*)&(pInputData[j]));
        }
    }

    StringCbCopyN(pOutput, cchOutput * sizeof(WCHAR), (WCHAR*)&(pInputData[2]), sizeInputData-2);
    return TRUE;
}

// Reads ID3V2 Tags text field
BOOL ReadID3V2Text(char* pInputData, int sizeInputData, WCHAR* pOutput, int cchOutput){

    if(sizeInputData<1) return FALSE;

    BYTE encoding = pInputData[0];
    return ReadID3V2StringImpl(&(pInputData[1]), sizeInputData-1, encoding, pOutput, cchOutput);
}

//Returns the position after the next null terminator character in the string starting from the specified start index.
//If the null terminator is not found, returns the string size.
int SkipUntilNullTerminator(char* pData, int startIndex, int size, BYTE encoding){
    int pos=startIndex;
    char ch;

    if(encoding == ID32_ENCODING_ISO || encoding == ID32_ENCODING_UTF8){
        while(1){
            if(pos>=size)break;
            ch=pData[pos];
            pos++;
            if(ch==0) break;
        }
    }
    else{
        while(1){
            if(pos>=size-1) return size;
            ch=pData[pos];
            char ch_next = pData[pos+1];
            pos++;

            if(ch==0 && ch_next==0) {
                pos++;
                break;
            }
        }
    }

    return pos;
}

// Reads ID3V2 Tags embedded picture
BOOL ReadID3V2Picture(char* pInputData, int sizeInputData, IMAGE_DATA* pOutput){

    if(sizeInputData<5) return FALSE;

    int pos=0;
    char mime[20]="";
    char ch;
    BYTE encoding = (BYTE)pInputData[pos];
    pos++;

    for(int i=0;i<sizeInputData;i++){
        ch=pInputData[pos];
        if(i<sizeof(mime)-1) mime[i]=ch;
        pos++;
        if(ch==0) break;
    }

    BYTE pictype = (BYTE)pInputData[pos];
    pos++;

    if(pos>=sizeInputData) return FALSE;

    //skip description
    pos=SkipUntilNullTerminator(pInputData,pos,sizeInputData,encoding);

    int pic_size = sizeInputData-pos;

    if(pic_size <= 5) return FALSE; //too small to contain valid picture
    else if(pic_size > 150 * 1024 * 1024) return FALSE; //too large!
    
    //read image
    BYTE* pData = (BYTE*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,pic_size);

    if(pData == nullptr) return FALSE;

    memcpy(pData, &(pInputData[pos]), pic_size);
    pOutput->pic_type = pictype;
    pOutput->size = pic_size;
    pOutput->pData = pData;
    StringCchCopyA(pOutput->mime_type, sizeof(pOutput->mime_type), mime);
    return TRUE;
}

//Read ID3v2 User-defined URL tag (WXXX)
BOOL ReadID3V2Url(char* pInputData, int sizeInputData, WCHAR* pOutput, int cchOutput){

    if(sizeInputData<3) return FALSE;

    int pos=0;
    BYTE encoding = (BYTE)pInputData[pos];
    pos++;

    //skip URL description
    pos = SkipUntilNullTerminator(pInputData, pos, sizeInputData, encoding);
    int urlLength = sizeInputData - pos;
    
    //read URL
    if(urlLength<=5) return FALSE;
    
    int bytesConverted = MultiByteToWideChar(CP_ISO88591_LATIN,0,&(pInputData[pos]),urlLength, pOutput, cchOutput);

    if(bytesConverted>0) return TRUE;
    else return FALSE;
}

//Read ID3v2 comment tag (COMM)
BOOL ReadID3V2Comments(char* pInputData, int sizeInputData, WCHAR* pOutput, int cchOutput){

    if(sizeInputData<=5) return FALSE;

    int pos=0;
    WCHAR buf[2048]=L"";
    BOOL res = FALSE;
    BYTE encoding = (BYTE)pInputData[pos];
    pos++;

    //skip language code
    pos+=3;
    
    int pos_full = SkipUntilNullTerminator(pInputData,pos,sizeInputData,encoding);
    int len_short = pos_full - pos - 1;
    int len_full = sizeInputData - pos_full;
    StringCchCopy(pOutput, cchOutput, L"");
    
    //read short description
    if(len_short>0 && len_short<sizeInputData){
        res = ReadID3V2StringImpl(&(pInputData[pos]), len_short, encoding, buf, sizeof(buf)/sizeof(WCHAR));
    }

    if(res!=FALSE && lstrlen(buf)>0) {
        StringCchCat(pOutput, cchOutput, buf);
        StringCchCat(pOutput, cchOutput, L"\n");
    }

    //read full description
    res = FALSE;
    ZeroMemory(buf, sizeof(buf));

    if(len_full>0 && len_full<sizeInputData){
        res = ReadID3V2StringImpl(&(pInputData[pos_full]), len_full, encoding, buf, sizeof(buf)/sizeof(WCHAR));
    }

    if(res!=FALSE && lstrlen(buf)>0) {
        StringCchCat(pOutput, cchOutput, buf);
    }
    
    return TRUE;
}

BOOL ReadTagsV22(char* pInputData, int sizeInputData,TAGS_GENERIC* pOutput){
    //ID3v2.2
    //https://mutagen-specs.readthedocs.io/en/latest/id3/id3v2.2.html
    int i=0;
    int bytesRead;
    ID3V22_FRAME_HEADER header;
    char buf[2048]="";

    while(1){

        if(i>=sizeInputData) break;

        ZeroMemory(buf, sizeof(buf));
        bytesRead = ReadV22Frame(pInputData+i, sizeInputData-i, &header, buf, sizeof(buf));

        if(bytesRead == 0) break;

        // Read tag content
        if(strncmp(header.id,"TT2",3)==0){ //title
            ReadID3V2Text(buf, header.size, pOutput->title, sizeof(pOutput->title) / sizeof(WCHAR));
        }
        else if(strncmp(header.id,"TAL",3)==0){ //album
            ReadID3V2Text(buf, header.size, pOutput->album, sizeof(pOutput->album) / sizeof(WCHAR));
        }
        else if(strncmp(header.id,"TP1",3)==0){ //artist
            ReadID3V2Text(buf, header.size, pOutput->artist, sizeof(pOutput->artist) / sizeof(WCHAR));
        }
        else if(strncmp(header.id,"TCM",3)==0){ //composer
            ReadID3V2Text(buf, header.size, pOutput->composer, sizeof(pOutput->composer) / sizeof(WCHAR));
        }
        else if(strncmp(header.id,"TYE",3)==0){ //year
            ReadID3V2Text(buf, header.size, pOutput->year, sizeof(pOutput->year) / sizeof(WCHAR));
        }
        
        i+=bytesRead;
    }//end while

    if(i>=6) return TRUE;
    else return FALSE;
}

DWORD ReadSyncsafeInteger(BYTE arr[]){
    //Read ID3v2 syncsafe integer from bytes
    //https://github.com/id3/ID3v2.4/blob/master/id3v2.40-structure.txt#L610
    DWORD_UNION extractor={0};
    DWORD_UNION packer={0};

    extractor.bytes[0]=arr[0];
    extractor.bytes[1]=arr[1];
    extractor.bytes[2]=arr[2];
    extractor.bytes[3]=arr[3];

    packer.bfe.byte1=extractor.bf.byte4;
    packer.bfe.byte2=extractor.bf.byte3;
    packer.bfe.byte3=extractor.bf.byte2;
    packer.bfe.byte4=extractor.bf.byte1;
    packer.bfe.dummy=0;

    return packer.dword;
}

//Read ID3v2 tags (UTF16 file path).
//When readCover parameter is TRUE, the function tries to load embedded cover from APIC tag. In this case the output
//structure should be freed after use by calling TagsFree.
BOOL ReadTagsV2(WCHAR* fname,TAGS_GENERIC* out, BOOL readCover){

    HANDLE hFile=0;
    DWORD dwCount=0;
    BOOL res=0;
    BOOL unsync=FALSE;
    ID32_HEADER header={0};
    DWORD_UNION extractor={0};
    DWORD_UNION packer={0};
    ID32_EXTHEADER* pextheader=0;
    ID32_FRAME_HEADER fh={0};
    ID3V24_EXTHEADER extheader4={0};
    char* pSourceTags=NULL;DWORD SourceSize=0;
    char* pOutputTags=NULL;DWORD OutputRealSize=0;
    char buf[1024]="";
    TCHAR wbuf[1024]=L"";
    WORD BOM=0;

    DWORD i=0,j=0;

    memset(&extractor,0,sizeof(DWORD));memset(&packer,0,sizeof(DWORD));
    TagsFree(out);

    hFile=CreateFile(fname,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(hFile==INVALID_HANDLE_VALUE) return FALSE;

    res=ReadFile(hFile,&header,sizeof(ID32_HEADER),&dwCount,NULL);
    if(res==FALSE){CloseHandle(hFile);return FALSE;}

    if((header.sig[0]=='I'&&header.sig[1]=='D'&&header.sig[2]=='3')==FALSE){
        CloseHandle(hFile);
        return FALSE;
    }

    if(header.ver[0]>0x04){CloseHandle(hFile);return FALSE;}
    
    //raw tags size
    SourceSize = ReadSyncsafeInteger(header.size);

    if(SourceSize<=3){
        //tags are empty or too small to contain useful data
        CloseHandle(hFile);
        return FALSE;
    }
    else if(SourceSize>200*1024*1024){
        //too big size - exit to prevent running out of memory
        CloseHandle(hFile);
        return FALSE;
    }

    if((header.flags&ID32F_SYNC)>0 && header.ver[0]>=4){
        //unsyncronization is not implemented for ID3v2.4
        CloseHandle(hFile);
        return FALSE;
    }

    pSourceTags=(char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,SourceSize);

    if(pSourceTags==NULL){CloseHandle(hFile);return FALSE;}

    ReadFile(hFile,pSourceTags,SourceSize,&dwCount,NULL);
    pOutputTags=(char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,SourceSize);

    if(pOutputTags==NULL){
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(),0,pSourceTags);
        return FALSE;
    }

    j=0;

    //unsyncronization
    for(i=0;i<=SourceSize-3;i++){
        if((header.flags&ID32F_SYNC)>0){
            if(pSourceTags[i]==0xFF&&pSourceTags[i+1]==0&&((pSourceTags[i+2]&ID32_SYNC_MASK)>0)){
                pOutputTags[j]=pSourceTags[i];
                pOutputTags[j+1]=pSourceTags[i+2];
                i+=2;
                j+=2;
                continue;
            }

            if(pSourceTags[i]==0xFF&&pSourceTags[i+1]==0&&pSourceTags[i+2]==0){
                pOutputTags[j]=(char)0xFF;
                pOutputTags[j+1]=0;
                i+=2;
                j+=2;
                continue;
            }
        }
        pOutputTags[j]=pSourceTags[i];j++;
    }

    pOutputTags[j]=pSourceTags[i];j++;i++;
    pOutputTags[j]=pSourceTags[i];j++;i++;

    OutputRealSize=j;
    HeapFree(GetProcessHeap(),0,pSourceTags);
    pSourceTags=NULL;

    if(header.ver[0] <= 2){ //ID3v2.2
        ReadTagsV22(pOutputTags, OutputRealSize, out);
        goto exit;
    }

    //ID3v2.3 or ID3v2.4
    i=0;

    if((header.flags&ID32F_EXTHEADER)!=0){

        if(header.ver[0]>=4){
            memcpy(&extheader4, &(pOutputTags[i]), sizeof(ID3V24_EXTHEADER));
            DWORD extheader4_size = ReadSyncsafeInteger(extheader4.size);

            if(extheader4_size>=OutputRealSize) goto exit;

            if(extheader4_size<sizeof(ID3V24_EXTHEADER)) extheader4_size=sizeof(ID3V24_EXTHEADER);

            i+=extheader4_size;
        }
        else{
            pextheader=(ID32_EXTHEADER*)&(pOutputTags[i]);
            extractor.dword=pextheader->size;
            packer.bytes[0]=extractor.bytes[3];
            packer.bytes[1]=extractor.bytes[2];
            packer.bytes[2]=extractor.bytes[1];
            packer.bytes[3]=extractor.bytes[0];

            if(packer.dword>=20) packer.dword=10;

            i+=(4+packer.dword);
        }
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
        ReadID3V2Text(&(pOutputTags[i]), packer.dword, out->album, sizeof(out->album) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"TCOM",4)==0){
        ReadID3V2Text(&(pOutputTags[i]), packer.dword, out->composer, sizeof(out->composer) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"TYER",4)==0){
        ReadID3V2Text(&(pOutputTags[i]), packer.dword, out->year, sizeof(out->year) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"TPE1",4)==0){
        ReadID3V2Text(&(pOutputTags[i]), packer.dword, out->artist, sizeof(out->artist) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"TIT2",4)==0){
        ReadID3V2Text(&(pOutputTags[i]), packer.dword, out->title, sizeof(out->title) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"TDRC",4)==0){ //recording time
        StringCchCopy(wbuf, sizeof(wbuf) / sizeof(WCHAR), L"");
        res = ReadID3V2Text(&(pOutputTags[i]), packer.dword, wbuf, sizeof(wbuf) / sizeof(WCHAR));

        //year is first 4 chars
        if(res != FALSE && lstrlen(wbuf)>=4){
            StringCchCopyN(out->year, sizeof(out->year) / sizeof(WCHAR), wbuf, 4);
        }

        i+=packer.dword;
        continue;
    }

    if(readCover!=FALSE && strncmp((char*)fh.ID,"APIC",4)==0 && out->cover.pData == nullptr){ //embedded picture
        IMAGE_DATA img={0};
        res = ReadID3V2Picture(&(pOutputTags[i]), packer.dword, &img);
        
        if(res != FALSE) {
            out->cover = img;
        }

        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"WXXX",4)==0){ //User-defined URL
        ReadID3V2Url(&(pOutputTags[i]), packer.dword, out->URL, sizeof(out->URL) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(fh.ID[0]=='W' && fh.ID[1]!='X'){ //Predefined URL
        ReadID3V2StringImpl(&(pOutputTags[i]), packer.dword, ID32_ENCODING_ISO, out->URL, sizeof(out->URL) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"COMM",4)==0){ //Comment
        ReadID3V2Comments(&(pOutputTags[i]), packer.dword, out->comments, sizeof(out->comments) / sizeof(WCHAR));
        i+=packer.dword;
        continue;
    }

    if(strncmp((char*)fh.ID,"TLEN",4)==0){ //Duration
        if(pOutputTags[i]==ID32_ENCODING_ISO){
            i++;
            StringCchCopyNA(buf,1024,&(pOutputTags[i]),packer.dword-1);
            out->length=0;
            sscanf(buf,"%u",&(out->length));
            i+=packer.dword-1;continue;
        }
	    i+=packer.dword;continue;
    }

    i+=packer.dword;
}

exit:
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

for(i=0;i<(int)n;i++){
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

