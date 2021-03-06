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
extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern void SortContacts(hwnd); /*SortClcByTimer(hwnd);*/
extern ON_SETALLEXTRAICON_CYCLE;
extern BYTE LOCK_REPAINTING;
extern BOOL CLM_AUTOREBUILD_WAS_POSTED;
#define UNLOCK_REPAINT 13315
//processing of all the CLM_ messages incoming

LRESULT ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case CLM_ADDCONTACT:
			AddContactToTree(hwnd,dat,(HANDLE)wParam,1,0);
			RecalcScrollBar(hwnd,dat);
			SortCLC(hwnd,dat,1);
			break;

		case CLM_ADDGROUP:
		{	DWORD groupFlags;
			TCHAR *szName=(TCHAR*)CallService(MS_CLIST_GROUPGETNAME2,wParam,(LPARAM)&groupFlags); //TOD UNICODE
			if(szName==NULL) break;
			AddGroup(hwnd,dat,szName,groupFlags,wParam,0);
			RecalcScrollBar(hwnd,dat);
			break;
		}

		case CLM_ADDINFOITEM:
		{	int i;
			struct ClcContact *groupContact;
			struct ClcGroup *group;
			CLCINFOITEM *cii=(CLCINFOITEM*)lParam;
			if(cii->cbSize!=sizeof(CLCINFOITEM)) return (LRESULT)(HANDLE)NULL;
			if(cii->hParentGroup==NULL) group=&dat->list;
			else {
				if(!FindItem(hwnd,dat,(HANDLE)((DWORD)cii->hParentGroup|HCONTACT_ISGROUP),&groupContact,NULL,NULL,TRUE)) return (LRESULT)(HANDLE)NULL;
				group=groupContact->group;
			}
			i=AddInfoItemToGroup(group,cii->flags,cii->pszText); //UNICODE
			RecalcScrollBar(hwnd,dat);
			dat->NeedResort=TRUE;
			SortCLC(hwnd,dat,1);
			return (LRESULT)group->contact[i].hContact|HCONTACT_ISINFO;
		}

		case CLM_AUTOREBUILD:
			KillTimer(hwnd,TIMERID_REBUILDAFTER);
			// Give some time to gather other events too		
			SetTimer(hwnd,TIMERID_REBUILDAFTER,500,NULL);
			CLM_AUTOREBUILD_WAS_POSTED=FALSE;
			break;

		case CLM_DELETEITEM:
			DeleteItemFromTree(hwnd,(HANDLE)wParam);
			SortCLC(hwnd,dat,1);
			RecalcScrollBar(hwnd,dat);
			break;

		case CLM_EDITLABEL:
			SendMessage(hwnd,CLM_SELECTITEM,wParam,0);
			BeginRenameSelection(hwnd,dat);
			break;

		case CLM_ENDEDITLABELNOW:
			EndRename(hwnd,dat,wParam);
			break;

		case CLM_ENSUREVISIBLE:
		{	struct ClcContact *contact;
			struct ClcGroup *group,*tgroup;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,&group,NULL,FALSE)) break;
			for(tgroup=group;tgroup;tgroup=tgroup->parent) SetGroupExpand(hwnd,dat,tgroup,1);
			EnsureVisible(hwnd,dat,GetRowsPriorTo(&dat->list,group,((unsigned)contact-(unsigned)group->contact)/sizeof(struct ClcContact)),0);
			break;
		}

		case CLM_EXPAND:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,TRUE)) break;
			if(contact->type!=CLCIT_GROUP) break;
			SetGroupExpand(hwnd,dat,contact->group,lParam);
			break;
		}

		case CLM_FINDCONTACT:
			if(!FindItem(hwnd,dat,(HANDLE)wParam,NULL,NULL,NULL,FALSE)) return (LRESULT)(HANDLE)NULL;
			return wParam;

		case CLM_FINDGROUP:
			if(!FindItem(hwnd,dat,(HANDLE)(wParam|HCONTACT_ISGROUP),NULL,NULL,NULL,TRUE)) return (LRESULT)(HANDLE)NULL;
			return wParam|HCONTACT_ISGROUP;

		case CLM_GETBKCOLOR:
			return dat->bkColour;

		case CLM_GETCHECKMARK:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) return 0;
			return (contact->flags&CONTACTF_CHECKED)!=0;
		}

		case CLM_GETCOUNT:
			return GetGroupContentsCount(&dat->list,0);

		case CLM_GETEDITCONTROL:
			return (LRESULT)dat->hwndRenameEdit;

		case CLM_GETEXPAND:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,TRUE)) return CLE_INVALID;
			if(contact->type!=CLCIT_GROUP) return CLE_INVALID;
			return contact->group->expanded;
		}

		case CLM_GETEXTRACOLUMNS:
			return dat->extraColumnsCount;

		case CLM_GETEXTRAIMAGE:
		{	struct ClcContact *contact;
			if(LOWORD(lParam)>=dat->extraColumnsCount) return 0xFF;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) return 0xFF;
			return contact->iExtraImage[LOWORD(lParam)];
		}

		case CLM_GETEXTRAIMAGELIST:
			return (LRESULT)dat->himlExtraColumns;

		case CLM_GETFONT:
			if(wParam<0 || wParam>FONTID_MAX) return 0;
			return (LRESULT)dat->fontInfo[wParam].hFont;

		case CLM_GETHIDEOFFLINEROOT:
			return DBGetContactSettingByte(NULL,"CLC","HideOfflineRoot",0);

		case CLM_GETINDENT:
			return dat->groupIndent;

		case CLM_GETISEARCHSTRING:
			lstrcpy((TCHAR*)lParam,dat->szQuickSearch);
			return lstrlen(dat->szQuickSearch);  //TO ANSI or flag

		case CLM_GETITEMTEXT:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) return 0;
			lstrcpy((TCHAR*)lParam,contact->szText); /// TODO TO ANSI or ADD flag
			return lstrlen(contact->szText);
		}

		case CLM_GETITEMTYPE:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) return CLCIT_INVALID;
			return contact->type;
		}

		case CLM_GETLEFTMARGIN:
			return dat->leftMargin;

		case CLM_GETNEXTITEM:
		{	struct ClcContact *contact;
			struct ClcGroup *group;
			int i;

			if(wParam!=CLGN_ROOT) {
				if(!FindItem(hwnd,dat,(HANDLE)lParam,&contact,&group,NULL,FALSE)) return (LRESULT)(HANDLE)NULL;
				i=((int)contact-(int)group->contact)/sizeof(struct ClcContact);
			}
			switch(wParam) {
				case CLGN_ROOT:
					if(dat->list.contactCount) return (LRESULT)ContactToHItem(&dat->list.contact[0]);
					else return (LRESULT)(HANDLE)NULL;
				case CLGN_CHILD:
					if(contact->type!=CLCIT_GROUP) return (LRESULT)(HANDLE)NULL;
					group=contact->group;
					if(group->contactCount==0) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[0]);
				case CLGN_PARENT:
					return group->groupId|HCONTACT_ISGROUP;
				case CLGN_NEXT:
					if(i>=group->contactCount) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[i+1]);
				case CLGN_PREVIOUS:
					if(i<=0) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[i-1]);
				case CLGN_NEXTCONTACT:
					for(i++;i<group->contactCount;i++)
						if(group->contact[i].type==CLCIT_CONTACT) break;
					if(i>=group->contactCount) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[i]);
				case CLGN_PREVIOUSCONTACT:
					if(i>=group->contactCount) return (LRESULT)(HANDLE)NULL;
					for(i--;i>=0;i--)
						if(group->contact[i].type==CLCIT_CONTACT) break;
					if(i<0) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[i]);
				case CLGN_NEXTGROUP:
					for(i++;i<group->contactCount;i++)
						if(group->contact[i].type==CLCIT_GROUP) break;
					if(i>=group->contactCount) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[i]);
				case CLGN_PREVIOUSGROUP:
					if(i>=group->contactCount) return (LRESULT)(HANDLE)NULL;
					for(i--;i>=0;i--)
						if(group->contact[i].type==CLCIT_GROUP) break;
					if(i<0) return (LRESULT)(HANDLE)NULL;
					return (LRESULT)ContactToHItem(&group->contact[i]);
			}
			return (LRESULT)(HANDLE)NULL;
		}

		case CLM_GETSCROLLTIME:
			return dat->scrollTime;

		case CLM_GETSELECTION:
		{	struct ClcContact *contact;
			if(GetRowByIndex(dat,dat->selection,&contact,NULL)==-1) return (LRESULT)(HANDLE)NULL;
			return (LRESULT)ContactToHItem(contact);
		}

		case CLM_GETTEXTCOLOR:
			if(wParam<0 || wParam>FONTID_MAX) return 0;
			return (LRESULT)dat->fontInfo[wParam].colour;

		case CLM_HITTEST:
		{	struct ClcContact *contact;
			DWORD hitFlags;
			int hit;
			hit=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),&contact,NULL,&hitFlags);
			if(wParam) *(PDWORD)wParam=hitFlags;
			if(hit==-1) return (LRESULT)(HANDLE)NULL;
			return (LRESULT)ContactToHItem(contact);
		}

		case CLM_SELECTITEM:
		{	struct ClcContact *contact;
			struct ClcGroup *group,*tgroup;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,&group,NULL,FALSE)) break;
			for(tgroup=group;tgroup;tgroup=tgroup->parent) SetGroupExpand(hwnd,dat,tgroup,1);
            if (contact->isSubcontact==0)
			    dat->selection=GetRowsPriorTo(&dat->list,group,((unsigned)contact-(unsigned)group->contact)/sizeof(struct ClcContact));
            else
                dat->selection=contact->isSubcontact+GetRowsPriorTo(&dat->list,group,((unsigned)contact->subcontacts-(unsigned)group->contact)/sizeof(struct ClcContact));
			EnsureVisible(hwnd,dat,dat->selection,0);
			break;
		}

		case CLM_SETBKBITMAP:
			if(!dat->bkChanged && dat->hBmpBackground) {DeleteObject(dat->hBmpBackground); dat->hBmpBackground=NULL;}
			dat->hBmpBackground=(HBITMAP)lParam;
			dat->backgroundBmpUse=wParam;
			dat->bkChanged=1;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;

		case CLM_SETBKCOLOR:
			if(!dat->bkChanged && dat->hBmpBackground) {DeleteObject(dat->hBmpBackground); dat->hBmpBackground=NULL;}
			dat->bkColour=wParam;
			dat->bkChanged=1;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;

		case CLM_SETCHECKMARK:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) return 0;
			if(lParam) contact->flags|=CONTACTF_CHECKED;
			else contact->flags&=~CONTACTF_CHECKED;
			RecalculateGroupCheckboxes(hwnd,dat);
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;
		}

		case CLM_SETEXTRACOLUMNS:
			if(wParam>MAXEXTRACOLUMNS) return 0;
			dat->extraColumnsCount=wParam;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;

		case CLM_SETEXTRAIMAGE:
		{	struct ClcContact *contact;	
			if(LOWORD(lParam)>=dat->extraColumnsCount||LOWORD(lParam)<0) return 0;
            if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) return 0;
			//fixme
			if (IsBadCodePtr((FARPROC)contact)) 
			{
				TRACE("BadCodePtr in CLM_SETEXTRAIMAGE contact\r\n");
				return 0;
			}
			if (IsBadCodePtr((FARPROC)contact->iExtraImage)) 
			{
				TRACE("BadCodePtr in CLM_SETEXTRAIMAGE contact->iExtraImage\r\n");
				return 0;
			}
 			contact->iExtraImage[LOWORD(lParam)]=(BYTE)HIWORD(lParam);
            if (!ON_SETALLEXTRAICON_CYCLE)
			    InvalidateRectZ(hwnd,NULL,FALSE);
			break;
		}

		case CLM_SETEXTRAIMAGELIST:
			dat->himlExtraColumns=(HIMAGELIST)lParam;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;

		case CLM_SETEXTRACOLUMNSSPACE:
			dat->extraColumnSpacing=(int)wParam;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;


			case CLM_SETFONT:
			if(HIWORD(lParam)<0 || HIWORD(lParam)>FONTID_MAX) return 0;
			dat->fontInfo[HIWORD(lParam)].hFont=(HFONT)wParam;
			dat->fontInfo[HIWORD(lParam)].changed=1;

			RowHeights_GetMaxRowHeight(dat, hwnd);

      if(LOWORD(lParam))
				InvalidateRectZ(hwnd,NULL,FALSE);
			break;


		case CLM_SETGREYOUTFLAGS:
			dat->greyoutFlags=wParam;
			break;

		case CLM_SETHIDEEMPTYGROUPS:
			{
				BOOL old=((GetWindowLong(hwnd,GWL_STYLE)&CLS_HIDEEMPTYGROUPS)!=0);
				BOOL newval=old;
				if(wParam) SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)|CLS_HIDEEMPTYGROUPS);
				else SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~CLS_HIDEEMPTYGROUPS);
				newval=((GetWindowLong(hwnd,GWL_STYLE)&CLS_HIDEEMPTYGROUPS)!=0);
				if (newval!=old)
					SendMessage(hwnd,CLM_AUTOREBUILD,0,0);
				break;
			}

		case CLM_SETHIDEOFFLINEROOT:
			DBWriteContactSettingByte(NULL,"CLC","HideOfflineRoot",(BYTE)wParam);
			SendMessage(hwnd,CLM_AUTOREBUILD,0,0);
			break;

		case CLM_SETINDENT:
			dat->groupIndent=wParam;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;

		case CLM_SETITEMTEXT:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) break;
      if (contact->szText) mir_free(contact->szText);
	    contact->szText=mir_strdupT((TCHAR*)lParam); //TODO ANSI to Unicode (add flag)
			//lstrcpynA(contact->szText,(char*)lParam,sizeof(contact->szText));
			SortCLC(hwnd,dat,1);
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;
		}

		case CLM_SETLEFTMARGIN:
			dat->leftMargin=wParam;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;

		case CLM_SETOFFLINEMODES:
			dat->offlineModes=wParam;
			SendMessage(hwnd,CLM_AUTOREBUILD,0,0);
			break;

		case CLM_SETSCROLLTIME:
			dat->scrollTime=wParam;
			break;

		case CLM_SETTEXTCOLOR:
            //break; /// black font fix inignore list
			if(wParam<0 || wParam>FONTID_MAX) break;
            dat->fontInfo[wParam].colour=lParam;
            dat->force_in_dialog=TRUE;
      // set same color for second and third line
            if (wParam==FONTID_CONTACTS)
            {
              dat->fontInfo[FONTID_SECONDLINE].colour=lParam;
              dat->fontInfo[FONTID_THIRDLINE].colour=lParam;
            }

			break;

		case CLM_SETUSEGROUPS:
			if(wParam) SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)|CLS_USEGROUPS);
			else SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~CLS_USEGROUPS);
            SendMessage(hwnd,CLM_AUTOREBUILD,0,0);
			break;
	}
	return 0;
}