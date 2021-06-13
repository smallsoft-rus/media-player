#include "player_dshow.h"
#include "errors.h"
#include "resource.h"

//imports
void Close();
void EnableFullScreen();
void DisableFullScreen();
void GetMultimediaInfoString(WCHAR* text,size_t size);
bool SetVideoWindow(HWND hParent);
void SetVideoRect();
extern bool fShowNextImage;
extern HWND hWnd;
extern PLAYER_STATE PlayerState;
extern bool IsPlayingVideo;
extern long Volume;
extern SMPSETTINGS Settings; //settings
extern HWND hVideoWindow; //UI

//module state
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
GUID guidStreamSubType={0,0,0,{0,0,0,0,0,0,0,0}};

//implementation

BOOL InsertSplitter(TCHAR* file,const SPLITTER_DATA* sd){
IPin* pin1=NULL;
IPin* pin2=NULL;
HRESULT hr;
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

bool pin_GetAudioInfo(IPin* pin,SMP_AUDIOINFO* pAudioInfo){
    IEnumMediaTypes * pEnum=NULL;
    AM_MEDIA_TYPE* amt=NULL;
    WAVEFORMATEX* wf=NULL;
    HRESULT hr;
    ULONG res=0;

    hr=pin->EnumMediaTypes(&pEnum);
    if(FAILED(hr))return false;
    hr=pEnum->Next(1,&amt,&res);
    if(FAILED(hr)||res==0){pEnum->Release();return false;}
    if(amt->formattype!=FORMAT_WaveFormatEx){pEnum->Release();return false;}
    wf=(WAVEFORMATEX*)amt->pbFormat;
    pAudioInfo->chans=wf->nChannels;
    pAudioInfo->BitsPerSample=wf->wBitsPerSample;
    pAudioInfo->BitsPerSecond=wf->nAvgBytesPerSec*8;
    pAudioInfo->nFreq=wf->nSamplesPerSec;
    pAudioInfo->wFormatTag=wf->wFormatTag;

    if(amt->subtype==MEDIASUBTYPE_DOLBY_AC3){
	    pAudioInfo->wFormatTag=AUDIO_AC3;
    }

    if(amt->subtype==MEDIASUBTYPE_MPEG2_AUDIO){
        pAudioInfo->wFormatTag=AUDIO_MPEG2AAC;
    }
    pEnum->Release();
    MyDeleteMediaType(amt);
    return true;
}

bool pin_GetVideoInfo(IPin* pin,SMP_VIDEOINFO* pVideoInfo){
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
if(FAILED(hr))return false;
hr=pEnum->Next(1,&amt,&res);
if(FAILED(hr)||res==0){pEnum->Release();return false;}
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
if(bi==NULL){pEnum->Release();return false;}
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
return true;

}

WORD DS_GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType){
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

void DS_Player_Close(){
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
}

BOOL DS_Player_OpenFile(WCHAR* filename){
    HRESULT hr;
    GUID tf=TIME_FORMAT_MEDIA_TIME;
    TCHAR ext[8]=L"";
    bool res;
    BOOL success=FALSE;
    bool autobuild=false;    

    GetFileExtension(filename,ext);

    // Create the filter graph manager and query for interfaces.
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IGraphBuilder, (void **)&pGraph);

    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}
    hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}
    hr = pGraph->QueryInterface(IID_IMediaEventEx, (void **)&pEvent);
    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}
    hr = pGraph->QueryInterface(IID_IMediaSeeking, (void **)&pSeek);
    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}
    hr = pGraph->QueryInterface(IID_IBasicAudio, (void **)&pAudio);
    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}
    hr = pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow);
    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}
    hr = pGraph->QueryInterface(IID_IBasicVideo, (void **)&pVideo);
    if(FAILED(hr)){ShowError(hr,SYSTEM_ERROR);Close();return FALSE;}

    //handle URL
    if(IsURL(filename)!=FALSE){
        hr = pGraph->RenderFile(filename, NULL);

        if(FAILED(hr)){
            HandlePlayError(hr,filename);
            Close();
            return FALSE;
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
        return FALSE;
    }

    autobuild=true;

    play:
    SearchFilters();    

#ifdef DEBUG
    LogMessage(L"DS_Player_OpenFile",TRUE);
    LogMessage(filename,FALSE);

    if(autobuild) LogMessage(L"Codecs selected automatically",FALSE);

    LogAllFilters(pGraph);

    WCHAR strMediaInfo[5000]=L"";
    GetMultimediaInfoString(strMediaInfo,5000);
    LogMessage(strMediaInfo,FALSE);
    LogMessage(L"***************",FALSE);
#endif

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

    pAudio->put_Volume(Volume);
    return TRUE;
}

