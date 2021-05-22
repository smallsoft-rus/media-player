/* Small Media Player tests 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "CppUnitTest.h"
#include "PlayListEditor.h"

//imported from ui.obj
extern void InitApplication();
extern void RunMessageLoop();
extern void UnloadApplication();
extern void InitResources(HMODULE h);

//imported from RegistryModule.obj
extern void Init_ProgramFileName();

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

DWORD TestsThreadStart_ThreadID=0;

DWORD WINAPI TestsThreadStart(LPVOID lpThreadParameter){

    //background thread that processes windows messages so we can test 
    //things that depend on UI

    RunMessageLoop();
    UnloadApplication();
    return 0;
}

//Entry point for test app instance
void TestsMain(){

    //partial initialization
    Init_ProgramFileName();
    //tests must fetch resource from DLL instead of EXE
    InitResources(GetModuleHandle(L"SmallMediaPlayer_Tests.dll"));
    InitApplication();

    //run background thread with message loop    
    HANDLE hThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)TestsThreadStart,NULL,
        THREAD_PRIORITY_NORMAL,&TestsThreadStart_ThreadID
        );
    
}

namespace SmallMediaPlayer_Tests
{		
    TEST_CLASS(PlaylistTests)
    {
    public:

        PlaylistTests(){
            //run test application instance
            TestsMain();
        }

        ~PlaylistTests(){
            PostThreadMessage(TestsThreadStart_ThreadID,WM_QUIT,0,0);
        }
	
        TEST_METHOD(Test_AddPlaylistElement)
        {
            AddPlaylistElement(L"c:\\test.mp3");

            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\test.mp3",buf,true);
        }

    };
}