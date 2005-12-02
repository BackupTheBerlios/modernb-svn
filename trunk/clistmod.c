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
#include <m_file.h>
#include <m_addcontact.h>
#include "clist.h"
#include "commonprototypes.h"
extern void Docking_GetMonitorRectFromWindow(HWND hWnd,RECT *rc);
int HideWindow(HWND hwndContactList, int mode);
extern int SmoothAlphaTransition(HWND hwnd, BYTE GoalAlpha, BOOL wParam);
extern int DefaultImageListColorDepth;
void InitGroupMenus(void);
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int AddMainMenuItem(WPARAM wParam,LPARAM lParam);
int AddContactMenuItem(WPARAM wParam,LPARAM lParam);
int InitCustomMenus(void);
void UninitCustomMenus(void);
int InitCListEvents(void);
void UninitCListEvents(void);
void InitDisplayNameCache(SortedList *list);
int ContactSettingChanged(WPARAM wParam,LPARAM lParam);
int ContactAdded(WPARAM wParam,LPARAM lParam);
int ContactDeleted(WPARAM wParam,LPARAM lParam);
int GetContactDisplayName(WPARAM wParam,LPARAM lParam);
int InvalidateDisplayName(WPARAM wParam,LPARAM lParam);
int CListOptInit(WPARAM wParam,LPARAM lParam);
int SkinOptInit(WPARAM wParam,LPARAM lParam);
void TrayIconUpdateBase(const char *szChangedProto);
int EventsProcessContactDoubleClick(HANDLE hContact);
int TrayIconProcessMessage(WPARAM wParam,LPARAM lParam);
int TrayIconPauseAutoHide(WPARAM wParam,LPARAM lParam);
void TrayIconIconsChanged(void);
int InitGroupServices(void);
int CompareContacts(WPARAM wParam,LPARAM lParam);
int ContactChangeGroup(WPARAM wParam,LPARAM lParam);
int ShowHide(WPARAM wParam,LPARAM lParam);
int SetHideOffline(WPARAM wParam,LPARAM lParam);
int ToggleHideOffline(WPARAM wParam,LPARAM lParam);
int Docking_ProcessWindowMessage(WPARAM wParam,LPARAM lParam);
int Docking_IsDocked(WPARAM wParam,LPARAM lParam);
int HotkeysProcessMessage(WPARAM wParam,LPARAM lParam);
int CListIconsChanged(WPARAM wParam,LPARAM lParam);
int MenuProcessCommand(WPARAM wParam,LPARAM lParam);
void InitDisplayNameCache(SortedList *list);
void FreeDisplayNameCache(SortedList *list);
void InitTray(void);

extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int ActivateSubContainers(BOOL active);
extern int BehindEdgeSettings;

HANDLE hContactDoubleClicked,hStatusModeChangeEvent,hContactIconChangedEvent;
HIMAGELIST hCListImages;
extern int currentDesiredStatusMode;
BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE,SIZE_T,SIZE_T);
extern BYTE nameOrder[];
extern SortedList lContactsCache;
extern HBITMAP GetCurrentWindowImage();


extern int BehindEdge_State;
extern int BehindEdgeSettings;
extern WORD BehindEdgeShowDelay;
extern WORD BehindEdgeHideDelay;
extern WORD BehindEdgeBorderSize;

static int statusModeList[]={ID_STATUS_OFFLINE,ID_STATUS_ONLINE,ID_STATUS_AWAY,ID_STATUS_NA,ID_STATUS_OCCUPIED,ID_STATUS_DND,ID_STATUS_FREECHAT,ID_STATUS_INVISIBLE,ID_STATUS_ONTHEPHONE,ID_STATUS_OUTTOLUNCH};
static int skinIconStatusList[]={SKINICON_STATUS_OFFLINE,SKINICON_STATUS_ONLINE,SKINICON_STATUS_AWAY,SKINICON_STATUS_NA,SKINICON_STATUS_OCCUPIED,SKINICON_STATUS_DND,SKINICON_STATUS_FREE4CHAT,SKINICON_STATUS_INVISIBLE,SKINICON_STATUS_ONTHEPHONE,SKINICON_STATUS_OUTTOLUNCH};
struct ProtoIconIndex {
	char *szProto;
	int iIconBase;
} static *protoIconIndex;
static int protoIconIndexCount;
static HANDLE hProtoAckHook;
static HANDLE hSettingChanged;

