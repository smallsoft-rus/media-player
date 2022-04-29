/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include <MMReg.h>

//extern vars
extern HWND hVideoWindow;

//declarations of vars
extern HWND hWnd;
extern PLAYER_STATE PlayerState;
extern bool IsPlayingVideo;
extern bool fShowNextImage;
extern bool FullScreen;
extern RECT VRect;

//exports

BOOL Player_OpenFileCore(WCHAR* filename, BOOL useDirectShow, BOOL useMediaFoundation);
BOOL Player_OpenFile(WCHAR* filename);
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
void SetVolume(DWORD vol);
bool SetVideoWindow(HWND hParent);
void SetVideoRect();
void DisableFullScreen();
void EnableFullScreen();
WORD GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType);
void GetMultimediaInfoString(WCHAR* text,size_t size);
DWORD GetVolume();
PLAYER_IMPL Player_GetCurrentImpl();

//Redraws video window (called on WM_PAINT)
LRESULT Player_OnPaint(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

// Sets callback function to notify about player events (called from UI)
void Player_SetEventCallback(PLAYER_EVENT_CALLBACK callback);

// Processes messages related to playback events. 
// Called by window procedure (in ui.cpp) that receives messages.
void Player_ProcessNotify(WPARAM NotifyValue);

// Show property page for the specified node
void ShowPropertyPage(TOPOLOGY_NODE node);

LRESULT Player_InitWindows(HWND hVideo,HWND hEvent);
void Player_OnMfEvent(HWND hwnd, WPARAM pUnkPtr);

#endif
