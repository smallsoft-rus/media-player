/* Small Media Player 
 * Copyright (c) 2022,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "common.h"
#include <stdint.h>
#include <strsafe.h>

typedef struct LANGANDCODEPAGE {
      WORD wLanguage;
      WORD wCodePage;
} STRUCT_LANGANDCODEPAGE;

BOOL SMP_GetModuleFromObject(IUnknown* pObject, WCHAR* output, DWORD cchOutput){
    HMODULE hModule = NULL;
    WCHAR module[MAX_PATH]={0};

    //get QueryInterface function address (first in vtable)
    uintptr_t* p = (uintptr_t*)(pObject);
    uintptr_t addr = *p;

    //get module from function address
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, 
        (LPCWSTR)(addr), &hModule);

    if(hModule == NULL) return FALSE;

    //get module file path
    ZeroMemory(output, sizeof(WCHAR)*cchOutput);
    DWORD dwRes = GetModuleFileName(hModule,output,cchOutput);

    if(dwRes == 0 || dwRes >= cchOutput) return FALSE;
    else return TRUE;
}

WCHAR* GetVersionEntry(void* lpData,STRUCT_LANGANDCODEPAGE *lpTranslate, UINT cbTranslate,const WCHAR* name){

    HRESULT hr;
    WCHAR SubBlock[50]=L"";
    LPVOID lpBuffer=NULL;
    UINT dwBytes=0;
    BOOL res=FALSE;

    //get version info string in the first language available
    for(UINT i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++ )
    {
      hr = StringCchPrintfW(SubBlock, 50,
            TEXT("\\StringFileInfo\\%04x%04x\\%s"),
            lpTranslate[i].wLanguage,
            lpTranslate[i].wCodePage,
            name);

  	  if (FAILED(hr)) continue;
      
      lpBuffer=NULL;
  	  res=VerQueryValueW(lpData, 
                SubBlock, 
                &lpBuffer, 
                &dwBytes);

      if(res==FALSE)continue;
      if(lpBuffer==NULL)continue;

      WCHAR* pStr=(WCHAR*)lpBuffer;
      return pStr;
  	}

    return NULL;
}

BOOL SMP_GetFileVersionInfo(const WCHAR* file, WCHAR* output, int cchOutput){

    DWORD unused=0;
    DWORD size=GetFileVersionInfoSizeW(file,&unused);
    if(size==0) return FALSE;

    void* lpData=LocalAlloc(LMEM_ZEROINIT,size);
    if(lpData==NULL) return FALSE;

    BOOL res = GetFileVersionInfoW(file,0,size,lpData);
    if(res==FALSE){LocalFree(lpData);return FALSE;}

    STRUCT_LANGANDCODEPAGE *lpTranslate=NULL;
    UINT cbTranslate=0;
    WCHAR SubBlock[50]=L"";
    LPVOID lpBuffer=NULL;
    UINT dwBytes=0;
    WCHAR buf[500]=L"";

    res=VerQueryValueW(lpData, TEXT("\\VarFileInfo\\Translation"),
              (LPVOID*)&lpTranslate,
              &cbTranslate);

    if(res==FALSE){LocalFree(lpData);return FALSE;}

    WCHAR* pDescr=GetVersionEntry(lpData,lpTranslate,cbTranslate,L"FileDescription");

    if(pDescr!=NULL){
        StringCchCat(output,cchOutput,L"Описание: ");
        StringCchCat(output,cchOutput,pDescr);
        StringCchCat(output,cchOutput,L"\r\n");
    }

    WCHAR* pName=GetVersionEntry(lpData,lpTranslate,cbTranslate,L"ProductName");

    if(pName!=NULL){
        StringCchCat(output,cchOutput,L"Название продукта: ");
        StringCchCat(output,cchOutput,pName);
        StringCchCat(output,cchOutput,L"\r\n");
    }
    
    WCHAR* pFileVersion=GetVersionEntry(lpData,lpTranslate,cbTranslate,L"FileVersion");

    if(pFileVersion!=NULL){
        StringCchCat(output,cchOutput,L"Версия файла: ");
        StringCchCat(output,cchOutput,pFileVersion);
        StringCchCat(output,cchOutput,L"\r\n");
    }
    
    WCHAR* pProductVersion=GetVersionEntry(lpData,lpTranslate,cbTranslate,L"ProductVersion");

    if(pProductVersion!=NULL){
        StringCchCat(output,cchOutput,L"Версия продукта: ");
        StringCchCat(output,cchOutput,pProductVersion);
    }

    LocalFree(lpData);
    return TRUE;
}