////////// By FYR/////////////
int ExtIconFromStatusMode(HANDLE hContact, const char *szProto,int status)
{
	if (DBGetContactSettingByte(NULL,"CLC","Meta",0)==1)
		return IconFromStatusMode(szProto,status);
	if (szProto!=NULL)
		if (MyStrCmp(szProto,"MetaContacts")==0)      {
			hContact=(HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(UINT)hContact,0);
			if (hContact!=0)            {
				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(UINT)hContact,0);
				status=DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
			}
		}
		return IconFromStatusMode(szProto,status);
}
/////////// End by FYR ////////


static int SetStatusMode(WPARAM wParam,LPARAM lParam)
{
	//todo: check wParam is valid so people can't use this to run random menu items
	MenuProcessCommand(MAKEWPARAM(LOWORD(wParam),MPCF_MAINMENU),0);
	return 0;
}

static int GetStatusMode(WPARAM wParam,LPARAM lParam)
{
	return currentDesiredStatusMode;
}

static int GetStatusModeDescriptionW(WPARAM wParam,LPARAM lParam)
{
	static TCHAR szMode[64];
	TCHAR *descr;
	int noPrefixReqd=0;
	switch(wParam) {
case ID_STATUS_OFFLINE:	descr=TranslateT("Offline"); noPrefixReqd=1; break;
case ID_STATUS_CONNECTING: descr=TranslateT("Connecting"); noPrefixReqd=1; break;
case ID_STATUS_ONLINE: descr=TranslateT("Online"); noPrefixReqd=1; break;
case ID_STATUS_AWAY: descr=TranslateT("Away"); break;
case ID_STATUS_DND:	descr=TranslateT("DND"); break;
case ID_STATUS_NA: descr=TranslateT("NA"); break;
case ID_STATUS_OCCUPIED: descr=TranslateT("Occupied"); break;
case ID_STATUS_FREECHAT: descr=TranslateT("Free for chat"); break;
case ID_STATUS_INVISIBLE: descr=TranslateT("Invisible"); break;
case ID_STATUS_OUTTOLUNCH: descr=TranslateT("Out to lunch"); break;
case ID_STATUS_ONTHEPHONE: descr=TranslateT("On the phone"); break;
case ID_STATUS_IDLE: descr=TranslateT("Idle"); break;
default:
	if(wParam>ID_STATUS_CONNECTING && wParam<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES) {
		_sntprintf(szMode,sizeof(szMode),TranslateT("Connecting (attempt %d)"),wParam-ID_STATUS_CONNECTING+1);
		return (int)szMode;
	}
	return (int)(TCHAR*)NULL;
	}
	if(noPrefixReqd || !(lParam&GSMDF_PREFIXONLINE)) return (int)descr;
	lstrcpy(szMode,TranslateT("Online"));
	lstrcat(szMode,TEXT(": "));
	lstrcat(szMode,descr);
	return (int)szMode;
}
static int GetStatusModeDescription(WPARAM wParam,LPARAM lParam)
{
#ifdef UNICODE
	if (!(lParam&CNF_UNICODE))
	{
		static char szMode[64]={0};
		TCHAR *buf1=(TCHAR*)GetStatusModeDescriptionW(wParam,lParam);
		char *buf2=u2a(buf1);
		_snprintf(szMode,sizeof(szMode),"%s",buf2);
		mir_free(buf2);
		return (int)szMode;
	}
	else
#endif
	return GetStatusModeDescriptionW(wParam,lParam);
}
static int ProtocolAck(WPARAM wParam,LPARAM lParam)
{
	ACKDATA *ack=(ACKDATA*)lParam;

	/*	if (ack->type==ACKTYPE_AWAYMSG && ack->lParam) {
	DBVARIANT dbv;
	if (!DBGetContactSetting(ack->hContact, "CList", "StatusMsg", &dbv)) {
	if (!MyStrCmp(dbv.pszVal, (char *)ack->lParam)) {
	DBFreeVariant(&dbv);
	return 0;
	}
	DBFreeVariant(&dbv);
	}
	DBWriteContactSettingString(ack->hContact, "CList", "StatusMsg", (char *)ack->lParam);
	return 0;
	}
	*/
	if(ack->type!=ACKTYPE_STATUS) return 0;
	CallService(MS_CLUI_PROTOCOLSTATUSCHANGED,ack->lParam,(LPARAM)ack->szModule);

	if((int)ack->hProcess<ID_STATUS_ONLINE && ack->lParam>=ID_STATUS_ONLINE) {
		DWORD caps;
		caps=(DWORD)CallProtoService(ack->szModule,PS_GETCAPS,PFLAGNUM_1,0);
		if(caps&PF1_SERVERCLIST) {
			HANDLE hContact;
			char *szProto;

			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
			while(hContact) {
				pdisplayNameCacheEntry cacheEntry;
				cacheEntry=GetContactFullCacheEntry(hContact);

				szProto=cacheEntry->szProto;
				if(szProto!=NULL && !MyStrCmp(szProto,ack->szModule))
					if(DBGetContactSettingByte(hContact,"CList","Delete",0))
						CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact,0);
				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
			}
		}
	}

	TrayIconUpdateBase(ack->szModule);
	return 0;
}

