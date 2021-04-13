/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include "errors.h"

const int ERRORWINDOW_WIDTH = 400;
const int ERRORWINDOW_HEIGHT = 200;

//module state
HWND hErrorWnd = NULL; //window to print errors
HWND hEdit = NULL; //edit control in error window

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

void HandleError(const WCHAR* message,const WCHAR* info){ //export
	AddErrorMessage(message);
}

void HandlePlayError(HRESULT hr, const WCHAR* file){ //export
	WCHAR buf[100]=L"";
	WCHAR mes[MAX_ERROR_TEXT_LEN]=L"";
	const int output_len=1000;
	WCHAR output[output_len]=L"";

	//get directshow error message
	AMGetErrorTextW(hr,mes,sizeof(mes));

	StringCchCopyW(output,output_len,L"Ошибка воспроизведения ");
	StringCchPrintfW(buf,100,L"0x%x: ",(UINT)hr);
	StringCchCatW(output,output_len,buf);
	StringCchCatW(output,output_len,mes);
	StringCchCatW(output,output_len,L" - ");
	StringCchCatW(output,output_len,file);
	
	HandleError(output,L"");
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
