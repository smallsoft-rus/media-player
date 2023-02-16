/* Small Media Player 
 * Copyright (c) 2023,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include <stdio.h>
#include <strsafe.h>
#include "tags.h"

int n_processed = 0;
int n_tagsV1 = 0;
int n_tagsV2 = 0;

void StressTestsImpl(WCHAR* filePath){
    
    TAGS_GENERIC data = {0};
    BOOL res = ReadTagsV1(filePath,&data);

    if(res!=FALSE) n_tagsV1++;

    res = ReadTagsV2(filePath,&data,TRUE);
    TagsFree(&data);

    if(res!=FALSE) n_tagsV2++;

    n_processed++;
}

void EnsureTrailingBackslash(WCHAR* str, int cch){
    int len = lstrlen(str);

    if(len<1) return;

    if(str[len-1]!='\\') StringCchCat(str, cch, L"\\");
}

void WalkFilesInDirectory(const WCHAR* dir, const WCHAR* ext){

    WCHAR mask[1024]=L"";
    WCHAR fname[1024]=L"";
    HANDLE hSearch=NULL;
    WIN32_FIND_DATAW fd={0};
		
    StringCchCopy(mask,1024,dir);
    StringCchCat(mask,1024,L"*.");
    StringCchCat(mask,1024,ext);
    
    //find files
    hSearch=FindFirstFileW(mask,&fd);

    if(hSearch==INVALID_HANDLE_VALUE) return;

    StringCchCopy(fname,1024,dir);
    StringCchCat(fname,1024,fd.cFileName);
    StressTestsImpl(fname);

    while(FindNextFileW(hSearch,&fd)){
        StringCchCopy(fname,1024,dir);
        StringCchCat(fname,1024,fd.cFileName);
        StressTestsImpl(fname);
    }

    FindClose(hSearch);
}

void WalkDirectory(const WCHAR* inputDir){

    WCHAR dir[1024]=L"";
    WCHAR mask[1024]=L"";
    WCHAR fname[1024]=L"";
    HANDLE hSearch=NULL;
    WIN32_FIND_DATAW fd={0};

    StringCchCopy(dir,1024,inputDir);
    EnsureTrailingBackslash(dir,1024);
    WalkFilesInDirectory(dir, L"mp3");

    //find subdirectories
    StringCchCopy(mask,1024,dir);
    StringCchCat(mask,1024,L"*");
    hSearch=FindFirstFileW(mask,&fd);

    if(hSearch==INVALID_HANDLE_VALUE) return;
    
    while(1){
        if(lstrcmp(fd.cFileName,L".")!=0 && lstrcmp(fd.cFileName,L"..")!=0 && (fd.dwFileAttributes & (DWORD)FILE_ATTRIBUTE_DIRECTORY)!=0){
            StringCchCopy(fname,1024,dir);
            StringCchCat(fname,1024,fd.cFileName);
            WalkDirectory(fname);
        }

        if(FindNextFileW(hSearch,&fd) == FALSE) break;
    }

    FindClose(hSearch);
}

void RunStressTests(const WCHAR* inputDir){
    
    wprintf(L"Starting Small Media Player stress tests...\n");
    wprintf(L"Directory: %s\n\n", inputDir);
    DWORD t1 = GetTickCount();
    WalkDirectory(inputDir);
    DWORD t2 = GetTickCount();
    printf("\n *** Finished ***\n");
    printf("Files processed:       %d\n", n_processed);
    printf("Files with ID3v1 tags: %d\n", n_tagsV1);
    printf("Files with ID3v2 tags: %d\n", n_tagsV2);
    printf("Execution time, s:     %.2f\n", (t2-t1)/1000.0f);
}

int wmain(int argc, WCHAR* argv[])
{
    if(argc<2){
        printf("Error: Target directory is not specified!\n");
        return 1;
    }

    RunStressTests(argv[1]);
    return 0;
}

