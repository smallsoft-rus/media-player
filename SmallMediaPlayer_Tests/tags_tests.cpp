/* Small Media Player tests 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "CppUnitTest.h"
#include "tags.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

WCHAR Noise_MP3[]=L"..\\SmallMediaPlayer_Tests\\data\\noise.mp3";
WCHAR Robin_MP3[]=L"..\\SmallMediaPlayer_Tests\\data\\robin.mp3";
WCHAR Crow_FLAC[]=L"..\\SmallMediaPlayer_Tests\\data\\crow.flac";
WCHAR Horse_MP3[]=L"..\\SmallMediaPlayer_Tests\\data\\horse.mp3";

namespace SmallMediaPlayer_Tests
{		
    TEST_CLASS(TagTests)
    {
    public:
	
        TEST_METHOD(Test_ID3V1)
        {
            TAGS_GENERIC data = {0};
            BOOL res = ReadTagsV1(Noise_MP3,&data);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual((DWORD)TAG_ID3V1,(DWORD)data.type);
            Assert::AreEqual(L"Sea noise",data.title);
            Assert::AreEqual(L"Gregory",data.artist);
            Assert::AreEqual(L"",data.album);
            Assert::AreEqual(L"2007",data.year);
        }

        TEST_METHOD(Test_ID3V2)
        {
            TAGS_GENERIC data = {0};
            BOOL res = ReadTagsV2(Robin_MP3,&data,FALSE);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual((DWORD)TAG_ID3V2,(DWORD)data.type);
            Assert::AreEqual(L"1673",data.title);
            Assert::AreEqual(L"",data.artist);
            Assert::AreEqual(L"",data.album);
        }

        TEST_METHOD(Test_ID3V2_EmptyTags)
        {            
            TAGS_GENERIC data = {0};
            BOOL res = ReadTagsV2(Horse_MP3,&data,FALSE);
            Assert::IsTrue(res==FALSE);            
            Assert::AreEqual(L"",data.title);
            Assert::AreEqual(L"",data.artist);
            Assert::AreEqual(L"",data.album);
        }

        TEST_METHOD(Test_FlacVorbisComment)
        {
            TAGS_GENERIC data = {0};
            BOOL res = ReadFlacTags(Crow_FLAC,&data);
            Assert::IsTrue(res!=FALSE);
            Assert::AreEqual((DWORD)TAG_FLAC,(DWORD)data.type);
            Assert::AreEqual(L"0956",data.title);
            Assert::AreEqual(L"",data.artist);
            Assert::AreEqual(L"",data.album);
        }

    };
}
