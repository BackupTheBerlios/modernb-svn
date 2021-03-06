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
#ifndef _CLIST_H_
#define _CLIST_H_

void LoadContactTree(void);
int IconFromStatusMode(const char *szProto,int status);
int ExtIconFromStatusMode(HANDLE hContact, const char *szProto,int status);
HTREEITEM GetTreeItemByHContact(HANDLE hContact);
void TrayIconUpdateWithImageList(int iImage,const TCHAR *szNewTip,char *szPreferredProto);
void SortContacts(HWND hwnd);
void ChangeContactIcon(HANDLE hContact,int iIcon,int add);
int GetContactInfosForSort(HANDLE hContact,char **Proto,TCHAR **Name,int *Status);

typedef struct  {
int idxClientIcon;
int ClientID;
} ClientIcon;


typedef struct  {
	HANDLE hContact;
	TCHAR *name;
	int NameHash;
	char *szProto;
	boolean protoNotExists;
	int ProtoHash;
	int	status;
	int Hidden;
    int HiddenSubcontact;
	int noHiddenOffline;

	TCHAR *szGroup;
	int i;
	int ApparentMode;
	int NotOnList;
	int IdleTS;
	void *ClcContact;
	BYTE IsExpanded;
	boolean isUnknown;
	ClientIcon ci;
	char *MirVer;
} displayNameCacheEntry,*pdisplayNameCacheEntry;

pdisplayNameCacheEntry GetContactFullCacheEntry(HANDLE hContact);

#endif