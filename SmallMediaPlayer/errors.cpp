/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "errors.h"
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

extern void GetDataDir(WCHAR* dir,int maxcount); //SMPSettings.cpp

const int ERRORWINDOW_WIDTH = 400;
const int ERRORWINDOW_HEIGHT = 200;
const int MaxNameLen = 512;
const int MaxMessageLen = 1024;
const int MaxStackLen = 2048;

//module state
HWND hErrorWnd = NULL; //window to print errors
HWND hEdit = NULL; //edit control in error window
WCHAR strLogFile[MAX_PATH]=L"";
BOOL fLogFileInitialized=FALSE;
BOOL fEnableGUI=FALSE;

void CreateErrorWindow();

LRESULT CALLBACK ErrorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {			

	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		break;

	default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;

}

// Append message to the beginning of the current text in Errors window
//mes - pointer to the wide character string
//len - length of the string, not including null terminator
void AddErrorMessage(const WCHAR* mes) {
	CreateErrorWindow();

	const int size = 5000;
	WCHAR buf[size] = L"";
	StringCchCopy(buf, size, mes);
	StringCchCatN(buf, size, L"\r\n", 2);

	WCHAR text[size] = L"";
	int n=GetWindowTextW(hEdit, text, size);
	StringCchCatN(buf, size, text, n);
	
	SetWindowTextW(hEdit, buf);	
}

void WriteUtf8String(HANDLE hFile, const WCHAR* str){
    int c=0;
    char* utf8_string=NULL;
    DWORD dwCount=0;

    //get buffer size
    c=WideCharToMultiByte(CP_UTF8,0,str,-1,NULL,0,NULL,NULL);

    if(c<=1)return;

    //allocate buffer
    utf8_string=(char*)LocalAlloc(LMEM_ZEROINIT,c);
    if(utf8_string==NULL)return;

    //convert to UTF-8
    c=WideCharToMultiByte(CP_UTF8,0,str,-1,utf8_string,c,NULL,NULL);
    c--;//null terminator
    
    WriteFile(hFile,utf8_string,c,&dwCount,NULL);
    WriteFile(hFile,"\r\n",2,&dwCount,NULL);
    LocalFree(utf8_string);
}

//Gets filename from full path
WCHAR* FileNameFromPathW(WCHAR* path){

    int i;WCHAR* p;

    size_t len = lstrlenW(path);
    if(len<=3)return path;

    i = len - 2;
    p = path;

    while(TRUE){
        if(path[i] == '\\' || path[i] == '/') {p = &(path[i+1]);break;}
        i--;
        if(i<0)break;
    }

    return p;
}

//Gets filename from full path (ANSI)
char* FileNameFromPathA(char* path){

    int i;char* p;

    size_t len = lstrlenA(path);
    if(len<=3)return path;

    i = len - 2;
    p = path;

    while(TRUE){
        if(path[i] == '\\' || path[i] == '/') {p = &(path[i+1]);break;}
        i--;
        if(i<0)break;
    }

    return p;
}

//Prints stack trace based on context record
void PrintStackA( CONTEXT* ctx , char* dest, size_t cch) 
{
    //from https://github.com/MSDN-WhiteKnight/ErrLib/blob/master/ErrLib/ErrLib.cpp
    BOOL    result = FALSE;
    HANDLE  process= NULL;
    HANDLE  thread= NULL;
    HMODULE hModule= NULL;

    STACKFRAME64        stack;
    ULONG               frame=0;
    DWORD64             displacement=0;
    IMAGEHLP_LINE64 *line = NULL;
    DWORD disp = 0;

    char buffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(WCHAR)]={0};
    char name[MaxNameLen]={0};
    char module[MaxNameLen]={0};
    char* module_short = NULL;
    char strbuf[MAX_SYM_NAME*3]={0};
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    // Context record could be modified by StackWalk64, which causes crashes on x64 when the context comes from the
    // SEH exception information. So we create a copy here to prevent it.
    // https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64
    // https://github.com/MSDN-WhiteKnight/ErrLib/issues/2
    CONTEXT ctxCopy;
    memcpy(&ctxCopy, ctx, sizeof(CONTEXT)); 

    StringCchCopyA(dest,cch,"");
    memset( &stack, 0, sizeof( STACKFRAME64 ) );

    process                = GetCurrentProcess();
    thread                 = GetCurrentThread();
    displacement           = 0;
