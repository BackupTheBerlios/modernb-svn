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
#include "clc.h"
#include "commonprototypes.h"

//loads of stuff that didn't really fit anywhere else
extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern HANDLE hHideInfoTipEvent;
extern BOOL ON_SIZING_CYCLE;
extern void InvalidateDisplayNameCacheEntry(HANDLE hContact);

char *GetGroupCountsText(struct ClcData *dat,struct ClcContact *contact)
{
	static char szName[32];
	int onlineCount,totalCount;
	struct ClcGroup *group,*topgroup;
  if (IsBadCodePtr((FARPROC)contact) || IsBadCodePtr((FARPROC)dat)) return NULL;
	if(contact->type!=CLCIT_GROUP || !(dat->exStyle&CLS_EX_SHOWGROUPCOUNTS))
		return NULL;
	group=topgroup=contact->group;
	onlineCount=0;
  if (IsBadCodePtr((FARPROC)group)) return NULL;
	totalCount=group->totalMembers;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			if(group==topgroup) break;
			group=group->parent;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			totalCount+=group->totalMembers;
			continue;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_CONTACT)
			if(group->contact[group->scanIndex].flags&CONTACTF_ONLINE && !group->contact[group->scanIndex].isSubcontact) onlineCount++;
		group->scanIndex++;
	}
	if(onlineCount==0 && dat->exStyle&CLS_EX_HIDECOUNTSWHENEMPTY) return NULL;
	_snprintf(szName,sizeof(szName),"(%u/%u)",onlineCount,totalCount);
	return mir_strdup(szName);
}

BOOL RectHitTest(RECT *rc, int testx, int testy)
{
	return testx >= rc->left && testx < rc->right && testy >= rc->top && testy < rc->bottom;
}


int HitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags)
{
	struct ClcContact *hitcontact;
	struct ClcGroup *hitgroup;
	int hit;
	RECT clRect;

	if(flags) *flags=0;
	GetClientRect(hwnd,&clRect);
	if(testx<0 || testy<0 || testy>=clRect.bottom || testx>=clRect.right) {
		if(flags) {
			if(testx<0) *flags|=CLCHT_TOLEFT;
			else if(testx>=clRect.right) *flags|=CLCHT_TORIGHT;
			if(testy<0) *flags|=CLCHT_ABOVE;
			else if(testy>=clRect.bottom) *flags|=CLCHT_BELOW;
		}
		return -1;
	}
	if(testx<dat->leftMargin) {
		if(flags) *flags|=CLCHT_INLEFTMARGIN|CLCHT_NOWHERE;
		return -1;
	}

	// Get hit item 
	hit = RowHeights_HitTest(dat, dat->yScroll + testy);

	if (hit != -1) 
		hit = GetRowByIndex(dat, hit, &hitcontact, &hitgroup);
	
	if(hit==-1) {
		if(flags) *flags|=CLCHT_NOWHERE|CLCHT_BELOWITEMS;
		return -1;
	}
	if(contact) *contact=hitcontact;
	if(group) *group=hitgroup;
	/////////

	if (testx<hitcontact->pos_indent) 
	{
		if(flags) *flags|=CLCHT_ONITEMINDENT;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_check, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMCHECK;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_avatar, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMICON;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_icon, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMICON;
		return hit;
	}
	
//	if (testx>hitcontact->pos_extra) {
//		if(flags)
		{
//			int c = -1;
			int i;
			for(i = 0; i < dat->extraColumnsCount; i++) 
			{  
			if (RectHitTest(&hitcontact->pos_extra[i], testx, testy)) 
					{
				if(flags) *flags|=CLCHT_ONITEMEXTRA|(i<<24);
						return hit;
					}
				}
			}
	
	if (DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==1) // || DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==2)
		{
		if(flags) *flags|=CLCHT_ONITEMLABEL;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_label, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMLABEL;
		return hit;
	}

	if(flags) *flags|=CLCHT_NOWHERE;
	return -1;
}

void ScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth)
{
	DWORD startTick,nowTick;
	int oldy=dat->yScroll;
	RECT clRect,rcInvalidate;
	int maxy,previousy;

	if(dat->iHotTrack!=-1 && dat->yScroll != desty) {
		InvalidateItem(hwnd,dat,dat->iHotTrack);
		dat->iHotTrack=-1;
		ReleaseCapture();
	}
	GetClientRect(hwnd,&clRect);
	rcInvalidate=clRect;
	//maxy=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-clRect.bottom;
	maxy=RowHeights_GetTotalHeight(dat)-clRect.bottom;
	if(desty>maxy) desty=maxy;
	if(desty<0) desty=0;
	if(abs(desty-dat->yScroll)<4) noSmooth=1;
	if(!noSmooth && dat->exStyle&CLS_EX_NOSMOOTHSCROLLING) noSmooth=1;
	previousy=dat->yScroll;
	if(!noSmooth) {
		startTick=GetTickCount();
		for(;;) {
			nowTick=GetTickCount();
			if(nowTick>=startTick+dat->scrollTime) break;
			dat->yScroll=oldy+(desty-oldy)*(int)(nowTick-startTick)/dat->scrollTime;
			if(/*dat->backgroundBmpUse&CLBF_SCROLL || dat->hBmpBackground==NULL &&*/FALSE)
				ScrollWindowEx(hwnd,0,previousy-dat->yScroll,NULL,NULL,NULL,NULL,SW_INVALIDATE);
			else
      {
        UpdateFrameImage((WPARAM) hwnd, (LPARAM) 0); 
				//InvalidateRectZ(hwnd,NULL,FALSE);
      }
			previousy=dat->yScroll;
		  SetScrollPos(hwnd,SB_VERT,dat->yScroll,TRUE);
      UpdateFrameImage((WPARAM) hwnd, (LPARAM) 0); 
			UpdateWindow(hwnd);
		}
	}
	dat->yScroll=desty;
	if((dat->backgroundBmpUse&CLBF_SCROLL || dat->hBmpBackground==NULL) && FALSE)
		ScrollWindowEx(hwnd,0,previousy-dat->yScroll,NULL,NULL,NULL,NULL,SW_INVALIDATE);
	else
		InvalidateRectZ(hwnd,NULL,FALSE);
	SetScrollPos(hwnd,SB_VERT,dat->yScroll,TRUE);
}

