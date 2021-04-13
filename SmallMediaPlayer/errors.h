/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef ERRORS_H
#define ERRORS_H

#include <windows.h>
#include <CommCtrl.h>
#include <strsafe.h>
#include <dshow.h>

void HandleError(const WCHAR* message,const WCHAR* info);
void HandlePlayError(HRESULT hr, const WCHAR* file);

#endif
