/* Small Media Player 
 * Copyright (c) 2021,  MSDN.WhiteKnight (https://github.com/smallsoft-rus/media-player) 
 * License: BSD 2.0 */
#ifndef SCROLL_H
#define SCROLL_H

#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

class ScrollbarControl{
public:
    ScrollbarControl(){;}
    ScrollbarControl(int type,HWND handle,int min,int max,UINT page){
        si.cbSize=sizeof(SCROLLINFO);
        si.nMin=min;si.nMax=max;
        si.nPage=page;
        si.fMask=SIF_PAGE|SIF_RANGE;
        SetScrollInfo(handle,type,&si,TRUE);
        hwnd=handle;
        BarType=type;
        Inc=10;        
        };
        
    void SetTrackPosition(int pos){
        si.nPos=pos;
        si.fMask=SIF_POS;
        SetScrollInfo(hwnd,BarType,&si,TRUE);
        }
    void SetIncrement(int value){
        Inc=value;}
    void ProcessScrollEvent(short EventType,short TrackPos){
        switch(EventType){
            
            case SB_LINEUP:SetTrackPosition(si.nPos-Inc);break;
            
            case SB_LINEDOWN:SetTrackPosition(si.nPos+Inc);break;
            
            case SB_PAGEUP:SetTrackPosition(si.nPos-si.nPage);break;
            
             case SB_PAGEDOWN:SetTrackPosition(si.nPos+si.nPage);break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:SetTrackPosition(TrackPos);break;
            
            };
            
            }
    int GetTrackPos(){
		si.fMask=SIF_POS;
        GetScrollInfo(hwnd,BarType,&si);
		return si.nPos;}
      
private:SCROLLINFO si;
HWND hwnd;
int BarType;
int Inc;


    };

#endif