void EnsureVisible(HWND hwnd,struct ClcData *dat,int iItem,int partialOk)
{
	int itemy,newy;
	int moved=0;
	RECT clRect;


	if (dat==NULL||IsBadCodePtr((void *)dat)||!dat->row_heights) 
	{
		TRACE("dat is null __FILE____LINE__");
		return ;
	};

	GetClientRect(hwnd,&clRect);
	itemy=RowHeights_GetItemTopY(dat,iItem);
	if(partialOk) {
		if(itemy+dat->row_heights[iItem]-1<dat->yScroll) {newy=itemy; moved=1;}
		else if(itemy>=dat->yScroll+clRect.bottom) {newy=itemy-clRect.bottom+dat->row_heights[iItem]; moved=1;}
	}
	else {
		if(itemy<dat->yScroll) {newy=itemy; moved=1;}
		else if(itemy>=dat->yScroll+clRect.bottom-dat->row_heights[iItem]) {newy=itemy-clRect.bottom+dat->row_heights[iItem]; moved=1;}
	}
	if(moved)
		ScrollTo(hwnd,dat,newy,0);
}

void RecalcScrollBar(HWND hwnd,struct ClcData *dat)
{
	SCROLLINFO si={0};
	RECT clRect;
	NMCLISTCONTROL nm;

#ifdef _DEBUG
    TRACE("RecalcScrollBar\r\n");
#endif

	RowHeights_CalcRowHeights(dat, hwnd);

	GetClientRect(hwnd,&clRect);
	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;
	si.nMin=0;
	//si.nMax=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-1;
	si.nMax=RowHeights_GetTotalHeight(dat)-1;
	si.nPage=clRect.bottom;
	si.nPos=dat->yScroll;
	
	nm.hdr.code=CLN_LISTSIZECHANGE;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.pt.y=si.nMax;

	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);       //post

	GetClientRect(hwnd,&clRect);
	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;
	si.nMin=0;
	//si.nMax=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-1;
	si.nMax=RowHeights_GetTotalHeight(dat)-1;
	si.nPage=clRect.bottom;
	si.nPos=dat->yScroll;

	if ( GetWindowLong(hwnd,GWL_STYLE)&CLS_CONTACTLIST ) {
		if ( dat->noVScrollbar==0 ) SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
		//else SetScrollInfo(hwnd,SB_VERT,&si,FALSE);
	} else SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
    ON_SIZING_CYCLE=1;
	ScrollTo(hwnd,dat,dat->yScroll,1);
    ON_SIZING_CYCLE=0;
	//ShowScrollBar(hwnd,SB_VERT,dat->noVScrollbar==1 ? FALSE : TRUE);
	//nm.hdr.code=CLN_LISTSIZECHANGE;
	//nm.hdr.hwndFrom=hwnd;
	//nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	//nm.pt.y=si.nMax;
	//SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}

void SetGroupExpand(HWND hwnd,struct ClcData *dat,struct ClcGroup *group,int newState)
{
	int contentCount;
	int groupy;
	int newy;
	int posy;
	RECT clRect;
	NMCLISTCONTROL nm;

	if(newState==-1) group->expanded^=1;
	else {
		if(group->expanded==(newState!=0)) return;
		group->expanded=newState!=0;
	}
	InvalidateRectZ(hwnd,NULL,FALSE);

	if (group->expanded)
	contentCount=GetGroupContentsCount(group,1);
	else
		contentCount=0;

	groupy=GetRowsPriorTo(&dat->list,group,-1);
	if(dat->selection>groupy && dat->selection<groupy+contentCount) dat->selection=groupy;
	RecalcScrollBar(hwnd,dat);

	GetClientRect(hwnd,&clRect);
	newy=dat->yScroll;
	posy = RowHeights_GetItemBottomY(dat, groupy+contentCount);
	if(posy>=newy+clRect.bottom)
		newy=posy-clRect.bottom;
	posy = RowHeights_GetItemTopY(dat, groupy);
	if(newy>posy) newy=posy;
	ScrollTo(hwnd,dat,newy,0);
	nm.hdr.code=CLN_EXPANDED;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.hItem=(HANDLE)group->groupId;
	nm.action=group->expanded;
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}

void DoSelectionDefaultAction(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;

	if(dat->selection==-1) return;
	dat->szQuickSearch[0]=0;
	if(GetRowByIndex(dat,dat->selection,&contact,NULL)==-1) return;
	if(contact->type==CLCIT_GROUP)
		SetGroupExpand(hwnd,dat,contact->group,-1);
	if(contact->type==CLCIT_CONTACT)
		CallService(MS_CLIST_CONTACTDOUBLECLICKED,(WPARAM)contact->hContact,0);
}

