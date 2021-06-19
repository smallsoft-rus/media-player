/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef PLAYER_MF_H
#define PLAYER_MF_H
#include <stdio.h>
#include <shobjidl.h> 
#include <shlwapi.h>
#include <strsafe.h>
#include <new>

// Media Foundation headers
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr.h>

#include "common.h"

#define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

MY_DEFINE_GUID(MFMediaType_Audio,
0x73647561, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
MY_DEFINE_GUID(MFMediaType_Video,
0x73646976, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
MY_DEFINE_GUID(MF_EVENT_TOPOLOGY_STATUS,
0x30c5018d, 0x9a53, 0x454b, 0xad, 0x9e, 0x6d, 0x5f, 0x8f, 0xa7, 0xc4, 0x3b);
MY_DEFINE_GUID(MR_VIDEO_RENDER_SERVICE, 
    0x1092a86c, 
    0xab1a, 
    0x459a, 
    0xa3, 0x36, 0x83, 0x1f, 0xbc, 0x4d, 0x11, 0xff 
);



template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}


enum MF_PlayerState
{
    Closed = 0,     // No session.
    Ready,          // Session was created, ready to open a file. 
    OpenPending,    // Session is opening a file.
    Started,        // Session is playing a file.
    Paused,         // Session is paused.
    Stopped,        // Session is stopped (ready to play). 
    Closing         // Application has closed the session, but is waiting for MESessionClosed.
};

HRESULT CreateMediaSource(PCWSTR pszURL, IMFMediaSource **ppSource);

HRESULT CreatePlaybackTopology(IMFMediaSource *pSource, 
    IMFPresentationDescriptor *pPD, HWND hVideoWnd,IMFTopology **ppTopology);

class MfPlayer : public IMFAsyncCallback
{
public:
    static HRESULT CreateInstance(HWND hVideo, HWND hEvent, MfPlayer **ppPlayer);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFAsyncCallback methods
    STDMETHODIMP  GetParameters(DWORD*, DWORD*)
    {
        // Implementation of this method is optional.
        return E_NOTIMPL;
    }
    STDMETHODIMP  Invoke(IMFAsyncResult* pAsyncResult);

    // Playback
    HRESULT       OpenURL(const WCHAR *sURL);
    HRESULT       Play();
    HRESULT       Pause();
    HRESULT       Stop();
    HRESULT       Shutdown();
    HRESULT       HandleEvent(UINT_PTR pUnkPtr);
    MF_PlayerState   GetState() const { return m_state; }

    // Video functionality
    HRESULT       Repaint();
    HRESULT       ResizeVideo(WORD width, WORD height);
    
    BOOL          HasVideo() const { return (m_pVideoDisplay != NULL);  }

protected:
    
    // Constructor is private. Use static CreateInstance method to instantiate.
    MfPlayer(HWND hVideo, HWND hEvent);

    // Destructor is private. Caller should call Release.
    virtual ~MfPlayer(); 

    HRESULT Initialize();
    HRESULT CreateSession();
    HRESULT CloseSession();
    HRESULT StartPlayback();

    // Media event handlers
    virtual HRESULT OnTopologyStatus(IMFMediaEvent *pEvent);
    virtual HRESULT OnPresentationEnded(IMFMediaEvent *pEvent);
    virtual HRESULT OnNewPresentation(IMFMediaEvent *pEvent);

    // Override to handle additional session events.
    virtual HRESULT OnSessionEvent(IMFMediaEvent*, MediaEventType) 
    { 
        return S_OK; 
    }

protected:
    long                    m_nRefCount;        // Reference count.

    IMFMediaSession         *m_pSession;
    IMFMediaSource          *m_pSource;
    IMFVideoDisplayControl  *m_pVideoDisplay;

    HWND                    m_hwndVideo;        // Video window.
    HWND                    m_hwndEvent;        // App window to receive events.
    MF_PlayerState             m_state;            // Current state of the media session.
    HANDLE                  m_hCloseEvent;      // Event to wait on while closing.

    bool m_fDelayedPause;
};

template <class Q>
HRESULT GetEventObject(IMFMediaEvent *pEvent, Q **ppObject)
{
    *ppObject = NULL;   // zero output

    PROPVARIANT var;
    HRESULT hr = pEvent->GetValue(&var);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_UNKNOWN)
        {
            hr = var.punkVal->QueryInterface(ppObject);
        }
        else
        {
            hr = MF_E_INVALIDTYPE;
        }
        PropVariantClear(&var);
    }
    return hr;
}

extern MfPlayer *g_pPlayer;

HRESULT CreateMediaSource(PCWSTR pszURL, IMFMediaSource **ppSource);

HRESULT CreatePlaybackTopology(IMFMediaSource *pSource, 
    IMFPresentationDescriptor *pPD, HWND hVideoWnd,IMFTopology **ppTopology);

BOOL MF_Player_OpenFile(PWSTR file);
LRESULT MF_Player_InitWindows(HWND hVideo,HWND hEvent);
void MF_OnPlayerEvent(HWND hwnd, WPARAM pUnkPtr);

// Forward declarations of functions included in this code module:

INT_PTR CALLBACK    OpenUrlDialogProc(HWND, UINT, WPARAM, LPARAM);
void                NotifyError(HWND hwnd, const WCHAR *sErrorMessage, HRESULT hr);
void                UpdateUI(HWND hwnd, MF_PlayerState state);
HRESULT             AllocGetWindowText(HWND hwnd, WCHAR **pszText, DWORD *pcchLen);

// Message handlers
void                OnOpenURL(HWND hwnd);

void                OnPaint(HWND hwnd);
void                OnResize(WORD width, WORD height);

#endif
