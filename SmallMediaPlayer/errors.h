/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef ERRORS_H
#define ERRORS_H

#include <windows.h>
#include <CommCtrl.h>
#include <strsafe.h>
#include <dshow.h>

typedef int SMP_ALERTTYPE;
const int SMP_ALERT_NONBLOCKING = 0; //modeless errors window
const int SMP_ALERT_BLOCKING = 1;    //modal message box
const int SMP_ALERT_SILENT = 2;      //no visible UI

void InitErrorHandler();
void HandleError(const WCHAR* message,SMP_ALERTTYPE alerttype, const WCHAR* info);
void HandleError(const WCHAR* message,SMP_ALERTTYPE alerttype, const WCHAR* info, CONTEXT* pContext);
void HandlePlayError(HRESULT hr, const WCHAR* file);
void HandleMediaError(HRESULT hr);
void LogMessage(const WCHAR* message, BOOL fTime);

#endif
