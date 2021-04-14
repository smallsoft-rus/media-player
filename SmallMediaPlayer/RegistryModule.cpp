/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "RegistryModule.h"
#include "errors.h"

WCHAR ProgramFileName[256]=L"";

BOOL RegisterFolderAssociation(){
	HKEY hKey;
	LONG lRes;
	WCHAR buf[456];
	WCHAR txt[400];
        
lstrcpy(buf,L"(SMP)Добавить в список");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,L"Folder\\shell\\smp_add",&hKey);
	if(lRes!=ERROR_SUCCESS){
		StringCchPrintf(txt,400,L"Ошибка доступа к реестру: CreateKey вернул 0x%04x",lRes);
		HandleError(txt,SMP_ALERT_BLOCKING,L"");
		return FALSE;
	}

	lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	
	RegCloseKey(hKey);
	if(lRes!=ERROR_SUCCESS){
		StringCchPrintf(txt,400,L"Ошибка записи в реестр: RegSetValueEx вернул 0x%04x",lRes);
		HandleError(txt,SMP_ALERT_BLOCKING,L"");
		return FALSE;
	}


	lstrcpy(buf,L"\"");
        lstrcat(buf,ProgramFileName);
        lstrcat(buf,L"\" /f \"%1\"");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,L"Folder\\shell\\smp_add\\command",&hKey);

	if(lRes!=ERROR_SUCCESS){
		HandleError(L"Ошибка работы с реестром",SMP_ALERT_BLOCKING,L"");
		return FALSE;
	}

	lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	RegCloseKey(hKey);
	if(lRes!=ERROR_SUCCESS)
		return FALSE;

	MessageBox(0,L"Операция выполнена!",L"Успех",MB_OK|MB_ICONINFORMATION);
        return TRUE;
}

void DestroyFolderAssociation(){

	TCHAR buf[256];
	LONG lRes;
        
	lstrcpy(buf,L"Folder\\shell\\smp_add\\command");
	lRes=RegDeleteKey(HKEY_CLASSES_ROOT,buf);

	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром",SMP_ALERT_BLOCKING,L"");}

	lstrcpy(buf,L"Folder\\shell\\smp_add");
	lRes=RegDeleteKey(HKEY_CLASSES_ROOT,buf);

	if(lRes!=ERROR_SUCCESS){
		HandleError(L"Ошибка работы с реестром",SMP_ALERT_BLOCKING,L"");
		return;
	}

    MessageBox(0,L"Операция выполнена!",L"Успех",MB_OK|MB_ICONINFORMATION);
}


void RegisterFileAssociation(WCHAR* ext){
 LONG lRes;
 HKEY hKey=NULL;
 WCHAR buf[256];
 WCHAR type_name[256];
 WCHAR key_name[256];
 DWORD size=256;
 WCHAR txt[400];

 if(lstrcmp(ext,L"")==0){MessageBox(0,L"Введите расширение",0,0);return;}

 size=sizeof(type_name);
  lstrcpy(buf,L".");
  lstrcat(buf,ext);

 lRes=RegOpenKeyW(HKEY_CLASSES_ROOT,buf,&hKey);

 if(lRes==ERROR_SUCCESS){
   lRes=RegQueryValueEx(hKey,NULL,NULL,NULL,(LPBYTE)type_name,&size);
   if(lRes!=ERROR_SUCCESS){
        lstrcpy(type_name,L"mediafile");
        lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)type_name,wcslen(type_name)*2+1);

		 if(lRes!=ERROR_SUCCESS){
			HandleError(L"Cannot set type name",SMP_ALERT_BLOCKING,L"");
			goto Cleanup;
		 }
        }
   }
 else {
 lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,buf,&hKey);
 if(lRes!=ERROR_SUCCESS){
		StringCchPrintf(txt,400,L"Ошибка доступа к реестру: RegCreateKey вернул 0x%04x",lRes);
		HandleError(txt,SMP_ALERT_BLOCKING,L"");
		goto Cleanup;
 }
  lstrcpy(type_name,L"mediafile");
  lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)type_name,wcslen(type_name)*2+1);
	if(lRes!=ERROR_SUCCESS){
		StringCchPrintf(txt,400,L"Ошибка записи в реестр: RegSetValueEx вернул 0x%04x",lRes);
		HandleError(txt,SMP_ALERT_BLOCKING,L"");
		goto Cleanup;
	}
  }

  RegCloseKey(hKey);hKey=NULL;


	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_open");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,key_name,&hKey);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Cant create smp_open key",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	lstrcpy(buf,L"Открыть в SMP");
    lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	if(lRes!=ERROR_SUCCESS){
		StringCchPrintf(txt,400,L"Ошибка записи в реестр: RegSetValueEx вернул 0x%04x",lRes);
		HandleError(txt,SMP_ALERT_BLOCKING,L"");
		goto Cleanup;
	}
	RegCloseKey(hKey);hKey=NULL;

	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_open\\command");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,key_name,&hKey);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	lstrcpy(buf,L"\"");
        lstrcat(buf,ProgramFileName);
        lstrcat(buf,L"\" \"%1\"");
	lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	RegCloseKey(hKey);hKey=NULL;

	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_add");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,key_name,&hKey);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	lstrcpy(buf,L"(SMP)Добавить в список");
   lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	RegCloseKey(hKey);hKey=NULL;

