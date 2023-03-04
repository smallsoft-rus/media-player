/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "player.h"
#include "player_dshow.h"
#include "player_mf.h"
#include "tags.h"
#include "errors.h"
#include "resource.h"

extern SMPSETTINGS Settings;
extern TAGS_GENERIC OpenedFileTags;
extern bool fOpenedFileTags;

// Represents entry in media formats table (used to lookup human-readable format names)
typedef struct tagMEDIA_FORMAT_ENTRY{

    // Format tag or other identifier
    int code;

    // Format short name (displayed in main window)
    const WCHAR* pShortName;

    // Format full name (displayed in media info)
    const WCHAR* pFullName;
}MEDIA_FORMAT_ENTRY;

PLAYER_IMPL CurrentImpl=IMPL_DSHOW;

HWND hWnd;//for notify

PLAYER_STATE PlayerState=FILE_NOT_LOADED;
DWORD pos;
DWORD VolumeX=0;
bool IsPlayingVideo=false;

bool fShowNextImage=false;
bool FullScreen=false;

// Pointer to function that receives playback event notifications.
// Set by Player_SetEventCallback, invoked by OnPlayerEvent.
PLAYER_EVENT_CALLBACK pfnEventCallback=NULL;

#define AUDIO_FORMATS_TABLE_COUNT (sizeof(AudioFormatsTable)/sizeof(AudioFormatsTable[0]))
#define FILE_FORMATS_TABLE_COUNT (sizeof(FileFormatsTable)/sizeof(FileFormatsTable[0]))

MEDIA_FORMAT_ENTRY AudioFormatsTable[] = {
    {WAVE_FORMAT_PCM,                    L"PCM",        L"PCM Waveform Audio"},
    {WAVE_FORMAT_ADPCM,                  L"ADPCM",      L"Adaptive Differential PCM"},
    {AUDIO_IMA4_ADPCM,                   L"IMA4 ADPCM", L"Apple IMA4 ADPCM"},
    {AUDIO_DVI_ADPCM,                    L"DVI ADPCM",  L"DVI ADPCM"},
    {AUDIO_MPEG1,                        L"MP1/2",      L"MPEG1 Layer 1/2"},
    {WAVE_FORMAT_MPEGLAYER3,             L"MP3",        L"MPEG1 Layer 3"},
    {WAVE_FORMAT_DOLBY_AC3_SPDIF,        L"AC3",        L"DOLBY AC3"},
    {AUDIO_DOLBY_AC3,                    L"AC3",        L"DOLBY AC3"},
    {AUDIO_AC3,                          L"AC3",        L"DOLBY AC3"},
    {WAVE_FORMAT_WMAVOICE9,              L"WMA",        L"Windows Media Audio"},
    {WAVE_FORMAT_WMAVOICE10,             L"WMA",        L"Windows Media Audio"},
    {WAVE_FORMAT_MSAUDIO1,               L"WMA",        L"Windows Media Audio"},
    {WAVE_FORMAT_WMAUDIO2,               L"WMA",        L"Windows Media Audio"},
    {WAVE_FORMAT_WMAUDIO3,               L"WMA",        L"Windows Media Audio"},
    {WAVE_FORMAT_WMAUDIO_LOSSLESS,       L"WMA",        L"Windows Media Audio"},
    {WAVE_FORMAT_WMASPDIF,               L"WMA",        L"Windows Media Audio"},
    {AUDIO_AAC,                          L"AAC",        L"Advanced Audio Coding (AAC)"},
    {AUDIO_AAC2,                         L"AAC",        L"Advanced Audio Coding (AAC)"},
    {AUDIO_AAC3,                         L"AAC",        L"Advanced Audio Coding (AAC)"},
    {AUDIO_AAC4,                         L"AAC",        L"Advanced Audio Coding (AAC)"},
    {AUDIO_MPEG4AAC,                     L"AAC",        L"Advanced Audio Coding (AAC)"},    
    {WAVE_FORMAT_MPEG_RAW_AAC,           L"AAC",        L"Advanced Audio Coding (AAC)"},    
    {WAVE_FORMAT_NOKIA_MPEG_RAW_AAC,     L"AAC",        L"Advanced Audio Coding (AAC)"},    
    {WAVE_FORMAT_VODAFONE_MPEG_RAW_AAC,  L"AAC",        L"Advanced Audio Coding (AAC)"},
    {WAVE_FORMAT_MPEG_HEAAC,             L"AAC",        L"Advanced Audio Coding (AAC)"},
    {WAVE_FORMAT_MPEG_ADTS_AAC,          L"AAC-ADTS",   L"AAC-ADTS"},
    {WAVE_FORMAT_NOKIA_MPEG_ADTS_AAC,    L"AAC-ADTS",   L"AAC-ADTS"},
    {WAVE_FORMAT_VODAFONE_MPEG_ADTS_AAC, L"AAC-ADTS",   L"AAC-ADTS"},
    {AUDIO_LAV_AAC_ADTS,                 L"AAC-ADTS",   L"AAC-ADTS"},
    {AUDIO_FLAC,                         L"FLAC",       L"Free Lossless Audio Codec (FLAC)"},
    {AUDIO_WAVEPACK,                     L"WavePack",   L"WavePack"},
    {AUDIO_AMR,                          L"AMR",        L"VOICEAGE AMR"},
    {AUDIO_AMR_NB,                       L"AMR NB",     L"AMR Narrowband"},
    {AUDIO_AMR_WB,                       L"AMR WB",     L"AMR Wideband"},
    {AUDIO_MPEG2AAC,                     L"MPEG2",      L"MPEG2"},
    {AUDIO_APE,                          L"APE",        L"Monkey's Audio (APE)"},
    {WAVE_FORMAT_DTS2,                   L"DTS",        L"Digital Theater Systems (DTS)"},
    {AUDIO_DVD_LPCM,                     L"DVD",        L"DVD LPCM Audio"}
};

