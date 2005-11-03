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
#include "clist.h"
#include "m_metacontacts.h"

//routines for managing adding/removal of items in the list, including sorting
extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int CompareContacts(WPARAM wParam,LPARAM lParam);
extern void ClearClcContactCache(struct ClcData *dat,HANDLE hContact);
extern void SortContacts(hwnd); /*SortClcByTimer(hwnd);*/
extern BOOL FillRect255Alpha(HDC memdc,RECT *fr);
extern BOOL LOCK_IMAGE_UPDATING;
int lastGroupId=0;

void AddSubcontacts(struct ClcData *dat, struct ClcContact * cont)
{
	int subcount,i,j;
	HANDLE hsub;
	pdisplayNameCacheEntry cacheEntry;
	cacheEntry=GetContactFullCacheEntry(cont->hContact);
	TRACE("Proceed AddSubcontacts\r\n");
	subcount=(int)CallService(MS_MC_GETNUMCONTACTS,(WPARAM)cont->hContact,0);
	
	if (subcount <= 0) 
	{
		cont->isSubcontact=0;
		cont->subcontacts=NULL;
		cont->SubAllocated=0;
		return;
	}

	cont->SubExpanded=(DBGetContactSettingByte(cont->hContact,"CList","Expanded",0) && (DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1)));
	cont->isSubcontact=0;
	cont->subcontacts=(struct ClcContact *) mir_alloc(sizeof(struct ClcContact)*subcount);
	ZeroMemory(cont->subcontacts, sizeof(struct ClcContact)*subcount);
	cont->SubAllocated=subcount;
	i=0;
	for (j=0; j<subcount; j++)
	{
		hsub=(HANDLE)CallService(MS_MC_GETSUBCONTACT,(WPARAM)cont->hContact,j);
		cacheEntry=GetContactFullCacheEntry(hsub);		
		if (!(DBGetContactSettingByte(NULL,"CLC","MetaHideOfflineSub",1) && DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT) )||
			cacheEntry->status!=ID_STATUS_OFFLINE )
		{
			cont->subcontacts[i].hContact=cacheEntry->hContact;

			cont->subcontacts[i].avatar_pos = AVATAR_POS_DONT_HAVE;
			Cache_GetAvatar(dat, &cont->subcontacts[i]);
            
            cont->subcontacts[i].iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)cacheEntry->hContact,0);
			memset(cont->subcontacts[i].iExtraImage,0xFF,sizeof(cont->subcontacts[i].iExtraImage));
			cont->subcontacts[i].proto=cacheEntry->szProto;		

			//lstrcpyn(cont->subcontacts[i].szText,cacheEntry->name,sizeof(cont->subcontacts[i].szText));
			Cache_GetText(dat, &cont->subcontacts[i]);
			cont->subcontacts[i].type=CLCIT_CONTACT;
			cont->subcontacts[i].flags=0;//CONTACTF_ONLINE;
			cont->subcontacts[i].isSubcontact=i+1;
            cont->subcontacts[i].subcontacts=cont;
			cont->subcontacts[i].image_is_special=FALSE;
			cont->subcontacts[i].status=cacheEntry->status;
            {
                int apparentMode;
                char *szProto;  
                int idleMode;
                szProto=cacheEntry->szProto;
	            if(szProto!=NULL&&!IsHiddenMode(dat,cacheEntry->status))
		            cont->subcontacts[i].flags|=CONTACTF_ONLINE;
	            apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
                apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
                if(apparentMode==ID_STATUS_OFFLINE)	cont->subcontacts[i].flags|=CONTACTF_INVISTO;
	            else if(apparentMode==ID_STATUS_ONLINE) cont->subcontacts[i].flags|=CONTACTF_VISTO;
	            else if(apparentMode) cont->subcontacts[i].flags|=CONTACTF_VISTO|CONTACTF_INVISTO;
	            if(cacheEntry->NotOnList) cont->subcontacts[i].flags|=CONTACTF_NOTONLIST;
                idleMode=szProto!=NULL?cacheEntry->IdleTS:0;
	            if (idleMode) cont->subcontacts[i].flags|=CONTACTF_IDLE;
            }
			i++;
		}
		
	}
	cont->SubAllocated=i;
	if (!i && cont->subcontacts != NULL) mir_free(cont->subcontacts);
}


static int AddItemToGroup(struct ClcGroup *group,int iAboveItem)
{
	if (group==NULL) return 0;

	if(++group->contactCount>group->allocedCount) {
		group->allocedCount+=GROUP_ALLOCATE_STEP;
      //  if (group->contact) mir_free(group->contact);
		if(group->contact)	
		group->contact=(struct ClcContact*)mir_realloc(group->contact,sizeof(struct ClcContact)*group->allocedCount);
			else 
			group->contact=(struct ClcContact*)mir_alloc(sizeof(struct ClcContact)*group->allocedCount);
			
		if (group->contact==NULL||IsBadCodePtr((FARPROC)group->contact))
		{
			TRACE("!!!Bad Realloc AddItemToGroup");
			DebugBreak();
		}
	}
	memmove(group->contact+iAboveItem+1,group->contact+iAboveItem,sizeof(struct ClcContact)*(group->contactCount-iAboveItem-1));
    memset(&(group->contact[iAboveItem]),0,sizeof((group->contact[iAboveItem])));
	group->contact[iAboveItem].type=CLCIT_DIVIDER;
	//group->contact[iAboveItem].flags=0;
	memset(group->contact[iAboveItem].iExtraImage,0xFF,sizeof(group->contact[iAboveItem].iExtraImage));
  group->contact[iAboveItem].szText=NULL;
  group->contact[iAboveItem].szSecondLineText=NULL;
  group->contact[iAboveItem].szThirdLineText=NULL;
	//group->contact[iAboveItem].szText[0]='\0';
	//group->contact[iAboveItem].szSecondLineText[0]='\0';
	//group->contact[iAboveItem].szThirdLineText[0]='\0';
	//group->contact[iAboveItem].SubAllocated=0;
	//group->contact[iAboveItem].subcontacts=NULL;
	//group->contact[iAboveItem].SubExpanded=0;
	
	ClearRowByIndexCache();
	return iAboveItem;
}