int IconFromStatusMode(const char *szProto,int status)
{
	int index,i;

	for(index=0;index<sizeof(statusModeList)/sizeof(statusModeList[0]);index++)
		if(status==statusModeList[index]) break;
	if(index==sizeof(statusModeList)/sizeof(statusModeList[0])) index=0;
	if(szProto==NULL) return index+1;
	for(i=0;i<protoIconIndexCount;i++) {
		if(MyStrCmp(szProto,protoIconIndex[i].szProto)) continue;
		return protoIconIndex[i].iIconBase+index;
	}
	return 1;
}

int GetContactIconC(pdisplayNameCacheEntry cacheEntry)
{
	return ExtIconFromStatusMode(cacheEntry->hContact,cacheEntry->szProto,cacheEntry->szProto==NULL ? ID_STATUS_OFFLINE : cacheEntry->status);
}

int GetContactIcon(WPARAM wParam,LPARAM lParam)
{
	char *szProto;
	int status;

	pdisplayNameCacheEntry cacheEntry;
	cacheEntry=GetContactFullCacheEntry((HANDLE)wParam);

	//szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
	szProto=cacheEntry->szProto;
	status=cacheEntry->status;
	return ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:status); //by FYR

}

static int ContactListShutdownProc(WPARAM wParam,LPARAM lParam)
{
	HANDLE hContact,hNext;

	//remove transitory contacts
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		hNext=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		if(DBGetContactSettingByte(hContact,"CList","NotOnList",0))
			CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact,0);
		hContact=hNext;
	}
	ImageList_Destroy(hCListImages);
	UnhookEvent(hProtoAckHook);
	UnhookEvent(hSettingChanged);
	UninitCustomMenus();
	UninitCListEvents();
	FreeDisplayNameCache(&lContactsCache);
	if (protoIconIndex) mir_free(protoIconIndex);
	DestroyHookableEvent(hContactDoubleClicked);
	return 0;
}

static int ContactListModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	int i,protoCount,j,iImg;
	PROTOCOLDESCRIPTOR **protoList;
	TRACE("ContactListModulesLoaded Start\r\n");

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&protoList);
	protoIconIndexCount=0;
	protoIconIndex=NULL;
	for(i=0;i<protoCount;i++) {
		if(protoList[i]->type!=PROTOTYPE_PROTOCOL) continue;
		protoIconIndex=(struct ProtoIconIndex*)mir_realloc(protoIconIndex,sizeof(struct ProtoIconIndex)*(protoIconIndexCount+1));
		protoIconIndex[protoIconIndexCount].szProto=protoList[i]->szName;
		for(j=0;j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++) {
			iImg=ImageList_AddIcon(hCListImages,LoadSkinnedProtoIcon(protoList[i]->szName,statusModeList[j]));
			if(j==0) protoIconIndex[protoIconIndexCount].iIconBase=iImg;
		}
		protoIconIndexCount++;
	}
	//	LoadContactTree();
	TRACE("ContactListModulesLoaded Done\r\n");
	return 0;
}


static int ContactDoubleClicked(WPARAM wParam,LPARAM lParam)
{

	// Check and an event from the CList queue for this hContact
	if (EventsProcessContactDoubleClick((HANDLE)wParam))
		NotifyEventHooks(hContactDoubleClicked, wParam, 0);

	return 0;

}


