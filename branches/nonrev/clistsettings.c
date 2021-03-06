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
#include "m_clui.h"
#include "m_clc.h"
#include "clist.h"
#include "dblists.h"
#include "commonprototypes.h"

void InsertContactIntoTree(HANDLE hContact,int status);
extern HWND hwndContactTree;
extern HANDLE hClcWindowList;
static displayNameCacheEntry *displayNameCache;

int PostAutoRebuidMessage(HWND hwnd);
static int displayNameCacheSize;

BOOL CLM_AUTOREBUILD_WAS_POSTED=FALSE;
SortedList lContactsCache;
int GetNameForContact(HANDLE hContact,int flag,boolean *isUnknown);
char *GetProtoForContact(HANDLE hContact);
int GetStatusForContact(HANDLE hContact,char *szProto);
TCHAR *UnknownConctactTranslatedName;
extern boolean OnModulesLoadedCalled;
void InvalidateDisplayNameCacheEntryByPDNE(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType);
extern int GetClientIconByMirVer(pdisplayNameCacheEntry pdnce);

static int DumpElem( pdisplayNameCacheEntry pdnce )
{
	char buf[256];
	if (pdnce==NULL)
	{
		MessageBoxA(0,"DumpElem Called with null","",0);
		return (0);
	}
	if (pdnce->name) sprintf(buf,"%x: hc:%x, %s\r\n",pdnce,pdnce->hContact,pdnce->name);
	else
		sprintf(buf,"%x: hc:%x\r\n",pdnce,pdnce->hContact);

	TRACE(buf);
	return 0;
};



static int handleCompare( void* c1, void* c2 )
{

	int p1, p2;

	displayNameCacheEntry	*dnce1=c1;
	displayNameCacheEntry	*dnce2=c2;

	p1=(int)dnce1->hContact;
	p2=(int)dnce2->hContact;

	if ( p1 == p2 )
		return 0;

	return p1 - p2;
}
static int handleQsortCompare( void** c1, void** c2 )
{
	//heh damn c pointer writing...i only want to call handleCompare(c1^,c2^) as in pascal.
	return (handleCompare((void *)*( long *)c1,(void *)*( long *)c2));		
}

void InitDisplayNameCache(SortedList *list)
{
	int i;
	HANDLE hContact;

	memset(list,0,sizeof(SortedList));
	list->dumpFunc =DumpElem;
	list->sortFunc=handleCompare;
	list->sortQsortFunc=handleQsortCompare;
	list->increment=CallService(MS_DB_CONTACT_GETCOUNT,0,0)+1;

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	i=0;
	while (hContact!=0)
	{
		displayNameCacheEntry *pdnce;
		pdnce=mir_calloc(1,sizeof(displayNameCacheEntry));
		pdnce->hContact=hContact;
		InvalidateDisplayNameCacheEntryByPDNE(hContact,pdnce,0);
		List_Insert(list,pdnce,i);
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		i++;
	}


	//List_Dump(list);
	List_Sort(list);
	//	List_Dump(list);
}
int gdnc=0;
void FreeDisplayNameCache(SortedList *list)
{
	int i;

	for(i=0;i<(list->realCount);i++)
	{
		pdisplayNameCacheEntry pdnce;
		pdnce=list->items[i];
		if (pdnce&&pdnce->name) mir_free(pdnce->name);
		pdnce->name=NULL;
		//if (pdnce&&pdnce->szProto) mir_free(pdnce->szProto);//proto is system string
		if (pdnce&&pdnce->szGroup) mir_free(pdnce->szGroup);
		if (pdnce&&pdnce->MirVer) mir_free(pdnce->MirVer);

		mir_free(pdnce);
	};
	gdnc++;		
	//mir_free(displayNameCache);
	//displayNameCache=NULL;
	//displayNameCacheSize=0;

	List_Destroy(list);
	list=NULL;

}

DWORD NameHashFunction(const char *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK
	__asm {		   //this breaks if szStr is empty
		xor  edx,edx
			xor  eax,eax
			mov  esi,szStr
			mov  al,[esi]
			xor  cl,cl
lph_top:	 //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
			xor  edx,eax
				inc  esi
				xor  eax,eax
				and  cl,31
				mov  al,[esi]
				add  cl,5
					test al,al
					rol  eax,cl		 //rol is u-pipe only, but pairable
					//rol doesn't touch z-flag
					jnz  lph_top  //5 clock tick loop. not bad.

					xor  eax,edx
	}
#else
	DWORD hash=0;
	int i;
	int shift=0;
	for(i=0;szStr[i];i++) {
		hash^=szStr[i]<<shift;
		if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
		shift=(shift+5)&0x1F;
	}
	return hash;
#endif
}