struct ClcGroup *AddGroup(HWND hwnd,struct ClcData *dat,const char *szName,DWORD flags,int groupId,int calcTotalMembers)
{
	char *pBackslash,*pNextField,szThisField[120-MAXEXTRACOLUMNS];
    char *HiddenGroup=NULL;
	struct ClcGroup *group=&dat->list;
	int i,compareResult;

	ClearRowByIndexCache();	
	dat->NeedResort=1;

    if (GetWindowLong(hwnd,GWL_STYLE)&CLS_USEGROUPS)                            //groups is using
        if (ServiceExists(MS_MC_GETDEFAULTCONTACT))                             //metacontacts dll is loaded  
            if (DBGetContactSettingByte(NULL,"MetaContacts","Enabled",1));      //and enabled
        {
            HiddenGroup=DBGetString(NULL,"MetaContacts","HiddenGroupName");
           // if (!HiddenGroup) strdup("MetaContacts Hidden Group"); 
            if (szName)
            if (!MyStrCmp(HiddenGroup,szName))                                    //group is metahidden
            {   
                if (HiddenGroup) mir_free(HiddenGroup);
                return NULL;
            }
        }
    if (HiddenGroup) mir_free(HiddenGroup);
            

	if(!(GetWindowLong(hwnd,GWL_STYLE)&CLS_USEGROUPS)) return &dat->list;
	pNextField=(char*)szName;
	do {
		pBackslash=strchr(pNextField,'\\');
		if(pBackslash==NULL) {
			lstrcpyn(szThisField,pNextField,sizeof(szThisField));
			pNextField=NULL;
		}
		else {
			lstrcpyn(szThisField,pNextField,min(sizeof(szThisField),pBackslash-pNextField+1));
			pNextField=pBackslash+1;
		}
		compareResult=1;
		for(i=0;i<group->contactCount;i++) {
			if(group->contact[i].type==CLCIT_CONTACT) break;
			if(group->contact[i].type!=CLCIT_GROUP) continue;
			compareResult=lstrcmp(szThisField,group->contact[i].szText);
			if(compareResult==0) {
				if(pNextField==NULL && flags!=(DWORD)-1) {
					group->contact[i].groupId=(WORD)groupId;
					group=group->contact[i].group;
					group->expanded=(flags&GROUPF_EXPANDED)!=0;
					group->hideOffline=(flags&GROUPF_HIDEOFFLINE)!=0;
					group->groupId=groupId;
				}
				else group=group->contact[i].group;
				break;
			}
			if(pNextField==NULL && group->contact[i].groupId==0) break;
			if(groupId && group->contact[i].groupId>groupId) break;
		}
		if(compareResult) {
            if(groupId==0) 
                {
                  return NULL;
                  groupId=lastGroupId++;      
                }//return NULL;
			i=AddItemToGroup(group,i);
			group->contact[i].type=CLCIT_GROUP;
      if (group->contact[i].szText) mir_free(group->contact[i].szText);
			group->contact[i].szText=mir_strdup(szThisField);
			group->contact[i].groupId=(WORD)(pNextField?0:groupId);
			group->contact[i].group=(struct ClcGroup*)mir_alloc(sizeof(struct ClcGroup));
			group->contact[i].group->parent=group;
			group=group->contact[i].group;
			group->allocedCount=group->contactCount=0;
			group->contact=NULL;
			if(flags==(DWORD)-1 || pNextField!=NULL) {
				group->expanded=0;
				group->hideOffline=0;
			}
			else {
				group->expanded=(flags&GROUPF_EXPANDED)!=0;
				group->hideOffline=(flags&GROUPF_HIDEOFFLINE)!=0;
			}
			group->groupId=pNextField?0:groupId;
			group->totalMembers=0;
			if(flags!=(DWORD)-1 && pNextField==NULL && calcTotalMembers) {
				HANDLE hContact;
				//DBVARIANT dbv;
				int tick;
				DWORD style=GetWindowLong(hwnd,GWL_STYLE);
				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
				tick=GetTickCount();
				while(hContact) {
					pdisplayNameCacheEntry cacheEntry;
					cacheEntry=GetContactFullCacheEntry(hContact);

					if(cacheEntry->szGroup!=NULL&&MyStrLen(cacheEntry->szGroup)!=0) {
						if(!lstrcmp(cacheEntry->szGroup,szName) && (style&CLS_SHOWHIDDEN || !cacheEntry->Hidden))
							group->totalMembers++;
						//mir_free(dbv.pszVal);
					}
					hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
				}
#ifdef _DEBUG
        {
          char buf[255];
				tick=GetTickCount()-tick;
				sprintf(buf,"AddGroup Finds %d\r\n",tick);
				TRACE(buf);
        }
#endif
			}
		}
	} while(pNextField);
	