#if !defined(_M_AMD64)
    stack.AddrPC.Offset    = (*ctx).Eip;
    stack.AddrPC.Mode      = AddrModeFlat;
    stack.AddrStack.Offset = (*ctx).Esp;
    stack.AddrStack.Mode   = AddrModeFlat;
    stack.AddrFrame.Offset = (*ctx).Ebp;
    stack.AddrFrame.Mode   = AddrModeFlat;
#endif

    for( frame = 0; ; frame++ )
    {
        //get next call from stack
        result = StackWalk64
        (
#if defined(_M_AMD64)
            IMAGE_FILE_MACHINE_AMD64
#else
            IMAGE_FILE_MACHINE_I386
#endif
            ,
            process,
            thread,
            &stack,
            &ctxCopy,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if( !result ) break;

        //get symbol name for address
        ZeroMemory(buffer,sizeof(buffer));
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        BOOL symbolFound = SymFromAddr(process, ( ULONG64 )stack.AddrPC.Offset, &displacement, pSymbol);
        if(symbolFound == FALSE){ //name not available, output address instead
               StringCchPrintfA(pSymbol->Name,MAX_SYM_NAME,"0x%llx",(DWORD64)stack.AddrPC.Offset);
        }

        line = (IMAGEHLP_LINE64 *)LocalAlloc(LMEM_ZEROINIT, sizeof(IMAGEHLP_LINE64));
        ZeroMemory(line,sizeof(IMAGEHLP_LINE64));
        line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        //get module name
        hModule = NULL;
        lstrcpyA(module,"");
        module_short=&(module[0]);

        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, 
                (LPCTSTR)(stack.AddrPC.Offset), &hModule);

        if(hModule != NULL){
                GetModuleFileNameA(hModule,module,MaxNameLen);
                module_short = FileNameFromPathA(module);
        }

        //try to get line
        BOOL lineinfoFound = FALSE;

        if (symbolFound != FALSE) {
            // Only try to find line info when symbol is found - fixes crash when Win7 DbgHelp reads PDB symbols 
            // built with /DEBUG:FASTLINK option
            // (https://github.com/MSDN-WhiteKnight/ErrLib/issues/2)
            lineinfoFound = SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, line);
        }

        if (lineinfoFound != FALSE)
        {
            char* shortname="";
            shortname=FileNameFromPathA(line->FileName);

            StringCchPrintfA(strbuf,sizeof(strbuf)/sizeof(WCHAR),
                                "  in %s!%s + 0x%02llx (%s: %lu)\n", module_short,pSymbol->Name, 
                                displacement,shortname, line->LineNumber
                                );

            StringCchCatA(dest,cch,strbuf);
        }
        else{
            //failed to get line, output address instead
            StringCchPrintfA(strbuf,sizeof(strbuf)/sizeof(WCHAR),
                                "  in %s!%s + 0x%02llx (address: 0x%llx)\n", module_short,pSymbol->Name, 
                                displacement,(DWORD64)(stack.AddrPC.Offset - pSymbol->ModBase));
            StringCchCatA(dest,cch,strbuf);
        }
        
        LocalFree(line);
        line = NULL;
        if(frame > 9999)break;
    }//end for
}

void HandleError(
    const WCHAR* message,SMP_ALERTTYPE alerttype, const WCHAR* info
    ){ //export
    HandleError(message,alerttype,info,NULL);
}

