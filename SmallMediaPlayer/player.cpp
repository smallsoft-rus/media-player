/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include <math.h>
#include "player.h"
#include "player_dshow.h"
#include "player_mf.h"
#include "tags.h"
#include "errors.h"
#include "resource.h"

extern SMPSETTINGS Settings;
extern TAGS_GENERIC OpenedFileTags;
extern bool fOpenedFileTags;

PLAYER_IMPL CurrentImpl=IMPL_DSHOW;

HWND hWnd;//for notify

PLAYER_STATE PlayerState=FILE_NOT_LOADED;
DWORD pos;
long Volume=0;
long VolumeX=0;
bool IsPlayingVideo=false;

bool fShowNextImage=false;
bool FullScreen=false;

// Pointer to function that receives playback event notifications.
// Set by Player_SetEventCallback, invoked by OnPlayerEvent.
PLAYER_EVENT_CALLBACK pfnEventCallback=NULL;

void Player_SetEventCallback(PLAYER_EVENT_CALLBACK callback){
    pfnEventCallback=callback;
}

// Invokes event callback (called by implementation)
void OnPlayerEvent(PLAYER_EVENT evt){
    if(pfnEventCallback==NULL)return;
    pfnEventCallback(evt);
}

void Player_ProcessNotify(WPARAM NotifyValue){
    DS_ProcessNotify(NotifyValue);
}

WORD GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType){
    return DS_GetMultimediaInfo(pAudioInfo,pVideoInfo,pStreamType);
}

void GetMultimediaInfoString(WCHAR* text,size_t size){
WORD wRes=0;
SMP_AUDIOINFO ai={0};
SMP_VIDEOINFO vi={0};
FOURCC_EXTRACTOR* fcc=NULL;
TCHAR buf[MAX_PATH];
SMP_STREAM stream=STREAM_UNKNOWN;

wRes=GetMultimediaInfo(&ai,&vi,&stream);
StringCchCopy(text,size,L"\n==ВХОДНОЙ ПОТОК:==\n");
StringCchCat(text,size,L"Формат: ");
switch(stream){
case STREAM_AVI:StringCchCat(text,size,L"Audio-Video Interleaved\n");break;
case STREAM_ASF:StringCchCat(text,size,L"Advanced Systems Format\n");break;
case STREAM_MPEG1:StringCchCat(text,size,L"MPEG1\n");break;
case STREAM_MPEG1VCD:StringCchCat(text,size,L"MPEG1 VideoCD\n");break;
case STREAM_MPEG2:StringCchCat(text,size,L"MPEG2\n");break;
case STREAM_WAVE:StringCchCat(text,size,L"Waveform Audio\n");break;
case STREAM_QUICKTIME:StringCchCat(text,size,L"Apple(tm) Quick Time Movie\n");break;
case STREAM_UNKNOWN:default:StringCchCat(text,size,L"[нет данных]\n");break;
}
StringCchCat(text,5000,L"==АУДИО:==\n");
if(wRes==INFORES_AUDIO||wRes==INFORES_BOTH){
	StringCchCat(text,5000,L"Формат: ");
	switch(ai.wFormatTag){
		case WAVE_FORMAT_PCM:StringCchCat(text,size,L"PCM Waveform Audio");break;
		case AUDIO_DVI_ADPCM:StringCchCat(text,size,L"DVI ADPCM");break; 
		case AUDIO_MPEG1:StringCchCat(text,size,L"MPEG1 Layer 1/2");break;
		case 	WAVE_FORMAT_MPEGLAYER3:StringCchCat(text,size,L"MPEG1 Layer3");break;
		case WAVE_FORMAT_DOLBY_AC3_SPDIF:
		case AUDIO_AC3 :StringCchCat(text,size,L"DOLBY AC3");break;
		case WAVE_FORMAT_WMAVOICE9:		case WAVE_FORMAT_WMAVOICE10:		case WAVE_FORMAT_MSAUDIO1:             
		case WAVE_FORMAT_WMAUDIO2:		case WAVE_FORMAT_WMAUDIO3:	case  WAVE_FORMAT_WMAUDIO_LOSSLESS:        
		case WAVE_FORMAT_WMASPDIF: StringCchCat(text,size,L"Windows Media Audio");break; 
		case AUDIO_AAC4:case AUDIO_AAC3:case AUDIO_AAC2 :case AUDIO_AAC :
		case AUDIO_MPEG4AAC: 
		case  WAVE_FORMAT_MPEG_ADTS_AAC:case WAVE_FORMAT_MPEG_RAW_AAC:case WAVE_FORMAT_NOKIA_MPEG_ADTS_AAC:        
		case WAVE_FORMAT_NOKIA_MPEG_RAW_AAC:case WAVE_FORMAT_VODAFONE_MPEG_ADTS_AAC:     
		case WAVE_FORMAT_VODAFONE_MPEG_RAW_AAC:
			StringCchCat(text,size,L"Advanced Audio Coding (AAC)");break; 
		case AUDIO_FLAC:StringCchCat(text,size,L"Free Lossless Audio Codec (FLAC)");break; 
		case AUDIO_WAVEPACK:StringCchCat(text,size,L"WavePack");break; 
		case AUDIO_AMR:StringCchCat(text,size,L"VOICEAGE AMR");break; 
		case AUDIO_MPEG2AAC:StringCchCat(text,size,L"MPEG2");break; 
		default:StringCchCat(text,size,L"неизвестен");break;
	}
	StringCchPrintf(buf,256,L"\nКаналов: %d\nРазрешение: %d бит\nЧастота: %d Гц\n",(int)ai.chans,(int)ai.BitsPerSample,(int)ai.nFreq);
	StringCchCat(text,size,buf);
	StringCchPrintf(buf,256,L"Скорость: %d кбит/с\n",(int)(ai.BitsPerSecond/1000.0));StringCchCat(text,size,buf);
}
else{StringCchCat(text,size,L"[Нет данных]\n");}
StringCchCat(text,size,L"==ВИДЕО:==\n");
if(wRes==INFORES_VIDEO||wRes==INFORES_BOTH){
fcc=(FOURCC_EXTRACTOR*)&vi.dwVideoCodec;
StringCchCat(text,size,L"Кодек: ");

if(vi.dwVideoCodec==BI_RGB){
	switch(vi.VideoType){
	case VIDEOTYPE_MPEG1:StringCchCat(text,size,L"MPEG1\n");break;
	case VIDEOTYPE_MPEG2:StringCchCat(text,size,L"MPEG2\n");break;
	case VIDEOTYPE_VIDEO:default:StringCchCat(text,size,L"RGB\n");
	}
}
else{
	StringCchPrintf(buf,256,L"%c%c%c%c\n",fcc->chars[0],fcc->chars[1],fcc->chars[2],fcc->chars[3]);
	StringCchCat(text,size,buf);
}
StringCchPrintf(buf,256,L"Глубина цвета: %d бит\nКадров в секунду: %d\n",(int)vi.BitsPerPixel,(int)vi.FramesPerSecond);
StringCchCat(text,size,buf);
StringCchPrintf(buf,256,L"Разрешение: %d x %d \n",(int)vi.width,(int)vi.height);
StringCchCat(text,size,buf);
StringCchPrintf(buf,256,L"Разрешение(эфф.): %d x %d \n",(int)(vi.rcSource.right-vi.rcSource.left),(int)(vi.rcSource.bottom-vi.rcSource.top));
StringCchCat(text,size,buf);
}
else{StringCchCat(text,size,L"[Нет данных]\n");}

}