	ClearRowByIndexCache();
	return group;
}

void FreeGroup(struct ClcGroup *group)
{
	int i;
	if (group==NULL||IsBadCodePtr((FARPROC)group)) return;

	for(i=0;i<group->contactCount;i++) {
		if(group->contact && group->contact[i].type==CLCIT_GROUP) {
			FreeGroup(group->contact[i].group);
			mir_free(group->contact[i].group);      //**//
		}
	}
	if(group->allocedCount)
	{	
		if (group->contact->SubAllocated)
		{
			if (group->contact->subcontacts && !group->contact->isSubcontact) 
			{
				int i;
				for ( i = 0 ; i < group->contact->SubAllocated ; i++ )
				{
					Cache_DestroySmileyList(group->contact->subcontacts[i].plText);
					Cache_DestroySmileyList(group->contact->subcontacts[i].plSecondLineText);
					Cache_DestroySmileyList(group->contact->subcontacts[i].plThirdLineText);
          if (group->contact->subcontacts[i].szText) mir_free(group->contact->subcontacts[i].szText);
          if (group->contact->subcontacts[i].szSecondLineText) mir_free(group->contact->subcontacts[i].szSecondLineText);
          if (group->contact->subcontacts[i].szThirdLineText) mir_free(group->contact->subcontacts[i].szThirdLineText);
				}

				mir_free(group->contact->subcontacts);
			}
		}

		if(group->contact) 
		{
			Cache_DestroySmileyList(group->contact->plText);
			Cache_DestroySmileyList(group->contact->plSecondLineText);
			Cache_DestroySmileyList(group->contact->plThirdLineText);
      if (group->contact->szText) mir_free(group->contact->szText);
      if (group->contact->szSecondLineText) mir_free(group->contact->szSecondLineText);
      if (group->contact->szThirdLineText) mir_free(group->contact->szThirdLineText);
			mir_free(group->contact);
		}
	}
	group->allocedCount=0;
    //mir_free(group->contact);
	group->contact=NULL;
	group->contactCount=0;   
	ClearRowByIndexCache();
}

static int iInfoItemUniqueHandle=0;
int AddInfoItemToGroup(struct ClcGroup *group,int flags,const char *pszText)
{
	int i=0;

	if(flags&CLCIIF_BELOWCONTACTS)
		i=group->contactCount;
	else if(flags&CLCIIF_BELOWGROUPS) {
		for(;i<group->contactCount;i++)
			if(group->contact[i].type==CLCIT_CONTACT) break;
	}
	else
		for(;i<group->contactCount;i++)
			if(group->contact[i].type!=CLCIT_INFO) break;
	i=AddItemToGroup(group,i);
	iInfoItemUniqueHandle=(iInfoItemUniqueHandle+1)&0xFFFF;
	if(iInfoItemUniqueHandle==0) ++iInfoItemUniqueHandle;
	group->contact[i].type=CLCIT_INFO;
	group->contact[i].flags=(BYTE)flags;
	group->contact[i].hContact=(HANDLE)++iInfoItemUniqueHandle;
  if (group->contact[i].szText) mir_free(group->contact[i].szText);
	group->contact[i].szText=mir_strdup(pszText); 
	//lstrcpyn(group->contact[i].szText,pszText,sizeof(group->contact[i].szText));
	ClearRowByIndexCache();
	return i;
}
static struct ClcContact * AddContactToGroup(struct ClcData *dat,struct ClcGroup *group,pdisplayNameCacheEntry cacheEntry)
{
	char *szProto;
	WORD apparentMode;
	DWORD idleMode;
	HANDLE hContact;
	int i;
//	DBVARIANT dbv;
	if (cacheEntry==NULL) return NULL;
	if (group==NULL) return NULL;
	if (dat==NULL) return NULL;
	hContact=cacheEntry->hContact;
	//ClearClcContactCache(hContact);

//ShowTracePopup("AddContactToGroup");

	dat->NeedResort=1;
	for(i=group->contactCount-1;i>=0;i--)
		if(group->contact[i].type!=CLCIT_INFO || !(group->contact[i].flags&CLCIIF_BELOWCONTACTS)) break;
	i=AddItemToGroup(group,i+1);
	group->contact[i].type=CLCIT_CONTACT;
	group->contact[i].SubAllocated=0;
	group->contact[i].isSubcontact=0;
	group->contact[i].subcontacts=NULL;
  group->contact[i].szText=NULL;
  group->contact[i].szSecondLineText=NULL;
  group->contact[i].szThirdLineText=NULL;
	group->contact[i].image_is_special=FALSE;
	group->contact[i].status=cacheEntry->status;
	