int FindRowByText(HWND hwnd,struct ClcData *dat,const TCHAR *text,int prefixOk)
{
	struct ClcGroup *group=&dat->list;
	int testlen=lstrlen(text);

	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
			group->scanIndex++;
			continue;
		}
		if(group->contact[group->scanIndex].type!=CLCIT_DIVIDER) {
			if((prefixOk && !_tcsnicmp(text,group->contact[group->scanIndex].szText,testlen)) ||
			   (!prefixOk && !lstrcmpi(text,group->contact[group->scanIndex].szText))) {
				struct ClcGroup *contactGroup=group;
				int contactScanIndex=group->scanIndex;
				for(;group;group=group->parent) SetGroupExpand(hwnd,dat,group,1);
				return GetRowsPriorTo(&dat->list,contactGroup,contactScanIndex);
			}
			if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				if(!(dat->exStyle&CLS_EX_QUICKSEARCHVISONLY) || group->contact[group->scanIndex].group->expanded) {
					group=group->contact[group->scanIndex].group;
					group->scanIndex=0;
					continue;
				}
			}
		}
		group->scanIndex++;
	}
	return -1;
}

void EndRename(HWND hwnd,struct ClcData *dat,int save)
{   
    HWND hwndEdit;
    if (!dat) return;
	hwndEdit=dat->hwndRenameEdit;

	if(dat->hwndRenameEdit==NULL) return;
	dat->hwndRenameEdit=NULL;
	if(save) {
		TCHAR text[120];
		struct ClcContact *contact;
		GetWindowText(hwndEdit,text,sizeof(text));
		if(GetRowByIndex(dat,dat->selection,&contact,NULL)!=-1) {
			if(lstrcmp(contact->szText,text)) {
				if(contact->type==CLCIT_GROUP&&!_tcsstr(text,TEXT("\\"))) {
					TCHAR szFullName[256];
					if(contact->group->parent && contact->group->parent->parent)
					{
						TCHAR * tc=(TCHAR*)CallService(MS_CLIST_GROUPGETNAMET,(WPARAM)contact->group->parent->groupId,(LPARAM)(int*)NULL);
							_sntprintf(szFullName,sizeof(szFullName),TEXT("%s\\%s"),tc,text);						
					}
					else lstrcpyn(szFullName,text,sizeof(szFullName));
					CallService(MS_CLIST_GROUPRENAME,contact->groupId,(LPARAM)szFullName);
				}
				else if(contact->type==CLCIT_CONTACT) {
					TCHAR *otherName=mir_strdupT((TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)contact->hContact,GCDNF_NOMYHANDLE|GCDNF_UNICODE/*|TODO: UNICODE*/));
					InvalidateDisplayNameCacheEntry(contact->hContact);
					if(text[0]==TEXT('\0')) {
						DBDeleteContactSetting(contact->hContact,"CList","MyHandle");
					}
					else {
						if(!lstrcmp(otherName,text)) DBDeleteContactSetting(contact->hContact,"CList","MyHandle");
						else DBWriteContactSettingTString(contact->hContact,"CList","MyHandle",text);
					}
					if (otherName) mir_free(otherName);
					InvalidateDisplayNameCacheEntry(contact->hContact);
				}
			}
		}
	}
	DestroyWindow(hwndEdit);
}

void DeleteFromContactList(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;
	if(dat->selection==-1) return;
	dat->szQuickSearch[0]=0;
	if(GetRowByIndex(dat,dat->selection,&contact,NULL)==-1) return;
	switch (contact->type) {
		case CLCIT_GROUP: CallService(MS_CLIST_GROUPDELETE,(WPARAM)(HANDLE)contact->groupId,0); break;
		case CLCIT_CONTACT: 
			CallService("CList/DeleteContactCommand",(WPARAM)(HANDLE)
			contact->hContact,(LPARAM)hwnd); break;
	}
}

static WNDPROC OldRenameEditWndProc;
static LRESULT CALLBACK RenameEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   // struct ClcData* dat=(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA);   
	switch(msg) {
		case WM_KEYDOWN:
			switch(wParam) {
				case VK_RETURN:
					EndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA),1);
					return 0;
				case VK_ESCAPE:
					EndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA),0);
					return 0;
			}
			break;
		case WM_GETDLGCODE:
			if(lParam) {
				MSG *msg=(MSG*)lParam;
				if(msg->message==WM_KEYDOWN && msg->wParam==VK_TAB) return 0;
				if(msg->message==WM_CHAR && msg->wParam=='\t') return 0;
			}
			return DLGC_WANTMESSAGE;
		case WM_KILLFOCUS:
			EndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA),1);
			return 0;
	}
	return CallWindowProc(OldRenameEditWndProc,hwnd,msg,wParam,lParam);
}

