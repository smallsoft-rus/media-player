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

// LAV Splitter: 171252A0-8820-4AFE-9DF8-5C92B2D66B04
SMP_DEFINE_GUID(CLSID_LavSplitter, 0x171252A0, 0x8820, 0x4AFE, 0x9D, 0xF8, 0x5C, 0x92, 0xB2, 0xD6, 0x6B, 0x04);
// https://github.com/Nevcairiel/LAVFilters/blob/master/common/includes/moreuuids.h

const SPLITTER_DATA sdAsfReader={SOURCE_FILTER,true};
const SPLITTER_DATA sdBassSource={L"DC-Bass Source",true};
const SPLITTER_DATA sdGretechMp3={L"Gretech MP3 Source Filter",true};
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