	group->contact[i].iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)hContact,0);
	cacheEntry=GetContactFullCacheEntry(hContact);
	group->contact[i].hContact=hContact;

	group->contact[i].avatar_pos = AVATAR_POS_DONT_HAVE;
	Cache_GetAvatar(dat, &group->contact[i]);

	
	//cacheEntry->ClcContact=&(group->contact[i]);
	//SetClcContactCacheItem(dat,hContact,&(group->contact[i]));

	szProto=cacheEntry->szProto;
	if(szProto!=NULL&&!IsHiddenMode(dat,cacheEntry->status))
		group->contact[i].flags|=CONTACTF_ONLINE;
	apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
	if(apparentMode==ID_STATUS_OFFLINE)	group->contact[i].flags|=CONTACTF_INVISTO;
	else if(apparentMode==ID_STATUS_ONLINE) group->contact[i].flags|=CONTACTF_VISTO;
	else if(apparentMode) group->contact[i].flags|=CONTACTF_VISTO|CONTACTF_INVISTO;
	if(cacheEntry->NotOnList) group->contact[i].flags|=CONTACTF_NOTONLIST;
	idleMode=szProto!=NULL?cacheEntry->IdleTS:0;
	if (idleMode) group->contact[i].flags|=CONTACTF_IDLE;

	//lstrcpyn(group->contact[i].szText,cacheEntry->name,sizeof(group->contact[i].szText));
	group->contact[i].proto = szProto;
	
	Cache_GetText(dat, &group->contact[i]);
	ClearRowByIndexCache();
	return &(group->contact[i]);
}
void * AddTempGroup(HWND hwnd,struct ClcData *dat,const char *szName,DWORD flags,int groupId,int calcTotalMembers)
{
     int i=0;
     int f=0;
     char * szGroupName;
     DWORD groupFlags;
	 if (WildCompare(szName,"* Hidden Group",0))
	 {
		 if(ServiceExists(MS_MC_ADDTOMETA)) return NULL;
		 else return(&dat->list);
	 } 
	 for(i=1;;i++) 
     {
	    szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
	    if(szGroupName==NULL) break;
        if (boolstrcmpi(szGroupName,szName)) f=1;
	 }
     if (!f)
     {
        char buf[20];
        char b2[255];
        void * res=NULL;
		_snprintf(buf,sizeof(buf),"%d",(i-1));
        _snprintf(b2,sizeof(b2),"#%s",szName);
        b2[0]=1|GROUPF_EXPANDED;
		DBWriteContactSettingString(NULL,"CListGroups",buf,b2);
        CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);      
        res=AddGroup(hwnd,dat,szName,groupFlags,i,0);
        return res;
     }
    return NULL;
}
void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline)
{
	struct ClcGroup *group;
	struct ClcContact * cont;
	pdisplayNameCacheEntry cacheEntry;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	WORD status;
	char *szProto;
	
	if (FindItem(hwnd,dat,hContact,NULL,NULL,NULL,FALSE)==1){return;};	
	cacheEntry=GetContactFullCacheEntry(hContact);
	if (cacheEntry==NULL) return;
    if (dat->IsMetaContactsEnabled && cacheEntry->HiddenSubcontact) return;   ///-----
	szProto=cacheEntry->szProto;


	//char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
	
	dat->NeedResort=1;
	ClearRowByIndexCache();
	ClearClcContactCache(dat,hContact);
	
	if(style&CLS_NOHIDEOFFLINE) checkHideOffline=0;
	if(checkHideOffline) {
		if(szProto==NULL) status=ID_STATUS_OFFLINE;
		else status=cacheEntry->status;
	}

	if(MyStrLen(cacheEntry->szGroup)==0)
		group=&dat->list;
	else {
		group=AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
		if(group==NULL) {
			int i,len;
			DWORD groupFlags;
			char *szGroupName;
			if(!(style&CLS_HIDEEMPTYGROUPS)) {/*mir_free(dbv.pszVal);*/AddTempGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);return;}
			if(checkHideOffline && IsHiddenMode(dat,status)) {
				for(i=1;;i++) {
					szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
					if(szGroupName==NULL) {/*mir_free(dbv.pszVal);*/ return;}   //never happens
					if(!lstrcmp(szGroupName,cacheEntry->szGroup)) break;
				}
				if(groupFlags&GROUPF_HIDEOFFLINE) {/*mir_free(dbv.pszVal);*/ return;}
			}
			for(i=1;;i++) {
				szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
				if(szGroupName==NULL) {/*mir_free(dbv.pszVal);*/ return;}   //never happens
				if(!lstrcmp(szGroupName,cacheEntry->szGroup)) break;
				len=lstrlen(szGroupName);
				if(!strncmp(szGroupName,cacheEntry->szGroup,len) && cacheEntry->szGroup[len]=='\\')
					AddGroup(hwnd,dat,szGroupName,groupFlags,i,1);
			}
			group=AddGroup(hwnd,dat,cacheEntry->szGroup,groupFlags,i,1);
		}
	//	mir_free(dbv.pszVal);
	}
    if (cacheEntry->status==ID_STATUS_OFFLINE)
           if (DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0))
                        group=&dat->list;
	if(checkHideOffline) {
		if(IsHiddenMode(dat,status) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
			if(updateTotalCount) group->totalMembers++;
			return;
		}
	}
    if(dat->IsMetaContactsEnabled &&  cacheEntry->HiddenSubcontact) return;
    if(!dat->IsMetaContactsEnabled && !MyStrCmp(cacheEntry->szProto,"MetaContacts")) return;
	cont=AddContactToGroup(dat,group,cacheEntry);
	if (cont)	
			if (cont->proto)
		{	
			cont->SubAllocated=0;
			if (MyStrCmp(cont->proto,"MetaContacts")==0)
				AddSubcontacts(dat,cont);
		}
	if(updateTotalCount && group) group->totalMembers++;
	ClearRowByIndexCache();
}