void BeginRenameSelection(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;
	struct ClcGroup *group;
	int indent,x,y,subident, h,w;
	RECT clRect;
  RECT r;


	KillTimer(hwnd,TIMERID_RENAME);
	ReleaseCapture();
	dat->iHotTrack=-1;
	dat->selection=GetRowByIndex(dat,dat->selection,&contact,&group);
	if(dat->selection==-1) return;
	if(contact->type!=CLCIT_CONTACT && contact->type!=CLCIT_GROUP) return;
	
	if (contact->type==CLCIT_CONTACT && contact->isSubcontact)
        subident=dat->subIndent;
	else 
		subident=0;
	
	for(indent=0;group->parent;indent++,group=group->parent);
	GetClientRect(hwnd,&clRect);
    x=indent*dat->groupIndent+dat->iconXSpace-2+subident;
	w=clRect.right-x;
	y=RowHeights_GetItemTopY(dat, dat->selection)-dat->yScroll;
    h=dat->row_heights[dat->selection];
    {
        int i;
        for (i=0; i<=FONTID_MAX; i++)
           if (h<dat->fontInfo[i].fontHeight+2) h=dat->fontInfo[i].fontHeight+2;
    }
    //TODO contact->pos_label 

  
{
      
       RECT rectW;      
       GetWindowRect(hwnd,&rectW);
//       w=contact->pos_full_first_row.right-contact->pos_full_first_row.left;
//       h=contact->pos_full_first_row.bottom-contact->pos_full_first_row.top;
       //w=clRect.right-x;
       x+=rectW.left;//+contact->pos_full_first_row.left;
       y+=rectW.top;//+contact->pos_full_first_row.top;
  }

  {
    int a=0;
    if (contact->type==CLCIT_GROUP)
    {
      if (dat->row_align_group_mode==1) a|=ES_CENTER;
      else if (dat->row_align_group_mode==2) a|=ES_RIGHT;
    }
    if (dat->text_rtl) a|=EN_ALIGN_RTL_EC;
	  dat->hwndRenameEdit=CreateWindow(TEXT("EDIT"),contact->szText,WS_POPUP|WS_BORDER|ES_AUTOHSCROLL|a,x,y,w,h,hwnd,NULL,g_hInst,NULL);
  }
    SetWindowLong(dat->hwndRenameEdit,GWL_STYLE,GetWindowLong(dat->hwndRenameEdit,GWL_STYLE)&(~WS_CAPTION)|WS_BORDER);
    SetWindowLong(dat->hwndRenameEdit,GWL_USERDATA,(LONG)dat);
	OldRenameEditWndProc=(WNDPROC)SetWindowLong(dat->hwndRenameEdit,GWL_WNDPROC,(LONG)RenameEditSubclassProc);
	SendMessage(dat->hwndRenameEdit,WM_SETFONT,(WPARAM)(contact->type==CLCIT_GROUP?dat->fontInfo[FONTID_GROUPS].hFont:dat->fontInfo[FONTID_CONTACTS].hFont),0);
	SendMessage(dat->hwndRenameEdit,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN|EC_USEFONTINFO,0);
	SendMessage(dat->hwndRenameEdit,EM_SETSEL,0,(LPARAM)(-1));
 // SetWindowLong(dat->hwndRenameEdit,GWL_USERDATA,(LONG)hwnd);
    r.top=1;
    r.bottom=h-1;
    r.left=0;
    r.right=w;

    //ES_MULTILINE

    SendMessage(dat->hwndRenameEdit,EM_SETRECT,0,(LPARAM)(&r));
    
	ShowWindowNew(dat->hwndRenameEdit,SW_SHOW);
  SetWindowPos(dat->hwndRenameEdit,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetFocus(dat->hwndRenameEdit);
}

int GetDropTargetInformation(HWND hwnd,struct ClcData *dat,POINT pt)
{
	RECT clRect;
	int hit;
	struct ClcContact *contact,*movecontact;
	struct ClcGroup *group,*movegroup;
	DWORD hitFlags;

	GetClientRect(hwnd,&clRect);
	dat->selection=dat->iDragItem;
	dat->iInsertionMark=-1;
	if(!PtInRect(&clRect,pt)) return DROPTARGET_OUTSIDE;

	hit=HitTest(hwnd,dat,pt.x,pt.y,&contact,&group,&hitFlags);
	GetRowByIndex(dat,dat->iDragItem,&movecontact,&movegroup);
	if(hit==dat->iDragItem) return DROPTARGET_ONSELF;
	if(hit==-1 || hitFlags&CLCHT_ONITEMEXTRA) return DROPTARGET_ONNOTHING;

	if(movecontact->type==CLCIT_GROUP) {
		struct ClcContact *bottomcontact=NULL,*topcontact=NULL;
		struct ClcGroup *topgroup=NULL;
		int topItem=-1,bottomItem;
		int ok=0;
		if(pt.y+dat->yScroll<RowHeights_GetItemTopY(dat,hit)+dat->insertionMarkHitHeight) {
			//could be insertion mark (above)
			topItem=hit-1; bottomItem=hit;
			bottomcontact=contact;
			topItem=GetRowByIndex(dat,topItem,&topcontact,&topgroup);
			ok=1;
		}
		if(pt.y+dat->yScroll>=RowHeights_GetItemTopY(dat,hit+1)-dat->insertionMarkHitHeight) {
			//could be insertion mark (below)
			topItem=hit; bottomItem=hit+1;
			topcontact=contact; topgroup=group;
			bottomItem=GetRowByIndex(dat,bottomItem,&bottomcontact,NULL);
			ok=1;
		}
		if(ok) {
			ok=0;
			if(bottomItem==-1 || bottomcontact->type!=CLCIT_GROUP) {	   //need to special-case moving to end
				if(topItem!=dat->iDragItem) {
					for(;topgroup;topgroup=topgroup->parent) {
						if(topgroup==movecontact->group) break;
						if(topgroup==movecontact->group->parent) {ok=1; break;}
					}
					if(ok) bottomItem=topItem+1;
				}
			}
			else if(bottomItem!=dat->iDragItem && bottomcontact->type==CLCIT_GROUP && bottomcontact->group->parent==movecontact->group->parent) {
				if(bottomcontact!=movecontact+1) ok=1;
			}
			if(ok) {
			    dat->iInsertionMark=bottomItem;
				dat->selection=-1;
				return DROPTARGET_INSERTION;
			}
		}
	}
	if(contact->type==CLCIT_GROUP) {
		if(dat->iInsertionMark==-1) {
			if(movecontact->type==CLCIT_GROUP) {	 //check not moving onto its own subgroup
				for(;group;group=group->parent) if(group==movecontact->group) return DROPTARGET_ONSELF;
			}
			dat->selection=hit;
			return DROPTARGET_ONGROUP;
		}
	}
    dat->selection=hit;

    if (!MyStrCmp(contact->proto,"MetaContacts")&& (ServiceExists(MS_MC_ADDTOMETA))) return DROPTARGET_ONMETACONTACT;
    if (contact->isSubcontact && (ServiceExists(MS_MC_ADDTOMETA))) return DROPTARGET_ONSUBCONTACT;
	return DROPTARGET_ONCONTACT;
}

int ClcStatusToPf2(int status)
{
	switch(status) {
		case ID_STATUS_ONLINE: return PF2_ONLINE;
		case ID_STATUS_AWAY: return PF2_SHORTAWAY;
		case ID_STATUS_DND: return PF2_HEAVYDND;
		case ID_STATUS_NA: return PF2_LONGAWAY;
		case ID_STATUS_OCCUPIED: return PF2_LIGHTDND;
		case ID_STATUS_FREECHAT: return PF2_FREECHAT;
		case ID_STATUS_INVISIBLE: return PF2_INVISIBLE;
		case ID_STATUS_ONTHEPHONE: return PF2_ONTHEPHONE;
		case ID_STATUS_OUTTOLUNCH: return PF2_OUTTOLUNCH;
		case ID_STATUS_OFFLINE: return MODEF_OFFLINE;
	}
	return 0;
}

int IsHiddenMode(struct ClcData *dat,int status)
{
	return dat->offlineModes&ClcStatusToPf2(status);
}

void HideInfoTip(HWND hwnd,struct ClcData *dat)
{
	CLCINFOTIP it={0};

	if(dat->hInfoTipItem==NULL) return;
	it.isGroup=IsHContactGroup(dat->hInfoTipItem);
	it.hItem=(HANDLE)((unsigned)dat->hInfoTipItem&~HCONTACT_ISGROUP);
	it.cbSize=sizeof(it);
	dat->hInfoTipItem=NULL;
	NotifyEventHooks(hHideInfoTipEvent,0,(LPARAM)&it);
}

void NotifyNewContact(HWND hwnd,HANDLE hContact)
{
	NMCLISTCONTROL nm;
	nm.hdr.code=CLN_NEWCONTACT;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.flags=0;
	nm.hItem=hContact;
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}

void LoadClcOptions(HWND hwnd,struct ClcData *dat)
{ 
  int i;
//	dat->rowHeight=DBGetContactSettingByte(NULL,"CLC","RowHeight",CLCDEFAULT_ROWHEIGHT);
	{	
		LOGFONTA lf;
        HFONT holdfont;
		SIZE fontSize;
		HDC hdc=GetDC(hwnd);
		for(i=0;i<=FONTID_MAX;i++) {
			if(!dat->fontInfo[i].changed) DeleteObject(dat->fontInfo[i].hFont);
			GetFontSetting(i,&lf,&dat->fontInfo[i].colour);
			{
				LONG height;
				HDC hdc=GetDC(NULL);
				height=lf.lfHeight;
				lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
				ReleaseDC(NULL,hdc);				
                
				dat->fontInfo[i].hFont=CreateFontIndirectA(&lf);
            
            lf.lfHeight=height;
			}
            
			dat->fontInfo[i].changed=0;
			holdfont=SelectObject(hdc,dat->fontInfo[i].hFont);
			GetTextExtentPoint32A(hdc,"x",1,&fontSize);
			dat->fontInfo[i].fontHeight=fontSize.cy;
//			if(fontSize.cy>dat->rowHeight && (!DBGetContactSettingByte(NULL,"CLC","DoNotCheckFontSize",0))) dat->rowHeight=fontSize.cy;
			if(holdfont) SelectObject(hdc,holdfont);
		}
		ReleaseDC(hwnd,hdc);
	}
	// Row
	dat->row_min_heigh = DBGetContactSettingWord(NULL,"CList","MinRowHeight",CLCDEFAULT_ROWHEIGHT);
	dat->row_border = DBGetContactSettingWord(NULL,"CList","RowBorder",2);
	dat->row_variable_height = DBGetContactSettingByte(NULL,"CList","VariableRowHeight",1);
	dat->row_align_left_items_to_left = DBGetContactSettingByte(NULL,"CList","AlignLeftItemsToLeft",1);
	dat->row_hide_group_icon = DBGetContactSettingByte(NULL,"CList","HideGroupsIcon",0);
	dat->row_align_right_items_to_right = DBGetContactSettingByte(NULL,"CList","AlignRightItemsToRight",1);
//TODO: Add to settings
  dat->row_align_group_mode=DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",0);
	for (i = 0 ; i < NUM_ITEM_TYPE ; i++)
	{
		char tmp[128];
		mir_snprintf(tmp, sizeof(tmp), "RowPos%d", i);
		dat->row_items[i] = DBGetContactSettingWord(NULL, "CList", tmp, i);
	}

	// Avatar
	if (hwndContactTree == hwnd  || hwndContactTree==NULL)
	{
	dat->avatars_show = DBGetContactSettingByte(NULL,"CList","AvatarsShow",0);
	dat->avatars_draw_border = DBGetContactSettingByte(NULL,"CList","AvatarsDrawBorders",0);
	dat->avatars_border_color = (COLORREF)DBGetContactSettingDword(NULL,"CList","AvatarsBorderColor",0);
	dat->avatars_round_corners = DBGetContactSettingByte(NULL,"CList","AvatarsRoundCorners",1);
	dat->avatars_use_custom_corner_size = DBGetContactSettingByte(NULL,"CList","AvatarsUseCustomCornerSize",0);
	dat->avatars_custom_corner_size = DBGetContactSettingWord(NULL,"CList","AvatarsCustomCornerSize",4);
	dat->avatars_ignore_size_for_row_height = DBGetContactSettingByte(NULL,"CList","AvatarsIgnoreSizeForRow",0);
	dat->avatars_draw_overlay = DBGetContactSettingByte(NULL,"CList","AvatarsDrawOverlay",0);
	dat->avatars_overlay_type = DBGetContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_NORMAL);
	dat->avatars_size = DBGetContactSettingWord(NULL,"CList","AvatarsSize",30);
	}
	else
	{
		dat->avatars_show = 0;
		dat->avatars_draw_border = 0;
		dat->avatars_border_color = 0;
		dat->avatars_round_corners = 0;
		dat->avatars_use_custom_corner_size = 0;
		dat->avatars_custom_corner_size = 4;
		dat->avatars_ignore_size_for_row_height = 0;
		dat->avatars_draw_overlay = 0;
		dat->avatars_overlay_type = SETTING_AVATAR_OVERLAY_TYPE_NORMAL;
		dat->avatars_size = 30;
	}

	// Icon
	if (hwndContactTree == hwnd|| hwndContactTree==NULL)
	{
	dat->icon_hide_on_avatar = DBGetContactSettingByte(NULL,"CList","IconHideOnAvatar",0);
	dat->icon_draw_on_avatar_space = DBGetContactSettingByte(NULL,"CList","IconDrawOnAvatarSpace",0);
	dat->icon_ignore_size_for_row_height = DBGetContactSettingByte(NULL,"CList","IconIgnoreSizeForRownHeight",0);
	}
	else
	{
		dat->icon_hide_on_avatar = 0;
		dat->icon_draw_on_avatar_space = 0;
		dat->icon_ignore_size_for_row_height = 0;
	}

	// Contact time
	if (hwndContactTree == hwnd|| hwndContactTree==NULL)
	{
		dat->contact_time_show = DBGetContactSettingByte(NULL,"CList","ContactTimeShow",0);
		dat->contact_time_show_only_if_different = DBGetContactSettingByte(NULL,"CList","ContactTimeShowOnlyIfDifferent",1);
	}
	else
	{
		dat->contact_time_show = 0;
		dat->contact_time_show_only_if_different = 0;
	}

	{
		const time_t now = time(NULL);
		struct tm gmt = *gmtime(&now);
		time_t gmt_time;
		//gmt.tm_isdst = -1;
		gmt_time = mktime(&gmt);
		dat->local_gmt_diff = (int)difftime(now, gmt_time);

		gmt = *gmtime(&now);
		gmt.tm_isdst = -1;
		gmt_time = mktime(&gmt);
		dat->local_gmt_diff_dst = (int)difftime(now, gmt_time);
	}


	// Text
	dat->text_rtl = DBGetContactSettingByte(NULL,"CList","TextRTL",0);
	dat->text_align_right = DBGetContactSettingByte(NULL,"CList","TextAlignToRight",0);
	dat->text_replace_smileys = DBGetContactSettingByte(NULL,"CList","TextReplaceSmileys",1);
	dat->text_resize_smileys = DBGetContactSettingByte(NULL,"CList","TextResizeSmileys",1);
	dat->text_smiley_height = 0;
	dat->text_use_protocol_smileys = DBGetContactSettingByte(NULL,"CList","TextUseProtocolSmileys",1);

	if (hwndContactTree == hwnd|| hwndContactTree==NULL)
	{
	dat->text_ignore_size_for_row_height = DBGetContactSettingByte(NULL,"CList","TextIgnoreSizeForRownHeight",0);
	}
	else
	{
		dat->text_ignore_size_for_row_height = 0;
	}

	// First line
	dat->first_line_draw_smileys = DBGetContactSettingByte(NULL,"CList","FirstLineDrawSmileys",1);

	// Second line
	if (hwndContactTree == hwnd || hwndContactTree==NULL)
	{
	dat->second_line_show = DBGetContactSettingByte(NULL,"CList","SecondLineShow",1);
	dat->second_line_top_space = DBGetContactSettingWord(NULL,"CList","SecondLineTopSpace",2);
	dat->second_line_draw_smileys = DBGetContactSettingByte(NULL,"CList","SecondLineDrawSmileys",1);
	dat->second_line_type = DBGetContactSettingWord(NULL,"CList","SecondLineType",TEXT_STATUS_MESSAGE);
	{
		DBVARIANT dbv;
		
		if (!DBGetContactSettingTString(NULL, "CList","SecondLineText", &dbv))
		{
			lstrcpyn(dat->second_line_text, dbv.ptszVal, SIZEOF(dat->second_line_text)-1);
			dat->second_line_text[SIZEOF(dat->second_line_text)-1] = '\0';
			DBFreeVariant(&dbv);
		}
		else
		{
			dat->second_line_text[0] = '\0';
		}
	}
	dat->second_line_xstatus_has_priority = DBGetContactSettingByte(NULL,"CList","SecondLineXStatusHasPriority",1);
    dat->second_line_show_status_if_no_away=DBGetContactSettingByte(NULL,"CList","SecondLineShowStatusIfNoAway",0);
	dat->second_line_use_name_and_message_for_xstatus = DBGetContactSettingByte(NULL,"CList","SecondLineUseNameAndMessageForXStatus",0);
	}
	else
	{
		dat->second_line_show = 0;
		dat->second_line_top_space = 0;
		dat->second_line_draw_smileys = 0;
		dat->second_line_type = TEXT_STATUS_MESSAGE;
		dat->second_line_text[0] = '\0';
		dat->second_line_xstatus_has_priority = 1;
		dat->second_line_use_name_and_message_for_xstatus = 0;
	}


	// Third line
	if (hwndContactTree == hwnd || hwndContactTree==NULL)
	{
	dat->third_line_show = DBGetContactSettingByte(NULL,"CList","ThirdLineShow",0);
	dat->third_line_top_space = DBGetContactSettingWord(NULL,"CList","ThirdLineTopSpace",2);
	dat->third_line_draw_smileys = DBGetContactSettingByte(NULL,"CList","ThirdLineDrawSmileys",0);
	dat->third_line_type = DBGetContactSettingWord(NULL,"CList","ThirdLineType",TEXT_STATUS);
	{
		DBVARIANT dbv;
		
		if (!DBGetContactSettingTString(NULL, "CList","ThirdLineText", &dbv))
		{
			lstrcpyn(dat->third_line_text, dbv.ptszVal, SIZEOF(dat->third_line_text)-1);
			dat->third_line_text[SIZEOF(dat->third_line_text)-1] = '\0';
			DBFreeVariant(&dbv);
		}
		else
		{
			dat->third_line_text[0] = '\0';
		}
	}
	dat->third_line_xstatus_has_priority = DBGetContactSettingByte(NULL,"CList","ThirdLineXStatusHasPriority",1);
        dat->third_line_show_status_if_no_away=DBGetContactSettingByte(NULL,"CList","ThirdLineShowStatusIfNoAway",0);
        dat->third_line_use_name_and_message_for_xstatus = DBGetContactSettingByte(NULL,"CList","ThirdLineUseNameAndMessageForXStatus",0);
	}
	else
	{
		dat->third_line_show = 0;
		dat->third_line_top_space = 0;
		dat->third_line_draw_smileys = 0;
		dat->third_line_type = TEXT_STATUS_MESSAGE;
		dat->third_line_text[0] = '\0';
		dat->third_line_xstatus_has_priority = 1;
		dat->third_line_use_name_and_message_for_xstatus = 0;
	}

	dat->leftMargin=DBGetContactSettingByte(NULL,"CLC","LeftMargin",CLCDEFAULT_LEFTMARGIN);
	dat->rightMargin=DBGetContactSettingByte(NULL,"CLC","RightMargin",CLCDEFAULT_RIGHTMARGIN);
	dat->exStyle=DBGetContactSettingDword(NULL,"CLC","ExStyle",GetDefaultExStyle());
	dat->scrollTime=DBGetContactSettingWord(NULL,"CLC","ScrollTime",CLCDEFAULT_SCROLLTIME);
  dat->force_in_dialog=0;
	dat->groupIndent=DBGetContactSettingByte(NULL,"CLC","GroupIndent",CLCDEFAULT_GROUPINDENT);
    dat->subIndent=DBGetContactSettingByte(NULL,"CLC","SubIndent",CLCDEFAULT_GROUPINDENT);
	dat->gammaCorrection=DBGetContactSettingByte(NULL,"CLC","GammaCorrect",CLCDEFAULT_GAMMACORRECT);
	dat->showIdle=DBGetContactSettingByte(NULL,"CLC","ShowIdle",CLCDEFAULT_SHOWIDLE);
	dat->noVScrollbar=DBGetContactSettingByte(NULL,"CLC","NoVScrollBar",0);
	SendMessage(hwnd,INTM_SCROLLBARCHANGED,0,0);
	//ShowScrollBar(hwnd,SB_VERT,dat->noVScrollbar==1 ? FALSE : TRUE);
	if(!dat->bkChanged) {
		DBVARIANT dbv;
		dat->bkColour=DBGetContactSettingDword(NULL,"CLC","BkColour",CLCDEFAULT_BKCOLOUR);
		if(dat->hBmpBackground) {DeleteObject(dat->hBmpBackground); dat->hBmpBackground=NULL;}
		if(DBGetContactSettingByte(NULL,"CLC","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if(!DBGetContactSetting(NULL,"CLC","BkBitmap",&dbv)) {
				dat->hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
				mir_free(dbv.pszVal);
        DBFreeVariant(&dbv);
			}
		}
		dat->backgroundBmpUse=DBGetContactSettingWord(NULL,"CLC","BkBmpUse",CLCDEFAULT_BKBMPUSE);

        dat->MenuBkColor=DBGetContactSettingDword(NULL,"Menu","BkColour",CLCDEFAULT_BKCOLOUR);
        dat->MenuBkHiColor=DBGetContactSettingDword(NULL,"Menu","SelBkColour",CLCDEFAULT_SELBKCOLOUR);
        
        dat->MenuTextColor=DBGetContactSettingDword(NULL,"Menu","TextColour",CLCDEFAULT_TEXTCOLOUR);
        dat->MenuTextHiColor=DBGetContactSettingDword(NULL,"Menu","SelTextColour",CLCDEFAULT_SELTEXTCOLOUR);

        if (dat->hMenuBackground) {DeleteObject(dat->hMenuBackground); dat->hMenuBackground=NULL;}
		if(DBGetContactSettingByte(NULL,"Menu","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if(!DBGetContactSetting(NULL,"Menu","BkBitmap",&dbv)) {
				dat->hMenuBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
				mir_free(dbv.pszVal);
        DBFreeVariant(&dbv);
			}
		}
        dat->MenuBmpUse=DBGetContactSettingWord(NULL,"Menu","BkBmpUse",CLCDEFAULT_BKBMPUSE);
	}
    
	dat->greyoutFlags=DBGetContactSettingDword(NULL,"CLC","GreyoutFlags",CLCDEFAULT_GREYOUTFLAGS);
	dat->offlineModes=DBGetContactSettingDword(NULL,"CLC","OfflineModes",CLCDEFAULT_OFFLINEMODES);
	dat->selBkColour=DBGetContactSettingDword(NULL,"CLC","SelBkColour",CLCDEFAULT_SELBKCOLOUR);
	dat->selTextColour=DBGetContactSettingDword(NULL,"CLC","SelTextColour",CLCDEFAULT_SELTEXTCOLOUR);
	dat->hotTextColour=DBGetContactSettingDword(NULL,"CLC","HotTextColour",CLCDEFAULT_HOTTEXTCOLOUR);
	dat->quickSearchColour=DBGetContactSettingDword(NULL,"CLC","QuickSearchColour",CLCDEFAULT_QUICKSEARCHCOLOUR);
    dat->IsMetaContactsEnabled=
                DBGetContactSettingByte(NULL,"MetaContacts","Enabled",1) && ServiceExists(MS_MC_GETDEFAULTCONTACT);
    {

        dat->MetaIgnoreEmptyExtra=DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",1);
        dat->expandMeta=DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1);
    /*
        style=GetWindowLong(hwnd,GWL_STYLE);
        if (dat->MetaIgnoreEmptyExtra) 
            style&=CLS_EX_MULTICOLUMNALIGNLEFT;
        else
            style&=!(CLS_EX_MULTICOLUMNALIGNLEFT)
    */
    }
	{	NMHDR hdr;
		hdr.code=CLN_OPTIONSCHANGED;
		hdr.hwndFrom=hwnd;
		hdr.idFrom=GetDlgCtrlID(hwnd);
		SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&hdr);
	}
	SendMessage(hwnd,WM_SIZE,0,0);
}

