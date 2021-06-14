/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef DSHOW_HELPERS_H
#define DSHOW_HELPERS_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <dshow.h>

#define SOURCE_FILTER L"WM ASF Reader"

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

const SPLITTER_DATA sdAsfReader={SOURCE_FILTER,true};
const SPLITTER_DATA sdBassSource={L"DC-Bass Source",true};
const SPLITTER_DATA sdGretechMp3={L"Gretech MP3 Source Filter",true};
const SPLITTER_DATA sdAviSource={L"AVI/WAV File Source",true};
const SPLITTER_DATA sdAviSplitter={L"AVI Splitter",false};
const SPLITTER_DATA sdGretechAvi={L"Gretech AVI Source Filter",true};
const SPLITTER_DATA sdMpeg1Splitter={L"MPEG-I Stream Splitter",false};
const SPLITTER_DATA sdMpeg2Splitter={L"MPEG-2 Splitter",false};
const SPLITTER_DATA sdMpegSource={L"Mpeg Source",true};
const SPLITTER_DATA sdMpegSrcGabest={L"MPC - Mpeg Source (Gabest)",true};
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