void CheckPDNCE(pdisplayNameCacheEntry pdnce)
{
	boolean getedname=FALSE;
	if (pdnce!=NULL)
	{
		/*
		{
		char buf[256];
		sprintf(buf,"LoadCacheDispEntry %x \r\n",pdnce);
		TRACE(buf);
		}
		*/
		if (pdnce->szProto==NULL&&pdnce->protoNotExists==FALSE)
		{
			//if (pdnce->szProto) mir_free(pdnce->szProto);
			pdnce->szProto=GetProtoForContact(pdnce->hContact);
			if (pdnce->szProto==NULL) 
			{
				pdnce->protoNotExists=FALSE;
			}else
			{
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)pdnce->szProto)==(int)NULL &&0)
				{
					pdnce->protoNotExists=TRUE;
				}else
				{
					if(pdnce->szProto&&pdnce->name) 
					{
						mir_free(pdnce->name);
						pdnce->name=NULL;
					}
				}
			}
			/*
			{
			char buf[256];
			sprintf(buf,"LoadCacheDispEntry_Proto %x %s ProtoIsLoaded: %s\r\n",pdnce,(pdnce->szProto?pdnce->szProto:"NoProtocol"),(pdnce->protoNotExists)?"NO":"FALSE/UNKNOWN");
			TRACE(buf);
			}
			*/

		}

		if (pdnce->name==NULL)
		{			
			getedname=TRUE;
			if (pdnce->protoNotExists)
			{
				pdnce->name=mir_strdupT(TranslateT("_NoProtocol_"));
			}
			else
			{
				if (OnModulesLoadedCalled)
				{
					pdnce->name=(TCHAR *)GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
				}
				else
				{
					pdnce->name=(TCHAR *)GetNameForContact(pdnce->hContact,0,NULL); //TODO UNICODE
				}
			}	

			//pdnce->NameHash=NameHashFunction(pdnce->name);
		}
		else
		{
			if (pdnce->isUnknown&&pdnce->szProto&&pdnce->protoNotExists==TRUE&&OnModulesLoadedCalled)
			{
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)pdnce->szProto)==(int)NULL)
				{
					pdnce->protoNotExists=FALSE;						

					/*	
					{
					char buf[256];
					sprintf(buf,"LoadCacheDispEntry_Proto %x %s Now Loaded !!! \r\n",pdnce,(pdnce->szProto?pdnce->szProto:"NoProtocol"));
					TRACE(buf);
					}
					*/

					mir_free(pdnce->name);
					pdnce->name=(TCHAR *)GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
					getedname=TRUE;
				}

			}

		}

		/*			{
		if (getedname)
		{				
		char buf[256];
		sprintf(buf,"LoadCacheDispEntry_GetedName %x %s isUnknown: %x\r\n",pdnce,(pdnce->name?pdnce->name:""),pdnce->isUnknown);
		TRACE(buf);
		}

		}		
		*/


		if (pdnce->status==0)
		{
			pdnce->status=GetStatusForContact(pdnce->hContact,pdnce->szProto);
		}
		if (pdnce->szGroup==NULL)
		{
			DBVARIANT dbv;

			if (!DBGetContactSettingTString(pdnce->hContact,"CList","Group",&dbv))
			{
				pdnce->szGroup=mir_strdupT(dbv.ptszVal);
				mir_free(dbv.ptszVal);
				DBFreeVariant(&dbv);
			}else
			{
				pdnce->szGroup=mir_strdupT(TEXT(""));
			}

		}
		if (pdnce->Hidden==-1)
		{
			pdnce->Hidden=DBGetContactSettingByte(pdnce->hContact,"CList","Hidden",0);
		}
		//if (pdnce->HiddenSubcontact==-1)
		//{
		pdnce->HiddenSubcontact=DBGetContactSettingByte(pdnce->hContact,"MetaContacts","IsSubcontact",0);
		//};

		if (pdnce->noHiddenOffline==-1)
		{
			pdnce->noHiddenOffline=DBGetContactSettingByte(pdnce->hContact,"CList","noOffline",0);
		}

		if (pdnce->IdleTS==-1)
		{
			pdnce->IdleTS=DBGetContactSettingDword(pdnce->hContact,pdnce->szProto,"IdleTS",0);
		};

		if (pdnce->ApparentMode==-1)
		{
			pdnce->ApparentMode=DBGetContactSettingWord(pdnce->hContact,pdnce->szProto,"ApparentMode",0);
		};				
		if (pdnce->NotOnList==-1)
		{
			pdnce->NotOnList=DBGetContactSettingByte(pdnce->hContact,"CList","NotOnList",0);
		};		

		if (pdnce->IsExpanded==-1)
		{
			pdnce->IsExpanded=DBGetContactSettingByte(pdnce->hContact,"CList","Expanded",0);
		}

		if (pdnce->MirVer==NULL||pdnce->ci.idxClientIcon==-2||pdnce->ci.idxClientIcon==-2)
		{
			if (pdnce->MirVer) mir_free(pdnce->MirVer);
			pdnce->MirVer=DBGetStringA(pdnce->hContact,pdnce->szProto,"MirVer");
			if (!pdnce->MirVer) pdnce->MirVer=mir_strdup("");
			pdnce->ci.ClientID=-1;
			pdnce->ci.idxClientIcon=-1;
			if (pdnce->MirVer!=NULL)
			{
				GetClientIconByMirVer(pdnce);
			}

		}
	}
}