struct ClcGroup *RemoveItemFromGroup(HWND hwnd,struct ClcGroup *group,struct ClcContact *contact,int updateTotalCount)
{
	int iContact;
	struct ClcData* dat=(struct ClcData*)GetWindowLong(hwnd,0);

	
	ClearRowByIndexCache();
	if(contact->type==CLCIT_CONTACT) ClearClcContactCache(dat,contact->hContact);

	iContact=((unsigned)contact-(unsigned)group->contact)/sizeof(struct ClcContact);
	if(iContact>=group->contactCount) return group;
	if(contact->type==CLCIT_GROUP) {
		FreeGroup(contact->group);
		mir_free(contact->group);
	}
	group->contactCount--;
	if(updateTotalCount && contact->type==CLCIT_CONTACT) group->totalMembers--;
	memmove(group->contact+iContact,group->contact+iContact+1,sizeof(struct ClcContact)*(group->contactCount-iContact));
	if((GetWindowLong(hwnd,GWL_STYLE)&CLS_HIDEEMPTYGROUPS) && group->contactCount==0) {
		int i;
		if(group->parent==NULL) return group;
		for(i=0;i<group->parent->contactCount;i++)
			if(group->parent->contact[i].type==CLCIT_GROUP 
				&& group->parent->contact[i].groupId==group->groupId) break;
		if(i==group->parent->contactCount) return group;  //never happens
		return RemoveItemFromGroup(hwnd,group->parent,&group->parent->contact[i],0);
	}

	
	ClearRowByIndexCache();
	return group;
}

void DeleteItemFromTree(HWND hwnd,HANDLE hItem)
{
	struct ClcContact *contact;
	struct ClcGroup *group;
	struct ClcData *dat=(struct ClcData*)GetWindowLong(hwnd,0);
	ClearRowByIndexCache();
	dat->NeedResort=1;
	
	if(!FindItem(hwnd,dat,hItem,&contact,&group,NULL,TRUE)) {
		DBVARIANT dbv;
		int i,nameOffset;
		if(!IsHContactContact(hItem)) return;
		ClearClcContactCache(dat,hItem);

    if(DBGetContactSetting(hItem,"CList","Group",&dbv)) {DBFreeVariant(&dbv); return;}

		//decrease member counts of all parent groups too
		group=&dat->list;
		nameOffset=0;
		for(i=0;;i++) {
			if(group->scanIndex==group->contactCount) break;
			if(group->contact[i].type==CLCIT_GROUP) {
				int len=lstrlen(group->contact[i].szText);
				if(!strncmp(group->contact[i].szText,dbv.pszVal+nameOffset,len) && (dbv.pszVal[nameOffset+len]=='\\' || dbv.pszVal[nameOffset+len]=='\0')) {
					group->totalMembers--;
					if(dbv.pszVal[nameOffset+len]=='\0') break;
				}
			}
		}
		mir_free(dbv.pszVal);
    DBFreeVariant(&dbv);
	}
	else RemoveItemFromGroup(hwnd,group,contact,1);

	ClearRowByIndexCache();
}



