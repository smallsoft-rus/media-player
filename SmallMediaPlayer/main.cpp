/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "\
"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/SECTION:.text,EWR")
#pragma comment(linker,"/ENTRY:New_WinMain")

//imported from ui.cpp
extern void InitApplication();
extern void RunMessageLoop();
extern void UnloadApplication();
extern void EnsureSingleInstance();
extern void ShowUI();

// Entry point
void New_WinMain(void){

    EnsureSingleInstance();
    InitApplication();
    ShowUI();
    RunMessageLoop();
    UnloadApplication();

}