pdisplayNameCacheEntry GetDisplayNameCacheEntry(HANDLE hContact)
{
	{
		displayNameCacheEntry dnce={0}, *pdnce,*pdnce2;

		dnce.hContact=hContact;
		pdnce=List_Find(&lContactsCache,&dnce);
		if (pdnce==NULL)
		{
			pdnce=mir_calloc(1,sizeof(displayNameCacheEntry));
			ZeroMemory(pdnce, sizeof(displayNameCacheEntry));
			pdnce->hContact=hContact;
			List_Insert(&lContactsCache,pdnce,0);
#ifdef _DEBUG
			OutputDebugStringA("List Dump after INSERTING:\n -------------------\n");
			List_Dump(&lContactsCache);
#endif			
			List_Sort(&lContactsCache);
#ifdef _DEBUG
			OutputDebugStringA("\nList Dump after SORTING:\n -------------------\n");
			List_Dump(&lContactsCache);
#endif	
			pdnce2=List_Find(&lContactsCache,&dnce);//for check
			if (pdnce2->hContact!=pdnce->hContact)
			{
				MessageBoxA(0,"pdnce2->hContact (after inserting and sorting) is not the same as pdnce->hContact ","Error in GetDisplayNameCacheEntry",MB_OK|MB_ICONEXCLAMATION);
				return (pdnce2);
			};

			if (pdnce2!=pdnce)
			{
				MessageBoxA(0,"pdnce2 (after inserting and sorting) is not the same as pdnce","Error in GetDisplayNameCacheEntry",MB_OK|MB_ICONEXCLAMATION);
				return (pdnce2);
			}
		};

		if (pdnce!=NULL) CheckPDNCE(pdnce);
		return (pdnce);

	}
}
void InvalidateDisplayNameCacheEntryByPDNE(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType)
{
	if (hContact==NULL) return;
	if (pdnce==NULL) return;
	if (pdnce)
	{
		//#ifdef _DEBUG
		//    {
		//      char buf[256];
		//      sprintf(buf,"InvDisNmCaEn %x %s SettingType: %x\r\n",pdnce,(pdnce->name?pdnce->name:""),SettingType);
		//      TRACE(buf);
		//    }
		//#endif
		if (SettingType==-1||SettingType==DBVT_DELETED)
		{		
			if (pdnce->name) mir_free(pdnce->name);
			pdnce->name=NULL;
			if (pdnce->szGroup) mir_free(pdnce->szGroup);
			// if (pdnce->szProto) mir_free(pdnce->szProto);   //free proto
			pdnce->szGroup=NULL;

			pdnce->Hidden=-1;
			pdnce->HiddenSubcontact=-1;
			pdnce->protoNotExists=FALSE;
			pdnce->szProto=NULL;
			pdnce->status=0;
			pdnce->IdleTS=-1;
			pdnce->ApparentMode=-1;
			pdnce->NotOnList=-1;
			pdnce->isUnknown=FALSE;
			pdnce->noHiddenOffline=-1;
			pdnce->IsExpanded=-1;
			if (pdnce->MirVer) mir_free(pdnce->MirVer);
			pdnce->MirVer=NULL;

			pdnce->ci.ClientID=-2;
			pdnce->ci.idxClientIcon=-2;

			return;
		}
		if (SettingType==DBVT_ASCIIZ||SettingType==DBVT_BLOB)
		{
			if (pdnce->name) mir_free(pdnce->name);
			if (pdnce->szGroup) mir_free(pdnce->szGroup);
			//if (pdnce->szProto) mir_free(pdnce->szProto);
			if (pdnce->MirVer) mir_free(pdnce->MirVer);			
			pdnce->name=NULL;			
			pdnce->szGroup=NULL;
			pdnce->szProto=NULL;				
			pdnce->MirVer=NULL;
			pdnce->ci.ClientID=-2;
			pdnce->ci.idxClientIcon=-2;

			return;
		}
		// in other cases clear all binary cache
		pdnce->Hidden=-1;
		pdnce->HiddenSubcontact=-1;
		pdnce->protoNotExists=FALSE;
		pdnce->status=0;
		pdnce->IdleTS=-1;
		pdnce->ApparentMode=-1;
		pdnce->NotOnList=-1;
		pdnce->isUnknown=FALSE;
		pdnce->noHiddenOffline=-1;
		pdnce->IsExpanded=-1;
		//Can cause start delay
   		    if (pdnce->MirVer) mir_free(pdnce->MirVer);
			pdnce->MirVer=NULL;
			pdnce->ci.ClientID=-2;
			pdnce->ci.idxClientIcon=-2;

	};
};
void InvalidateDisplayNameCacheEntry(HANDLE hContact)
{
	if(hContact==INVALID_HANDLE_VALUE) {
		FreeDisplayNameCache(&lContactsCache);
		InitDisplayNameCache(&lContactsCache);
		SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
		//TRACE("InvDisNmCaEn full\r\n");
	}
	else {
 		pdisplayNameCacheEntry pdnce=GetDisplayNameCacheEntry(hContact);
		InvalidateDisplayNameCacheEntryByPDNE(hContact,pdnce,-1);
	}
}
char *GetContactCachedProtocol(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->szProto) return cacheEntry->szProto;

	return (NULL);
};
char *GetProtoForContact(HANDLE hContact)
{
	char *szProto=NULL;
	szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
	return szProto;
	/*
	DBVARIANT dbv;
	DBCONTACTGETSETTING dbcgs;
	char name[32];

	dbv.type=DBVT_ASCIIZ;
	dbv.pszVal=name;
	dbv.cchVal=sizeof(name);
	dbcgs.pValue=&dbv;
	dbcgs.szModule="Protocol";
	dbcgs.szSetting="p";
	if(CallService(MS_DB_CONTACT_GETSETTINGSTATIC,(WPARAM)hContact,(LPARAM)&dbcgs)) return NULL;
	szProto=mir_strdup(dbcgs.pValue->pszVal);
	if (szProto==NULL) return(NULL);
	return((szProto));
	*/
};