#define GSIF_HASMEMBERS   0x80000000
#define GSIF_ALLCHECKED   0x40000000
#define GSIF_INDEXMASK    0x3FFFFFFF
void RecalculateGroupCheckboxes(HWND hwnd,struct ClcData *dat)
{
	struct ClcGroup *group;
	int check;

	group=&dat->list;
	group->scanIndex=GSIF_ALLCHECKED;
	for(;;) {
		if((group->scanIndex&GSIF_INDEXMASK)==group->contactCount) {
			check=(group->scanIndex&(GSIF_HASMEMBERS|GSIF_ALLCHECKED))==(GSIF_HASMEMBERS|GSIF_ALLCHECKED);
			group=group->parent;
			if(group==NULL) break;
			if(check) group->contact[(group->scanIndex&GSIF_INDEXMASK)].flags|=CONTACTF_CHECKED;
			else {
				group->contact[(group->scanIndex&GSIF_INDEXMASK)].flags&=~CONTACTF_CHECKED;
				group->scanIndex&=~GSIF_ALLCHECKED;
			}
		}
		else if(group->contact[(group->scanIndex&GSIF_INDEXMASK)].type==CLCIT_GROUP) {
			group=group->contact[(group->scanIndex&GSIF_INDEXMASK)].group;
			group->scanIndex=GSIF_ALLCHECKED;
			continue;
		}
		else if(group->contact[(group->scanIndex&GSIF_INDEXMASK)].type==CLCIT_CONTACT) {
			group->scanIndex|=GSIF_HASMEMBERS;
			if(!(group->contact[(group->scanIndex&GSIF_INDEXMASK)].flags&CONTACTF_CHECKED))
				group->scanIndex&=~GSIF_ALLCHECKED;
		}
		group->scanIndex++;
	}
}

