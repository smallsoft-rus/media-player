/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "\
"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/SECTION:.text,EWR")

#include <Windows.h>

//imported from ui.cpp
extern void InitApplication(BOOL useErrorGUI);
extern void RunMessageLoop();
extern void UnloadApplication();
extern void EnsureSingleInstance();
extern void ShowUI();
extern void InitPlayerState();
extern void InitResources(HMODULE h);

//imported from SMPSettings.cpp
extern void LoadSettings();

//imported from RegistryModule.cpp
extern void Init_ProgramFileName();

// Entry point
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR pstrCmdLine, int nCmdShow){

    EnsureSingleInstance();
    Init_ProgramFileName();
    LoadSettings();

    //resource module handle is for EXE in real app
    InitResources(GetModuleHandle(NULL));

    InitApplication(TRUE);
    InitPlayerState();
    ShowUI();
    RunMessageLoop();
    UnloadApplication();

}