static BOOL CALLBACK AskForConfirmationDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hWnd);
			{
				LOGFONTA lf;
				HFONT hFont;

				hFont = (HFONT)SendDlgItemMessage(hWnd, IDYES, WM_GETFONT, 0, 0);
				GetObject(hFont, sizeof(lf), &lf);
				lf.lfWeight = FW_BOLD;
				SendDlgItemMessage(hWnd, IDC_TOPLINE, WM_SETFONT, (WPARAM)CreateFontIndirectA(&lf), 0);
			}
			{
				TCHAR szFormat[256];
				TCHAR szFinal[256];
				TCHAR * ch=(TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, lParam, 0); //TODO UNICODE
				GetDlgItemText(hWnd, IDC_TOPLINE, szFormat, sizeof(szFormat));
				_sntprintf(szFinal, sizeof(szFinal), szFormat, ch);
				SetDlgItemText(hWnd, IDC_TOPLINE, szFinal);
				mir_free(ch);
			}
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
			return TRUE;
		}

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDYES:
				if (IsDlgButtonChecked(hWnd, IDC_HIDE))
				{
					EndDialog(hWnd, IDC_HIDE);
					break;
				}
				//fall through
			case IDCANCEL:
			case IDNO:
				EndDialog(hWnd, LOWORD(wParam));
				break;
			}
			break;
		}

	case WM_CLOSE:
		SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDNO, BN_CLICKED), 0);
		break;

	case WM_DESTROY:
		DeleteObject((HFONT)SendDlgItemMessage(hWnd, IDC_TOPLINE, WM_GETFONT, 0, 0));
		break;
	}

	return FALSE;

}



static int MenuItem_DeleteContact(WPARAM wParam,LPARAM lParam)
{
	//see notes about deleting contacts on PF1_SERVERCLIST servers in m_protosvc.h
	int action;

	if (DBGetContactSettingByte(NULL, "CList", "ConfirmDelete", SETTING_CONFIRMDELETE_DEFAULT))
	{
		// Ask user for confirmation, and if the contact should be archived (hidden, not deleted)
		action = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DELETECONTACT), (HWND)lParam, AskForConfirmationDlgProc, wParam);
	}
	else
	{
		action = IDYES;
	}


	switch(action) {

		// Delete contact
case IDYES:
	{
		char *szProto;

		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
		if (szProto != NULL)
		{
			// Check if protocol uses server side lists
			DWORD caps;

			caps = (DWORD)CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0);
			if (caps&PF1_SERVERCLIST)
			{
				int status;

				status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
				if (status == ID_STATUS_OFFLINE || (status >= ID_STATUS_CONNECTING && status<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES))
				{
					// Set a flag so we remember to delete the contact when the protocol goes online the next time
					DBWriteContactSettingByte((HANDLE)wParam, "CList", "Delete", 1);
					MessageBoxA(NULL, Translate("This contact is on an instant messaging system which stores its contact list on a central server. The contact will be removed from the server and from your contact list when you next connect to that network."), Translate("Delete Contact"), MB_OK);
					return 0;
				}
			}
		}
		CallService(MS_DB_CONTACT_DELETE, wParam, 0);
		break;
	}

	// Archive contact
case IDC_HIDE:
	{
		DBWriteContactSettingByte((HANDLE)wParam,"CList","Hidden",1);
		break;
	}

	}

	return 0;

}



static int MenuItem_AddContactToList(WPARAM wParam,LPARAM lParam)
{
	ADDCONTACTSTRUCT acs={0};

	acs.handle=(HANDLE)wParam;
	acs.handleType=HANDLE_CONTACT;
	acs.szProto="";

	CallService(MS_ADDCONTACT_SHOW,(WPARAM)NULL,(LPARAM)&acs);
	return 0;
}

static int GetIconsImageList(WPARAM wParam,LPARAM lParam)
{
	return (int)hCListImages;
}

static int ContactFilesDropped(WPARAM wParam,LPARAM lParam)
{
	CallService(MS_FILE_SENDSPECIFICFILES,wParam,lParam);
	return 0;
}

