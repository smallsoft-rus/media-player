/* Small Media Player tests 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "CppUnitTest.h"
#include "player.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SmallMediaPlayer_Tests
{		
    TEST_CLASS(PlayerTests)
    {
    public:
	
        TEST_METHOD(Test_GetAudioFormatString)
        {
            const WCHAR* str = Player_GetAudioFormatString(WAVE_FORMAT_PCM, FALSE);
            Assert::AreEqual(L"PCM", str);
            str = Player_GetAudioFormatString(WAVE_FORMAT_PCM, TRUE);
            Assert::AreEqual(L"PCM Waveform Audio", str);

            str = Player_GetAudioFormatString(WAVE_FORMAT_MPEGLAYER3, FALSE);
            Assert::AreEqual(L"MP3", str);
            str = Player_GetAudioFormatString(WAVE_FORMAT_MPEGLAYER3, TRUE);
            Assert::AreEqual(L"MPEG1 Layer 3", str);

            str = Player_GetAudioFormatString(0xA200, FALSE);
            Assert::AreEqual(L"неизвестно", str);
        }

        TEST_METHOD(Test_GetFileFormatString)
        {
            const WCHAR* str = Player_GetFileFormatString(STREAM_AVI, FALSE);
            Assert::AreEqual(L"AVI", str);
            str = Player_GetFileFormatString(STREAM_AVI, TRUE);
            Assert::AreEqual(L"Audio-Video Interleaved", str);

            str = Player_GetFileFormatString(STREAM_MPEG2, FALSE);
            Assert::AreEqual(L"MPEG2", str);
            str = Player_GetFileFormatString(STREAM_MPEG2, TRUE);
            Assert::AreEqual(L"MPEG2", str);

            str = Player_GetFileFormatString((SMP_STREAM)0xFFFF, FALSE);
            Assert::AreEqual(L"неизвестно", str);
        }
    };
}
