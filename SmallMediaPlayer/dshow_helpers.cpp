﻿/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include <stdint.h>
#include "dshow_helpers.h"

void LogMessage(const WCHAR* message, BOOL fTime); //from errors.cpp

IBaseFilter* DShow_CreateFilter(const GUID& clsid){
    HRESULT hr;
    IBaseFilter* ret = NULL;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&ret);

    if (FAILED(hr)) return NULL;
    else return ret;
}

IBaseFilter* FindFilter(TCHAR* FilterName){
	HRESULT hr; 
// Create the System Device Enumerator.
ICreateDevEnum *pSysDevEnum = NULL;
hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
    IID_ICreateDevEnum, (void **)&pSysDevEnum);
if (FAILED(hr))
{    return NULL;}

// Obtain a class enumerator for the dshow filters category.
IEnumMoniker *pEnumCat = NULL;
hr = pSysDevEnum->CreateClassEnumerator(CLSID_LegacyAmFilterCategory, &pEnumCat, 0);

if (hr == S_OK) 
{	
    // Enumerate the monikers.
    IMoniker *pMoniker = NULL;
    ULONG cFetched;
    while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
    {
        IPropertyBag *pPropBag;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
            (void **)&pPropBag);
        if (SUCCEEDED(hr))
        {
            // To retrieve the filter's friendly name, do the following:
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropBag->Read(L"FriendlyName", &varName, 0);
            if (SUCCEEDED(hr)){
				if(wcscmp(FilterName,varName.bstrVal)==0){
					
            // To create an instance of the filter, do the following:
            IBaseFilter *pFilter;
            hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
                (void**)&pFilter);
            // Now add the filter to the graph. 
            //Remember to release pFilter later.
			VariantClear(&varName);
			pPropBag->Release();
			pMoniker->Release();
			pEnumCat->Release();
			pSysDevEnum->Release();
			return pFilter;

				}
			}
			
            pPropBag->Release();
        }
        pMoniker->Release();
    }
    pEnumCat->Release();
}
pSysDevEnum->Release();
return NULL; 

}
void MyFreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        // Unecessary because pUnk should not be used, but safest.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}
void MyDeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != NULL)
    {
        MyFreeMediaType(*pmt); 
        CoTaskMemFree(pmt);
    }
}
void ShowError(HRESULT hr,ERROR_TYPE et){
	TCHAR buf[MAX_ERROR_TEXT_LEN]=L"";
	TCHAR message[256]=L"";

switch(et){
	case SYSTEM_ERROR:AMGetErrorTextW(hr,buf,sizeof(buf));
						  lstrcpy(message,L"Системная ошибка");break;
	case PLAY_ERROR:AMGetErrorTextW(hr,buf,sizeof(buf));
		lstrcpy(message,L"Ошибка воспроизведения");break;
	case ERROR_NOTSUPPORTED:lstrcpy(message,L"Ошибка");
		lstrcpy(buf,L"Операция не поддерживается");break;
	case ERROR_NOPROPERTIES:lstrcpy(message,L"Ошибка");
		lstrcpy(buf,L"Декодер не имеет настраиваемых свойств");break;
	
}
MessageBox(NULL,buf,message,MB_OK|MB_ICONERROR); 
}
HRESULT GetUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin)           // Receives a pointer to the pin.
{
    *ppPin = 0;
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
		
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PinDir)
        {
            IPin *pTmp = 0;
            hr = pPin->ConnectedTo(&pTmp);
            if (SUCCEEDED(hr))  // Already connected, not the pin we want.
            {
                pTmp->Release();
            }
            else  // Unconnected, this is the pin we want.
            {
                pEnum->Release();
                *ppPin = pPin;
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return E_FAIL;
}

IPin* GetOutputPin(    IBaseFilter *pFilter )          
{
    
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return NULL;
		
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PINDIR_OUTPUT)
        {
			pEnum->Release();
			return pPin;
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return NULL;
}

HRESULT FindPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    TCHAR* name,
    IPin **ppPin)           // Receives a pointer to the pin.
{
    
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
	PIN_INFO pi={0};
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
		
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
		pPin->QueryPinInfo(&pi);
		if(lstrcmp(pi.achName,name)==0){
		pEnum->Release();
		*ppPin=pPin;
		return S_OK;
		}
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return E_FAIL;
}


BOOL CheckMediaType(IPin* pin,GUID mediatype){
IEnumMediaTypes* pEnum;
AM_MEDIA_TYPE* mt=NULL;


pin->EnumMediaTypes(&pEnum);
while(pEnum->Next(1,&mt,NULL)==S_OK){
	if(mt->majortype==mediatype){
		MyDeleteMediaType(mt);
		pEnum->Release();
				return TRUE;}
	 MyDeleteMediaType(mt);
	 
}
pEnum->Release();

return FALSE;

}