// File stream (container) formats table
MEDIA_FORMAT_ENTRY FileFormatsTable[] = {
    {STREAM_AVI,           L"AVI",           L"Audio-Video Interleaved"},
    {STREAM_ASF,           L"ASF",           L"Advanced Systems Format"},
    {STREAM_MPEG1,         L"MPEG1",         L"MPEG1"},
    {STREAM_MPEG1VCD,      L"MPEG1 VideoCD", L"MPEG1 VideoCD"},
    {STREAM_MPEG2,         L"MPEG2",         L"MPEG2"},
    {STREAM_WAVE,          L"WAV",           L"Waveform Audio"},
    {STREAM_QUICKTIME,     L"Quick Time",    L"Apple(tm) Quick Time Movie"},
    {STREAM_AIFF,          L"AIFF",          L"Audio Interchange File Format"}    
};

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
    if(CurrentImpl == IMPL_DSHOW) {
        return DS_GetMultimediaInfo(pAudioInfo,pVideoInfo,pStreamType);
    }
    else {
        if(g_pPlayer!=NULL) return g_pPlayer->GetMultimediaInfo(pAudioInfo, pVideoInfo, pStreamType);
        else return INFORES_NO; 
    }
}

const WCHAR* Player_GetAudioFormatString(WORD wFormatTag, BOOL full){
    for(int i=0;i<AUDIO_FORMATS_TABLE_COUNT;i++){
        if(AudioFormatsTable[i].code != (int)wFormatTag) continue;

        if(full) return AudioFormatsTable[i].pFullName;
        else return AudioFormatsTable[i].pShortName;
    }

    return L"неизвестно";
}

const WCHAR* Player_GetFileFormatString(SMP_STREAM code, BOOL full){
    for(int i=0;i<FILE_FORMATS_TABLE_COUNT;i++){
        if(FileFormatsTable[i].code != (int)code) continue;

        if(full) return FileFormatsTable[i].pFullName;
        else return FileFormatsTable[i].pShortName;
    }

    return L"неизвестно";
}

