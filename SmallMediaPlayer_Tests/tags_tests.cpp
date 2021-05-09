/* Small Media Player tests 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "CppUnitTest.h"
#include "tags.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

WCHAR Noise_MP3[]=L"..\\SmallMediaPlayer_Tests\\data\\noise.mp3";

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
            Assert::AreEqual(L"Шум моря",data.title);
            Assert::AreEqual(L"Gregory",data.artist);
            Assert::AreEqual(L"",data.album);
        }

    };
}