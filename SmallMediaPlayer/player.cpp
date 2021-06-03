/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "player.h"
#include "tags.h"
#include "resource.h"
#include "errors.h"
#include <math.h>

extern SMPSETTINGS Settings;
extern TAGS_GENERIC OpenedFileTags;
extern bool fOpenedFileTags;

HWND hWnd;//for notify

PLAYER_STATE PlayerState=FILE_NOT_LOADED;
bool IsPlayingCDA=false;
DWORD DeviceID=0;
UINT TrackNumber=0;
DWORD pos;
long Volume=0;
long VolumeX=0;
bool IsPlayingVideo=false;

bool fShowNextImage=false;
bool FullScreen=false;

static IGraphBuilder *pGraph = NULL;
static IMediaControl *pControl = NULL;
static IMediaSeeking* pSeek =NULL;
 IMediaEventEx   *pEvent = NULL;

static IBasicAudio* pAudio=NULL;
static IVideoWindow* pVideoWindow=NULL;
static IBasicVideo* pVideo=NULL;

IBaseFilter* pSource=NULL;
IBaseFilter* pSplitter=NULL;
IBaseFilter* pAudioDecoder=NULL;
IBaseFilter* pAudioRenderer=NULL;
IBaseFilter* pVideoDecoder=NULL;
IBaseFilter* pVideoRenderer=NULL;
static bool fUseSplitter=false;
GUID guidStreamSubType=GUID_NULL;

void GetFileExtension(TCHAR* fname,TCHAR* ext){
int i=0,c=0;
TCHAR* s=NULL;
c=lstrlen(fname);
for(i=c-1;i>c-8;i--){
	if(fname[i]==L'.'||fname[i]==L'\\'||fname[i]==L'/'){s=&(fname[i+1]);break;}

}
if(s==NULL){lstrcpy(ext,L"");return;}
lstrcpy(ext,s);
}

void GetFileExtensionA(char* fname,char* ext){
int i=0,c=0;
char* s=NULL;
c=lstrlenA(fname);
for(i=c-1;i>c-8;i--){
	if(fname[i]=='.'||fname[i]=='\\'||fname[i]=='/'){s=&(fname[i+1]);break;}

}
if(s==NULL){lstrcpyA(ext,"");return;}
lstrcpyA(ext,s);
}

BOOL IsURL(TCHAR* str){
int i;
int c;
c=lstrlen(str)-4;
for(i=0;i<c;i++){
if(str[i]==0)return FALSE;
if(str[i]==L':'&&str[i+1]==L'/'&&str[i+2]==L'/')return TRUE;

}
return FALSE;
}

DWORD PlayCDATrack(TCHAR* fname){

TCHAR chr=0;
TCHAR *p=NULL;
DWORD len;
DWORD min,sec;
int i;
UINT n=1;
int res;
DWORD dwReturn;
TCHAR buf[256];
TCHAR str[256];
	MCI_OPEN_PARMS mciOpenParms={0};
	MCI_PLAY_PARMS mciPlayParms={0};
	MCI_SET_PARMS mciSetParms={0};

p=GetShortName(fname);

	res=swscanf(p,L"Track%u.cda",&n);
if(res!=1){swscanf(p,L"TRACK%u.CDA",&n);}
TrackNumber=n;

if(PlayerState==PLAYING||PlayerState==PAUSED)Stop();
	if(PlayerState!=FILE_NOT_LOADED)Close();
    // Open the device by specifying  filename.
    // MCI will choose a device capable of playing the specified file.

    mciOpenParms.lpstrElementName = fname;
	
	if (dwReturn = mciSendCommand(0, MCI_OPEN,
        MCI_OPEN_ELEMENT, 
       (DWORD)(LPVOID) &mciOpenParms))
    {
        // Failed to open device. Don't close it; just return error.
		HandleError(L"Не удалось открыть устройство AudioCD",SMP_ALERT_BLOCKING,L"");
		return dwReturn;
	       
    }
	// The device opened successfully; get the device ID.
    DeviceID = mciOpenParms.wDeviceID;
	
	PlayerState=STOPPED;
	IsPlayingCDA=true;
	mciSetParms.dwTimeFormat=MCI_FORMAT_TMSF;
	dwReturn=mciSendCommand( DeviceID,MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mciSetParms);

	if(dwReturn!=0)HandleError(L"Невозможно установить формат времени",SMP_ALERT_BLOCKING,L"");

    // Begin playback. The window procedure function for the parent 
    // window will be notified with an MM_MCINOTIFY message when 
    // playback is complete. At this time, the window procedure closes 
    // the device.
len=GetLength();
sec=len/1000;
min=sec/60;
sec=sec-min*60;

    mciPlayParms.dwCallback = (DWORD) hWnd;
	mciPlayParms.dwFrom = MCI_MAKE_TMSF(n, 0, 0, 0);
    mciPlayParms.dwTo = MCI_MAKE_TMSF(n , min, sec, 0);

    if (dwReturn = mciSendCommand(DeviceID, MCI_PLAY, MCI_FROM | MCI_TO |MCI_NOTIFY, 
        (DWORD)(LPVOID) &mciPlayParms))
    {
        Close();
		HandleError(L"Ошибка воспроизведения AudioCD",SMP_ALERT_BLOCKING,L"");
		PlayerState=FILE_NOT_LOADED;
		IsPlayingCDA=false;
        return (dwReturn);
    }
	PlayerState=PLAYING;

return dwReturn;
}