void GetMultimediaInfoString(WCHAR* text,size_t size){
    WORD wRes=0;
    SMP_AUDIOINFO ai={0};
    SMP_VIDEOINFO vi={0};
    FOURCC_EXTRACTOR* fcc=NULL;
    TCHAR buf[MAX_PATH];
    SMP_STREAM stream=STREAM_UNKNOWN;
    const WCHAR* pFormatName = L"";

    wRes=GetMultimediaInfo(&ai,&vi,&stream);

    StringCchCopy(text,size,L"\n==ВХОДНОЙ ПОТОК:==\n");
    StringCchCat(text,size,L"Формат: ");
    pFormatName = Player_GetFileFormatString(stream, TRUE);
    StringCchCat(text, size, pFormatName);

    StringCchCat(text,5000,L"\n==АУДИО:==\n");

    if(wRes==INFORES_AUDIO||wRes==INFORES_BOTH){
        StringCchCat(text,size,L"Формат: ");
        pFormatName = Player_GetAudioFormatString(ai.wFormatTag, TRUE);
        StringCchCat(text, size, pFormatName);

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

BOOL Player_OpenFileCore(WCHAR* filename, BOOL useDirectShow, BOOL useMediaFoundation){
    if(PlayerState!=FILE_NOT_LOADED){Close();}
    fShowNextImage=false;
        
    HRESULT hr=E_FAIL;
    //try Media Foundation
    if(useMediaFoundation != FALSE){
        hr = MF_Player_OpenFile(filename);
    }

    if(SUCCEEDED(hr)){
        CurrentImpl=IMPL_MF;
    }
    else{
        //if failed, try DirectShow
        if(useDirectShow != FALSE){
            hr = DS_Player_OpenFile(filename);

            if(SUCCEEDED(hr)){
                CurrentImpl=IMPL_DSHOW;
            }
        }
    }

    if(FAILED(hr)){
        HandlePlayError(hr,filename);
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

BOOL Player_OpenFile(WCHAR* filename){
    BOOL useDirectShow = TRUE;
    BOOL useMediaFoundation = TRUE;
    return Player_OpenFileCore(filename, useDirectShow, useMediaFoundation);
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

    BOOL res;
    if(CurrentImpl==IMPL_MF){
        HRESULT hr=g_pPlayer->SetPosition(0);
        res=SUCCEEDED(hr);
    }
    else{
        res=DS_Player_Rewind();
    }

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

DWORD GetVolume()
{	
    return VolumeX;
}

void SetVolume(DWORD x)
{
    if(x<0)x=0;
    if(x>100)x=100;

    VolumeX=x; //store volume in percents
    
    if(PlayerState==FILE_NOT_LOADED)return;
    
    if(CurrentImpl==IMPL_MF){
        //Media Foundation
        g_pPlayer->SetVolume(x);
    }
    else{
        //DirectShow
        DS_Player_SetVolume(x);
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
        BOOL res = g_pPlayer->ShowCodecProperties(node);

        if(res == FALSE){
            MessageBox(NULL, L"Невозможно показать страницу свойств", NULL, MB_OK);
        }
    }    
    else {
        DS_ShowPropertyPage(node);
    }
}

LRESULT Player_InitWindows(HWND hVideo,HWND hEvent){
    return MF_Player_InitWindows(hVideo,hEvent);
}

void Player_OnMfEvent(HWND hwnd, WPARAM pUnkPtr){
    MF_OnPlayerEvent(hwnd,pUnkPtr);
}

PLAYER_IMPL Player_GetCurrentImpl(){
    return CurrentImpl;
}

LRESULT Player_OnPaint(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam){

    //Media Foundation implementation need this call to have video repainted correctly on pause
    //(issue https://github.com/smallsoft-rus/media-player/issues/20).
    //DirectShow handles this automatically.

    if(CurrentImpl == IMPL_MF) return MF_Player_OnPaint(hWnd);
    else return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