int GetStatusForContact(HANDLE hContact,char *szProto)
{
	int status=ID_STATUS_OFFLINE;
	if (szProto)
	{
		status=DBGetContactSettingWord((HANDLE)hContact,szProto,"Status",ID_STATUS_OFFLINE);
	}
	return (status);
}

int GetNameForContact(HANDLE hContact,int flag,boolean *isUnknown)
{
	CONTACTINFO ci;
	TCHAR *buffer;
	//return mir_strdup("Test"); //test

	if (UnknownConctactTranslatedName==NULL) UnknownConctactTranslatedName=(TranslateT("'(Unknown Contact)'"));

	ZeroMemory(&ci,sizeof(ci));
	if (isUnknown) *isUnknown=FALSE;
	ci.cbSize = sizeof(ci);
	ci.hContact = (HANDLE)hContact;
	if (ci.hContact==NULL) ci.szProto = "ICQ";
	if (TRUE)
	{

		ci.dwFlag = (int)flag==GCDNF_NOMYHANDLE?CNF_DISPLAYNC:CNF_DISPLAY;
		ci.dwFlag|=CNF_UNICODET;
		if (!CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci)) {
			if (ci.type==CNFT_ASCIIZ) {
				buffer = (TCHAR*)mir_alloc((lstrlen(ci.pszVal)+1)*sizeof(TCHAR));
				_sntprintf(buffer,lstrlen(ci.pszVal)+1,_T("%s"),ci.pszVal);
				mir_free(ci.pszVal);
				//!!! need change
				if (isUnknown&&!lstrcmp(buffer,UnknownConctactTranslatedName))
				{
					*isUnknown=TRUE;
				}
				TRACET(buffer);
				TRACE("\n");
				return (int)buffer;
			}
			if (ci.type==CNFT_DWORD) {

				buffer = (TCHAR*)mir_alloc(15*sizeof(TCHAR));
				_sntprintf(buffer,15,_T("%u"),ci.dVal);
				TRACET(buffer);
				TRACE("\n");
				return (int)buffer;
			}
		}
	}
	CallContactService((HANDLE)hContact,PSS_GETINFO,SGIF_MINIMAL,0);
	if (isUnknown) *isUnknown=TRUE;
	buffer=TranslateT("'(Unknown Contact)'");
	return (int)mir_strdupT(buffer);
}