BOOL InsertSplitter(TCHAR* file,const SPLITTER_DATA* sd){
IPin* pin1=NULL;
IPin* pin2=NULL;
HRESULT hr;
BOOL res;
IFileSourceFilter* fs=NULL;

if(sd->IsSource==true){pSource=FindFilter(sd->FilterName);}
else{	
	pSource=FindFilter(L"File Source (Async.)");
	
	
}
if(pSource==NULL){goto end_fail;}

hr=pGraph->AddFilter(pSource,L"source");
if(FAILED(hr)){goto end_fail;}
hr=pSource->QueryInterface(IID_IFileSourceFilter,(void**)&fs);
if(FAILED(hr)){goto end_fail;}
hr=fs->Load(file,NULL);
fs->Release();
if(FAILED(hr)){goto end_fail;}

if(sd->IsSource==false){
	fUseSplitter=true;
	hr=GetUnconnectedPin(pSource,PINDIR_OUTPUT,&pin1);
	if(FAILED(hr)){goto end_fail;}
	pSplitter=FindFilter(sd->FilterName);
	if(pSplitter==NULL){goto end_fail;}
	hr=pGraph->AddFilter(pSplitter,L"splitter");
if(FAILED(hr)){goto end_fail;}
	hr=GetUnconnectedPin(pSplitter,PINDIR_INPUT,&pin2);
	if(FAILED(hr)){goto end_fail;}
	hr=pGraph->ConnectDirect(pin1,pin2,NULL);
	if(FAILED(hr)){goto end_fail;}

}
else{
fUseSplitter=false;
}

if(pin1!=NULL)pin1->Release();
if(pin2!=NULL)pin2->Release();

return TRUE;

end_fail:

if(pin1!=NULL)pin1->Release();
if(pin2!=NULL)pin2->Release();
if(pSplitter!=NULL){pGraph->RemoveFilter(pSplitter);pSplitter->Release();pSplitter=NULL;}
if(pSource!=NULL){pGraph->RemoveFilter(pSource);pSource->Release();pSource=NULL;}
return FALSE;

}

BOOL InsertAudioDecoder(IPin* pin,TCHAR* lpFilterName){
HRESULT hr;
IBaseFilter* pf=NULL;
IPin* pin2=NULL;
IPin* pins[10];
IEnumPins* pEnum=NULL;
ULONG c,i;
PIN_DIRECTION PinDir;

pf=FindFilter(lpFilterName);
if(pf==NULL)goto end_fail;
hr=pGraph->AddFilter(pf,L"audiodecoder");
if(FAILED(hr))goto end_fail;
hr=GetUnconnectedPin(pf,PINDIR_INPUT,&pin2);
if(FAILED(hr))goto end_fail;
hr=pGraph->ConnectDirect(pin,pin2,NULL);
if(FAILED(hr))goto end_fail;
pin2->Release();pin2=NULL;
hr=pf->EnumPins(&pEnum);
if(FAILED(hr))goto end_fail;
hr=pEnum->Next(10,pins,&c);
if(FAILED(hr))c=0;
for(i=0;i<c;i++){
	pins[i]->QueryDirection(&PinDir);
	if(PinDir==PINDIR_OUTPUT)pGraph->Render(pins[i]);
	pins[i]->Release();
}

if(pEnum!=NULL){pEnum->Release();}

pAudioDecoder=pf;

return TRUE;
end_fail:
if(pf!=NULL){pGraph->RemoveFilter(pf),pf->Release();}
if(pin2!=NULL){pin2->Release();}
if(pEnum!=NULL){pEnum->Release();}
return FALSE;

}

BOOL InsertVideoDecoder(IPin* pin,TCHAR* lpFilterName,TCHAR* lpPinName){
HRESULT hr;
IBaseFilter* pf=NULL;
IPin* pin2=NULL;
IPin* pins[10];
IEnumPins* pEnum=NULL;
ULONG c,i;
PIN_DIRECTION PinDir;

pf=FindFilter(lpFilterName);
if(pf==NULL)goto end_fail;
hr=pGraph->AddFilter(pf,L"videodecoder");
if(FAILED(hr))goto end_fail;
hr=FindPin(pf,lpPinName,&pin2);

if(hr==E_FAIL){goto end_fail;}
hr=pGraph->Connect(pin,pin2);
if(FAILED(hr)){goto end_fail;}
pin2->Release();pin2=NULL;
hr=pf->EnumPins(&pEnum);
if(FAILED(hr))goto end_fail;
hr=pEnum->Next(10,pins,&c);
if(FAILED(hr))c=0;
for(i=0;i<c;i++){
	pins[i]->QueryDirection(&PinDir);
	if(PinDir==PINDIR_OUTPUT)pGraph->Render(pins[i]);
	pins[i]->Release();
}

if(pEnum!=NULL){pEnum->Release();}
if(pin2!=NULL){pin2->Release();}
pVideoDecoder=pf;


return TRUE;
end_fail:
if(pEnum!=NULL){pEnum->Release();}
if(pf!=NULL){pGraph->RemoveFilter(pf),pf->Release();}
if(pin2!=NULL){pin2->Release();}
return FALSE;

}

