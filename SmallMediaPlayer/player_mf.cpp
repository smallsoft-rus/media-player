/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#include <assert.h>
#include "player_mf.h"
#include "errors.h"

extern HWND hVideoWindow;

//from player.cpp
extern HWND hWnd;
extern PLAYER_STATE PlayerState;
extern long VolumeX;
void OnPlayerEvent(PLAYER_EVENT evt);

//forward decl
extern HANDLE MF_hOpenEvent;
extern HRESULT MF_OpenEvent_LastResult;
HRESULT GetSourceDuration(IMFMediaSource *pSource, MFTIME *pDuration);
WORD GetSourceInfo(IMFMediaSource *pSource, SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType);

//  Static class method to create the MfPlayer object.

HRESULT MfPlayer::CreateInstance(
    HWND hVideo,                  // Video window.
    HWND hEvent,                  // Window to receive notifications.
    MfPlayer **ppPlayer)           // Receives a pointer to the CPlayer object.
{
    if (ppPlayer == NULL)
    {
        return E_POINTER;
    }

    MfPlayer *pPlayer = new (std::nothrow) MfPlayer(hVideo, hEvent);
    if (pPlayer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pPlayer->Initialize();
    if (SUCCEEDED(hr))
    {
        *ppPlayer = pPlayer;
    }
    else
    {
        pPlayer->Release();
    }
    return hr;
}

HRESULT MfPlayer::Initialize()
{
    // Start up Media Foundation platform.
    HRESULT hr = MFStartup(MF_VERSION);
    if (SUCCEEDED(hr))
    {
        m_hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hCloseEvent == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

MfPlayer::MfPlayer(HWND hVideo, HWND hEvent) : 
    m_pSession(NULL),
    m_pSource(NULL),
    m_pVideoDisplay(NULL),
    m_hwndVideo(hVideo),
    m_hwndEvent(hEvent),
    m_state(Closed),
    m_hCloseEvent(NULL),
    m_nRefCount(1)
{
    nSeekPending=0;
    dwSeekPos=0;
}

MfPlayer::~MfPlayer()
{
    assert(m_pSession == NULL);  
    // If FALSE, the app did not call Shutdown().

    // When CPlayer calls IMediaEventGenerator::BeginGetEvent on the
    // media session, it causes the media session to hold a reference 
    // count on the CPlayer. 
    
    // This creates a circular reference count between CPlayer and the 
    // media session. Calling Shutdown breaks the circular reference 
    // count.

    // If CreateInstance fails, the application will not call 
    // Shutdown. To handle that case, call Shutdown in the destructor. 

    Shutdown();
}

// IUnknown methods

HRESULT MfPlayer::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(MfPlayer, IMFAsyncCallback),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG MfPlayer::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG MfPlayer::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    return uCount;
}

//  Open a URL for playback.
HRESULT MfPlayer::OpenURL(const WCHAR *sURL)
{
    if(this->m_state == OpenPending){
        //async open operation in progress
        return MF_E_INVALIDREQUEST;
    }

    // 1. Create a new media session.
    // 2. Create the media source.
    // 3. Create the topology.
    // 4. Queue the topology [asynchronous]
    // 5. Start playback [asynchronous - does not happen in this method.]

    IMFTopology *pTopology = NULL;
    IMFPresentationDescriptor* pSourcePD = NULL;

    // Create the media session.
    HRESULT hr = CreateSession();
    if (FAILED(hr))
    {
        goto done;
    }

    // Create the media source.
    hr = CreateMediaSource(sURL, &m_pSource);
    if (FAILED(hr))
    {
        goto done;
    }

    // Create the presentation descriptor for the media source.
    hr = m_pSource->CreatePresentationDescriptor(&pSourcePD);
    if (FAILED(hr))
    {
        goto done;
    }

    // Create a partial topology.
    hr = CreatePlaybackTopology(m_pSource, pSourcePD, m_hwndVideo, &pTopology);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the topology on the media session.
    hr = m_pSession->SetTopology(MFSESSION_SETTOPOLOGY_IMMEDIATE, pTopology);
    if (FAILED(hr))
    {
        goto done;
    }

    m_state = OpenPending;

    // If SetTopology succeeds, the media session will queue an 
    // MESessionTopologySet event.

done:
    if (FAILED(hr))
    {
        m_state = Closed;
    }

    PlayerState=FILE_NOT_LOADED;
    nSeekPending=0;
    SafeRelease(&pSourcePD);
    SafeRelease(&pTopology);
    return hr;
}

//  Pause playback.
HRESULT MfPlayer::Pause()    
{
    if (m_state != Started)
    {
        return MF_E_INVALIDREQUEST;
    }
    if (m_pSession == NULL || m_pSource == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pSession->Pause();
    if (SUCCEEDED(hr))
    {
        m_state = Paused;
        PlayerState=PAUSED;
    }

    return hr;
}

// Stop playback.
HRESULT MfPlayer::Stop()
{
    if (m_state != Started && m_state != Paused)
    {
        return MF_E_INVALIDREQUEST;
    }
    if (m_pSession == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pSession->Stop();
    if (SUCCEEDED(hr))
    {
        m_state = Stopped;
        PlayerState=STOPPED;
    }
    return hr;
}

//  Repaint the video window. Call this method on WM_PAINT.

HRESULT MfPlayer::Repaint()
{
    if (m_pVideoDisplay)
    {
        return m_pVideoDisplay->RepaintVideo();
    }
    else
    {
        return S_OK;
    }
}

//  Resize the video rectangle.
//
//  Call this method if the size of the video window changes.

HRESULT MfPlayer::ResizeVideo(WORD width, WORD height)
{
    if (m_pVideoDisplay)
    {
        // Set the destination rectangle.
        // Leave the default source rectangle (0,0,1,1).

        RECT rcDest = { 0, 0, width, height };

        return m_pVideoDisplay->SetVideoPosition(NULL, &rcDest);
    }
    else
    {
        return S_OK;
    }
}

//  Callback for the asynchronous BeginGetEvent method.

HRESULT MfPlayer::Invoke(IMFAsyncResult *pResult)
{

#ifdef DEBUG
    LogMessage(L"MfPlayer::Invoke",TRUE);
    WCHAR buf[100]=L"";
    StringCchPrintf(buf,100,L"State: %u", (UINT)this->m_state);
    LogMessage(buf,FALSE);

    if(m_pSession == NULL) {
        HandleError(L"Session is NULL in MfPlayer::Invoke",SMP_ALERT_NONBLOCKING,L"",NULL);
        return MF_E_INVALIDREQUEST;
    }
#else
    if(m_pSession == NULL) {
        //Fix for issue https://github.com/smallsoft-rus/media-player/issues/15
        return MF_E_INVALIDREQUEST;
    }
#endif

    MediaEventType meType = MEUnknown;  // Event type

    IMFMediaEvent *pEvent = NULL;

    // Get the event from the event queue.
    HRESULT hr = m_pSession->EndGetEvent(pResult, &pEvent);
    if (FAILED(hr))
    {
#ifdef DEBUG        
        HandleMfError(hr,L"m_pSession->EndGetEvent failed in MfPlayer::Invoke",L"");
#endif
        goto done;
    }

#ifdef DEBUG
    LogMessage(L"m_pSession->EndGetEvent success",FALSE);    
#endif

    // Get the event type. 
    hr = pEvent->GetType(&meType);
    if (FAILED(hr))
    {
        goto done;
    }

#ifdef DEBUG
    StringCchPrintf(buf,100,L"Event type: %u", (UINT)meType);
    LogMessage(buf,FALSE);
#endif

    if (meType == MESessionClosed)
    {
        // The session was closed. 
        // The application is waiting on the m_hCloseEvent event handle. 
        SetEvent(m_hCloseEvent);
    }
    else
    {
        // For all other events, get the next event in the queue.
        hr = m_pSession->BeginGetEvent(this, NULL);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Check the application state. 
        
    // If a call to IMFMediaSession::Close is pending, it means the 
    // application is waiting on the m_hCloseEvent event and
    // the application's message loop is blocked. 

    // Otherwise, post a private window message to the application. 

    if (m_state != Closing)
    {
        // Leave a reference count on the event.
        pEvent->AddRef();

        PostMessage(m_hwndEvent, UM_PLAYER_EVENT, 
            (WPARAM)pEvent, (LPARAM)meType);
    }

done:
    SafeRelease(&pEvent);
    return S_OK;
}

HRESULT MfPlayer::HandleEvent(UINT_PTR pEventPtr)
{
    HRESULT hrStatus = S_OK;            
    MediaEventType meType = MEUnknown;  

    IMFMediaEvent *pEvent = (IMFMediaEvent*)pEventPtr;

    if (pEvent == NULL)
    {
        return E_POINTER;
    }

    // Get the event type.
    HRESULT hr = pEvent->GetType(&meType);
    if (FAILED(hr))
    {
        goto done;
    }

    // Get the event status. If the operation that triggered the event 
    // did not succeed, the status is a failure code.
    hr = pEvent->GetStatus(&hrStatus);

    // Check if the async operation succeeded.
    if (SUCCEEDED(hr) && FAILED(hrStatus)) 
    {
        hr = hrStatus;
    }
    if (FAILED(hr))
    {
        goto done;
    }

    switch(meType)
    {
    case MESessionTopologyStatus:
        hr = OnTopologyStatus(pEvent);
        break;

    case MEEndOfPresentation:
        hr = OnPresentationEnded(pEvent);
        break;

    case MENewPresentation:
        hr = OnNewPresentation(pEvent);
        break;

    case MESessionStarted:
        //StartPlayback or SetPosition request complete
        nSeekPending--;
        break;

    default:
        hr = OnSessionEvent(pEvent, meType);
        break;
    }

done:
    SafeRelease(&pEvent);
    return hr;
}

//  Release all resources held by this object.
HRESULT MfPlayer::Shutdown()
{
    // Close the session
    HRESULT hr = Close();

    // Shutdown the Media Foundation platform
    MFShutdown();

    return hr;
}

HRESULT MfPlayer::Close()
{
    if(this->m_state == Closing || this->m_state == Closed){
        //no need to close 
        return S_OK;
    }

    // Close the session
    HRESULT hr = CloseSession();

    if (m_hCloseEvent)
    {
        CloseHandle(m_hCloseEvent);
        m_hCloseEvent = NULL;
    }

    return hr;
}

/// Protected methods

HRESULT MfPlayer::OnTopologyStatus(IMFMediaEvent *pEvent)
{
    UINT32 status; 

    HRESULT hr = pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status);
    if (SUCCEEDED(hr) && (status == MF_TOPOSTATUS_READY))
    {
        SafeRelease(&m_pVideoDisplay);

        // Get the IMFVideoDisplayControl interface from EVR. This call is
        // expected to fail if the media file does not have a video stream.

        (void)MFGetService(m_pSession, MR_VIDEO_RENDER_SERVICE, 
            IID_PPV_ARGS(&m_pVideoDisplay));

        //File opened, the state is now "Stopped"
        this->m_state=Stopped;

        //Signal file opened event
        SetEvent(MF_hOpenEvent);
        MF_OpenEvent_LastResult = S_OK;

        //restore volume
        this->SetVolume(VolumeX);
    }
    return hr;
}


//  Handler for MEEndOfPresentation event.
HRESULT MfPlayer::OnPresentationEnded(IMFMediaEvent *pEvent)
{
    // The session puts itself into the stopped state automatically.
    m_state = Stopped;
    PlayerState=STOPPED;
    OnPlayerEvent(EVT_COMPLETE);
    return S_OK;
}

//  Handler for MENewPresentation event.
//
//  This event is sent if the media source has a new presentation, which 
//  requires a new topology. 

HRESULT MfPlayer::OnNewPresentation(IMFMediaEvent *pEvent)
{
    IMFPresentationDescriptor *pPD = NULL;
    IMFTopology *pTopology = NULL;

    // Get the presentation descriptor from the event.
    HRESULT hr = GetEventObject(pEvent, &pPD);
    if (FAILED(hr))
    {
        goto done;
    }

    // Create a partial topology.
    hr = CreatePlaybackTopology(m_pSource, pPD,  m_hwndVideo,&pTopology);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the topology on the media session.
    hr = m_pSession->SetTopology(0, pTopology);
    if (FAILED(hr))
    {
        goto done;
    }

    m_state = OpenPending;

done:
    SafeRelease(&pTopology);
    SafeRelease(&pPD);
    return S_OK;
}

//  Create a new instance of the media session.
HRESULT MfPlayer::CreateSession()
{
    // Close the old session, if any.
    HRESULT hr = CloseSession();
    if (FAILED(hr))
    {
        goto done;
    }

    assert(m_state == Closed);

    // Create the media session.
    hr = MFCreateMediaSession(NULL, &m_pSession);
    if (FAILED(hr))
    {
        goto done;
    }

    // Start pulling events from the media session
    hr = m_pSession->BeginGetEvent((IMFAsyncCallback*)this, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    m_state = Ready;
    PlayerState=FILE_NOT_LOADED;

done:
    return hr;
}

//  Close the media session. 
HRESULT MfPlayer::CloseSession()
{
    //  The IMFMediaSession::Close method is asynchronous, but the 
    //  MfPlayer::CloseSession method waits on the MESessionClosed event.
    //  
    //  MESessionClosed is guaranteed to be the last event that the 
    //  media session fires.

    HRESULT hr = S_OK;

    SafeRelease(&m_pVideoDisplay);

    // First close the media session.
    if (m_pSession)
    {
        DWORD dwWaitResult = 0;

        m_state = Closing;
           
        hr = m_pSession->Close();
        // Wait for the close operation to complete
        if (SUCCEEDED(hr))
        {
            dwWaitResult = WaitForSingleObject(m_hCloseEvent, 5000);
            if (dwWaitResult == WAIT_TIMEOUT)
            {
                assert(FALSE);
            }
            // Now there will be no more events from this session.
        }
    }

    // Complete shutdown operations.
    if (SUCCEEDED(hr))
    {
        // Shut down the media source. (Synchronous operation, no events.)
        if (m_pSource)
        {
            (void)m_pSource->Shutdown();
        }
        // Shut down the media session. (Synchronous operation, no events.)
        if (m_pSession)
        {
            (void)m_pSession->Shutdown();
        }
    }

    SafeRelease(&m_pSource);
    SafeRelease(&m_pSession);
    m_state = Closed;
    PlayerState=FILE_NOT_LOADED;
    return hr;
}

//  Start playback from the current position. 
HRESULT MfPlayer::StartPlayback()
{
    assert(m_pSession != NULL);

    PROPVARIANT varStart;
    PropVariantInit(&varStart);

    HRESULT hr = m_pSession->Start(&GUID_NULL, &varStart);
    if (SUCCEEDED(hr))
    {
        // Note: Start is an asynchronous operation. However, we
        // can treat our state as being already started. If Start
        // fails later, we'll get an MESessionStarted event with
        // an error code, and we will update our state then.
        m_state = Started;

        //StartPlayback is technically seeking to the zero time
        nSeekPending++;
        dwSeekPos=0;
    }
    PropVariantClear(&varStart);
    return hr;
}

HRESULT MfPlayer::SetPosition(LONGLONG newpos){
    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    varStart.vt=VT_I8;
    varStart.hVal.QuadPart=TIME_KOEFF*newpos;
    
    HRESULT hr = m_pSession->Start(&GUID_NULL, &varStart);
    nSeekPending++;
    dwSeekPos=newpos;
    PropVariantClear(&varStart);

    if(PlayerState == PAUSED)m_pSession->Pause();
    
    return hr;
}

HRESULT MfPlayer::SetVolume(DWORD vol){

    if(m_pSession==NULL) return MF_E_INVALIDREQUEST;

    IMFAudioStreamVolume* pVolume=NULL;
    HRESULT hr=MFGetService(m_pSession, MR_STREAM_VOLUME_SERVICE,IID_PPV_ARGS(&pVolume));
    if(FAILED(hr))goto end;

    UINT c=0;
    pVolume->GetChannelCount(&c);

    for(UINT i=0;i<c;i++){
        hr=pVolume->SetChannelVolume(i,vol/100.0f);
    }

end:SafeRelease(&pVolume);
    return hr;
}

//  Start playback from paused or stopped.
HRESULT MfPlayer::Play()
{
    if (m_state != Paused && m_state != Stopped)
    {
        return MF_E_INVALIDREQUEST;
    }
    if (m_pSession == NULL || m_pSource == NULL)
    {
        return E_UNEXPECTED;
    }
    return StartPlayback();
}

DWORD MfPlayer::GetLength(){
    MFTIME t=0;
    HRESULT hr=GetSourceDuration(this->m_pSource,&t);

    if(SUCCEEDED(hr))return t/TIME_KOEFF;
    else return 0;
}

DWORD MfPlayer::GetPosition(){

    // If seek operation is pending, clock can report zero time incorrectly
    // return the last requested position to prevent scrollbar jumping to left
    if(nSeekPending>0) return dwSeekPos;

    IMFClock* pClock=NULL;
    IMFPresentationClock* ppClock=NULL;
    HRESULT hr;
    DWORD ret=0;
    hr=m_pSession->GetClock(&pClock);
    if(FAILED(hr)) goto end;

    hr=pClock->QueryInterface(IID_PPV_ARGS(&ppClock));
    if(FAILED(hr)) goto end;

    MFTIME t=0;
    hr=ppClock->GetTime(&t);
    if(FAILED(hr)) goto end;

    ret = t/TIME_KOEFF;

end:SafeRelease(&pClock);
    SafeRelease(&ppClock);

    return ret;
}

WORD MfPlayer::GetMultimediaInfo(SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType){

    if(PlayerState == FILE_NOT_LOADED) return INFORES_NO;
    if(this->m_pSource == NULL) return INFORES_NO;

    return GetSourceInfo(this->m_pSource, pAudioInfo, pVideoInfo, pStreamType);
}

//  Create a media source from a URL.
HRESULT CreateMediaSource(PCWSTR sURL, IMFMediaSource **ppSource)
{
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

    IMFSourceResolver* pSourceResolver = NULL;
    IUnknown* pSource = NULL;

    // Create the source resolver.
    HRESULT hr = MFCreateSourceResolver(&pSourceResolver);
    if (FAILED(hr))
    {
        goto done;
    }

    // Use the source resolver to create the media source.

    hr = pSourceResolver->CreateObjectFromURL(
        sURL,                       // URL of the source.
        MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
        NULL,                       // Optional property store.
        &ObjectType,        // Receives the created object type. 
        &pSource            // Receives a pointer to the media source.
        );
    if (FAILED(hr))
    {
        goto done;
    }

    // Get the IMFMediaSource interface from the media source.
    hr = pSource->QueryInterface(IID_PPV_ARGS(ppSource));

done:
    SafeRelease(&pSourceResolver);
    SafeRelease(&pSource);
    return hr;
}

//  Create an activation object for a renderer, based on the stream media type.

HRESULT CreateMediaSinkActivate(
    IMFStreamDescriptor *pSourceSD,     // Pointer to the stream descriptor.
    HWND hVideoWindow,                  // Handle to the video clipping window.
    IMFActivate **ppActivate
)
{
    IMFMediaTypeHandler *pHandler = NULL;
    IMFActivate *pActivate = NULL;

    // Get the media type handler for the stream.
    HRESULT hr = pSourceSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr))
    {
        goto done;
    }

    // Get the major media type.
    GUID guidMajorType;
    hr = pHandler->GetMajorType(&guidMajorType);
    if (FAILED(hr))
    {
        goto done;
    }
 
    // Create an IMFActivate object for the renderer, based on the media type.
    if (MFMediaType_Audio == guidMajorType)
    {
        // Create the audio renderer.
        hr = MFCreateAudioRendererActivate(&pActivate);
    }
    else if (MFMediaType_Video == guidMajorType)
    {
        // Create the video renderer.
        hr = MFCreateVideoRendererActivate(hVideoWindow, &pActivate);
    }
    else
    {
        // Unknown stream type. 
        hr = E_FAIL;
        // Optionally, you could deselect this stream instead of failing.
    }
    if (FAILED(hr))
    {
        goto done;
    }
 
    // Return IMFActivate pointer to caller.
    *ppActivate = pActivate;
    (*ppActivate)->AddRef();

done:
    SafeRelease(&pHandler);
    SafeRelease(&pActivate);
    return hr;
}

// Add a source node to a topology.
HRESULT AddSourceNode(
    IMFTopology *pTopology,           // Topology.
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    IMFStreamDescriptor *pSD,         // Stream descriptor.
    IMFTopologyNode **ppNode)         // Receives the node pointer.
{
    IMFTopologyNode *pNode = NULL;

    // Create the node.
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the attributes.
    hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
    if (FAILED(hr))
    {
        goto done;
    }
    
    // Add the node to the topology.
    hr = pTopology->AddNode(pNode);
    if (FAILED(hr))
    {
        goto done;
    }

    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}

// Add an output node to a topology.
HRESULT AddOutputNode(
    IMFTopology *pTopology,     // Topology.
    IMFActivate *pActivate,     // Media sink activation object.
    DWORD dwId,                 // Identifier of the stream sink.
    IMFTopologyNode **ppNode)   // Receives the node pointer.
{
    IMFTopologyNode *pNode = NULL;

    // Create the node.
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the object pointer.
    hr = pNode->SetObject(pActivate);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the stream sink ID attribute.
    hr = pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    if (FAILED(hr))
    {
        goto done;
    }

    // Add the node to the topology.
    hr = pTopology->AddNode(pNode);
    if (FAILED(hr))
    {
        goto done;
    }

    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}

//  Add a topology branch for one stream.
//
//  For each stream, this function does the following:
//
//    1. Creates a source node associated with the stream. 
//    2. Creates an output node for the renderer. 
//    3. Connects the two nodes.
//
//  The media session will add any decoders that are needed.

HRESULT AddBranchToPartialTopology(
    IMFTopology *pTopology,         // Topology.
    IMFMediaSource *pSource,        // Media source.
    IMFPresentationDescriptor *pPD, // Presentation descriptor.
    DWORD iStream,                  // Stream index.
    HWND hVideoWnd)                 // Window for video playback.
{
    IMFStreamDescriptor *pSD = NULL;
    IMFActivate         *pSinkActivate = NULL;
    IMFTopologyNode     *pSourceNode = NULL;
    IMFTopologyNode     *pOutputNode = NULL;

    BOOL fSelected = FALSE;

    HRESULT hr = pPD->GetStreamDescriptorByIndex(iStream, &fSelected, &pSD);
    if (FAILED(hr))
    {
        goto done;
    }

    if (fSelected)
    {
        // Create the media sink activation object.
        hr = CreateMediaSinkActivate(pSD, hVideoWnd, &pSinkActivate);
        if (FAILED(hr))
        {
            goto done;
        }

        // Add a source node for this stream.
        hr = AddSourceNode(pTopology, pSource, pPD, pSD, &pSourceNode);
        if (FAILED(hr))
        {
            goto done;
        }

        // Create the output node for the renderer.
        hr = AddOutputNode(pTopology, pSinkActivate, 0, &pOutputNode);
        if (FAILED(hr))
        {
            goto done;
        }

        // Connect the source node to the output node.
        hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
    }
    // else: If not selected, don't add the branch. 

done:
    SafeRelease(&pSD);
    SafeRelease(&pSinkActivate);
    SafeRelease(&pSourceNode);
    SafeRelease(&pOutputNode);
    return hr;
}

//  Create a playback topology from a media source.
HRESULT CreatePlaybackTopology(
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    HWND hVideoWnd,                   // Video window.
    IMFTopology **ppTopology)         // Receives a pointer to the topology.
{
    IMFTopology *pTopology = NULL;
    DWORD cSourceStreams = 0;

    // Create a new topology.
    HRESULT hr = MFCreateTopology(&pTopology);
    if (FAILED(hr))
    {
        goto done;
    }
    
    // Get the number of streams in the media source.
    hr = pPD->GetStreamDescriptorCount(&cSourceStreams);
    if (FAILED(hr))
    {
        goto done;
    }

    // For each stream, create the topology nodes and add them to the topology.
    for (DWORD i = 0; i < cSourceStreams; i++)
    {
        hr = AddBranchToPartialTopology(pTopology, pSource, pPD, i, hVideoWnd);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Return the IMFTopology pointer to the caller.
    *ppTopology = pTopology;
    (*ppTopology)->AddRef();

done:
    SafeRelease(&pTopology);
    return hr;
}

//***** Application *********

BOOL        g_bRepaintClient = TRUE;            // Repaint the application client area?
MfPlayer     *g_pPlayer = NULL;                  // Global player object.

//event that signals when async file open operation is finished
HANDLE MF_hOpenEvent=NULL;

//the result of the last async file open operation
HRESULT MF_OpenEvent_LastResult = S_OK;

// Note: After WM_CREATE is processed, g_pPlayer remains valid until the
// window is destroyed.

//  Open an audio/video file.
HRESULT MF_Player_OpenFile(PWSTR file)
{
    //run async file open operation
    ResetEvent(MF_hOpenEvent);
    MF_OpenEvent_LastResult = S_OK;

    HRESULT hr = g_pPlayer->OpenURL(file);

    if (SUCCEEDED(hr))
    {
        UpdateUI(hWnd, OpenPending);

        //wait until async file open operation is completed
        AwaitHandle(MF_hOpenEvent);

        //check results of async operation
        if(FAILED(MF_OpenEvent_LastResult)){
            UpdateUI(hWnd, Closed);
            return MF_OpenEvent_LastResult;
        }

#ifdef DEBUG
    LogMessage(L"MF_Player_OpenFile",TRUE);
    LogMessage(file,FALSE);
#endif

        return S_OK;
    }
    else
    {
        UpdateUI(hWnd, Closed);
        return hr;
    }
}

//  Handler for WM_CREATE message.
LRESULT MF_Player_InitWindows(HWND hVideo,HWND hEvent)
{
    MF_hOpenEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    // Initialize the player object.
    HRESULT hr = MfPlayer::CreateInstance(hVideo, hEvent, &g_pPlayer); 
    if (SUCCEEDED(hr))
    {
        UpdateUI(hVideo, Closed);
        return 0;   // Success.
    }
    else
    {
        NotifyError(NULL, L"Could not initialize the player object.", hr);
        return -1;  // Destroy the window
    }
}

//  Handler for WM_PAINT messages.
void OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (g_pPlayer && g_pPlayer->HasVideo())
    {
        // Video is playing. Ask the player to repaint.
        g_pPlayer->Repaint();
    }
    else
    {
        // The video is not playing, so we must paint the application window.
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH) COLOR_WINDOW);
    }
    EndPaint(hwnd, &ps);
}

//  Handler for WM_SIZE messages.
void OnResize(WORD width, WORD height)
{
    if (g_pPlayer)
    {
        g_pPlayer->ResizeVideo(width, height);
    }
}

// Update the application UI to reflect the current state.

void UpdateUI(HWND hwnd, MF_PlayerState state)
{
    BOOL bWaiting = FALSE;
    BOOL bPlayback = FALSE;

    assert(g_pPlayer != NULL);

    switch (state)
    {
    case OpenPending:
        bWaiting = TRUE;
        break;

    case Started:
        bPlayback = TRUE;
        break;

    case Paused:
        bPlayback = TRUE;
        break;
    }

    if (bPlayback && g_pPlayer->HasVideo())
    {
        ShowWindow(hVideoWindow,SW_SHOW);
        g_bRepaintClient = FALSE;
    }
    else
    {
        g_bRepaintClient = TRUE;
    }
}

//  Show a message box with an error message.
void NotifyError(HWND hwnd, PCWSTR pszErrorMessage, HRESULT hrErr)
{
    const size_t MESSAGE_LEN = 512;
    WCHAR message[MESSAGE_LEN];

    if (SUCCEEDED(StringCchPrintf(message, MESSAGE_LEN, L"%s (HRESULT = 0x%X)", 
        pszErrorMessage, hrErr)))
    {
        MessageBox(hwnd, message, NULL, MB_OK | MB_ICONERROR);
    }
}

HRESULT GetSourceDuration(IMFMediaSource *pSource, MFTIME *pDuration)
{
    //https://docs.microsoft.com/en-us/windows/win32/medfound/how-to-find-the-duration-of-a-media-file
    *pDuration = 0;

    IMFPresentationDescriptor *pPD = NULL;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (SUCCEEDED(hr))
    {
        hr = pPD->GetUINT64(MF_PD_DURATION, (UINT64*)pDuration);
        pPD->Release();
    }
    return hr;
}

WORD GetSourceInfo(IMFMediaSource *pSource, SMP_AUDIOINFO* pAudioInfo,SMP_VIDEOINFO* pVideoInfo,SMP_STREAM* pStreamType){
    IMFPresentationDescriptor* pPD = NULL;
    IMFStreamDescriptor* pStream = NULL;
    IMFMediaTypeHandler* pTypeHandler = NULL;
    IMFMediaType* pMediaType = NULL;
    DWORD count = 0;
    BOOL selected = FALSE;
    GUID majorType;
    GUID subtype;
    bool hasAudio = false;
    bool hasVideo = false;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);

    if(FAILED(hr)) goto End;
    
    hr = pPD->GetStreamDescriptorCount(&count);

    if(FAILED(hr)) goto End;

    for(DWORD i=0;i<count; i++){
        hr = pPD->GetStreamDescriptorByIndex(i, &selected, &pStream);
        if(FAILED(hr)) continue;

        hr = pStream->GetMediaTypeHandler(&pTypeHandler);

        if(FAILED(hr)) {
            SafeRelease(&pStream);
            continue;
        }

        hr = pTypeHandler->GetCurrentMediaType(&pMediaType);

        if(FAILED(hr)) {
            //if there's no current media type, try the first supported one
            DWORD typeCount = 0;
            pTypeHandler->GetMediaTypeCount(&typeCount);
            if(typeCount>0) hr = pTypeHandler->GetMediaTypeByIndex(0, &pMediaType);
        }

        if(FAILED(hr)) {
            SafeRelease(&pStream);
            SafeRelease(&pTypeHandler);
            continue;
        }
        
        ZeroMemory(&majorType, sizeof(GUID));
        pMediaType->GetMajorType(&majorType);

        ZeroMemory(&subtype, sizeof(GUID));
        pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);

        if(majorType == MFMediaType_Audio){
            hasAudio = true;

            //Format tag is a first DWORD in GUID (https://docs.microsoft.com/en-us/windows/win32/medfound/audio-subtype-guids)
            pAudioInfo->wFormatTag = (WORD)subtype.Data1;
            
            //Get audio stream properties
            UINT32 numChannels = 0;
            pMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &numChannels);
            if(numChannels>255) numChannels=255;
            pAudioInfo->chans = (BYTE)numChannels;

            UINT32 bitsPerSample = 0;
            pMediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
            pAudioInfo->BitsPerSample = (int)bitsPerSample;

            UINT32 bytesPerSec = 0;
            pMediaType->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bytesPerSec);
            pAudioInfo->BitsPerSecond = (int)bytesPerSec*8;

            UINT32 samplesPerSec = 0;
            pMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSec);
            pAudioInfo->nFreq = (int)samplesPerSec;
        }
        else if(majorType == MFMediaType_Video){
            hasVideo = true;
            
            //Codec FOURCC is the first dword in GUID
            //(https://docs.microsoft.com/en-us/windows/win32/medfound/video-subtype-guids#creating-subtype-guids-from-fourccs-and-d3dformat-values)
            DWORD fourcc = (DWORD)subtype.Data1;

            if(subtype == MFVideoFormat_MPG1) {
                pVideoInfo->VideoType = VIDEOTYPE_MPEG1;
                pVideoInfo->dwVideoCodec = VIDEO_MPEG1;
            }
            else if(subtype == MFVideoFormat_MPEG2){
                pVideoInfo->VideoType = VIDEOTYPE_MPEG2;
            }
            else if(subtype == MFVideoFormat_RGB8){
                pVideoInfo->VideoType = VIDEOTYPE_VIDEO;
                pVideoInfo->dwVideoCodec = fourcc;
                pVideoInfo->BitsPerPixel = 8;
            }
            else if(subtype == MFVideoFormat_RGB24){
                pVideoInfo->VideoType = VIDEOTYPE_VIDEO;
                pVideoInfo->dwVideoCodec = fourcc;
                pVideoInfo->BitsPerPixel = 24;
            }
            else if(subtype == MFVideoFormat_RGB32){
                pVideoInfo->VideoType = VIDEOTYPE_VIDEO;
                pVideoInfo->dwVideoCodec = fourcc;
                pVideoInfo->BitsPerPixel = 32;
            }
            else { //other format identified by FOURCC
                pVideoInfo->VideoType=VIDEOTYPE_VIDEO;
                pVideoInfo->dwVideoCodec = fourcc;
            }

            //Get video stream properties
            UINT32 width = 0;
            UINT32 height = 0;
            MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
            pVideoInfo->width = (int)width;
            pVideoInfo->height = (int)height;

            UINT32 nom = 0;
            UINT32 denom = 0;
            MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &nom, &denom);
            if(denom!=0) pVideoInfo->FramesPerSecond = (int)(nom/denom);
        }

        SafeRelease(&pStream);
        SafeRelease(&pTypeHandler);
        SafeRelease(&pMediaType);
    }

End:SafeRelease(&pPD);

    if(hasAudio) {
        if(hasVideo) return INFORES_BOTH;
        else return INFORES_AUDIO;
    }
    else {
        if(hasVideo) return INFORES_VIDEO;
        return INFORES_NO;
    }
}

// Handler for Media Session events.
void MF_OnPlayerEvent(HWND hwnd, WPARAM pUnkPtr)
{
    HRESULT hr = g_pPlayer->HandleEvent(pUnkPtr);

    if (FAILED(hr))
    {
        if(g_pPlayer->GetState() == OpenPending){
            g_pPlayer->Close();
        }
        else
        {
            NotifyError(hwnd, L"OnPlayerEvent error", hr);
        }

        //communicate failure to async open operation
        MF_OpenEvent_LastResult = hr;
        SetEvent(MF_hOpenEvent);
    }

    UpdateUI(hwnd, g_pPlayer->GetState());
}