void RebuildEntireList(HWND hwnd,struct ClcData *dat)
{
//	char *szProto;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	HANDLE hContact;
	struct ClcContact * cont;
	struct ClcGroup *group;
	//DBVARIANT dbv;
	int tick=GetTickCount();
    KillTimer(hwnd,TIMERID_REBUILDAFTER);
    
    //EnterCriticalSection(&(dat->lockitemCS));
//ShowTracePopup("RebuildEntireList");

#ifdef _DEBUG
	{
		static int num_calls = 0;
		char tmp[128];
		mir_snprintf(tmp, sizeof(tmp), "*********************   RebuildEntireList (%d)\r\n", num_calls);
		num_calls++;
		TRACE(tmp);
	}
#endif 

	ClearRowByIndexCache();
	ClearClcContactCache(dat,INVALID_HANDLE_VALUE);
	ImageArray_Clear(&dat->avatar_cache);
	RowHeights_Clear(dat);
	RowHeights_GetMaxRowHeight(dat, hwnd);

	dat->list.expanded=1;
	dat->list.hideOffline=DBGetContactSettingByte(NULL,"CLC","HideOfflineRoot",0);
	dat->list.contactCount=0;
	dat->list.totalMembers=0;
	dat->NeedResort=1;
	dat->selection=-1;
	dat->HiLightMode=DBGetContactSettingByte(NULL,"CLC","HiLightMode",0);
	{
		int i;
		char *szGroupName;
		DWORD groupFlags;

		for(i=1;;i++) {
			szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
			if(szGroupName==NULL) break;
			AddGroup(hwnd,dat,szGroupName,groupFlags,i,0);
		}
        lastGroupId=i;
        
	}

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact) {
		
		pdisplayNameCacheEntry cacheEntry;
		cont=NULL;
		cacheEntry=GetContactFullCacheEntry(hContact);
		//cacheEntry->ClcContact=NULL;
		ClearClcContactCache(dat,hContact);

		

		if((dat->IsMetaContactsEnabled||MyStrCmp(cacheEntry->szProto,"MetaContacts"))&&(style&CLS_SHOWHIDDEN || !cacheEntry->Hidden) && (!cacheEntry->HiddenSubcontact || !dat->IsMetaContactsEnabled )) {
			if(MyStrLen(cacheEntry->szGroup)==0)
				group=&dat->list;
			else {
				group=AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
                if (!group) group=AddTempGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
				//mir_free(dbv.pszVal);
			}
            if(group!=NULL) {
                if (cacheEntry->status==ID_STATUS_OFFLINE)
                    if (DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0))
                        group=&dat->list;
				group->totalMembers++;
				if(!(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
					//szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
					if(cacheEntry->szProto==NULL) {
						if(!IsHiddenMode(dat,ID_STATUS_OFFLINE)||cacheEntry->noHiddenOffline)
							cont=AddContactToGroup(dat,group,cacheEntry);
					}
					else
						if(!IsHiddenMode(dat,cacheEntry->status)||cacheEntry->noHiddenOffline)
							cont=AddContactToGroup(dat,group,cacheEntry);
				}
				else cont=AddContactToGroup(dat,group,cacheEntry);
			}
		}
		if (cont)	
			if (cont->proto)
		{	
			cont->SubAllocated=0;
			if (MyStrCmp(cont->proto,"MetaContacts")==0)
				AddSubcontacts(dat,cont);
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}

	if(style&CLS_HIDEEMPTYGROUPS) {
		group=&dat->list;
		group->scanIndex=0;
		for(;;) {
			if(group->scanIndex==group->contactCount) {
				group=group->parent;
				if(group==NULL) break;
			}
			else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				if(group->contact[group->scanIndex].group->contactCount==0) {
					group=RemoveItemFromGroup(hwnd,group,&group->contact[group->scanIndex],0);
				}
				else {
					group=group->contact[group->scanIndex].group;
					group->scanIndex=0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}

	SortCLC(hwnd,dat,0);
  // LOCK_IMAGE_UPDATING=0;
  //LeaveCriticalSection(&(dat->lockitemCS));
#ifdef _DEBUG
	tick=GetTickCount()-tick;
	{
	char buf[255];
	//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
	sprintf(buf,"RebuildEntireList %d \r\n",tick);

	TRACE(buf);
	DBWriteContactSettingDword((HANDLE)0,"CLUI","PF:Last RebuildEntireList Time:",tick);
	}	
#endif
}


int GetNewSelection(struct ClcGroup *group, int selection, int direction)
{
	int lastcount=0, count=0;//group->contactCount;
	struct ClcGroup *topgroup=group;
	if (selection<0) {
		return 0;
	}
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
			group->scanIndex++;
			continue;
		}
		if (count>=selection) return count;
		lastcount = count;
		count++;
/*		if ((group->contact[group->scanIndex].type==CLCIT_CONTACT) && (group->contact[group->scanIndex].flags & CONTACTF_STATUSMSG)) {
			count++;
		}
*/
		if (!direction) {
			if (count>selection) return lastcount;
		}
		if(group->contact[group->scanIndex].type==CLCIT_GROUP && (group->contact[group->scanIndex].group->expanded)) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
		//	count+=group->contactCount;
			continue;
		}
		group->scanIndex++;
	}
	return lastcount;
 }

int GetGroupContentsCount(struct ClcGroup *group,int visibleOnly)
{
	int count=group->contactCount;
	struct ClcGroup *topgroup=group;

	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			if(group==topgroup) break;
			group=group->parent;
             if (!group) return 0;
			group->scanIndex++;
			continue;

		}
        else if (group->contact[group->scanIndex].type==CLCIT_CONTACT && (group->contact[group->scanIndex].SubAllocated>0) && visibleOnly && group->contact[group->scanIndex].SubExpanded && (DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1)))
		{
			count+=group->contact[group->scanIndex].SubAllocated;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP && (!visibleOnly || group->contact[group->scanIndex].group->expanded)) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			count+=group->contactCount;
			continue;
		}
		group->scanIndex++;
	}
	return count;
}

static int __cdecl GroupSortProc(const struct ClcContact *contact1,const struct ClcContact *contact2)
{
	return lstrcmpi(contact1->szText,contact2->szText);
	
}

static int __cdecl ContactSortProc(const struct ClcContact *contact1,const struct ClcContact *contact2)
{
	int result;

	//result=CallService(MS_CLIST_CONTACTSCOMPARE,(WPARAM)contact1->hContact,(LPARAM)contact2->hContact);
	result=CompareContacts((WPARAM)contact1->hContact,(LPARAM)contact2->hContact);
	if(result) return result;
	//nothing to distinguish them, so make sure they stay in the same order
	return (int)contact2->hContact-(int)contact1->hContact;

	return 0;
}

static void InsertionSort(struct ClcContact *pContactArray,int nArray,int (*CompareProc)(const void*,const void*))
{
	int i,j;
	struct ClcContact testElement;

	for(i=1;i<nArray;i++) {
		if(CompareProc(&pContactArray[i-1],&pContactArray[i])>0) {
			testElement=pContactArray[i];
			for(j=i-2;j>=0;j--)
				if(CompareProc(&pContactArray[j],&testElement)<=0) break;
			j++;
			memmove(&pContactArray[j+1],&pContactArray[j],sizeof(struct ClcContact)*(i-j));
			pContactArray[j]=testElement;
		}
	}
}