BOOL pin_GetAudioInfo(IPin* pin,SMP_AUDIOINFO* pAudioInfo){
IEnumMediaTypes * pEnum=NULL;
AM_MEDIA_TYPE* amt=NULL;
WAVEFORMATEX* wf=NULL;
HRESULT hr;
ULONG res=0;

hr=pin->EnumMediaTypes(&pEnum);
if(FAILED(hr))return FALSE;
hr=pEnum->Next(1,&amt,&res);
if(FAILED(hr)||res==0){pEnum->Release();return FALSE;}
if(amt->formattype!=FORMAT_WaveFormatEx){pEnum->Release();return FALSE;}
wf=(WAVEFORMATEX*)amt->pbFormat;
pAudioInfo->chans=wf->nChannels;
pAudioInfo->BitsPerSample=wf->wBitsPerSample;
pAudioInfo->BitsPerSecond=wf->nAvgBytesPerSec*8;
pAudioInfo->nFreq=wf->nSamplesPerSec;
pAudioInfo->wFormatTag=wf->wFormatTag;
if(amt->subtype==MEDIASUBTYPE_DOLBY_AC3){
	pAudioInfo->wFormatTag=AUDIO_AC3;}
if(amt->subtype==MEDIASUBTYPE_MPEG2_AUDIO){
	pAudioInfo->wFormatTag=AUDIO_MPEG2AAC;}
pEnum->Release();
MyDeleteMediaType(amt);
return TRUE;

}

BOOL pin_GetVideoInfo(IPin* pin,SMP_VIDEOINFO* pVideoInfo){
IEnumMediaTypes * pEnum=NULL;
AM_MEDIA_TYPE* amt=NULL;
BITMAPINFOHEADER* bi=NULL;
REFERENCE_TIME time=0;
double t=0;
HRESULT hr;
ULONG res=0;
RECT rcSource={0};

pVideoInfo->VideoType=VIDEOTYPE_VIDEO;
hr=pin->EnumMediaTypes(&pEnum);
if(FAILED(hr))return FALSE;
hr=pEnum->Next(1,&amt,&res);
if(FAILED(hr)||res==0){pEnum->Release();return FALSE;}
if(amt->subtype==MEDIASUBTYPE_MPEG1Packet||amt->subtype==MEDIASUBTYPE_MPEG1Payload||amt->subtype==MEDIASUBTYPE_MPEG1Video){
	pVideoInfo->VideoType=VIDEOTYPE_MPEG1;
}
if(amt->subtype==MEDIASUBTYPE_MPEG2_VIDEO){
	pVideoInfo->VideoType=VIDEOTYPE_MPEG2;
}

if(amt->formattype==FORMAT_VideoInfo){
bi=(BITMAPINFOHEADER*)&(((VIDEOINFOHEADER*)(amt->pbFormat))->bmiHeader);
time=((VIDEOINFOHEADER*)(amt->pbFormat))->AvgTimePerFrame;
rcSource=((VIDEOINFOHEADER*)(amt->pbFormat))->rcSource;
}
if(amt->formattype==FORMAT_VideoInfo2){
bi=(BITMAPINFOHEADER*)&(((VIDEOINFOHEADER2*)(amt->pbFormat))->bmiHeader);
time=((VIDEOINFOHEADER2*)(amt->pbFormat))->AvgTimePerFrame;
rcSource=((VIDEOINFOHEADER2*)(amt->pbFormat))->rcSource;
}
if(amt->formattype==FORMAT_MPEGVideo){
bi=(BITMAPINFOHEADER*)&(((MPEG1VIDEOINFO*)(amt->pbFormat))->hdr.bmiHeader);
time=((MPEG1VIDEOINFO*)(amt->pbFormat))->hdr.AvgTimePerFrame;
rcSource=((MPEG1VIDEOINFO*)(amt->pbFormat))->hdr.rcSource;
}
if(amt->formattype==FORMAT_MPEG2Video){
bi=(BITMAPINFOHEADER*)&(((MPEG2VIDEOINFO*)(amt->pbFormat))->hdr.bmiHeader);
time=((MPEG2VIDEOINFO*)(amt->pbFormat))->hdr.AvgTimePerFrame;
rcSource=((MPEG2VIDEOINFO*)(amt->pbFormat))->hdr.rcSource;
}
if(bi==NULL){pEnum->Release();return FALSE;}
t=time/10000000.0;
pVideoInfo->width=bi->biWidth;
pVideoInfo->height=bi->biHeight;
pVideoInfo->BitsPerPixel=bi->biBitCount;
pVideoInfo->dwVideoCodec=bi->biCompression;
if(t!=0){
	pVideoInfo->FramesPerSecond=1.0/t;}
else{pVideoInfo->FramesPerSecond=0;}
pVideoInfo->rcSource=rcSource;
pEnum->Release();
MyDeleteMediaType(amt);
return TRUE;

}

