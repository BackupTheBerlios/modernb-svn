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

/* the CLC uses 3 different ways to identify elements in its list, this file
contains routines to convert between them.

1) struct ClcContact/struct ClcGroup pair. Only ever used within the duration
of a single operation, but used at some point in nearly everything
2) index integer. The 0-based number of the item from the top. Only visible
items are counted (ie not closed groups). Used for saving selection and drag
highlight
3) hItem handle. Either the hContact or (hGroup|HCONTACT_ISGROUP). Used
exclusively externally

1->2: GetRowsPriorTo()
1->3: ContactToHItem()
3->1: FindItem()
2->1: GetRowByIndex()
*/
extern void CheckPDNCE(pdisplayNameCacheEntry pdnce);


int GetRowsPriorTo(struct ClcGroup *group,struct ClcGroup *subgroup,int contactIndex)
{
	int count=0;
	BYTE k;
	k=DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1);
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
			group->scanIndex++;
			continue;
		}
		if(group==subgroup && contactIndex==group->scanIndex) return count;
		count++;
		/*		if ((group->contact[group->scanIndex].type==CLCIT_CONTACT) && (group->contact[group->scanIndex].flags & CONTACTF_STATUSMSG)) {
		count++;
		}
		*/
		if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			if(group->contact[group->scanIndex].group==subgroup && contactIndex==-1)
				return count-1;
			if(group->contact[group->scanIndex].group->expanded) {
				group=group->contact[group->scanIndex].group;
				group->scanIndex=0;
				continue;
			}
		}
		if(group->contact[group->scanIndex].type==CLCIT_CONTACT)
		{
			count+=(group->contact[group->scanIndex].SubAllocated*group->contact[group->scanIndex].SubExpanded*k);
		}
		group->scanIndex++;
	}
	return -1;
}

pdisplayNameCacheEntry GetCLCFullCacheEntry(struct ClcData *dat,HANDLE hContact)
{
	{
		displayNameCacheEntry dnce, *pdnce,*pdnce2;

		if (hContact==0) return NULL;
		dnce.hContact=hContact;

		pdnce=List_Find(&dat->lCLCContactsCache,&dnce);

		if (pdnce==NULL)
		{
			pdnce=mir_calloc(1,sizeof(displayNameCacheEntry));
			pdnce->hContact=hContact;
			List_Insert(&dat->lCLCContactsCache,pdnce,0);
			//List_Dump(&lContactsCache);
			List_Sort(&dat->lCLCContactsCache);
			//List_Dump(&lContactsCache);
			pdnce2=List_Find(&dat->lCLCContactsCache,&dnce);//for check
			if (pdnce2->hContact!=pdnce->hContact)
			{
				return (NULL);
			};

			if (pdnce2!=pdnce)
			{
				return (NULL);
			}
		};

		if (pdnce!=NULL) CheckPDNCE(pdnce);
		return (pdnce);

	}
}

void ClearClcContactCache(struct ClcData *dat,HANDLE hContact)
{
	/*
	pdisplayNameCacheEntry cacheEntry;

	if (hContact==INVALID_HANDLE_VALUE)
	{
	int i,tick;
	tick=GetTickCount();

	for(i=0;i<(dat->lCLCContactsCache.realCount);i++)
	{
	pdisplayNameCacheEntry pdnce;
	pdnce=dat->lCLCContactsCache.items[i];
	pdnce->ClcContact=NULL;
	}

	//FreeDisplayNameCache(&dat->lCLCContactsCache);
	//InitDisplayNameCache(&dat->lCLCContactsCache);			
	tick=GetTickCount()-tick;

	{ char buf[256];
	sprintf	(buf,"Clear full cache %d ms\r\n",tick);
	TRACE(buf);		
	}
	}
	else if(!IsHContactGroup(hContact)&&!IsHContactInfo(hContact))
	{
	{
	//				char buf[255];
	//sprintf(buf,"ClearCache %x,%x\r\n",dat,hContact);
	//TRACE(buf);
	}
	cacheEntry=GetCLCFullCacheEntry(dat,hContact);
	if (cacheEntry!=NULL)
	{
	cacheEntry->ClcContact=NULL;
	}
	}
	*/
}

/*
void SetClcContactCacheItem(struct ClcData *dat,HANDLE hContact,void *contact)
{
pdisplayNameCacheEntry cacheEntry;
if(!IsHContactGroup(hContact)&&!IsHContactInfo(hContact))
{
cacheEntry=GetCLCFullCacheEntry(dat,hContact);
if (cacheEntry!=NULL)
{
cacheEntry->ClcContact=contact;
};
}

}
*/


int FindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible, BOOL isIgnoreSubcontacts)
{
	int index=0, i;
	int nowVisible=1;
	struct ClcGroup *group=&dat->list;

	group->scanIndex=0;
	group=&dat->list;

	for(;;) {
		if(group->scanIndex==group->contactCount) {
			struct ClcGroup *tgroup;
			group=group->parent;
			if(group==NULL) break;
			nowVisible=1;
			for(tgroup=group;tgroup;tgroup=tgroup->parent)
			{
				if(!tgroup->expanded) 
				{
					nowVisible=0; 
					break;
				}
			}
			group->scanIndex++;
			continue;
		}
		if(nowVisible) index++;
		if((IsHContactGroup(hItem) && group->contact[group->scanIndex].type==CLCIT_GROUP && ((unsigned)hItem&~HCONTACT_ISGROUP)==group->contact[group->scanIndex].groupId) ||
			(IsHContactContact(hItem) && group->contact[group->scanIndex].type==CLCIT_CONTACT && group->contact[group->scanIndex].hContact==hItem) ||
			(IsHContactInfo(hItem) && group->contact[group->scanIndex].type==CLCIT_INFO && group->contact[group->scanIndex].hContact==(HANDLE)((unsigned)hItem&~HCONTACT_ISINFO))) 
		{
#ifdef _DEBUG
			if (IsBadWritePtr(&group->contact[group->scanIndex], sizeof(struct ClcContact)))
			{
				log1("FindIltem->IsBadWritePtr | 1o  [%08x]", &group->contact[group->scanIndex]);
				PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
				return 0;
			}
#endif
			if(isVisible) {
				if(!nowVisible) *isVisible=0;
				else {
					int posy = RowHeights_GetItemTopY(dat,index+1);
					if(posy<dat->yScroll) 
						*isVisible=0;
					else {
						RECT clRect;
						GetClientRect(hwnd,&clRect);
						if(posy>=dat->yScroll+clRect.bottom) *isVisible=0;
						else *isVisible=1;
					}
				}
			}
			if(contact) *contact=&group->contact[group->scanIndex];
			if(subgroup) *subgroup=group;
			return 1;
		}
		if (!isIgnoreSubcontacts && 
			IsHContactContact(hItem) &&
			group->contact[group->scanIndex].type == CLCIT_CONTACT &&
			group->contact[group->scanIndex].SubAllocated > 0)
		{
			for (i=0; i<group->contact[group->scanIndex].SubAllocated; i++)
			{
				if (group->contact[group->scanIndex].subcontacts[i].hContact == hItem)
				{	
#ifdef _DEBUG
					if (IsBadWritePtr(&group->contact[group->scanIndex].subcontacts[i], sizeof(struct ClcContact)))
					{
						log1("FindIltem->IsBadWritePtr | 2o  [%08x]", &group->contact[group->scanIndex].subcontacts[i]);
						PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
						return 0;
					}
#endif
					if(contact) *contact=&group->contact[group->scanIndex].subcontacts[i];
					if(subgroup) *subgroup=group;
					return 1;
				}
			}
		}

		if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			nowVisible&=group->expanded;
			continue;
		}
		group->scanIndex++;
	}

	return 0;
}
#define CacheArrSize 255
struct ClcGroup *CacheIndex[CacheArrSize]={NULL};
boolean CacheIndexClear=TRUE;
void ClearRowByIndexCache()
{
	if (!CacheIndexClear) 
	{
		memset(CacheIndex,0,sizeof(CacheIndex));
		CacheIndexClear=TRUE;
	};

}
int GetRowByIndex(struct ClcData *dat,int testindex,struct ClcContact **contact,struct ClcGroup **subgroup)
{
	int index=0,i;
	struct ClcGroup *group=&dat->list;

	if (testindex<0) return (-1);
	//	if (FALSE&&(testindex>0)&&testindex<CacheArrSize&&CacheIndex[testindex]!=NULL)
	//	{
	//					if(contact) *contact=&(CacheIndex[testindex])->contact[group->scanIndex];
	//					if(subgroup) *subgroup=(CacheIndex[testindex]);
	//					return (testindex);
	//	}else
	{
		group->scanIndex=0;
		for(;;) {
			if(group->scanIndex==group->contactCount) {
				group=group->parent;
				if(group==NULL) break;
				group->scanIndex++;
				continue;
			}
			if ((index>0) && (index<CacheArrSize)) 
			{
				CacheIndex[index]=group;
				CacheIndexClear=FALSE;
			};

			if(testindex==index) {
				if(contact) *contact=&group->contact[group->scanIndex];
				if(subgroup) *subgroup=group;
				return index;
			}

			if (group->contact[group->scanIndex].type==CLCIT_CONTACT)
				if (group->contact[group->scanIndex].SubAllocated)
					if (group->contact[group->scanIndex].SubExpanded && dat->expandMeta)
					{
						for (i=0;i<group->contact[group->scanIndex].SubAllocated;i++)
						{
							if ((index>0) && (index<CacheArrSize)) 
							{
								CacheIndex[index]=group;
								CacheIndexClear=FALSE;
							};
							index++;
							if(testindex==index) {
								if(contact) 
								{
									*contact=&group->contact[group->scanIndex].subcontacts[i];
									(*contact)->subcontacts=&(group->contact[group->scanIndex]);
								}

								if(subgroup) *subgroup=group;
								return index;
							}
						}

					}
					index++;
					/*			if ((group->contact[group->scanIndex].type==CLCIT_CONTACT) && (group->contact[group->scanIndex].flags & CONTACTF_STATUSMSG)) {
					index++;
					}
					*/
					if(group->contact[group->scanIndex].type==CLCIT_GROUP && group->contact[group->scanIndex].group->expanded) {
						group=group->contact[group->scanIndex].group;
						group->scanIndex=0;
						continue;
					}
					group->scanIndex++;
		}
	};
	return -1;
}

HANDLE ContactToHItem(struct ClcContact *contact)
{
	switch(contact->type) {
case CLCIT_CONTACT:
	return contact->hContact;
case CLCIT_GROUP:
	return (HANDLE)(contact->groupId|HCONTACT_ISGROUP);
case CLCIT_INFO:
	return (HANDLE)((DWORD)contact->hContact|HCONTACT_ISINFO);
	}
	return NULL;
}

HANDLE ContactToItemHandle(struct ClcContact *contact,DWORD *nmFlags)
{
	switch(contact->type) {
case CLCIT_CONTACT:
	return contact->hContact;
case CLCIT_GROUP:
	if(nmFlags) *nmFlags|=CLNF_ISGROUP;
	return (HANDLE)contact->groupId;
case CLCIT_INFO:
	if(nmFlags) *nmFlags|=CLNF_ISINFO;
	return (HANDLE)((DWORD)contact->hContact|HCONTACT_ISINFO);
	}
	return NULL;
}