/* Small Media Player tests 
 * Copyright (c) 2022,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>
#include "CppUnitTest.h"
#include "PlayListEditor.h"
#include "player.h"

//imported from ui.obj
extern void InitApplication(BOOL useErrorGUI);
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
    InitApplication(FALSE);
    
    TestsMain_Initialized=true;
}

void Assert_Contains(const WCHAR* str, const WCHAR* substr){
    const WCHAR* match = wcsstr(str, substr);
    Assert::IsTrue(match != NULL);
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
            ClearPlaylist();
            AddPlaylistElement(L"c:\\test.mp3");

            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            ClearPlaylist();
            
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\test.mp3",buf,true);
        }

        TEST_METHOD(Test_ClearPlaylist)
        {
            ClearPlaylist();
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
            ClearPlaylist();
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
            ClearPlaylist();
            Playlist_AddDirectory(L"..\\SmallMediaPlayer_Tests\\data\\");
            Assert::AreEqual((UINT)5,CountTracks);

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
            Assert::AreEqual(L"..\\SmallMediaPlayer_Tests\\data\\piha.mp3",buf,true);
            res=GetPlaylistElement(4,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"..\\SmallMediaPlayer_Tests\\data\\robin.mp3",buf,true);

            ClearPlaylist();
        }

        TEST_METHOD(Test_Playlist_SaveLoad)
        {
            ClearPlaylist();
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

        TEST_METHOD(Test_SaveTextPlaylist)
        {
            ClearPlaylist();
            AddPlaylistElement(L"c:\\music\\file1.mp3");
            AddPlaylistElement(L"c:\\music\\file2.mp3");
            AddPlaylistElement(L"c:\\music\\file3.mp3");

            WCHAR filepath[]=L"playlist.m3u";
            SaveTextPlaylist(filepath);
            ClearPlaylist();

            FILE* f = _wfopen(filepath,L"rt");
            WCHAR buf[500]=L"";
            WCHAR* p=NULL;
            std::vector<std::wstring> vect;

            while(1){
                p=fgetws(buf,500,f);
                if(p==NULL)break;
                vect.push_back(std::wstring(buf));
            }
            fclose(f);
            DeleteFileW(filepath);
            
            Assert::AreEqual(std::wstring(L"#~ENCODING: UTF8\n"),vect.at(1));
            Assert::AreEqual(std::wstring(L"#TAGS~file1.mp3~ ~ ~ \n"),vect.at(2));
            Assert::AreEqual(std::wstring(L"c:\\music\\file1.mp3\n"),vect.at(3));
            Assert::AreEqual(std::wstring(L"#TAGS~file2.mp3~ ~ ~ \n"),vect.at(4));
            Assert::AreEqual(std::wstring(L"c:\\music\\file2.mp3\n"),vect.at(5));
            Assert::AreEqual(std::wstring(L"#TAGS~file3.mp3~ ~ ~ \n"),vect.at(6));
            Assert::AreEqual(std::wstring(L"c:\\music\\file3.mp3\n"),vect.at(7));
        }

        TEST_METHOD(Test_LoadTextPlaylist)
        {
            WCHAR filepath[]=L"..\\SmallMediaPlayer_Tests\\data\\playlist.m3u";
            ClearPlaylist();
            LoadTextPlaylist(filepath);

            Assert::AreEqual((UINT)3,CountTracks);
            WCHAR buf[MAX_PATH]=L"";
            BOOL res=GetPlaylistElement(0,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\file1.mp3",buf,true);
            res=GetPlaylistElement(1,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\file2.mp3",buf,true);
            res=GetPlaylistElement(2,buf);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual(L"c:\\music\\file3.mp3",buf,true);

            ClearPlaylist();
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(Test_PlayTrackByNumber)
        TEST_OWNER(L"GUI")
        END_TEST_METHOD_ATTRIBUTE()
	    
        TEST_METHOD(Test_PlayTrackByNumber)
        {
            WCHAR buf[5000]=L"";
            ClearPlaylist();
            AddPlaylistElement(L"..\\SmallMediaPlayer_Tests\\data\\robin.mp3");
            PlayTrackByNumber(0);
            Assert::AreEqual((int)PLAYING,(int)PlayerState);
            Pause();            
            Assert::AreEqual((int)PAUSED,(int)PlayerState);

            DWORD len = GetLength();
            Assert::AreEqual(2.6f,len/1000.0f,0.25f); //seconds

            GetMultimediaInfoString(buf, 5000);
            Stop();
            ClearPlaylist();

            Assert_Contains(buf, L"Формат: MPEG1 Layer 3");
        }
        
        TEST_METHOD(Test_PlayTrackByNumber_Error)
        {
            ClearPlaylist();
            AddPlaylistElement(L"dummy.txt");
            PlayTrackByNumber(0);
            int state = (int)PlayerState;
            ClearPlaylist();

            Assert::AreEqual((int)FILE_NOT_LOADED, state);            
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(Test_OpenFile_DirectShow)
        TEST_OWNER(L"GUI")
        END_TEST_METHOD_ATTRIBUTE()
	    
        TEST_METHOD(Test_OpenFile_DirectShow)
        {
            WCHAR buf[5000]=L"";
            BOOL res = Player_OpenFileCore(L"..\\SmallMediaPlayer_Tests\\data\\robin.mp3", TRUE, FALSE);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual((int)STOPPED,(int)PlayerState);
            
            DWORD len = GetLength();
            Assert::AreEqual(2.6f,len/1000.0f,0.25f); //seconds

            GetMultimediaInfoString(buf, 5000);
            Assert_Contains(buf, L"Формат: MPEG1 Layer 3");
        }
    };
}