void HandleError(
    const WCHAR* message,SMP_ALERTTYPE alerttype, const WCHAR* info, CONTEXT* pContext
    ){ //export

    InitErrorHandler(TRUE);

    //write error to log file
    HANDLE hFile=CreateFile(
        strLogFile,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL
    );

    if(hFile!=INVALID_HANDLE_VALUE){
        DWORD dwRes=SetFilePointer(hFile,0,0,FILE_END);
        if(dwRes==INVALID_SET_FILE_POINTER )MessageBox(0,L"INVALID_SET_FILE_POINTER",0,0);

        //time
        SYSTEMTIME st={0};
        GetLocalTime(&st);
        char time_str[1024]="";

        StringCchPrintfA(
			time_str,sizeof(time_str),"%4d.%02d.%02d %2d:%02d",
			(int)st.wYear,(int)st.wMonth,(int)st.wDay,(int)st.wHour,(int)st.wMinute
        );

        DWORD dwCount=0;
        WriteFile(hFile,time_str,lstrlenA(time_str),&dwCount,NULL);
        WriteFile(hFile,"\r\n",2,&dwCount,NULL);

        //message
        WriteUtf8String(hFile,message);
        BOOL write_info=FALSE;

        //extra info
        if(info!=NULL){
			if(lstrlenW(info)>0)write_info=TRUE;
        }

        if(write_info!=FALSE){
            WriteUtf8String(hFile,info);
            WriteFile(hFile,"\r\n",2,&dwCount,NULL);
        }

#ifdef DEBUG
        //stack trace
        char stack[MaxStackLen]="";
        CONTEXT* p;
        CONTEXT ctx={0};

        if(pContext!=NULL){
            p=pContext;
        }
        else {
            RtlCaptureContext(&ctx);
            p=&ctx;
        }
        
        PrintStackA(p,stack,MaxStackLen);
        WriteFile(hFile,stack,lstrlenA(stack),&dwCount,NULL);
#endif

        WriteFile(hFile,"\r\n",2,&dwCount,NULL);
        CloseHandle(hFile);
    }

    //show error
    if(fEnableGUI != FALSE){
	    switch(alerttype){
	    case SMP_ALERT_BLOCKING:
		    MessageBoxW(NULL,message,NULL,MB_OK|MB_ICONERROR);
		    break;
	    case SMP_ALERT_SILENT:
		    break;
	    default: //non-blocking
		    AddErrorMessage(message);
	    }
    }
}

//writes message into log file, but does not show UI or write stack trace
void LogMessage(
    const WCHAR* message, BOOL fTime
    ){ //export

    InitErrorHandler(TRUE);

    //write error to log file
    HANDLE hFile=CreateFile(
        strLogFile,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL
    );

    if(hFile!=INVALID_HANDLE_VALUE){
        DWORD dwRes=SetFilePointer(hFile,0,0,FILE_END);
        DWORD dwCount=0;
        
        //time
        if(fTime!=FALSE){
            SYSTEMTIME st={0};
            GetLocalTime(&st);
            char time_str[1024]="";

            StringCchPrintfA(
			    time_str,sizeof(time_str),"%4d.%02d.%02d %2d:%02d",
			    (int)st.wYear,(int)st.wMonth,(int)st.wDay,(int)st.wHour,(int)st.wMinute
            );
            
            WriteFile(hFile,"\r\n",2,&dwCount,NULL);
            WriteFile(hFile,time_str,lstrlenA(time_str),&dwCount,NULL);
            WriteFile(hFile,"\r\n",2,&dwCount,NULL);
        }

        //message
        WriteUtf8String(hFile,message);
        CloseHandle(hFile);
    }
}

void HandleDirectShowError(const WCHAR* text, HRESULT hr, const WCHAR* file){ //export
	WCHAR buf[100]=L"";
	WCHAR mes[MAX_ERROR_TEXT_LEN]=L"";
	const int output_len=1500;
	WCHAR output[output_len]=L"";

	//get directshow error message
	AMGetErrorTextW(hr,mes,sizeof(mes));

	StringCchCopyW(output,output_len,text);
	StringCchPrintfW(buf,100,L" 0x%x: ",(UINT)hr);
	StringCchCatW(output,output_len,buf);
	StringCchCatW(output,output_len,mes);

    if(lstrlen(file)>0){
	    StringCchCatW(output,output_len,L" - ");
	    StringCchCatW(output,output_len,file);
    }
	
	HandleError(output,SMP_ALERT_NONBLOCKING,L"");
}

void HandlePlayError(HRESULT hr, const WCHAR* file){ //export
    HandleDirectShowError(L"Ошибка воспроизведения", hr, file);
}

