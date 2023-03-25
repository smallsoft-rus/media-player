/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef DSHOW_HELPERS_H
#define DSHOW_HELPERS_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "common.h"
#include <dshow.h>

typedef enum {
    SYSTEM_ERROR,
    PLAY_ERROR,
    ERROR_NOPROPERTIES,
    ERROR_NOTSUPPORTED
}ERROR_TYPE;

typedef	enum {
    MT_AUDIO,
    MT_AVI,
    MT_MPEG,
    MT_MKV
}MEDIATYPE;

typedef struct{
    TCHAR* FilterName;
    bool IsSource;
}SPLITTER_DATA;

// DC-Bass Source: ABE7B1D9-4B3E-4ACD-A0D1-92611D3A4492
SMP_DEFINE_GUID(CLSID_DCBassSource, 0xABE7B1D9, 0x4B3E, 0x4ACD, 0xA0, 0xD1, 0x92, 0x61, 0x1D, 0x3A, 0x44, 0x92);
// https://github.com/frafv/DCBassSource/blob/trunk/DCBassSource/DCBassSource.h

// LAV Splitter: 171252A0-8820-4AFE-9DF8-5C92B2D66B04
SMP_DEFINE_GUID(CLSID_LavSplitter, 0x171252A0, 0x8820, 0x4AFE, 0x9D, 0xF8, 0x5C, 0x92, 0xB2, 0xD6, 0x6B, 0x04);
// https://github.com/Nevcairiel/LAVFilters/blob/master/common/includes/moreuuids.h

// MPC - Mpeg Splitter (Gabest): DC257063-045F-4BE2-BD5B-E12279C464F0
SMP_DEFINE_GUID(CLSID_GabestMpegSplitter, 0xDC257063, 0x045F, 0x4BE2, 0xBD, 0x5B, 0xE1, 0x22, 0x79, 0xC4, 0x64, 0xF0);
// https://github.com/jeeb/mpc-be/blob/master/src/filters/parser/MpegSplitter/MpegSplitter.h

// DScaler Audio Decoder: {D2CA75C2-05A1-4915-88A8-D433F876D186}
SMP_DEFINE_GUID(CLSID_DScalerAudioDecoder, 0xD2CA75C2, 0x05A1, 0x4915, 0x88, 0xA8, 0xD4, 0x33, 0xF8, 0x76, 0xD1, 0x86);

// LAV Audio Decoder: {E8E73B6B-4CB3-44A4-BE99-4F7BCB96E491}
SMP_DEFINE_GUID(CLSID_LavAudioDecoder, 0xE8E73B6B, 0x4CB3, 0x44A4, 0xBE, 0x99, 0x4F, 0x7B, 0xCB, 0x96, 0xE4, 0x91);
// https://github.com/Nevcairiel/LAVFilters/blob/master/decoder/LAVAudio/LAVAudio.h

const SPLITTER_DATA sdAviSource={L"AVI/WAV File Source",true};
const SPLITTER_DATA sdAviSplitter={L"AVI Splitter",false};
const SPLITTER_DATA sdGretechAvi={L"Gretech AVI Source Filter",true};
const SPLITTER_DATA sdMpeg1Splitter={L"MPEG-I Stream Splitter",false};
const SPLITTER_DATA sdMpeg2Splitter={L"MPEG-2 Splitter",false};
const SPLITTER_DATA sdMpegSource={L"Mpeg Source",true};
const SPLITTER_DATA sdHaaliSplitter={L"Haali Media Splitter",true};
const SPLITTER_DATA sdMkvSource={L"Matroska Source",true};
const SPLITTER_DATA sdNeroSplitter={L"Nero Splitter",false};

//this module
void ShowError(HRESULT hr,ERROR_TYPE et);
void MyDeleteMediaType(AM_MEDIA_TYPE *pmt);
void MyFreeMediaType(AM_MEDIA_TYPE& mt);
BOOL ShowFilterProperties(IBaseFilter* pFilter);
BOOL CheckMediaType(IPin* pin,GUID mediatype);
IBaseFilter* FindFilter(TCHAR* FilterName);
IBaseFilter* DShow_CreateFilter(const GUID& clsid);

HRESULT GetUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin);
HRESULT FindPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    TCHAR* name,
    IPin **ppPin);

IBaseFilter* FindFileSource(IGraphBuilder* pGraph);
IBaseFilter* GetDownstreamFilter(IPin* PinOut);
IPin* GetOutputPin(    IBaseFilter *pFilter )  ;
BOOL GetSubType(IPin* pin,GUID* pGuid);
void LogAllFilters(IGraphBuilder* pGraph);

#endif