WORD GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType){
HRESULT hr;
IEnumPins* pEnum=NULL;
IBaseFilter* pf=NULL;
IPin* pins[10];
UINT i;
ULONG c=0;
PIN_DIRECTION PinDir;
bool fAudio=false;
bool fVideo=false;

if(pSplitter!=NULL)pf=pSplitter;
else pf=pSource;
if(pf==NULL)return INFORES_NO;

hr=pf->EnumPins(&pEnum);
if(FAILED(hr))return INFORES_NO;
if(SUCCEEDED(hr))hr=pEnum->Next(10,pins,&c);
if(FAILED(hr))c=0;
for(i=0;i<c;i++){
	pins[i]->QueryDirection(&PinDir);
	if(PinDir==PINDIR_INPUT)continue;
	if(CheckMediaType(pins[i],MEDIATYPE_Audio)!=FALSE){
		if(fAudio==true)continue;
		fAudio=pin_GetAudioInfo(pins[i],pAudioInfo);
		continue;
	}
		
	if(CheckMediaType(pins[i],MEDIATYPE_Video)!=FALSE){
		if(fVideo==true)continue;
		fVideo=pin_GetVideoInfo(pins[i],pVideoInfo);
		continue;}
}
	
for(i=0;i<c;i++){
	pins[i]->Release();
}
pEnum->Release();
*pStreamType=STREAM_UNKNOWN;
if(guidStreamSubType==MEDIASUBTYPE_Avi)*pStreamType=STREAM_AVI;
if(guidStreamSubType==MEDIASUBTYPE_Asf)*pStreamType=STREAM_ASF;
if(guidStreamSubType==MEDIASUBTYPE_MPEG1System||guidStreamSubType==MEDIASUBTYPE_MPEG1Audio
   ||guidStreamSubType==MEDIASUBTYPE_MPEG1Video)*pStreamType=STREAM_MPEG1;
if(guidStreamSubType==MEDIASUBTYPE_MPEG1VideoCD)*pStreamType=STREAM_MPEG1VCD;
if(guidStreamSubType==MEDIASUBTYPE_WAVE)*pStreamType=STREAM_WAVE;
if(guidStreamSubType==MEDIASUBTYPE_MPEG2_PROGRAM||
   guidStreamSubType==MEDIASUBTYPE_MPEG2_TRANSPORT||
   guidStreamSubType==MEDIASUBTYPE_MPEG2_VIDEO
   )*pStreamType=STREAM_MPEG2;
if(guidStreamSubType==MEDIASUBTYPE_QTMovie)*pStreamType=STREAM_QUICKTIME;
if(guidStreamSubType==GUID_NULL)*pStreamType=STREAM_UNKNOWN;

if(fAudio==false){
	if(fVideo==false)return INFORES_NO;
	else return INFORES_VIDEO;
}
else{
	if(fVideo==false)return INFORES_AUDIO;
	else return INFORES_BOTH;
}
return INFORES_BOTH;

}



void SearchFilters(){
HRESULT hr;
ULONG c,i;
IPin* pins[10];
IPin* pin=NULL;
PIN_DIRECTION PinDir;
IEnumPins* pEnum=NULL;


guidStreamSubType=GUID_NULL;
if(pSource==NULL){
pSource=FindFileSource(pGraph);
if(pSource==NULL){return;}
}

//resaecrh sourcefilter pins
hr=pSource->EnumPins(&pEnum);
	if(SUCCEEDED(hr))hr=pEnum->Next(10,pins,&c);
if(FAILED(hr))c=0;
for(i=0;i<c;i++){
	pins[i]->QueryDirection(&PinDir);
	if(PinDir==PINDIR_INPUT)continue;
	if(CheckMediaType(pins[i],MEDIATYPE_Audio)!=FALSE){
		if(pAudioDecoder==NULL)pAudioDecoder=GetDownstreamFilter(pins[i]);		
		continue;
	}
	if(CheckMediaType(pins[i],MEDIATYPE_Video)!=FALSE){
		if(pVideoDecoder==NULL)pVideoDecoder=GetDownstreamFilter(pins[i]);
		continue;}
	if(CheckMediaType(pins[i],MEDIATYPE_Stream)!=FALSE){
		if(pSplitter==NULL){pSplitter=GetDownstreamFilter(pins[i]);}
		continue;}	

}
	
for(i=0;i<c;i++){
	pins[i]->Release();
}
pEnum->Release();
if(pSplitter==NULL)goto check_render;
//ressearch splitter pins

hr=pSplitter->EnumPins(&pEnum);
	if(SUCCEEDED(hr))hr=pEnum->Next(10,pins,&c);
if(FAILED(hr))c=0;
for(i=0;i<c;i++){
	pins[i]->QueryDirection(&PinDir);
	if(PinDir==PINDIR_INPUT)continue;
	if(CheckMediaType(pins[i],MEDIATYPE_Audio)!=FALSE){
		if(pAudioDecoder==NULL)pAudioDecoder=GetDownstreamFilter(pins[i]);		
		continue;
	}
	if(CheckMediaType(pins[i],MEDIATYPE_Video)!=FALSE){
		if(pVideoDecoder==NULL)pVideoDecoder=GetDownstreamFilter(pins[i]);
		continue;}
}	
for(i=0;i<c;i++){
	pins[i]->Release();
}
pEnum->Release();
//get stream subtype
if(pSplitter!=NULL&&pSource!=NULL){
	if(pin!=NULL)pin->Release();
	pin=NULL;
	pin=GetOutputPin(pSource);
	if(pin!=NULL){GetSubType(pin,&guidStreamSubType);}	
	pin->Release();
}

check_render:
IAMAudioRendererStats* pRnd=NULL;
IBasicVideo* pVid=NULL;

if(pAudioDecoder!=NULL){
	hr=pAudioDecoder->QueryInterface(IID_IAMAudioRendererStats,(void**)&pRnd);
	if(hr==S_OK){
		pAudioRenderer=pAudioDecoder;pAudioDecoder=NULL;
		pRnd->Release();
	}
}

if(pVideoDecoder!=NULL){
	hr=pVideoDecoder->QueryInterface(IID_IBasicVideo,(void**)&pVid);
	if(hr==S_OK){
		pVideoRenderer=pVideoDecoder;pVideoDecoder=NULL;
		pVid->Release();
	}
}


if(pAudioDecoder!=NULL){
pin=GetOutputPin(pAudioDecoder);
if(pin!=NULL){
	pAudioRenderer=GetDownstreamFilter(pin);
	pin->Release();pin=NULL;}
}

if(pVideoDecoder!=NULL){
pin=GetOutputPin(pVideoDecoder);
if(pin!=NULL){
	pVideoRenderer=GetDownstreamFilter(pin);
	pin->Release();pin=NULL;}
}

}