void HandleMfError(HRESULT hr, const WCHAR* pszErrorMessage, const WCHAR* file){ //export

    const size_t MESSAGE_LEN = 512;
    WCHAR message[MESSAGE_LEN]=L"";
    StringCchPrintf(message, MESSAGE_LEN, L"%s (HRESULT = 0x%X)", 
        pszErrorMessage, hr);
    	
	const int output_len=1000;
	WCHAR output[output_len]=L"";

	StringCchCopyW(output,output_len,L"Ошибка Media Foundation. ");	
	StringCchCatW(output,output_len,message);
	StringCchCatW(output,output_len,L" - ");
	StringCchCatW(output,output_len,file);
	
	HandleError(output,SMP_ALERT_NONBLOCKING,L"");
}

void HandleMediaError(HRESULT hr){ //export
	WCHAR buf[100]=L"";
	WCHAR mes[MAX_ERROR_TEXT_LEN]=L"";
	const int output_len=1000;
	WCHAR output[output_len]=L"";

	//get directshow error message
	AMGetErrorTextW(hr,mes,sizeof(mes));

	StringCchCopyW(output,output_len,L"Ошибка DirectShow ");
	StringCchPrintfW(buf,100,L"0x%x: ",(UINT)hr);
	StringCchCatW(output,output_len,buf);
	StringCchCatW(output,output_len,mes);	
	
	HandleError(output,SMP_ALERT_SILENT,L"");
}

//called on unhandled exception
LONG WINAPI MyUnhandledExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo){

    WCHAR mes[MaxMessageLen]={0};

    //Ignore Ctrl+C
    if(ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_CONTROL_C_EXIT) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    //show and log error
    StringCchPrintfW(mes,MaxMessageLen,L"Exception 0x%x",ExceptionInfo->ExceptionRecord->ExceptionCode);
    CONTEXT* pContext= ExceptionInfo->ContextRecord;
    HandleError(mes,SMP_ALERT_BLOCKING,L"",pContext);

    return EXCEPTION_CONTINUE_SEARCH;
}

void InitErrorHandler(BOOL enableGUI){ //export

    if(fLogFileInitialized!=FALSE)return;

    fEnableGUI = enableGUI;

    //construct log file path
    GetDataDir(strLogFile,MAX_PATH);
    StringCchCat(strLogFile,MAX_PATH,L"error.log");

#ifdef DEBUG
    //initialize symbol handler
    SymInitialize( GetCurrentProcess(), NULL, TRUE );
#endif
    
    //set global unhandled exception filter which will be called without debugger's precense
    SetUnhandledExceptionFilter(&MyUnhandledExceptionFilter);

    fLogFileInitialized=TRUE;
}

void CreateErrorWindow() {

	if (hErrorWnd != NULL) {
		//if window is already created, make it visible
		ShowWindow(hErrorWnd, SW_SHOW);
		SetForegroundWindow(hErrorWnd);
		return;
	}

	wchar_t wclass_name[] = L"ErrorWindow"; 
	
	//create wclass structure
	WNDCLASSEX wc;
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = ErrorWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = wclass_name;
	wc.hIcon = NULL;
	wc.hIconSm = NULL;

	//register class
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Fail to register ErrorWindow class", L"Error", MB_ICONEXCLAMATION | MB_OK);
		ExitProcess(1);
	}

	wchar_t str[] = L"";

	//create window
	hErrorWnd = CreateWindowEx(
		0, wclass_name, L"Ошибки", WS_VISIBLE|WS_CAPTION|WS_SYSMENU, 100, 100,
		ERRORWINDOW_WIDTH, ERRORWINDOW_HEIGHT, NULL, NULL, GetModuleHandle(NULL), NULL
	);

	if (hErrorWnd == NULL)
	{
		MessageBox(NULL, L"Fail to create window", L"Error", MB_ICONEXCLAMATION | MB_OK);
		ExitProcess(1);
	}

	hEdit = CreateWindowEx(
		0, WC_EDIT, str, WS_VISIBLE|WS_CHILD|WS_VSCROLL|ES_READONLY|ES_MULTILINE, 5, 5,
		ERRORWINDOW_WIDTH-20, ERRORWINDOW_HEIGHT-50, hErrorWnd, NULL, GetModuleHandle(NULL), NULL
	);

	MessageBeep(MB_ICONERROR);

}