void Close(){

    Stop();
    DS_Player_Close();
    g_pPlayer->Close();
    
    PlayerState=FILE_NOT_LOADED;
}

BOOL Player_OpenFile(WCHAR* filename){
    if(PlayerState!=FILE_NOT_LOADED){Close();}
    fShowNextImage=false;

    //try DirectShow
    HRESULT hr = DS_Player_OpenFile(filename);

    if(SUCCEEDED(hr)){
        CurrentImpl=IMPL_DSHOW;
    }
    else{
        //if failed, try Media Foundation
        hr = MF_Player_OpenFile(filename);

        if(SUCCEEDED(hr)){
            CurrentImpl=IMPL_MF;
        }
    }

    if(FAILED(hr)){
        //HandlePlayError(hr,filename);
        HandleMfError(hr,L"Could not open the file.",filename);
        WPARAM wParam=MAKEWPARAM(ID_NEXTTRACK,0);
        PostMessage(hWnd,WM_COMMAND,wParam,0);
        return FALSE;
    }
    
    PlayerState=STOPPED;

    //initialize and show video window, if the file has video
    bool hasvideo=SetVideoWindow(hVideoWindow);

    if(hasvideo==false){
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

    return TRUE;
}

void PlayFile(TCHAR* filename){

    BOOL res=Player_OpenFile(filename);
    if(res==FALSE)return;
    Play();
}

void Play(){

    if(PlayerState==FILE_NOT_LOADED){
        HandleError(L"Файл не загружен",SMP_ALERT_BLOCKING,L"");
        return;
    }

    BOOL res;
    if(CurrentImpl==IMPL_DSHOW) {
        res = DS_Player_Play();
    }
    else {
        HRESULT hr=g_pPlayer->Play();
        res=SUCCEEDED(hr);
    }

    if(res==FALSE)return;

    PlayerState=PLAYING;
}

void Pause(){
    if(PlayerState!=PLAYING)return;
    BOOL res;

    if(CurrentImpl==IMPL_DSHOW){
       res = DS_Player_Pause();
    }
    else{
        HRESULT hr=g_pPlayer->Pause();
        res=SUCCEEDED(hr);
    }

    if(res==FALSE)return;
    PlayerState=PAUSED;
}

void Resume(){
    if(PlayerState!=PAUSED)return;
    BOOL res;

    if(CurrentImpl==IMPL_DSHOW){
       res=DS_Player_Resume();
    }
    else{
        HRESULT hr=g_pPlayer->Play();
        res=SUCCEEDED(hr);
    }

    if(res==FALSE)return;
    PlayerState=PLAYING;
}

void Stop(){
    if(PlayerState!=PLAYING)return;

    Pause();
    BOOL res;

    if(CurrentImpl==IMPL_DSHOW){
       res=DS_Player_Stop();
    }
    else{
        HRESULT hr=g_pPlayer->Stop();
        res=SUCCEEDED(hr);
    }

    if(res==FALSE)return;

    PlayerState=STOPPED;
    SetThreadExecutionState (ES_CONTINUOUS);
}

DWORD GetLength(){
    if(PlayerState==FILE_NOT_LOADED)return 0;
	if(fShowNextImage==true)return Settings.ImageDelay;

    if(CurrentImpl==IMPL_MF){
        return g_pPlayer->GetLength();
    }
    else{
        return DS_Player_GetLength();
    }
}

DWORD GetPosition(){
    if(PlayerState==FILE_NOT_LOADED||PlayerState==STOPPED)return 0;

    if(CurrentImpl==IMPL_MF){
        return g_pPlayer->GetPosition();
    }
    else {
        return DS_Player_GetPosition();
    }
}

void Rewind(){
    if(PlayerState==FILE_NOT_LOADED)return;

    if(CurrentImpl==IMPL_MF){
        //Not implemented
        return;
    }

    BOOL res = DS_Player_Rewind();

    if(res==FALSE)return;
    if(PlayerState==PLAYING)Play();
    if(PlayerState==PAUSED)PlayerState=STOPPED;
}

void SetPosition(LONGLONG pos){

    DWORD dur;
    BOOL res;

	if(PlayerState==FILE_NOT_LOADED) return;
	dur=GetLength();
	if(((DWORD)pos)>dur)return;

    if(CurrentImpl==IMPL_MF){
        HRESULT hr=g_pPlayer->SetPosition(pos);
        res=SUCCEEDED(hr);
    }
	else {
        res = DS_Player_SetPosition(pos);
    }

    if(res==FALSE)return;

    if(PlayerState==PLAYING)Play();
}

int GetVolume()
{	
    return VolumeX;
}

void SetVolume(long x)
{
    long y;

    if(x<0)x=0;
    if(x>100)x=100;

    VolumeX=x; //store volume in percents

    //audio-tapered control
    if(x==0) y=-10000;
    else y=(long)floor(2173.91f * log((float)x) - 10000);

    if(y<-10000)y=-10000;
    if(y>0)y=0;
    
    Volume=y; //store volume in DirectX units

    if(PlayerState==FILE_NOT_LOADED)return;
    
    if(CurrentImpl==IMPL_MF){
        //Media Foundation
        g_pPlayer->SetVolume(x);
    }
    else{
        //DirectShow
        DS_Player_SetVolume(y);
    }
}

bool SetVideoWindow(HWND hParent){
    if(PlayerState==FILE_NOT_LOADED)return false;

    if(CurrentImpl==IMPL_MF){
        if(g_pPlayer->HasVideo()){
            SetVideoRect();
            return true;
        }
        else return false;
    }

    return DS_SetVideoWindow(hParent);
}

void SetVideoRect(){
    if(PlayerState==FILE_NOT_LOADED)return;

    if(CurrentImpl==IMPL_MF){
        RECT rc={0};
        GetClientRect(hVideoWindow,&rc);
        OnResize(rc.right,rc.bottom);
        return;
    }

    DS_SetVideoRect();
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

void ShowPropertyPage(TOPOLOGY_NODE node){

    if(CurrentImpl==IMPL_MF){
        MessageBoxW(hWnd,L"Not implemented",L"",MB_OK);
        return;
    }

    DS_ShowPropertyPage(node);
}

LRESULT Player_InitWindows(HWND hVideo,HWND hEvent){
    return MF_Player_InitWindows(hVideo,hEvent);
}

void Player_OnMfEvent(HWND hwnd, WPARAM pUnkPtr){
    MF_OnPlayerEvent(hwnd,pUnkPtr);
}