BOOL BuildGraph(MEDIATYPE mt,TCHAR* file){
	IPin* pin=NULL;
	IPin* pins[10];
IEnumPins* pEnum=NULL;
ULONG c,i;
HRESULT hr;
BOOL res;
IBaseFilter* pf;
AM_MEDIA_TYPE* amt=NULL;
PIN_DIRECTION PinDir;

switch(mt){
	case MT_AUDIO:
		res=InsertSplitter(file,&sdBassSource);
	if(res==FALSE)res=InsertSplitter(file,&sdAsfReader);	
	if(res==FALSE)res=InsertSplitter(file,&sdGretechMp3);
	if(res==FALSE)res=InsertSplitter(file,&sdNeroSplitter);
	if(res==FALSE)res=InsertSplitter(file,&sdMpeg1Splitter);
	if(res==FALSE)goto end_fail;
	break;
	case MT_AVI:		
res=InsertSplitter(file,&sdAviSplitter);
if(res==FALSE)res=InsertSplitter(file,&sdGretechAvi);
if(res==FALSE)res=InsertSplitter(file,&sdNeroSplitter);
if(res==FALSE)res=InsertSplitter(file,&sdAviSource);
if(res==FALSE)goto end_fail;
break;
	case MT_MPEG:
res=InsertSplitter(file,&sdMpegSource);
if(res==FALSE)res=InsertSplitter(file,&sdMpeg1Splitter);
if(res==FALSE)res=InsertSplitter(file,&sdMpeg2Splitter);
if(res==FALSE)res=InsertSplitter(file,&sdMpegSrcGabest);
if(res==FALSE)res=InsertSplitter(file,&sdNeroSplitter);
if(res==FALSE)goto end_fail;
break;
	case MT_MKV:		
res=InsertSplitter(file,&sdMkvSource);
if(res==FALSE){res=InsertSplitter(file,&sdHaaliSplitter);}
if(res==FALSE){goto end_fail;}
break;

	default:goto end_fail;

}

	if(fUseSplitter==true)pf=pSplitter;
	else pf=pSource;
//connect splitter to decoders
bool fVideoRendered=false;
bool fAudioRendered=false;

	hr=pf->EnumPins(&pEnum);
	if(SUCCEEDED(hr))hr=pEnum->Next(10,pins,&c);
if(FAILED(hr))c=0;
for(i=0;i<c;i++){
	pins[i]->QueryDirection(&PinDir);
	if(PinDir==PINDIR_INPUT)continue;
	if(CheckMediaType(pins[i],MEDIATYPE_Audio)!=FALSE){
		if(fAudioRendered==true)goto next;
		res=InsertAudioDecoder(pins[i],L"ffdshow Audio Decoder");
		
		if(res==FALSE){pGraph->Render(pins[i]);}
		else {fAudioRendered=true;}
		continue;
	}
		
	if(CheckMediaType(pins[i],MEDIATYPE_Video)!=FALSE){
		if(fVideoRendered==true)goto next;
		res=InsertVideoDecoder(pins[i],L"ffdshow Video Decoder",L"In");
		
		if(res==FALSE){pGraph->Render(pins[i]);}
		else {fAudioRendered=true;}
		continue;}
	next:
	pGraph->Render(pins[i]);

}
	
for(i=0;i<c;i++){
	pins[i]->Release();
}
	

	if(pEnum!=NULL){pEnum->Release();pEnum=NULL;}
	if(pin!=NULL){pin->Release();pin=NULL;}
	
	return TRUE;
	
end_fail:
	
if(pSource!=NULL){pGraph->RemoveFilter(pSource);pSource->Release();pSource=NULL;}
if(pSplitter!=NULL){pGraph->RemoveFilter(pSplitter);pSplitter->Release();pSplitter=NULL;}
if(pEnum!=NULL){pEnum->Release();}

	return FALSE;
}

void Close(){

	Stop();
	

	if(IsPlayingCDA==true){
DWORD dwRes;
MCI_GENERIC_PARMS p={0};

if(PlayerState==FILE_NOT_LOADED)return;
dwRes=mciSendCommand(DeviceID,MCI_CLOSE,0,(DWORD)&p);
PlayerState=FILE_NOT_LOADED;
IsPlayingVideo=false;
IsPlayingCDA=false;
DeviceID=0;
return;
	}

	if(pSource!=NULL){pSource->Release();pSource=NULL;}
	if(pSplitter!=NULL){pSplitter->Release();pSplitter=NULL;}
	if(pAudioDecoder!=NULL){pAudioDecoder->Release();pAudioDecoder=NULL;}
	if(pAudioRenderer!=NULL){pAudioRenderer->Release();pAudioRenderer=NULL;}
	if(pVideoDecoder!=NULL){pVideoDecoder->Release();pVideoDecoder=NULL;}
	if(pVideoRenderer!=NULL){pVideoRenderer->Release();pVideoRenderer=NULL;}
	
	if(pControl!=NULL){pControl->Release();pControl=NULL;}
	if(pSeek!=NULL){pSeek->Release();pSeek=NULL;}


	if(pEvent!=NULL){
		pEvent->SetNotifyWindow(NULL, 0, 0);
		pEvent->Release();
		pEvent=NULL;
	}
	if(pAudio!=NULL){pAudio->Release();pAudio=NULL;}	
	if(pVideoWindow!=NULL){pVideoWindow->Release();pVideoWindow=NULL;}
	if(pVideo!=NULL){pVideo->Release();pVideo=NULL;}
	if(pGraph!=NULL){pGraph->Release();pGraph=NULL;}
	
	PlayerState=FILE_NOT_LOADED;
	
}


