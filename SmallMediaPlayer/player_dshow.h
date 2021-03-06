#ifndef PLAYER_DSHOW_H
#define PLAYER_DSHOW_H

#include "dshow_helpers.h"
#include <DvdMedia.h>
#include "common.h"
#include <MMReg.h>

//exports

void DS_ProcessNotify(WPARAM NotifyValue);
WORD DS_GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType);
void DS_Player_Close();
HRESULT DS_Player_OpenFile(WCHAR* filename);
BOOL DS_Player_Play();
BOOL DS_Player_Pause();
BOOL DS_Player_Resume();
BOOL DS_Player_Stop();
DWORD DS_Player_GetLength();
DWORD DS_Player_GetPosition();
BOOL DS_Player_Rewind();
BOOL DS_Player_SetPosition(LONGLONG pos);
void DS_Player_SetVolume(DWORD x);
bool DS_SetVideoWindow(HWND hParent);
void DS_SetVideoRect();
void DS_ShowPropertyPage(TOPOLOGY_NODE node);

#endif
