/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef COMMON_H
#define COMMON_H
#include <windows.h>

enum PLAYER_STATE
{
    FILE_NOT_LOADED,
    STOPPED,
    PAUSED,
    PLAYING
};

typedef enum {NORMAL,REPEAT_LIST,REPEAT_FILE,RANDOM} PLAYBACK_MODE;

typedef enum {
    EVT_UNKNOWN=0,EVT_USERABORT,EVT_FILE_CLOSED,EVT_ERRORABORT,EVT_COMPLETE
} PLAYER_EVENT;

typedef void (*PLAYER_EVENT_CALLBACK)(PLAYER_EVENT evt);

const int SMPVER_MAJOR = 2;
const int SMPVER_MINOR = 2;

typedef struct 
{
	int size;
	BYTE verMajor;
	BYTE verMinor;
	//settings
	bool fShowPlaylist;
	bool fShowCover;
	bool fLoadDefaultPls;
	bool fRememberPosition;
	bool fLoadWallpaper;
	bool fLoadDefWallpaper;
	WCHAR WallpaperFilePath[MAX_PATH];
	DWORD ImageDelay;
	//remembered window state
	int WndX;
	int WndY;
	int WndWidth;
	int WndHeight;
	bool WndMaximized;
	//remembered player state
	PLAYER_STATE LastPlayerState;
	PLAYBACK_MODE LastPlaybackMode;
	UINT LastFileIndex;
	DWORD LastPosition;
	long LastVolume;
}
SMPSETTINGS;

typedef enum {VIDEOTYPE_VIDEO=0,VIDEOTYPE_MPEG1=1,VIDEOTYPE_MPEG2=2}SMP_VIDEOTYPE;
typedef enum {STREAM_UNKNOWN=0,STREAM_AVI,STREAM_MPEG1,STREAM_MPEG1VCD,STREAM_MPEG2,
STREAM_QUICKTIME,STREAM_WAVE,STREAM_MIDI,STREAM_ASF}SMP_STREAM;

typedef struct {
    DWORD dwVideoCodec;
    int width;
    int height;
    int BitsPerPixel;
    int FramesPerSecond;
    SMP_VIDEOTYPE VideoType;
    RECT rcSource;
}SMP_VIDEOINFO;

typedef struct {
    BYTE chans;
    int BitsPerSample;
    int nFreq;
    int BitsPerSecond;
    WORD wFormatTag;
}SMP_AUDIOINFO;

typedef union{
    WORD word;
    char chars[4];
}FOURCC_EXTRACTOR;

#define VIDEO_YUV (0x56595559)
#define VIDEO_XVID (MAKEFOURCC('X','V','I','D'))
#define VIDEO_DIVX (MAKEFOURCC('D','I','V','X'))
#define VIDEO_MPEG4_1 (MAKEFOURCC('D','I','V','1'))
#define VIDEO_MPEG4_2 (MAKEFOURCC('D','I','V','2'))
#define VIDEO_DIV3 (MAKEFOURCC('D','I','V','3'))
#define VIDEO_DIV4 (MAKEFOURCC('D','I','V','4'))
#define VIDEO_DIV5 (MAKEFOURCC('D','I','V','5'))
#define VIDEO_DIV6 (MAKEFOURCC('D','I','V','6'))
#define VIDEO_H264 (MAKEFOURCC('H','2','6','4'))
#define VIDEO_HFYU (MAKEFOURCC('H','F','Y','U'))
#define VIDEO_MJPEG (MAKEFOURCC('M','J','P','G'))
#define VIDEO_MPEG1 (MAKEFOURCC('M','P','E','G'))
#define VIDEO_MPEG4VIDEO (MAKEFOURCC('M','P','4','V'))

#define AUDIO_MPEG1 (0x0050)
#define AUDIO_MPEG2AAC (0x0180)
#define AUDIO_AAC4 (0x00FF)
#define AUDIO_AAC3 (0x706D)
#define AUDIO_AAC2 (0x0AAC)
#define AUDIO_AAC (0xA106)
#define AUDIO_MPEG4AAC (0x4143)
#define AUDIO_WAVEPACK (0x5756)
#define AUDIO_OGG (0x674F)
#define AUDIO_OGG2 (0x6750)
#define AUDIO_OGG3 (0x6751)
#define AUDIO_OGG1P (0x676F)
#define AUDIO_OGG2P (0x6770)
#define AUDIO_OGG3P (0x6771)
#define AUDIO_FLAC (0xF1AC)
#define AUDIO_AMR (0x0136)
#define AUDIO_AC3 (0x2000)
#define AUDIO_DVI_ADPCM (0x0011)

#define INFORES_NO 0
#define INFORES_AUDIO 1
#define INFORES_VIDEO 2
#define INFORES_BOTH 3

#define MNUM_MAINMENU (0)
#define MNUM_PLAYLISTMENU (1)

#define UM_TRAYICON (WM_USER+1)

//timers
#define TM_NEXT_IMAGE 3
#define TM_HIDE_CURSOR 4

//DirectShow time unit
#define TIME_KOEFF 10000

BOOL inline IsCursorShown(){
	CURSORINFO ci={0};
	ci.cbSize=sizeof(CURSORINFO);
	GetCursorInfo(&ci);
	if(ci.flags==0)return FALSE;
	else return TRUE;

}

inline WCHAR* GetShortName(WCHAR* fullname){
    WCHAR* p;
    int i;
    p=fullname;
    for(i=0;i<MAX_PATH-1;i++){
        if(fullname[i]==0)break;
        if(fullname[i]==L'\\'||fullname[i]==L'/'){p=&(fullname[i+1]);}
    }
    return p;
}

inline char* GetShortNameA(char* fullname){
    char* p;
    int i;
    p=fullname;
    for(i=0;i<MAX_PATH-1;i++){
        if(fullname[i]==0)break;
        if(fullname[i]=='\\'||fullname[i]=='/'){p=&(fullname[i+1]);}
    }
    return p;
}

inline void GetFileExtension(const WCHAR* fname,WCHAR* ext){
    int i=0,c=0;
    const WCHAR* s=NULL;
    c=lstrlen(fname);

    for(i=c-1;i>c-8;i--){
        if(fname[i]==L'.'||fname[i]==L'\\'||fname[i]==L'/'){
            s=&(fname[i+1]);
            break;
        }
    }

    if(s==NULL){
        lstrcpy(ext,L"");
        return;
    }

    lstrcpy(ext,s);
}

inline void GetFileExtensionA(const char* fname,char* ext){
    int i=0,c=0;
    const char* s=NULL;
    c=lstrlenA(fname);
    for(i=c-1;i>c-8;i--){
        if(fname[i]=='.'||fname[i]=='\\'||fname[i]=='/'){
            s=&(fname[i+1]);
            break;
        }
    }

    if(s==NULL){
        lstrcpyA(ext,"");
        return;
    }

    lstrcpyA(ext,s);
}

inline BOOL IsURL(const WCHAR* str){
    int i;
    int c;
    c=lstrlen(str)-4;

    for(i=0;i<c;i++){
        if(str[i]==0)return FALSE;
        if(str[i]==L':'&&str[i+1]==L'/'&&str[i+2]==L'/')return TRUE;
    }
    return FALSE;
}

#endif