void PlayFile(TCHAR* filename){
HRESULT hr;
GUID tf=TIME_FORMAT_MEDIA_TIME;
TCHAR ext[8]=L"";
int i;

bool res;
BOOL success=FALSE;


if(PlayerState!=FILE_NOT_LOADED){Close();}
fShowNextImage=false;


GetFileExtension(filename,ext);
if(lstrcmp(ext,L"cda")==0){
	PlayCDATrack(filename);
return;}

// Create the filter graph manager and query for interfaces.
hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IGraphBuilder, (void **)&pGraph);

if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}
hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}
hr = pGraph->QueryInterface(IID_IMediaEventEx, (void **)&pEvent);
if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}
hr = pGraph->QueryInterface(IID_IMediaSeeking, (void **)&pSeek);
if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}
hr = pGraph->QueryInterface(IID_IBasicAudio, (void **)&pAudio);
if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}
hr = pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow);
if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}
hr = pGraph->QueryInterface(IID_IBasicVideo, (void **)&pVideo);
if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return;}

//handle URL
if(IsURL(filename)!=FALSE){
hr = pGraph->RenderFile(filename, NULL);

if(FAILED(hr)){
    HandlePlayError(hr,filename);
    Close();
    return;
}

goto play;
}

//Try dynamic build filter graph
if(lstrcmp(ext,L"mp3")==0||lstrcmp(ext,L"MP3")==0||lstrcmp(ext,L"Mp3")==0){
	success=BuildGraph(MT_AUDIO,filename);
	if(success!=FALSE)goto play;
	
}
if(lstrcmp(ext,L"avi")==0||lstrcmp(ext,L"AVI")==0){
	success=BuildGraph(MT_AVI,filename);
	if(success!=FALSE)goto play;
		}
if(lstrcmp(ext,L"mpg")==0||lstrcmp(ext,L"MPG")==0||lstrcmp(ext,L"MPEG")==0||lstrcmp(ext,L"mpeg")==0){
	success=BuildGraph(MT_MPEG,filename);
	if(success!=FALSE)goto play;
	}
if(lstrcmp(ext,L"mkv")==0||lstrcmp(ext,L"MKV")==0){
	success=BuildGraph(MT_MKV,filename);
	if(success!=FALSE)goto play;
		}

if(lstrcmp(ext,L"jpg")==0||lstrcmp(ext,L"JPG")==0||
   lstrcmp(ext,L"jpeg")==0||lstrcmp(ext,L"JPEG")==0||
   lstrcmp(ext,L"bmp")==0||lstrcmp(ext,L"BMP")==0||
   lstrcmp(ext,L"png")==0||lstrcmp(ext,L"PNG")==0||
   lstrcmp(ext,L"gif")==0||lstrcmp(ext,L"GIF")==0
   ){
		fShowNextImage=true;
	   SetTimer(hVideoWindow,TM_NEXT_IMAGE,Settings.ImageDelay,NULL);
	   
	}


//try auto build
hr = pGraph->RenderFile(filename, NULL);

if(FAILED(hr)){
	HandlePlayError(hr,filename);
	Close();
	WPARAM wParam=MAKEWPARAM(ID_NEXTTRACK,0);
	PostMessage(hWnd,WM_COMMAND,wParam,0);
	return;
}

play:
SearchFilters();
PlayerState=STOPPED;

pSeek->SetTimeFormat(&tf);

res=SetVideoWindow(hVideoWindow);
if(res==false){
	DisableFullScreen();	
	IsPlayingVideo=false;
	ShowWindow(hVideoWindow,SW_HIDE);
	SetThreadExecutionState (ES_CONTINUOUS|ES_SYSTEM_REQUIRED);
}
else {
	IsPlayingVideo=true;
	ShowWindow(hVideoWindow,SW_SHOW);
	SetThreadExecutionState (ES_CONTINUOUS|ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED);
}

hr = pControl->Run();
if(FAILED(hr)){
	HandlePlayError(hr,filename);
	Close();
	WPARAM wParam=MAKEWPARAM(ID_NEXTTRACK,0);
	PostMessage(hWnd,WM_COMMAND,wParam,0);
	return;
}

PlayerState=PLAYING;
pAudio->put_Volume(Volume);

pEvent->SetNotifyWindow((LONG_PTR)hWnd,MM_MCINOTIFY,0L);
}

void PlayCDAFrom(DWORD pos){  //pos in TMSF

DWORD dwReturn;
    MCI_PLAY_PARMS mciPlayParms;
	DWORD len,min,sec;

	if(PlayerState==FILE_NOT_LOADED){
		HandleError(L"Файл не загружен",SMP_ALERT_BLOCKING,L"");
		return;
	}
	
    // Begin playback. The window procedure function for the parent 
    // window will be notified with an MM_MCINOTIFY message when 
    // playback is complete. At this time, the window procedure closes 
    // the device.
len=GetLength();
sec=len/1000;
min=sec/60;
sec=sec-min*60;

    mciPlayParms.dwCallback = (DWORD) hWnd;
mciPlayParms.dwFrom = pos;
    mciPlayParms.dwTo = MCI_MAKE_TMSF(TrackNumber, min, sec, 0);
    if (dwReturn = mciSendCommand(DeviceID, MCI_PLAY,  MCI_FROM|MCI_TO |MCI_NOTIFY, 
        (DWORD)(LPVOID) &mciPlayParms))
    {
        Close();
		PlayerState=FILE_NOT_LOADED;
		IsPlayingCDA=false;
		return ;}
	
PlayerState=PLAYING;
return;

}