BOOL GetSubType(IPin* pin,GUID* pGuid){
IEnumMediaTypes* pEnum;
AM_MEDIA_TYPE* mt=NULL;
HRESULT hr;

pin->EnumMediaTypes(&pEnum);
hr=pEnum->Next(1,&mt,NULL);
if(FAILED(hr)){pEnum->Release();return FALSE;}
	*pGuid=mt->subtype;
	MyDeleteMediaType(mt);
	pEnum->Release();
	return TRUE;
}

BOOL ShowFilterProperties(IBaseFilter* pFilter){
ISpecifyPropertyPages *pProp;
HRESULT hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
if (SUCCEEDED(hr)) 
{
    // Get the filter's name and IUnknown pointer.
    FILTER_INFO FilterInfo;
    hr = pFilter->QueryFilterInfo(&FilterInfo); 
    IUnknown *pFilterUnk;
    pFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

    // Show the page. 
    CAUUID caGUID;
    pProp->GetPages(&caGUID);
    pProp->Release();
    OleCreatePropertyFrame(
        NULL,                   // Parent window
        0, 0,                   // Reserved
        FilterInfo.achName,     // Caption for the dialog box
        1,                      // Number of objects (just the filter)
        &pFilterUnk,            // Array of object pointers. 
        caGUID.cElems,          // Number of property pages
        caGUID.pElems,          // Array of property page CLSIDs
        0,                      // Locale identifier
        0, NULL                 // Reserved
    );

    // Clean up.
    pFilterUnk->Release();
    FilterInfo.pGraph->Release(); 
    CoTaskMemFree(caGUID.pElems);
	return TRUE;
}
return FALSE;
}

IBaseFilter* GetDownstreamFilter(IPin* PinOut){
IPin* pin=NULL;

HRESULT hr;
PIN_INFO pi={0};

hr=PinOut->ConnectedTo(&pin);
if(FAILED(hr))return NULL;
hr=pin->QueryPinInfo(&pi);
if(FAILED(hr))goto end_fail;
if(pi.pFilter==NULL)goto end_fail;
return pi.pFilter;

end_fail:
if(pi.pFilter!=NULL)pi.pFilter->Release();
if(pin!=NULL)pin->Release();
return NULL;

}

IBaseFilter* FindFileSource(IGraphBuilder* pGraph) 
{
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter=NULL;
	IFileSourceFilter* pSrc=NULL;
    ULONG cFetched;

    HRESULT hr = pGraph->EnumFilters(&pEnum);
    if (FAILED(hr)) return NULL;

    while(pEnum->Next(1, &pFilter, &cFetched) == S_OK)
    {
		hr=pFilter->QueryInterface(IID_IFileSourceFilter,(void**)&pSrc);
		if(hr==E_NOINTERFACE||pSrc==NULL){
			pFilter->Release();
			continue;}
		pSrc->Release();
		pEnum->Release();
		return pFilter;
    }

    pEnum->Release();
    return NULL;
}

#ifdef DEBUG
void LogFileVersionInfo(const WCHAR* file){

    const int bufSize = 5000;
    WCHAR buf[bufSize]={0};
    BOOL res = SMP_GetFileVersionInfo(file, buf, bufSize);

    if(res!=FALSE){
        LogMessage(buf, FALSE);
    }
}

//prints all filters in graph to log file
void LogAllFilters(IGraphBuilder* pGraph) 
{
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter=NULL;
    FILTER_INFO finfo={0};
    ULONG cFetched;
    WCHAR buf[500]=L"";
    HMODULE hModule = NULL;
    WCHAR module[MAX_PATH]={0};

    HRESULT hr = pGraph->EnumFilters(&pEnum);

    if (FAILED(hr)) {
        LogMessage(L"Failed to enumerate filters",TRUE);
        return;
    }

    LogMessage(L"*** FILTERS: ***",FALSE);

    while(pEnum->Next(1, &pFilter, &cFetched) == S_OK)
    {
        //name
        hr=pFilter->QueryFilterInfo(&finfo);

        if(FAILED(hr)){
            pFilter->Release();
            LogMessage(L"Failed to query filter info",TRUE);
            continue;
        }

        StringCchCopy(buf,500,L"  ");
        StringCchCat(buf,500,finfo.achName);
        LogMessage(buf,FALSE);
        if(finfo.pGraph!=NULL)finfo.pGraph->Release();

        //vendor
        LPWSTR pVendor=NULL;
        hr=pFilter->QueryVendorInfo(&pVendor);

        if(SUCCEEDED(hr) && pVendor!=NULL){
            StringCchCopy(buf,500,L"Vendor: ");
            StringCchCat(buf,500,pVendor);
            LogMessage(buf,FALSE);
            CoTaskMemFree(pVendor);
        }

        //module path
        ZeroMemory(module, sizeof(module));
        BOOL res = SMP_GetModuleFromObject(pFilter, module, MAX_PATH);

        if(res != FALSE){
            StringCchCopy(buf,500,L"Module: ");
            StringCchCat(buf,500,module);
            LogMessage(buf,FALSE);

            //module version info
            LogFileVersionInfo(module);
        }
        
        pFilter->Release();
    }

    LogMessage(L"***************",FALSE);
    pEnum->Release();
}
#endif