int LoadContactListModule(void)
{
	/*	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact!=NULL) {
	DBWriteContactSettingString(hContact, "CList", "StatusMsg", "");
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	*/
	HookEvent(ME_SYSTEM_SHUTDOWN,ContactListShutdownProc);
	HookEvent(ME_SYSTEM_MODULESLOADED,ContactListModulesLoaded);
	HookEvent(ME_OPT_INITIALISE,CListOptInit);
	HookEvent(ME_OPT_INITIALISE,SkinOptInit);
	hSettingChanged=HookEvent(ME_DB_CONTACT_SETTINGCHANGED,ContactSettingChanged);
	HookEvent(ME_DB_CONTACT_ADDED,ContactAdded);
	HookEvent(ME_DB_CONTACT_DELETED,ContactDeleted);
	hProtoAckHook=(HANDLE)HookEvent(ME_PROTO_ACK,ProtocolAck);
	hContactDoubleClicked=CreateHookableEvent(ME_CLIST_DOUBLECLICKED);
	hStatusModeChangeEvent=CreateHookableEvent(ME_CLIST_STATUSMODECHANGE);
	hContactIconChangedEvent=CreateHookableEvent(ME_CLIST_CONTACTICONCHANGED);
	CreateServiceFunction(MS_CLIST_CONTACTDOUBLECLICKED,ContactDoubleClicked);
	CreateServiceFunction(MS_CLIST_CONTACTFILESDROPPED,ContactFilesDropped);
	CreateServiceFunction(MS_CLIST_SETSTATUSMODE,SetStatusMode);
	CreateServiceFunction(MS_CLIST_GETSTATUSMODE,GetStatusMode);
	CreateServiceFunction(MS_CLIST_GETSTATUSMODEDESCRIPTION,GetStatusModeDescription);
	CreateServiceFunction(MS_CLIST_GETCONTACTDISPLAYNAME,GetContactDisplayName);
	CreateServiceFunction(MS_CLIST_INVALIDATEDISPLAYNAME,InvalidateDisplayName);
	CreateServiceFunction(MS_CLIST_TRAYICONPROCESSMESSAGE,TrayIconProcessMessage);
	CreateServiceFunction(MS_CLIST_PAUSEAUTOHIDE,TrayIconPauseAutoHide);
	CreateServiceFunction(MS_CLIST_CONTACTSCOMPARE,CompareContacts);
	CreateServiceFunction(MS_CLIST_CONTACTCHANGEGROUP,ContactChangeGroup);
	CreateServiceFunction(MS_CLIST_SHOWHIDE,ShowHide);
	CreateServiceFunction(MS_CLIST_SETHIDEOFFLINE,SetHideOffline);
	CreateServiceFunction(MS_CLIST_TOGGLEHIDEOFFLINE,ToggleHideOffline);

	CreateServiceFunction(MS_CLIST_DOCKINGPROCESSMESSAGE,Docking_ProcessWindowMessage);
	CreateServiceFunction(MS_CLIST_DOCKINGISDOCKED,Docking_IsDocked);
	CreateServiceFunction(MS_CLIST_HOTKEYSPROCESSMESSAGE,HotkeysProcessMessage);
	CreateServiceFunction(MS_CLIST_GETCONTACTICON,GetContactIcon);
	MySetProcessWorkingSetSize=(BOOL (WINAPI*)(HANDLE,SIZE_T,SIZE_T))GetProcAddress(GetModuleHandle(TEXT("kernel32")),"SetProcessWorkingSetSize");
	hCListImages = ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 32, 0);

	InitDisplayNameCache(&lContactsCache);
	InitCListEvents();
	InitCustomMenus();
	InitGroupServices();	
	InitTray();

	{	CLISTMENUITEM mi;
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	CreateServiceFunction("CList/DeleteContactCommand",MenuItem_DeleteContact);
	mi.position=2000070000;
	mi.flags=0;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_DELETE));
	mi.pszContactOwner=NULL;    //on every contact
	mi.pszName=Translate("De&lete");
	mi.pszService="CList/DeleteContactCommand";
	CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
	CreateServiceFunction("CList/AddToListContactCommand",MenuItem_AddContactToList);
	mi.position=-2050000000;
	mi.flags=CMIF_NOTONLIST;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ADDCONTACT));
	mi.pszName=Translate("&Add permanently to list");
	mi.pszService="CList/AddToListContactCommand";
	CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
	}


	HookEvent(ME_SKIN_ICONSCHANGED,CListIconsChanged);
	CreateServiceFunction(MS_CLIST_GETICONSIMAGELIST,GetIconsImageList);
	ImageList_AddIcon(hCListImages, LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_BLANK)));
	{	int i;
	for(i=0;i<sizeof(statusModeList)/sizeof(statusModeList[0]);i++)
		ImageList_AddIcon(hCListImages, LoadSkinnedIcon(skinIconStatusList[i]));
	}
	//see IMAGE_GROUP... in clist.h if you add more images above here
	ImageList_AddIcon(hCListImages, LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN));
	ImageList_AddIcon(hCListImages, LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT));

	//InitGroupMenus();

	return 0;
}


