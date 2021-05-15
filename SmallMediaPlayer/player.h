/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef PLAYER_H

#define PLAYER_H

#ifndef UNICODE
#define UNICODE
#endif

#include "DirectShowStuff.h"
#include <DvdMedia.h>
#include "common.h"

#define TIME_KOEFF 10000

//extern vars
extern HWND hVideoWindow;
	


//declaraations of vars
extern HWND hWnd;
extern PLAYER_STATE PlayerState;
extern IMediaEventEx   *pEvent;
extern bool IsPlayingCDA;
extern long Volume;
extern bool IsPlayingVideo;
extern bool fShowNextImage;
extern bool FullScreen;
extern RECT VRect;
extern IBaseFilter* pSource;
extern IBaseFilter* pSplitter;
extern IBaseFilter* pAudioDecoder;
extern IBaseFilter* pAudioRenderer;
extern IBaseFilter* pVideoDecoder;
extern IBaseFilter* pVideoRenderer;

void PlayFile(TCHAR* lpstrFileName);
void Play();
void Pause();
void Resume();
void Stop();
void Close();
DWORD GetLength();
DWORD GetPosition();
void Rewind();
void SetPosition(LONGLONG dwPos);
void SetVolume(long vol);
bool SetVideoWindow(HWND hParent);
void SetVideoRect();
void DisableFullScreen();
void EnableFullScreen();
WORD GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType);
int GetVolume();

#endif