static void SortGroup(struct ClcData *dat,struct ClcGroup *group,int useInsertionSort)
{
	int i,sortCount;

	for(i=group->contactCount-1;i>=0;i--) {
		if(group->contact[i].type==CLCIT_DIVIDER) {
			group->contactCount--;
			memmove(&group->contact[i],&group->contact[i+1],sizeof(struct ClcContact)*(group->contactCount-i));
		}
	}
	for(i=0;i<group->contactCount;i++)
		if(group->contact[i].type!=CLCIT_INFO) break;
	if(i>group->contactCount-2) return;
	if(group->contact[i].type==CLCIT_GROUP) {
		if(dat->exStyle&CLS_EX_SORTGROUPSALPHA) {
			for(sortCount=0;i+sortCount<group->contactCount;sortCount++)
				if(group->contact[i+sortCount].type!=CLCIT_GROUP) break;
			qsort(group->contact+i,sortCount,sizeof(struct ClcContact),GroupSortProc);
			i=i+sortCount;
		}
		for(;i<group->contactCount;i++)
			if(group->contact[i].type==CLCIT_CONTACT) break;
		if(group->contactCount-i<2) return;
	}
	for(sortCount=0;i+sortCount<group->contactCount;sortCount++)
		if(group->contact[i+sortCount].type!=CLCIT_CONTACT) break;
	if(useInsertionSort) InsertionSort(group->contact+i,sortCount,ContactSortProc);
	else qsort(group->contact+i,sortCount,sizeof(struct ClcContact),ContactSortProc);
	if(dat->exStyle&CLS_EX_DIVIDERONOFF) {
		int prevContactOnline=0;
		for(i=0;i<group->contactCount;i++) 
        {
			if(group->contact[i].type!=CLCIT_CONTACT && group->contact[i].type!=CLCIT_GROUP) continue;
            if ((group->contact[i].type==CLCIT_GROUP) &&  DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0)) prevContactOnline=1;
            if (group->contact[i].type==CLCIT_CONTACT)
                if(group->contact[i].flags&CONTACTF_ONLINE) prevContactOnline=1;
			    else 
                {
    				if(prevContactOnline) 
                    {
	    				i=AddItemToGroup(group,i);
		        		group->contact[i].type=CLCIT_DIVIDER;
                if (group->contact[i].szText) mir_free(group->contact[i].szText);
				    	  group->contact[i].szText=mir_strdup(Translate("Offline"));
				    }
				    break;
			    }
        }           
	}
	ClearRowByIndexCache();
}

void SortCLC(HWND hwnd,struct ClcData *dat,int useInsertionSort)
{
	struct ClcContact *selcontact;
	struct ClcGroup *group=&dat->list,*selgroup;
	int dividers=dat->exStyle&CLS_EX_DIVIDERONOFF;
	HANDLE hSelItem;
	int tick=GetTickCount();
    //LOCK_IMAGE_UPDATING=1;
	if (dat->NeedResort==1||1)
	{

		if(GetRowByIndex(dat,dat->selection,&selcontact,NULL)==-1) hSelItem=NULL;
		else hSelItem=ContactToHItem(selcontact);
		group->scanIndex=0;
		
		SortGroup(dat,group,useInsertionSort);
		
		for(;;) {
			if(group->scanIndex==group->contactCount) {
				group=group->parent;
				if(group==NULL) break;
			}
			else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				group=group->contact[group->scanIndex].group;
				group->scanIndex=0;
				SortGroup(dat,group,useInsertionSort);
				continue;
			}
			group->scanIndex++;
		}
		
		ClearClcContactCache(dat,INVALID_HANDLE_VALUE);

		if(hSelItem)
			if(FindItem(hwnd,dat,hSelItem,&selcontact,&selgroup,NULL,FALSE))
				dat->selection=GetRowsPriorTo(&dat->list,selgroup,selcontact-selgroup->contact);
		
		
		RecalcScrollBar(hwnd,dat);
		ClearRowByIndexCache();
	}else
	{
		//TRACE("Not need to sort\r\n");
	};
    
	InvalidateRectZ(hwnd,NULL,FALSE);
	dat->NeedResort=0;
   // LOCK_IMAGE_UPDATING=1;
   // RecalcScrollBar(hwnd,dat);
#ifdef _DEBUG
	tick=GetTickCount()-tick;
	{
	char buf[255];
	//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
		if (tick>5) 
		{
			sprintf(buf,"SortCLC %d \r\n",tick);
			TRACE(buf);
			DBWriteContactSettingDword((HANDLE)0,"CLUI","PF:Last SortCLC Time:",tick);
		}
	}
#endif	
}

struct SavedContactState_t {
	HANDLE hContact;
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	int checked;
  
};

struct SavedGroupState_t {
	int groupId,expanded;
};

struct SavedInfoState_t {
	int parentId;
	struct ClcContact contact;
};