void Play(){
HRESULT hr;
DWORD dwReturn;
MCI_PLAY_PARMS mciPlayParms;
DWORD len,min,sec;

	if(PlayerState==FILE_NOT_LOADED){
		HandleError(L"Файл не загружен",SMP_ALERT_BLOCKING,L"");
		return;
	}

	if(IsPlayingCDA==true){
		PlayCDAFrom(MCI_MAKE_TMSF(TrackNumber,0,0,0));
		return;
	}
hr = pControl->Run();
if(FAILED(hr)){
	HandlePlayError(hr,L"");
	Close();
	WPARAM wParam=MAKEWPARAM(ID_NEXTTRACK,0);
	PostMessage(hWnd,WM_COMMAND,wParam,0);
	return;
}

PlayerState=PLAYING;

pEvent->SetNotifyWindow((LONG_PTR)hWnd,MM_MCINOTIFY,0L);

}

void Pause(){
HRESULT hRes;
DWORD dwRes;
MCI_GENERIC_PARMS p={0};

if(PlayerState!=PLAYING)return;

if(IsPlayingCDA==true){
	pos=GetPosition();
dwRes=mciSendCommand(DeviceID,MCI_PAUSE,0,(DWORD)&p);

PlayerState=PAUSED;
return;
}
hRes=pControl->Pause();
if(FAILED(hRes)){
	
	return;
}
PlayerState=PAUSED;
}

void Resume(){
HRESULT hRes;
DWORD dwRes;
MCI_GENERIC_PARMS p={0};

if(PlayerState!=PAUSED)return;
if(IsPlayingCDA==true){

	PlayerState=PLAYING;
SetPosition(pos);

return;
}
hRes=pControl->Run();
if(FAILED(hRes)){
	
	return;
}
PlayerState=PLAYING;
}

void Stop(){
HRESULT hRes;
DWORD dwRes;
MCI_GENERIC_PARMS p={0};
if(PlayerState!=PLAYING)return;
if(IsPlayingCDA==true){
dwRes=mciSendCommand(DeviceID,MCI_STOP,0,(DWORD)&p);
PlayerState=STOPPED;
return;
}
Pause();
hRes=pControl->Stop();
if(FAILED(hRes)){
	Close();
	PlayerState=FILE_NOT_LOADED;
	return;
}

PlayerState=STOPPED;
SetThreadExecutionState (ES_CONTINUOUS);
}

DWORD GetLength(){
	
	HRESULT hRes;
	LONGLONG dur={0};
	MCI_STATUS_PARMS params={0};
	DWORD dwRes;
	DWORD len;

	if(PlayerState==FILE_NOT_LOADED)return 0;
	if(fShowNextImage==true)return Settings.ImageDelay;

if(IsPlayingCDA==true){	
params.dwItem=MCI_STATUS_LENGTH;
params.dwTrack=TrackNumber;
	dwRes=mciSendCommand(DeviceID,MCI_STATUS,MCI_STATUS_ITEM|MCI_TRACK,(DWORD)&params);
	if(dwRes!=0){
		return 0;
}
	len=MCI_MSF_MINUTE((params.dwReturn))*60+MCI_MSF_SECOND((params.dwReturn));
return len*(DWORD)1000;

}
	hRes=pSeek->GetDuration(&dur);
	if(FAILED(hRes)){
		return 0;
	}
return dur/TIME_KOEFF;
}

DWORD GetPosition(){
	HRESULT hRes;
	LONGLONG pos;
	LONGLONG stoppos;
	MCI_STATUS_PARMS params={0};
	DWORD dwRes;
	DWORD pos_msec;

	if(PlayerState==FILE_NOT_LOADED||PlayerState==STOPPED)return 0;
if(IsPlayingCDA==true){	
params.dwItem=MCI_STATUS_POSITION;
	dwRes=mciSendCommand(DeviceID,MCI_STATUS,MCI_STATUS_ITEM,(DWORD)&params);
	if(dwRes!=0){
		return 0;
}
	pos_msec=(MCI_TMSF_MINUTE(params.dwReturn)*60+MCI_TMSF_SECOND(params.dwReturn))*1000;
	
return pos_msec;
}
	hRes=pSeek->GetPositions(&pos,&stoppos);
	if(FAILED(hRes)){
	
	return 0;
}
return pos/TIME_KOEFF;
}

void Rewind(){
	HRESULT hRes;
LONGLONG pos=0;
MCI_SEEK_PARMS params={0};
	DWORD dwRes;
	if(PlayerState==FILE_NOT_LOADED)return ;
if(IsPlayingCDA==true){
	params.dwTo=MCI_MAKE_TMSF(TrackNumber,0,0,0);
dwRes=mciSendCommand(DeviceID,MCI_SEEK,MCI_TO,(DWORD)&params);
	if(dwRes!=0){		return;}
PlayCDAFrom(MCI_MAKE_TMSF(TrackNumber,0,0,0));return;
}
hRes=pSeek->SetPositions(&pos,AM_SEEKING_AbsolutePositioning,NULL,AM_SEEKING_NoPositioning);
	
	if(FAILED(hRes)){
	
	return;
}

if(PlayerState==PLAYING)Play();
if(PlayerState==PAUSED)PlayerState=STOPPED;
}

