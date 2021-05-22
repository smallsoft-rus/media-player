/* Small Media Player tests 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "CppUnitTest.h"
#include "PlayListEditor.h"

//imported from ui.obj
extern void InitApplication();
extern void InitResources(HMODULE h);

//imported from RegistryModule.obj
extern void Init_ProgramFileName();

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

bool TestsMain_Initialized=false;

//Entry point for test app instance
void TestsMain(){

    if(TestsMain_Initialized)return;

    //partial initialization
    Init_ProgramFileName();
    //tests must fetch resource from DLL instead of EXE
    InitResources(GetModuleHandle(L"SmallMediaPlayer_Tests.dll"));
    InitApplication();
    
    TestsMain_Initialized=true;
}

namespace SmallMediaPlayer_Tests
{		
    TEST_CLASS(PlaylistTests)
    {
    public:

        PlaylistTests(){
            //init test application instance
            TestsMain();
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