void SaveStateAndRebuildList(HWND hwnd,struct ClcData *dat)
{
	NMCLISTCONTROL nm;
	int i,j;
	struct SavedGroupState_t *savedGroup=NULL;
	int savedGroupCount=0,savedGroupAlloced=0;
	struct SavedContactState_t *savedContact=NULL;
	int savedContactCount=0,savedContactAlloced=0;
	struct SavedInfoState_t *savedInfo=NULL;
	int savedInfoCount=0,savedInfoAlloced=0;
	struct ClcGroup *group;
	struct ClcContact *contact;

	int tick=GetTickCount();
	int allocstep=1024;

  TRACE("SaveStateAndRebuildList\n");

	HideInfoTip(hwnd,dat);
	KillTimer(hwnd,TIMERID_INFOTIP);
  KillTimer(hwnd,TIMERID_REBUILDAFTER);
	KillTimer(hwnd,TIMERID_RENAME);
	EndRename(hwnd,dat,1);

	group=&dat->list;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			if(++savedGroupCount>savedGroupAlloced) {
				savedGroupAlloced+=allocstep;
				savedGroup=(struct SavedGroupState_t*)mir_realloc(savedGroup,sizeof(struct SavedGroupState_t)*savedGroupAlloced);
			}
			savedGroup[savedGroupCount-1].groupId=group->groupId;
			savedGroup[savedGroupCount-1].expanded=group->expanded;
			continue;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_CONTACT) {			
			if(++savedContactCount>savedContactAlloced) {
				savedContactAlloced+=allocstep;
				savedContact=(struct SavedContactState_t*)mir_realloc(savedContact,sizeof(struct SavedContactState_t)*savedContactAlloced);
			}
			savedContact[savedContactCount-1].hContact=group->contact[group->scanIndex].hContact;
			CopyMemory(savedContact[savedContactCount-1].iExtraImage,group->contact[group->scanIndex].iExtraImage,sizeof(group->contact[group->scanIndex].iExtraImage));
			savedContact[savedContactCount-1].checked=group->contact[group->scanIndex].flags&CONTACTF_CHECKED;
			if (group->contact[group->scanIndex].SubAllocated>0)
			{
				int l;
				for (l=0; l<group->contact[group->scanIndex].SubAllocated; l++)
				{
					if(++savedContactCount>savedContactAlloced) {
						savedContactAlloced+=allocstep;
						savedContact=(struct SavedContactState_t*)mir_realloc(savedContact,sizeof(struct SavedContactState_t)*savedContactAlloced);
					}
					savedContact[savedContactCount-1].hContact=group->contact[group->scanIndex].subcontacts[l].hContact;
					CopyMemory(savedContact[savedContactCount-1].iExtraImage ,group->contact[group->scanIndex].subcontacts[l].iExtraImage,sizeof(group->contact[group->scanIndex].iExtraImage));
					savedContact[savedContactCount-1].checked=group->contact[group->scanIndex].subcontacts[l].flags&CONTACTF_CHECKED;
                    

				}
			}

		}
		else if(group->contact[group->scanIndex].type==CLCIT_INFO) {
			if(++savedInfoCount>savedInfoAlloced) {
				savedInfoAlloced+=allocstep;
				savedInfo=(struct SavedInfoState_t*)mir_realloc(savedInfo,sizeof(struct SavedInfoState_t)*savedInfoAlloced);
			}
			if(group->parent==NULL) savedInfo[savedInfoCount-1].parentId=-1;
			else savedInfo[savedInfoCount-1].parentId=group->groupId;
			savedInfo[savedInfoCount-1].contact=group->contact[group->scanIndex];
		}
		group->scanIndex++;
	}

	FreeGroup(&dat->list);
	RebuildEntireList(hwnd,dat);

	group=&dat->list;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			for(i=0;i<savedGroupCount;i++)
				if(savedGroup[i].groupId==group->groupId) {
					group->expanded=savedGroup[i].expanded;
					break;
				}
			continue;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_CONTACT) {
			for(i=0;i<savedContactCount;i++)
				if(savedContact[i].hContact==group->contact[group->scanIndex].hContact) {
					CopyMemory(group->contact[group->scanIndex].iExtraImage,savedContact[i].iExtraImage,sizeof(group->contact[group->scanIndex].iExtraImage));
					if(savedContact[i].checked) group->contact[group->scanIndex].flags|=CONTACTF_CHECKED;
					break;	
				}
			if (group->contact[group->scanIndex].SubAllocated>0)
			{
				int l;
				for (l=0; l<group->contact[group->scanIndex].SubAllocated; l++)
					for(i=0;i<savedContactCount;i++)
						if(savedContact[i].hContact==group->contact[group->scanIndex].subcontacts[l].hContact) {

							CopyMemory(group->contact[group->scanIndex].subcontacts[l].iExtraImage,savedContact[i].iExtraImage,sizeof(group->contact[group->scanIndex].iExtraImage));
							if(savedContact[i].checked) group->contact[group->scanIndex].subcontacts[l].flags|=CONTACTF_CHECKED;
                            group->contact[group->scanIndex].subcontacts[l].subcontacts=&(group->contact[group->scanIndex]);
							break;	
						}	
			}
		}
		group->scanIndex++;
	}
	if(savedGroup) mir_free(savedGroup);
	if(savedContact) mir_free(savedContact);
	for(i=0;i<savedInfoCount;i++) {
		if(savedInfo[i].parentId==-1) group=&dat->list;
		else {
			if(!FindItem(hwnd,dat,(HANDLE)(savedInfo[i].parentId|HCONTACT_ISGROUP),&contact,NULL,NULL,TRUE)) continue;
			group=contact->group;
		}
		j=AddInfoItemToGroup(group,savedInfo[i].contact.flags,"");
		group->contact[j]=savedInfo[i].contact;
	}
	if(savedInfo) mir_free(savedInfo);
	RecalculateGroupCheckboxes(hwnd,dat);

	RecalcScrollBar(hwnd,dat);
	nm.hdr.code=CLN_LISTREBUILT;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);


	//srand(GetTickCount());
	
	tick=GetTickCount()-tick;
#ifdef _DEBUG
	{
	char buf[255];
	sprintf(buf,"SaveStateAndRebuildList %d \r\n",tick);
	TRACE(buf);
	}	
#endif

	ClearRowByIndexCache();
       // SetAllExtraIcons(hwnd,0);
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}