int GetContactHashsForSort(HANDLE hContact,int *ProtoHash,int *NameHash,int *Status)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry!=NULL)
	{
		if (ProtoHash!=NULL) *ProtoHash=cacheEntry->ProtoHash;
		if (NameHash!=NULL) *NameHash=cacheEntry->NameHash;
		if (Status!=NULL) *Status=cacheEntry->status;
	}
	return (0);
};

pdisplayNameCacheEntry GetContactFullCacheEntry(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry!=NULL)
	{
		return(cacheEntry);
	}
	return (NULL);
}
int GetContactInfosForSort(HANDLE hContact,char **Proto,TCHAR **Name,int *Status)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry!=NULL)
	{
		if (Proto!=NULL) *Proto=cacheEntry->szProto;
		if (Name!=NULL) *Name=cacheEntry->name;
		if (Status!=NULL) *Status=cacheEntry->status;
	}
	return (0);
};


int GetContactCachedStatus(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->status!=0) return cacheEntry->status;
	return (0);	
};

//----
//TODO UNICODE
int GetContactDisplayName(WPARAM wParam,LPARAM lParam)
{
	pdisplayNameCacheEntry cacheEntry=NULL;

#ifdef UNICODE
	if ( lParam & GCDNF_UNICODE)
#endif
	{
		if ((int)lParam!=GCDNF_NOMYHANDLE) {
			cacheEntry=GetDisplayNameCacheEntry((HANDLE)wParam);
			if (cacheEntry&&cacheEntry->name) return (int)cacheEntry->name;
			//cacheEntry=cacheEntry;
		return 0;//"-invalid-";
			//if(displayNameCache[cacheEntry].name) return (int)displayNameCache[cacheEntry].name;
		}
		return (GetNameForContact((HANDLE)wParam,lParam,NULL));
	}
#ifdef UNICODE
	else
	{
		TCHAR * b=NULL;
		char * temp=NULL;
		static char cachedname[120]={0};
		if ((int)lParam!=GCDNF_NOMYHANDLE) {
			cacheEntry=GetDisplayNameCacheEntry((HANDLE)wParam);
			if (cacheEntry&&cacheEntry->name) 
				b=cacheEntry->name;
			else
				return 0;//"-invalid-";
		}
		else
			b=(TCHAR*)GetNameForContact((HANDLE)wParam,lParam,NULL);	
		if (!b) return 0;
		temp=u2a(b);
		if (temp)
		{
			_snprintf(cachedname,sizeof(cachedname),"%s",temp);
			mir_free(temp);
			return (int)cachedname;
		}
		return 0;
	}
#endif
}

int InvalidateDisplayName(WPARAM wParam,LPARAM lParam) {
	InvalidateDisplayNameCacheEntry((HANDLE)wParam);
	return 0;
}

int ContactAdded(WPARAM wParam,LPARAM lParam)
{
	ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,(char*)GetContactCachedProtocol((HANDLE)wParam),ID_STATUS_OFFLINE),1); ///by FYR
	//	ChangeContactIcon((HANDLE)wParam,IconFromStatusMode((char*)GetContactCachedProtocol((HANDLE)wParam),ID_STATUS_OFFLINE),1);
	SortContacts(NULL);
	return 0;
}

int ContactDeleted(WPARAM wParam,LPARAM lParam)
{
	CallService(MS_CLUI_CONTACTDELETED,wParam,0);
	return 0;
}

extern void ReAskStatusMessage(HANDLE wParam);

