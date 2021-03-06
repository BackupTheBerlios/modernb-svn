/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "commonheaders.h"
#include "clist.h"
#include "m_skin_eng.h"
#include "commonprototypes.h"

#define WM_DOCKCALLBACK   (WM_USER+121)
#define WM_CREATEDOCKED   (WM_USER+122)
#define EDGESENSITIVITY   3

#define DOCKED_NONE    0
#define DOCKED_LEFT    1
#define DOCKED_RIGHT   2

BOOL LockSubframeMoving=0;
int docked=0;
int dock_prevent_moving=0;
static TempDock=0;
static int dock_drag_dx=0;
static int dock_drag_dy=0;
extern int OnMoving(HWND hwnd,RECT *lParam);
typedef HMONITOR WINAPI MyMonitorFromPoint(POINT,DWORD);
typedef BOOL WINAPI MyGetMonitorInfo(HMONITOR,LPMONITORINFO);

static void Docking_GetMonitorRectFromPoint(POINT pt,RECT *rc)
{
	HMODULE hUserInstance = GetModuleHandle(TEXT("user32"));

	MyMonitorFromPoint *LPMyMonitorFromPoint = (MyMonitorFromPoint*)GetProcAddress(hUserInstance,"MonitorFromPoint");
	if (LPMyMonitorFromPoint)
	{
		MONITORINFO monitorInfo;
		HMONITOR hMonitor = LPMyMonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST); // always returns a valid value
		monitorInfo.cbSize = sizeof(MONITORINFO);
		
		if ((MyGetMonitorInfo*)GetProcAddress(hUserInstance,"GetMonitorInfoA")(hMonitor,&monitorInfo))
		{
			CopyMemory(rc,&monitorInfo.rcMonitor,sizeof(RECT));
			return;
		}
	}

	// "generic" win95/NT support, also serves as failsafe
	rc->left = 0;
	rc->top = 0;
	rc->bottom = GetSystemMetrics(SM_CYSCREEN);
	rc->right = GetSystemMetrics(SM_CXSCREEN);
}

void Docking_GetMonitorRectFromWindow(HWND hWnd,RECT *rc)
{
	POINT ptWindow;
	GetWindowRect(hWnd,rc);
	ptWindow.x = rc->left;
	ptWindow.y = rc->top;
	Docking_GetMonitorRectFromPoint(ptWindow,rc);
}

static void Docking_AdjustPosition(HWND hwnd,RECT *rcDisplay,RECT *rc)
{
	APPBARDATA abd;

	ZeroMemory(&abd,sizeof(abd));
	abd.cbSize=sizeof(abd);
	abd.hWnd=hwnd;
	abd.uEdge=docked==DOCKED_LEFT?ABE_LEFT:ABE_RIGHT;
	abd.rc=*rc;
	abd.rc.top=rcDisplay->top;
	abd.rc.bottom=rcDisplay->bottom;
	if(docked==DOCKED_LEFT) {
		abd.rc.right=rcDisplay->left+abd.rc.right-abd.rc.left;
		abd.rc.left=rcDisplay->left;
	}
	else {
		abd.rc.left=rcDisplay->right-(abd.rc.right-abd.rc.left);
		abd.rc.right=rcDisplay->right;

	}
	SHAppBarMessage(ABM_SETPOS,&abd);
	*rc=abd.rc;
    {
        //RECT r;
        //SetWindowRect(hwnd,&r);
        //OnMoving(hwnd,&r);
    }
}

int Docking_IsDocked(WPARAM wParam,LPARAM lParam)
{
	return docked;
}

