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
    //tests must fetch resources from DLL instead of EXE
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
            ClearPlaylist();
            
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\test.mp3",buf,true);
        }

        TEST_METHOD(Test_ClearPlaylist)
        {
            AddPlaylistElement(L"c:\\music\\test.mp3");
            AddPlaylistElement(L"c:\\music\\test2.mp3");
            AddPlaylistElement(L"c:\\music\\test3.mp3");
            UINT count_before = CountTracks;
            ClearPlaylist();
            UINT count_after = CountTracks;
            
            Assert::AreEqual((UINT)3,count_before);
            Assert::AreEqual((UINT)0,count_after);

            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            Assert::IsTrue(res==FALSE);
        }

        TEST_METHOD(Test_DeletePlaylistElement)
        {
            AddPlaylistElement(L"c:\\music\\test.mp3");
            AddPlaylistElement(L"c:\\music\\test2.mp3");
            AddPlaylistElement(L"c:\\music\\test3.mp3");
            UINT count_before = CountTracks;
            DeletePlaylistElement(1);
            UINT count_after = CountTracks;
            
            Assert::AreEqual((UINT)3,count_before);
            Assert::AreEqual((UINT)2,count_after);

            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\test.mp3",buf,true);
            res=GetPlaylistElement(1,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\test3.mp3",buf,true);

            ClearPlaylist();
        }

        TEST_METHOD(Test_AddDirectory)
        {
            Playlist_AddDirectory(L"..\\SmallMediaPlayer_Tests\\data\\");
            Assert::AreEqual((UINT)4,CountTracks);

            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"..\\SmallMediaPlayer_Tests\\data\\crow.flac",buf,true);
            res=GetPlaylistElement(1,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"..\\SmallMediaPlayer_Tests\\data\\horse.mp3",buf,true);
            res=GetPlaylistElement(2,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"..\\SmallMediaPlayer_Tests\\data\\noise.mp3",buf,true);
            res=GetPlaylistElement(3,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"..\\SmallMediaPlayer_Tests\\data\\robin.mp3",buf,true);

            ClearPlaylist();
        }

        TEST_METHOD(Test_Playlist_SaveLoad)
        {
            AddPlaylistElement(L"c:\\music\\test.mp3");
            AddPlaylistElement(L"c:\\music\\test2.mp3");
            AddPlaylistElement(L"c:\\music\\test3.mp3");

            WCHAR filepath[]=L"file.m3u";
            SaveTextPlaylist(filepath);
            ClearPlaylist();
            LoadTextPlaylist(filepath);

            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\test.mp3",buf,true);
            res=GetPlaylistElement(1,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\test2.mp3",buf,true);
            res=GetPlaylistElement(2,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\test3.mp3",buf,true);

            ClearPlaylist();
            DeleteFileW(filepath);
        }
    };
}