void SetGroupChildCheckboxes(struct ClcGroup *group,int checked)
{
	int i;

	for(i=0;i<group->contactCount;i++) {
		if(group->contact[i].type==CLCIT_GROUP) {
			SetGroupChildCheckboxes(group->contact[i].group,checked);
			if(checked) group->contact[i].flags|=CONTACTF_CHECKED;
			else group->contact[i].flags&=~CONTACTF_CHECKED;
		}
		else if(group->contact[i].type==CLCIT_CONTACT) {
			if(checked) group->contact[i].flags|=CONTACTF_CHECKED;
			else group->contact[i].flags&=~CONTACTF_CHECKED;
		}
	}
}
void InvalidateItemByHandle(HWND hwnd, HANDLE hItem)
{
    struct ClcData *dat;
    int k=0;
    dat=(struct ClcData*)GetWindowLong(hwnd,0);
    if (dat)
    {
        if(hItem)
        {
            struct ClcContact *selcontact;
			struct ClcGroup *selgroup;
            if(FindItem(hwnd,dat,hItem,&selcontact,&selgroup,NULL,FALSE))
            {
                k=GetRowsPriorTo(&dat->list,selgroup,selcontact-selgroup->contact);
                k+=selcontact->isSubcontact;
                InvalidateItem(hwnd,dat,k);
            }
            
        }
    }

}


void InvalidateItem(HWND hwnd,struct ClcData *dat,int iItem)
{
	RECT rc;
    if (iItem==-1) return;
	GetClientRect(hwnd,&rc);
	rc.top=RowHeights_GetItemTopY(dat,iItem)-dat->yScroll;
	rc.bottom=rc.top+dat->row_heights[iItem];
	InvalidateRectZ(hwnd,&rc,FALSE);
}