void SetPosition(LONGLONG pos){
	HRESULT hRes;
DWORD dur;
MCI_SEEK_PARMS params={0};
UINT min;
UINT sec;
	DWORD dwRes;
	if(PlayerState==FILE_NOT_LOADED)return ;
	dur=GetLength();
	if(((DWORD)pos)>dur)return;

if(IsPlayingCDA==true){
sec=pos/1000;
min=sec/60;
sec=sec-60*min;

params.dwTo=MCI_MAKE_TMSF(TrackNumber,min,sec,0);
	dwRes=mciSendCommand(DeviceID,MCI_SEEK,MCI_TO,(DWORD)&params);
	if(dwRes!=0){return;}
	PlayCDAFrom(params.dwTo);
	return;
}

	pos*=TIME_KOEFF;
hRes=pSeek->SetPositions(&pos,AM_SEEKING_AbsolutePositioning,NULL,AM_SEEKING_NoPositioning);
	
	if(FAILED(hRes)){
	ShowError(hRes,PLAY_ERROR);
	return;
}

if(PlayerState==PLAYING)Play();
}

int GetVolume()
{	
    return VolumeX;
}

void SetVolume(long x)
{
    if(x<0)x=0;
    if(x>100)x=100;
    long y;

    //audio-tapered control
    if(x==0) y=-10000;
    else y=(long)floor(2173.91f * log((float)x) - 10000);

    if(y<-10000)y=-10000;
    if(y>0)y=0;
    VolumeX=x;
    Volume=y;

    if(PlayerState==FILE_NOT_LOADED)return;
    if(pAudio==NULL)return;
    if(IsPlayingCDA!=false){return;}
    pAudio->put_Volume(y);
}

bool SetVideoWindow(HWND hParent){
	HRESULT hr;
	RECT rc;
long DestWidth;
long DestHeight;
long SourceHeight;
long SourceWidth;
double factor;
long res;

	if(PlayerState==FILE_NOT_LOADED)return false;
	
	if(pVideoWindow==NULL)return false;

hr=pVideoWindow->put_WindowStyle(WS_CHILD|WS_VISIBLE);
if(hr!=S_OK){return false;}
hr=pVideoWindow->put_Owner((LONG_PTR)hParent);
if(hr!=S_OK){return false;}
hr=pVideoWindow->put_MessageDrain((LONG_PTR)hVideoWindow);
if(hr!=S_OK){return false;}

SetVideoRect();
pVideoWindow->put_Visible(OATRUE);
pVideoWindow->NotifyOwnerMessage((LONG_PTR)hParent,WM_PAINT,0,0);

return true;
}

void SetVideoRect(){
HRESULT hr;
	RECT rc;
long DestWidth;
long DestHeight;
long SourceHeight;
long SourceWidth;
double factor;
long res;

if(PlayerState==FILE_NOT_LOADED)return ;
if(pVideoWindow==NULL)return ;
if(pVideo==NULL)return ;
pVideoWindow->get_FullScreenMode(&res);
if(res==OATRUE)return;

GetClientRect(hVideoWindow,&rc);

hr=pVideoWindow->put_Height(rc.bottom);

hr=pVideoWindow->put_Width(rc.right);

hr=pVideoWindow->put_Left(0);

hr=pVideoWindow->put_Top(0);


pVideo->get_VideoHeight(&SourceHeight);
pVideo->get_VideoWidth(&SourceWidth);

DestWidth=rc.right;
factor=DestWidth/(double)SourceWidth;
DestHeight=factor*SourceHeight;
if(DestHeight>rc.bottom){
DestHeight=rc.bottom;
factor=DestHeight/(double)SourceHeight;
DestWidth=factor*SourceWidth;}

pVideo->SetDestinationPosition((rc.right-DestWidth)*0.5f,(rc.bottom-DestHeight)*0.5f,DestWidth,DestHeight);
}

RECT VRect={0};

void EnableFullScreen(){

	FullScreen=true;
	GetWindowRect(hVideoWindow,&VRect);
	
SetWindowLong(hVideoWindow,GWL_STYLE,WS_POPUP|WS_CLIPCHILDREN);
		SetWindowPos(hVideoWindow,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED);
		ShowWindow(hVideoWindow,SW_SHOWMAXIMIZED);
		
		SetVideoWindow(hVideoWindow);
		SetTimer(hVideoWindow,TM_HIDE_CURSOR,1000,NULL);

}
void DisableFullScreen(){
	
if(FullScreen==false)return;
FullScreen=false;


ShowWindow(hVideoWindow,SW_RESTORE);
SetWindowLong(hVideoWindow,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN);

		SetWindowPos(hVideoWindow,HWND_TOP,VRect.left,VRect.top,VRect.right-VRect.left,VRect.bottom-VRect.top,
			SWP_FRAMECHANGED| SWP_NOZORDER  );
		InvalidateRect(HWND_DESKTOP,NULL,TRUE);
UpdateWindow(HWND_DESKTOP);

SetVideoWindow(hVideoWindow);
		
		ShowWindow(hVideoWindow,SW_RESTORE);
		InvalidateRect(hVideoWindow,NULL,TRUE);
		UpdateWindow(hVideoWindow);
		if(IsCursorShown()==FALSE){ShowCursor(TRUE);}

}