int UnLoadContactListModule(void)
{
	FreeDisplayNameCache(&lContactsCache);

	return 0;
}
/*
Begin of Hrk's code for bug 
*/
#define GWVS_HIDDEN 1
#define GWVS_VISIBLE 2
#define GWVS_COVERED 3
#define GWVS_PARTIALLY_COVERED 4

int GetWindowVisibleState(HWND, int, int);
_inline DWORD GetDIBPixelColor(int X, int Y, int Width, int Height, int ByteWidth, BYTE * ptr)
{
	DWORD res=0;
	if (X>=0 && X<Width && Y>=0 && Y<Height && ptr)
		res=*((DWORD*)(ptr+ByteWidth*(Height-Y-1)+X*4));
	return res;
}

int GetWindowVisibleState(HWND hWnd, int iStepX, int iStepY) {
	RECT rc = { 0 };
	POINT pt = { 0 };
	HRGN rgn=NULL;
	register int i = 0, j = 0, width = 0, height = 0, iCountedDots = 0, iNotCoveredDots = 0;
	BOOL bPartiallyCovered = FALSE;
	HWND hAux = 0;
	int res=0;

	if (hWnd == NULL) {
		SetLastError(0x00000006); //Wrong handle
		return -1;
	}
	//Some defaults now. The routine is designed for thin and tall windows.
	if (iStepX <= 0) iStepX = 8;
	if (iStepY <= 0) iStepY = 16;

	if (IsIconic(hWnd) || !IsWindowVisible(hWnd))
		return GWVS_HIDDEN;
	else {
		int hstep,vstep;
		BITMAP bmp;
		HBITMAP WindowImage;
		int maxx=0;
		int maxy=0;
		int wx=0;
		int dx,dy;
		BYTE *ptr=NULL;
		HRGN rgn=NULL;
		WindowImage=LayeredFlag?GetCurrentWindowImage():0;
		if (WindowImage&&LayeredFlag)
		{
			GetObject(WindowImage,sizeof(BITMAP),&bmp);
			ptr=bmp.bmBits;
			maxx=bmp.bmWidth;
			maxy=bmp.bmHeight;
			wx=bmp.bmWidthBytes;
		}
		else
		{
			RECT rc;
			int i=0;
			rgn=CreateRectRgn(0,0,1,1);
			GetWindowRect(hWnd,&rc);
			GetWindowRgn(hWnd,rgn);
			OffsetRgn(rgn,rc.left,rc.top);
			GetRgnBox(rgn,&rc);	
			i=i;
			//maxx=rc.right;
			//maxy=rc.bottom;
		}
		GetWindowRect(hWnd, &rc);
		{
			RECT rcMonitor={0};
			Docking_GetMonitorRectFromWindow(hWnd,&rcMonitor);
			rc.top=rc.top<rcMonitor.top?rcMonitor.top:rc.top;
			rc.left=rc.left<rcMonitor.left?rcMonitor.left:rc.left;
			rc.bottom=rc.bottom>rcMonitor.bottom?rcMonitor.bottom:rc.bottom;
			rc.right=rc.right>rcMonitor.right?rcMonitor.right:rc.right;
		}
		width = rc.right - rc.left;
		height = rc.bottom- rc.top;
		dx=-rc.left;
		dy=-rc.top;
		hstep=width/iStepX;
		vstep=height/iStepY;
		hstep=hstep>0?hstep:1;
		vstep=vstep>0?vstep:1;

		for (i = rc.top; i < rc.bottom; i+=vstep) {
			pt.y = i;
			for (j = rc.left; j < rc.right; j+=hstep) {
				BOOL po=FALSE;
				pt.x = j;
				if (rgn) 
					po=PtInRegion(rgn,j,i);
				else
					po=(GetDIBPixelColor(j+dx,i+dy,maxx,maxy,wx,ptr)&0xFF000000)!=0;
				if (po||(!rgn&&ptr==0))
				{
					BOOL hWndFound=FALSE;
					hAux = WindowFromPoint(pt);
					do
					{
						if (hAux==hWnd) 
						{
							hWndFound=TRUE;
							break;
						}
						hAux = GetParent(hAux);
					}while(hAux!= NULL);

					if (hWndFound) //There's  window!
            iNotCoveredDots++; //Let's count the not covered dots.			
					//{
					//		  //bPartiallyCovered = TRUE;
          //           //iCountedDots++;
					//	    //break;
					//}
					//else             
					iCountedDots++; //Let's keep track of how many dots we checked.
				}
			}
		}    
		if (rgn) DeleteObject(rgn);
		if (iNotCoveredDots == iCountedDots) //Every dot was not covered: the window is visible.
			return GWVS_VISIBLE;
		else if (iNotCoveredDots == 0) //They're all covered!
			return GWVS_COVERED;
		else //There are dots which are visible, but they are not as many as the ones we counted: it's partially covered.
			return GWVS_PARTIALLY_COVERED;
	}
}
BYTE CALLED_FROM_SHOWHIDE=0;
int ShowHide(WPARAM wParam,LPARAM lParam) 
{
	HWND hwndContactList=(HWND)CallService(MS_CLUI_GETHWND,0,0);
	BOOL bShow = FALSE;

	int iVisibleState = GetWindowVisibleState(hwndContactList,0,0);
	int method;
	method=DBGetContactSettingByte(NULL, "ModernData", "HideBehind", 0);; //(0-none, 1-leftedge, 2-rightedge);
	if (method)
	{
		if (DBGetContactSettingByte(NULL, "ModernData", "BehindEdge", 0)==0 && lParam!=1)
		{
			//hide
			BehindEdge_Hide();
		}
		else
		{
			BehindEdge_Show();
		}
		bShow=TRUE;
		iVisibleState=GWVS_HIDDEN;
	}

	if (!method && DBGetContactSettingByte(NULL, "ModernData", "BehindEdge", 0)>0)
	{
		BehindEdgeSettings=DBGetContactSettingByte(NULL, "ModernData", "BehindEdge", 0);
		BehindEdge_Show();
		BehindEdgeSettings=0;
		BehindEdge_State=0;
		DBDeleteContactSetting(NULL, "ModernData", "BehindEdge");
	}

	//bShow is FALSE when we enter the switch if no hide behind edge.
	switch (iVisibleState) {
		case GWVS_PARTIALLY_COVERED:
			//If we don't want to bring it to top, we can use a simple break. This goes against readability ;-) but the comment explains it.			
		case GWVS_COVERED: //Fall through (and we're already falling)
			if (DBGetContactSettingByte(NULL,"CList","OnDesktop",0) || !DBGetContactSettingByte(NULL, "CList", "BringToFront", SETTING_BRINGTOFRONT_DEFAULT)) break;            
		case GWVS_HIDDEN:
			bShow = TRUE; break;
		case GWVS_VISIBLE: //This is not needed, but goes for readability.
			bShow = FALSE; break;
		case -1: //We can't get here, both hwndContactList and iStepX and iStepY are right.
			return 0;
	}
	if(bShow == TRUE || lParam) {
		WINDOWPLACEMENT pl={0};
		HMONITOR (WINAPI *MyMonitorFromWindow)(HWND,DWORD);
		RECT rcScreen,rcWindow;
		int offScreen=0;

		SystemParametersInfo(SPI_GETWORKAREA,0,&rcScreen,FALSE);
		GetWindowRect(hwndContactList,&rcWindow);

		ActivateSubContainers(TRUE);
		ShowWindowNew(hwndContactList, SW_RESTORE);

		if (!DBGetContactSettingByte(NULL,"CList","OnDesktop",0))
		{
			OnShowHide(hwndContactList,1);
			SetWindowPos(hwndContactList, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE);           
			CALLED_FROM_SHOWHIDE=1;
			BringWindowToTop(hwndContactList);			     
			if (!DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT))
				//&& ((DBGetContactSettingByte(NULL, "CList", "BringToFront", SETTING_BRINGTOFRONT_DEFAULT) /*&& iVisibleState>=2*/)))
				SetWindowPos(hwndContactList, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			SetForegroundWindow(hwndContactList);	     
			CALLED_FROM_SHOWHIDE=0;
		}
		else
		{
			SetWindowPos(hwndContactList, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			OnShowHide(hwndContactList,1);
			SetForegroundWindow(hwndContactList);	
		}
		DBWriteContactSettingByte(NULL,"CList","State",SETTING_STATE_NORMAL);
		//this forces the window onto the visible screen
		MyMonitorFromWindow=(HMONITOR (WINAPI *)(HWND,DWORD))GetProcAddress(GetModuleHandle(TEXT("USER32")),"MonitorFromWindow");
		if(MyMonitorFromWindow) {
			if(MyMonitorFromWindow(hwndContactList,0)==NULL) {
				BOOL (WINAPI *MyGetMonitorInfoA)(HMONITOR,LPMONITORINFO);
				MONITORINFO mi={0};
				HMONITOR hMonitor=MyMonitorFromWindow(hwndContactList,2);
				MyGetMonitorInfoA=(BOOL (WINAPI *)(HMONITOR,LPMONITORINFO))GetProcAddress(GetModuleHandle(TEXT("USER32")),"GetMonitorInfoA");
				mi.cbSize=sizeof(mi);
				MyGetMonitorInfoA(hMonitor,&mi);
				rcScreen=mi.rcWork;
				offScreen=1;
			}
		}
		else {
			RECT rcDest;
			if(IntersectRect(&rcDest,&rcScreen,&rcWindow)==0) offScreen=1;
		}
		if(offScreen) {
			if(rcWindow.top>=rcScreen.bottom) OffsetRect(&rcWindow,0,rcScreen.bottom-rcWindow.bottom);
			else if(rcWindow.bottom<=rcScreen.top) OffsetRect(&rcWindow,0,rcScreen.top-rcWindow.top);
			if(rcWindow.left>=rcScreen.right) OffsetRect(&rcWindow,rcScreen.right-rcWindow.right,0);
			else if(rcWindow.right<=rcScreen.left) OffsetRect(&rcWindow,rcScreen.left-rcWindow.left,0);
			SetWindowPos(hwndContactList,0,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,rcWindow.bottom-rcWindow.top,SWP_NOZORDER);
		}
		//if (DBGetContactSettingByte(NULL,"CList","OnDesktop",0))
		//    SetWindowPos(hwndContactList, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	}
	else { //It needs to be hidden

		HideWindow(hwndContactList, SW_HIDE);
		//OnShowHide(hwndContactList,0);

		DBWriteContactSettingByte(NULL,"CList","State",SETTING_STATE_HIDDEN);
		if(MySetProcessWorkingSetSize!=NULL && DBGetContactSettingByte(NULL,"CList","DisableWorkingSet",1)) MySetProcessWorkingSetSize(GetCurrentProcess(),-1,-1);
	}
	return 0;
}

int CListIconsChanged(WPARAM wParam,LPARAM lParam)
{
	int i,j;

	for(i=0;i<sizeof(statusModeList)/sizeof(statusModeList[0]);i++)
		ImageList_ReplaceIcon(hCListImages,i+1,LoadSkinnedIcon(skinIconStatusList[i]));
	ImageList_ReplaceIcon(hCListImages,IMAGE_GROUPOPEN, LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN));
	ImageList_ReplaceIcon(hCListImages,IMAGE_GROUPSHUT, LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT));
	for(i=0;i<protoIconIndexCount;i++)
		for(j=0;j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++)
			ImageList_ReplaceIcon(hCListImages,protoIconIndex[i].iIconBase+j,LoadSkinnedProtoIcon(protoIconIndex[i].szProto,statusModeList[j]));
	TrayIconIconsChanged();
	InvalidateRectZ((HWND)CallService(MS_CLUI_GETHWND,0,0),NULL,TRUE);
	return 0;
}

int HideWindow(HWND hwndContactList, int mode)
{
	TRACE("HIDE WINDOW\n");

	KillTimer(hwndContactList,1/*TM_AUTOALPHA*/);
	if (!BehindEdge_Hide())  return SmoothAlphaTransition(hwndContactList, 0, 1);
	return 0;
}