lstrcpy(key_name,type_name);
lstrcat(key_name,L"\\shell\\smp_add\\command");
lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,key_name,&hKey);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	lstrcpy(buf,L"\"");
        lstrcat(buf,ProgramFileName);
        lstrcat(buf,L"\"/a \"%1\"");
	lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	RegCloseKey(hKey);hKey=NULL;

	//set default action
	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,key_name,&hKey);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	lstrcpy(buf,L"smp_open");
        
	lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}

	MessageBox(0,L"Операция выполнена!",L"Успех",MB_OK|MB_ICONINFORMATION);

	Cleanup:;
	if(hKey!=NULL)RegCloseKey(hKey);
   }

void DestroyFileAssociation(TCHAR* ext){
LONG lRes;
 HKEY hKey=NULL;
 WCHAR buf[256];
 WCHAR type_name[256];
 WCHAR key_name[256];
 WCHAR txt[400];
 DWORD size=256;

 if(lstrcmp(ext,L"")==0){MessageBox(0,L"Введите расширение",0,0);return;}
//find type name
 size=sizeof(type_name);
  lstrcpy(buf,L".");
  lstrcat(buf,ext);
 lRes=RegOpenKeyW(HKEY_CLASSES_ROOT,buf,&hKey);
 if(lRes==ERROR_SUCCESS){
   lRes=RegQueryValueEx(hKey,NULL,NULL,NULL,(LPBYTE)type_name,&size);
   if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
   }
 else
 {
	StringCchPrintf(txt,400,L"Ошибка доступа к реестру: RegOpenKey вернул 0x%04x",lRes);
	HandleError(txt,SMP_ALERT_BLOCKING,L"");
	goto Cleanup;
 }
	RegCloseKey(hKey);hKey=NULL;

	//delete our subkey
	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_open\\command");
	RegDeleteKey(HKEY_CLASSES_ROOT,key_name);
	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_add\\command");
	RegDeleteKey(HKEY_CLASSES_ROOT,key_name);
	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_open");
	RegDeleteKey(HKEY_CLASSES_ROOT,key_name);
	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell\\smp_add");
	RegDeleteKey(HKEY_CLASSES_ROOT,key_name);

	//reset dafault action
	lstrcpy(key_name,type_name);
	lstrcat(key_name,L"\\shell");
	lRes=RegCreateKeyW(HKEY_CLASSES_ROOT,key_name,&hKey);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}

	lstrcpy(buf,L"open");        
	lRes=RegSetValueExW(hKey,NULL,0,REG_SZ,(BYTE*)buf,wcslen(buf)*2+1);
	if(lRes!=ERROR_SUCCESS){HandleError(L"Ошибка работы с реестром!",SMP_ALERT_BLOCKING,L"");goto Cleanup;}
	
	MessageBox(0,L"Операция выполнена!",L"Успех",MB_OK|MB_ICONINFORMATION);

Cleanup:
	if(hKey!=NULL)RegCloseKey(hKey);
	
}