BOOL DS_Player_Play(){
    HRESULT hr;

    hr = pControl->Run();

    if(FAILED(hr)){
        HandlePlayError(hr,L"");
        Close();
        WPARAM wParam=MAKEWPARAM(ID_NEXTTRACK,0);
        PostMessage(hWnd,WM_COMMAND,wParam,0);
        return FALSE;
    }
    
    pEvent->SetNotifyWindow((LONG_PTR)hWnd,MM_MCINOTIFY,0L);
    return TRUE;
}

BOOL DS_Player_Pause(){
HRESULT hRes;

hRes=pControl->Pause();
if(FAILED(hRes)){
	return FALSE;
}

return TRUE;
}

BOOL DS_Player_Resume(){
    HRESULT hRes;

    hRes=pControl->Run();

    if(FAILED(hRes)){return FALSE;}
        
    return TRUE;
}

BOOL DS_Player_Stop(){
    HRESULT hRes;

    hRes=pControl->Stop();
    if(FAILED(hRes)){
	    Close();
	    PlayerState=FILE_NOT_LOADED;
	    return FALSE;
    }

    return TRUE;
}

DWORD DS_Player_GetLength(){
	
    HRESULT hRes;
    LONGLONG dur={0};

	hRes=pSeek->GetDuration(&dur);
    if(FAILED(hRes)){
#ifdef DEBUG
        LogMessage(L"IMediaSeeking::GetDuration failed",FALSE);
        HandleMediaError(hRes);
#endif
		return 0;
    }
    return dur/TIME_KOEFF;
}

DWORD DS_Player_GetPosition(){
    HRESULT hRes;
    LONGLONG pos;
    LONGLONG stoppos;

    hRes=pSeek->GetPositions(&pos,&stoppos);

    if(FAILED(hRes)){	
        return 0;
    }

    return pos/TIME_KOEFF;
}

BOOL DS_Player_Rewind(){
    HRESULT hRes;
    LONGLONG pos=0;

    hRes=pSeek->SetPositions(&pos,AM_SEEKING_AbsolutePositioning,NULL,AM_SEEKING_NoPositioning);
	
    if(FAILED(hRes)){
        return FALSE;
    }

    return TRUE;
}

BOOL DS_Player_SetPosition(LONGLONG pos){
    HRESULT hRes;

    pos*=TIME_KOEFF;
    hRes=pSeek->SetPositions(&pos,AM_SEEKING_AbsolutePositioning,NULL,AM_SEEKING_NoPositioning);
	
    if(FAILED(hRes)){
        ShowError(hRes,PLAY_ERROR);
        return FALSE;
    }

    return TRUE;
}

void DS_Player_SetVolume(long value)
{
    if(pAudio==NULL)return;
    pAudio->put_Volume(value);
}

bool DS_SetVideoWindow(HWND hParent){
    HRESULT hr;        
	
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

void DS_SetVideoRect(){
    HRESULT hr;
    RECT rc;
    long DestWidth;
    long DestHeight;
    long SourceHeight;
    long SourceWidth;
    double factor;
    long res;

    if(pVideoWindow==NULL)return;
    if(pVideo==NULL)return;
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
        DestWidth=factor*SourceWidth;
    }

    pVideo->SetDestinationPosition((rc.right-DestWidth)*0.5f,(rc.bottom-DestHeight)*0.5f,DestWidth,DestHeight);
}