int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	DBVARIANT dbv;
	pdisplayNameCacheEntry pdnce;

	// Early exit
	if ((HANDLE)wParam == NULL)
		return 0;
	
//__try 
	{
		dbv.pszVal = NULL;
		pdnce=GetDisplayNameCacheEntry((HANDLE)wParam);

		if (pdnce==NULL)
		{
			TRACE("!!! Very bad pdnce not found.");
			if (dbv.pszVal) mir_free(dbv.pszVal);
			return 0;
		}
		if (pdnce->protoNotExists==FALSE && pdnce->szProto)
		{
				if (!strcmp(cws->szModule,pdnce->szProto))
			{
				InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);

					if (!strcmp(cws->szSetting,"IsSubcontact"))
				{
					PostMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
				}
				if (!MyStrCmp(cws->szSetting, "Status") ||
					 WildCompare((char*)cws->szSetting, (char*) "Status?",2))
				{
					
					if (!MyStrCmp(cws->szModule,"MetaContacts") && MyStrCmp(cws->szSetting, "Status"))
					{
						int res=0;
						//InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
						if (hwndContactTree && OnModulesLoadedCalled) 
							res=PostAutoRebuidMessage(hwndContactTree);
						if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",0)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",0)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
						{
							//	if (pdnce->status!=ID_STATUS_OFFLINE)  
							ReAskStatusMessage((HANDLE)wParam);  
						}
						DBFreeVariant(&dbv);
						return 0;
					}
					if (!(pdnce->Hidden==1)) 
					{		
						pdnce->status=cws->value.wVal;
						if (cws->value.wVal == ID_STATUS_OFFLINE) 
						{
							//if (DBGetContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",0)
							if (DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",0))
							{
								char a='\0';
								DBWriteContactSettingString((HANDLE)wParam,"CList","StatusMsg",&a);
							}
						}
						if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",0)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",0)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
						{
							//	if (pdnce->status!=ID_STATUS_OFFLINE)  
							ReAskStatusMessage((HANDLE)wParam);  
						}
						TRACE("From ContactSettingChanged: ");
						WindowList_Broadcast(hClcWindowList,INTM_STATUSCHANGED,wParam,0);
						ChangeContactIcon((HANDLE)wParam, ExtIconFromStatusMode((HANDLE)wParam,cws->szModule, cws->value.wVal), 0); //by FYR
						SortContacts(NULL);
					}
					else 
					{
						if (!(!MyStrCmp(cws->szSetting, "LogonTS")
							||!MyStrCmp(cws->szSetting, "TickTS")
							||!MyStrCmp(cws->szSetting, "InfoTS")
							))
						{
							SortContacts(NULL);
						}
						DBFreeVariant(&dbv);
						return 0;
					}
					//TRACE("Second SortContact\n");
				}
			}

			if(!strcmp(cws->szModule,"CList")) 
			{
				//name is null or (setting is myhandle)
				if (pdnce->name==NULL || !strcmp(cws->szSetting,"MyHandle"))
				{
					InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
				}
				else if (!strcmp(cws->szSetting,"Group")) 
				{
					InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
				}
				else if (!strcmp(cws->szSetting,"Hidden")) 
				{
					InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);		
					if(cws->value.type==DBVT_DELETED || cws->value.bVal==0) 
					{
						char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
						//				ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);
						ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);  //by FYR
					}
				}
				else if(!strcmp(cws->szSetting,"noOffline")) 
				{
					InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);		
				}
			}
			else if(!strcmp(cws->szModule,"Protocol")) 
			{
				if(!strcmp(cws->szSetting,"p")) 
				{
					char *szProto;

					TRACE("CHANGE: proto\r\n");
					InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);	
					if(cws->value.type==DBVT_DELETED) szProto=NULL;
					else szProto=cws->value.pszVal;
					ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),0); //by FYR
					//			ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),0);
				}
			}

			// Clean up
			if (dbv.pszVal)
				mir_free(dbv.pszVal);
		} 
		//__except (exceptFunction(GetExceptionInformation()) ) 
		//{ 
		//return 0; 
		//} 
	}
	return 0;
}
int PostAutoRebuidMessage(HWND hwnd)
{
	if (!CLM_AUTOREBUILD_WAS_POSTED)
		CLM_AUTOREBUILD_WAS_POSTED=PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
	return CLM_AUTOREBUILD_WAS_POSTED;
}
