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
#include "m_clc.h"
#include "m_clui.h"
#include "commonprototypes.h"


static int GetHwndTree(WPARAM wParam,LPARAM lParam)
{
	return (int)hwndContactTree;
}


static int GetHwnd(WPARAM wParam,LPARAM lParam)
{
	return (int)hwndContactList;
}
int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam)
{
   InvalidateFrameImage((WPARAM)hwndStatus,0);
	return 0;
}
int SortList(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
  //  SortClcByTimer(hwndContactList);
 //   HANDLE hClcWindowList1=(HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
    WindowList_Broadcast(hClcWindowList,WM_TIMER,TIMERID_DELAYEDRESORTCLC,0);
    WindowList_Broadcast(hClcWindowList,INTM_SCROLLBARCHANGED,0,0);
    
	return 0;
}

static int GroupAdded(WPARAM wParam,LPARAM lParam)
{
	//CLC does this automatically unless it's a new group
	if(lParam) {
		HANDLE hItem;
		TCHAR szFocusClass[64];
		HWND hwndFocus=GetFocus();

		GetClassName(hwndFocus,szFocusClass,sizeof(szFocusClass));
		if(!lstrcmp(szFocusClass,CLISTCONTROL_CLASS)) {
			hItem=(HANDLE)SendMessage(hwndFocus,CLM_FINDGROUP,wParam,0);
			if(hItem) SendMessage(hwndFocus,CLM_EDITLABEL,(WPARAM)hItem,0);
		}
	}
	return 0;
}

static int ContactSetIcon(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ContactDeleted(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ContactAdded(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ListBeginRebuild(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ListEndRebuild(WPARAM wParam,LPARAM lParam)
{
	int rebuild=0;
	//CLC does this automatically, but we need to force it if hideoffline or hideempty has changed
	if((DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT)==0)!=((GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_HIDEOFFLINE)==0)) {
		if(DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT))
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)|CLS_HIDEOFFLINE);
		else
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)&~CLS_HIDEOFFLINE);
		rebuild=1;
	}
	if((DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT)==0)!=((GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_HIDEEMPTYGROUPS)==0)) {
		if(DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT))
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)|CLS_HIDEEMPTYGROUPS);
		else
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)&~CLS_HIDEEMPTYGROUPS);
		rebuild=1;
	}
	if((DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT)==0)!=((GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_USEGROUPS)==0)) {
		if(DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT))
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)|CLS_USEGROUPS);
		else
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)&~CLS_USEGROUPS);
		rebuild=1;
	}
	if(rebuild) SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
	return 0;
}

static int ContactRenamed(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int GetCaps(WPARAM wParam,LPARAM lParam)
{
	switch(wParam) {
		case CLUICAPS_FLAGS1:
			return CLUIF_HIDEEMPTYGROUPS|CLUIF_DISABLEGROUPS|CLUIF_HASONTOPOPTION|CLUIF_HASAUTOHIDEOPTION;
	}
	return 0;
}

static int MetaSupportCheck(WPARAM wParam,LPARAM lParam)
{return 1;}
int LoadCluiServices(void)
{
	CreateServiceFunction(MS_CLUI_GETHWND,GetHwnd);
	CreateServiceFunction(MS_CLUI_GETHWNDTREE,GetHwndTree);
	CreateServiceFunction(MS_CLUI_PROTOCOLSTATUSCHANGED,CluiProtocolStatusChanged);
	CreateServiceFunction(MS_CLUI_GROUPADDED,GroupAdded);
	CreateServiceFunction(MS_CLUI_CONTACTSETICON,ContactSetIcon);
	CreateServiceFunction(MS_CLUI_CONTACTADDED,ContactAdded);
	CreateServiceFunction(MS_CLUI_CONTACTDELETED,ContactDeleted);
	CreateServiceFunction(MS_CLUI_CONTACTRENAMED,ContactRenamed);
	CreateServiceFunction(MS_CLUI_LISTBEGINREBUILD,ListBeginRebuild);
	CreateServiceFunction(MS_CLUI_LISTENDREBUILD,ListEndRebuild);
	CreateServiceFunction(MS_CLUI_SORTLIST,SortList);
	CreateServiceFunction(MS_CLUI_GETCAPS,GetCaps);
    CreateServiceFunction(MS_CLUI_METASUPPORT,MetaSupportCheck);
	return 0;
}