int Docking_ProcessWindowMessage(WPARAM wParam,LPARAM lParam)
{
	APPBARDATA abd;
	static int draggingTitle;
	MSG *msg=(MSG*)wParam;

	if(msg->message==WM_DESTROY) 
		DBWriteContactSettingByte(NULL,"CList","Docked",(BYTE)docked);
  
	if(!docked && msg->message!=WM_CREATE && msg->message!=WM_MOVING && msg->message!=WM_CREATEDOCKED && msg->message != WM_MOVE && msg->message != WM_SIZE) return 0;
	switch(msg->message) {
		case WM_CREATE:
			//if(GetSystemMetrics(SM_CMONITORS)>1) return 0;
			if(DBGetContactSettingByte(NULL,"CList","Docked",0)) 
            {
                PostMessage(msg->hwnd,WM_CREATEDOCKED,0,0);
            }
			draggingTitle=0;
			return 0;

		case WM_CREATEDOCKED:
			//we need to post a message just after creation to let main message function do some work
			docked=(int)(char)DBGetContactSettingByte(NULL,"CList","Docked",0);
			if(IsWindowVisible(msg->hwnd) && !IsIconic(msg->hwnd)) {
				RECT rc, rcMonitor;
				ZeroMemory(&abd,sizeof(abd));
				abd.cbSize=sizeof(abd);
				abd.hWnd=msg->hwnd;
				abd.lParam=0;
				abd.uCallbackMessage=WM_DOCKCALLBACK;
				SHAppBarMessage(ABM_NEW,&abd);
				GetWindowRect(msg->hwnd,&rc);
				Docking_GetMonitorRectFromWindow(msg->hwnd,&rcMonitor);
				Docking_AdjustPosition(msg->hwnd,&rcMonitor,&rc);
				MoveWindow(msg->hwnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
				dock_prevent_moving=0;
				OnMoving(msg->hwnd,&rc);
				dock_prevent_moving=1;
				ReposButtons(msg->hwnd,0,NULL);
			}
			break;
    case WM_CAPTURECHANGED:
      ReposButtons(msg->hwnd,0,NULL);
      return 0;
		case WM_ACTIVATE:
			ZeroMemory(&abd,sizeof(abd));
			abd.cbSize=sizeof(abd);
			abd.hWnd=msg->hwnd;
			SHAppBarMessage(ABM_ACTIVATE,&abd);
			return 0;
   case WM_SIZE:
      ReposButtons(msg->hwnd,1,NULL);
      return 0;

		case WM_WINDOWPOSCHANGED:
			{
			 if (docked) 
       {
         TRACE("REPOS\n");
         ReposButtons(msg->hwnd,0,NULL);
       }
      return 0;
			ZeroMemory(&abd,sizeof(abd));
			abd.cbSize=sizeof(abd);
			abd.hWnd=msg->hwnd;
			SHAppBarMessage(ABM_WINDOWPOSCHANGED,&abd);
      ReposButtons(msg->hwnd,0,NULL);
			return 0;
			}
		case WM_MOVING:
			{
				RECT rcMonitor;
				RECT rcWindow;
				RECT *rc;
				int dx=0;
				POINT ptCursor;
                if (docked) return 0;
				// stop early
				if(!(GetAsyncKeyState(VK_CONTROL)&0x8000)) return 0;

				// GetMessagePos() is no good, position is always unsigned
				GetCursorPos(&ptCursor);
				GetWindowRect(msg->hwnd,&rcWindow);
				dock_drag_dx=rcWindow.left-ptCursor.x;
				dock_drag_dy=rcWindow.top-ptCursor.y;
				Docking_GetMonitorRectFromPoint(ptCursor,&rcMonitor);
				
				if(((ptCursor.x<rcMonitor.left+EDGESENSITIVITY) 
					|| (ptCursor.x>=rcMonitor.right-EDGESENSITIVITY))
					)
				{
					ZeroMemory(&abd,sizeof(abd));
					abd.cbSize=sizeof(abd);
					abd.hWnd=msg->hwnd;
					abd.lParam=0;
					abd.uCallbackMessage=WM_DOCKCALLBACK;
					SHAppBarMessage(ABM_NEW,&abd);
					if(ptCursor.x<rcMonitor.left+EDGESENSITIVITY) docked=DOCKED_LEFT;
					else docked=DOCKED_RIGHT;
				//	TempDock=1;				
					GetWindowRect(msg->hwnd,(LPRECT)msg->lParam);
					rc=(RECT*)msg->lParam;
					if (docked==DOCKED_RIGHT)
						dx=(rc->right>rcMonitor.right)?rc->right-rcMonitor.right:0;
					else
						dx=(rc->left<rcMonitor.left)?rc->left-rcMonitor.left:0;
					OffsetRect(rc,-dx,0);
					Docking_AdjustPosition(msg->hwnd,(LPRECT)&rcMonitor,(LPRECT)msg->lParam);
					SendMessage(msg->hwnd,WM_SIZE,0,0);				
					dock_prevent_moving=0;
                    OnMoving(msg->hwnd,(LPRECT)msg->lParam);
					dock_prevent_moving=1;
					mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
					DBWriteContactSettingByte(NULL,"CList","Docked",(BYTE)docked);
          ReposButtons(msg->hwnd,0,NULL);
					return TRUE;
				}
				return 0;
			}
        case WM_EXITSIZEMOVE:
            {
              RECT rcMonitor;
              RECT rcWindow;
			  if (TempDock) TempDock=0;
             GetWindowRect(msg->hwnd,&rcWindow);
			  Docking_GetMonitorRectFromWindow(msg->hwnd,&rcMonitor);
			  Docking_AdjustPosition(msg->hwnd,&rcMonitor,&rcWindow);
			  *((LRESULT*)lParam)=TRUE;
			  dock_prevent_moving=0;
			  TRACE("WM_EXITSIZEMOVE");
			  SetWindowPos(msg->hwnd,0,rcWindow.left,rcWindow.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSENDCHANGING);
              OnMoving(msg->hwnd,&rcWindow);
              ReposButtons(msg->hwnd,0,NULL);//-=-=-=
			  dock_prevent_moving=1;		  
              //PostMessage(msg->hwnd,WM_MOVE,0,(MAKELPARAM(rcWindow.left,rcWindow.top)));
              return 1;
            }

		case WM_MOVE:
		{

			if(docked && 0) {
				RECT rc, rcMonitor;
				Docking_GetMonitorRectFromWindow(msg->hwnd,&rcMonitor);
				GetWindowRect(msg->hwnd,&rc);
				Docking_AdjustPosition(msg->hwnd,&rcMonitor,&rc);
				MoveWindow(msg->hwnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
        OnMoving(msg->hwnd,&rc); 
        ReposButtons(msg->hwnd,0,NULL);//-=-=-=
       
				return 1;
			}
      ReposButtons(msg->hwnd,2,NULL);
			return 0;
		}
		case WM_SIZING:
			{
        
			/*RECT rcMonitor;
			Docking_GetMonitorRectFromWindow(msg->hwnd,&rcMonitor);
			Docking_AdjustPosition(msg->hwnd,&rcMonitor,(LPRECT)msg->lParam);
			*((LRESULT*)lParam)=TRUE;
            */
			RECT rc;
			int dx=0, dy=0;
			int  d=0; 
      if (docked) ReposButtons(msg->hwnd,0,NULL);
      return FALSE;
			rc=*(RECT*)(msg->lParam);
			
			dock_prevent_moving=0;
			TRACE("WM_SIZING:  "); 
            OnMoving(msg->hwnd,&rc);
            //-=-=-=		
			return TRUE;
			}
		case WM_SHOWWINDOW:
			if(msg->lParam) return 0;
			if((msg->wParam && docked<0) || (!msg->wParam && docked>0)) docked=-docked;
			ZeroMemory(&abd,sizeof(abd));
			abd.cbSize=sizeof(abd);
			abd.hWnd=msg->hwnd;
			if(msg->wParam) {
				RECT rc, rcMonitor;
				Docking_GetMonitorRectFromWindow(msg->hwnd,&rcMonitor);
				abd.lParam=0;
				abd.uCallbackMessage=WM_DOCKCALLBACK;
				SHAppBarMessage(ABM_NEW,&abd);
				GetWindowRect(msg->hwnd,&rc);
				Docking_AdjustPosition(msg->hwnd,&rcMonitor,&rc);
				MoveWindow(msg->hwnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,FALSE);
				TRACE("WM_SHOWWINDOW:  ");
                OnMoving(msg->hwnd,&rc);
                ReposButtons(msg->hwnd,0,NULL);//-=-=-=
			}
			else {
				SHAppBarMessage(ABM_REMOVE,&abd);
			}
			return 0;
		case WM_NCHITTEST:
		{	LONG result;
			result=DefWindowProc(msg->hwnd,WM_NCHITTEST,msg->wParam,msg->lParam);
			if(result==HTSIZE || result==HTTOP || result==HTTOPLEFT || result==HTTOPRIGHT ||
			   result==HTBOTTOM || result==HTBOTTOMRIGHT || result==HTBOTTOMLEFT) {*((LRESULT*)lParam)=HTCLIENT; return TRUE;}
			if(docked==DOCKED_LEFT && result==HTLEFT) {*((LRESULT*)lParam)=HTCLIENT; return TRUE;}
			if(docked==DOCKED_RIGHT && result==HTRIGHT) {*((LRESULT*)lParam)=HTCLIENT; return TRUE;}
		
			
			return 0;
		}
		case WM_SYSCOMMAND:
			if((msg->wParam&0xFFF0)!=SC_MOVE) return 0;
			SetActiveWindow(msg->hwnd);
			SetCapture(msg->hwnd);
			draggingTitle=1;
			*((LRESULT*)lParam)=0;
			return TRUE;
		case WM_MOUSEMOVE:

			if(!draggingTitle) return 0;
		{	RECT rc;
			POINT pt;
			GetClientRect(msg->hwnd,&rc);
			if(((docked==DOCKED_LEFT || docked==-DOCKED_LEFT) && (short)LOWORD(msg->lParam)>rc.right) ||
			   ((docked==DOCKED_RIGHT || docked==-DOCKED_RIGHT) && (short)LOWORD(msg->lParam)<0)) {
				ReleaseCapture();
				draggingTitle=0;
				ZeroMemory(&abd,sizeof(abd));
				abd.cbSize=sizeof(abd);
				abd.hWnd=msg->hwnd;
				SHAppBarMessage(ABM_REMOVE,&abd);
				docked=0;
				GetCursorPos(&pt);
				PostMessage(msg->hwnd,WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
				SetWindowPos(msg->hwnd,0,pt.x-rc.right/2,pt.y-GetSystemMetrics(SM_CYFRAME)-GetSystemMetrics(SM_CYSMCAPTION)/2,DBGetContactSettingDword(NULL,"CList","Width",0),DBGetContactSettingDword(NULL,"CList","Height",0),SWP_NOZORDER);
				DBWriteContactSettingByte(NULL,"CList","Docked",(BYTE)docked);
       // ReposButtons(msg->hwnd,0);
			}
			return 1;
		}
		case WM_LBUTTONUP:
			if(draggingTitle) {
				ReleaseCapture();
				draggingTitle=0;
			}
			return 0;
		case WM_DOCKCALLBACK:
			switch(msg->wParam) {
				case ABN_WINDOWARRANGE:
					ShowWindowNew(msg->hwnd,msg->lParam?SW_HIDE:SW_SHOW);
                    {

						RECT rc, rcMonitor;
						Docking_GetMonitorRectFromWindow(msg->hwnd,&rcMonitor);
						GetWindowRect(msg->hwnd,&rc);
						Docking_AdjustPosition(msg->hwnd,&rcMonitor,&rc);
						OnMoving(msg->hwnd,&rc); //-=-=-=		
            ReposButtons(msg->hwnd,0,NULL);

						dock_prevent_moving=1;
					}
					break;
			}
			return TRUE;
		case WM_DESTROY:
			if(docked>0) {
				ZeroMemory(&abd,sizeof(abd));
				abd.cbSize=sizeof(abd);
				abd.hWnd=msg->hwnd;
				SHAppBarMessage(ABM_REMOVE,&abd);
        ReposButtons(msg->hwnd,0,NULL);
			}
			return 0;
	}
	return 0;
}
