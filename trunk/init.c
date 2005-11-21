/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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
#include "skinEngine.h"
extern HANDLE hSkinLoaded;
#include "version.h"

HINSTANCE g_hInst = 0;
PLUGINLINK * pluginLink;
struct MM_INTERFACE memoryManagerInterface;
static HANDLE hCListShutdown = 0;
extern HWND hwndContactList;
extern int LoadMoveToGroup();
int OnSkinLoad(WPARAM wParam, LPARAM lParam);
void UninitSkinHotKeys();

//from bgrcfg
//extern int BGModuleLoad();
//extern int BGModuleUnload();


PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
#ifndef _DEBUG
	"Modern Contact List Layered",
#else
	"DEBUG of Modern Contact List Layered",
#endif
	0,                              //will initiate later in MirandaPluginInfo
	"Display contacts, event notifications, protocol status with advantage visual modifications. Supported MW modifications, enchanced metacontact cooperation.",
	"Artem Shpynov and Ricardo Pescuma Domenecci, based on clist_mw by Bethoven",
	"shpynov@nm.ru" ,
	"Copyright 2000-2005 Miranda-IM project ["__DATE__" "__TIME__"]",
	"http://miranda-im.org/download/details.php?action=viewfile&id=2103",
	0,
	DEFMOD_CLISTALL
};

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
	g_hInst = hInstDLL;
	DisableThreadLibraryCalls(g_hInst);
	
	return TRUE;
}

int MakeVer(a,b,c,d)
{
    return (((((DWORD)(a))&0xFF)<<24)|((((DWORD)(b))&0xFF)<<16)|((((DWORD)(c))&0xFF)<<8)|(((DWORD)(d))&0xFF));
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION(0,4,0,0) ) return NULL;
    pluginInfo.version=MakeVer(PRODUCT_VERSION);
	return &pluginInfo;
}

int LoadContactListModule(void);
//int UnLoadContactListModule(void);
int LoadCLCModule(void); 
int LoadCLUIModule(); 

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{

//__try	
	{
	int *disableDefaultModule = 0;
	disableDefaultModule=(int*)CallService(MS_PLUGINS_GETDISABLEDEFAULTARRAY,0,0);
	if(!disableDefaultModule[DEFMOD_UICLUI]) if( LoadCLUIModule()) return 1;
	}
//__except (exceptFunction(GetExceptionInformation()) ) 
//{ 
//		return 0; 
//} 

	return 0;
}
int SetDrawer(WPARAM wParam,LPARAM lParam)
{
	pDrawerServiceStruct DSS=(pDrawerServiceStruct)wParam;
	if (DSS->cbSize!=sizeof(*DSS)) return -1;
	if (DSS->PluginName==NULL) return -1;
	if (DSS->PluginName==NULL) return -1;
	if (!ServiceExists(DSS->GetDrawFuncsServiceName)) return -1;

	
	SED.cbSize=sizeof(SED);
	SED.PaintClc=(void (__cdecl *)(HWND,struct ClcData *,HDC,RECT *,int ,ClcProtoStatus *,HIMAGELIST))CallService(DSS->GetDrawFuncsServiceName,CLUI_EXT_FUNC_PAINTCLC,0);
	if (!SED.PaintClc) return -1;
	return 0;
}

#include "time.h"
int __declspec(dllexport) CListInitialise(PLUGINLINK * link)
{
	int rc=0;
	pluginLink=link;
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif	
	// get the internal malloc/free()
	//__try
  {
	/*  {
		SYSTEMTIME utime,ctime;
		TIME_ZONE_INFORMATION timezone={0},tzi={0};
		DWORD tz=GetTimeZoneInformation(&tzi);			
		int a=(tz==TIME_ZONE_ID_STANDARD)?0:1;	
		char tim[256];
		char bu[255];
		int hour=-3;
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,bu,sizeof(bu));
		timezone.Bias=(hour-a)*60;
		GetSystemTime(&utime);
		SystemTimeToTzSpecificLocalTime(&timezone,&utime,&ctime);
		GetTimeFormat((LCID)LOCALE_USER_DEFAULT,TIME_NOSECONDS,&ctime,bu,&tim,sizeof(tim));
		a=a;

	  }*/
	TRACE("CListInitialise ClistMW\r\n");
    memset(&memoryManagerInterface,0,sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM)&memoryManagerInterface);
	
	memset(&SED,0,sizeof(SED));
	CreateServiceFunction(CLUI_SetDrawerService,SetDrawer);
	
    ///test///
    LoadSkinModule();
	rc=LoadContactListModule();
	if (rc==0) rc=LoadCLCModule();

	HookEvent(ME_SYSTEM_MODULESLOADED, systemModulesLoaded);
	LoadMoveToGroup();
	//BGModuleLoad();	
   
    //CallTest();
    TRACE("CListInitialise ClistMW...Done\r\n");

}

//
//__except (exceptFunction(GetExceptionInformation()) ) 
//{ 
//		return 0; 
//} 

	return rc;
}

// never called by a newer plugin loader.
int __declspec(dllexport) Load(PLUGINLINK * link)
{
	TRACE("Load ClistMW\r\n");
	MessageBoxA(0,"You Running Old Miranda, use >30-10-2004 version!","MultiWindow Clist",0);
	CListInitialise(link);
	return 1;
}

int __declspec(dllexport) Unload(void)
{
	TRACE("Unloading ClistMW\r\n");
    if (IsWindow(hwndContactList)) DestroyWindow(hwndContactList);
	//BGModuleUnload();
//    UnLoadContactListModule();
    UninitSkinHotKeys();
    UnhookEvent(hSkinLoaded);
    UnloadSkinModule();

   	hwndContactList=0;
  TRACE("***&&& NEED TO UNHOOK ALL EVENTS &&&***\r\n");
  UnhookAll();
	TRACE("Unloading ClistMW COMPLETE\r\n");

    
	return 0;
}
typedef struct _HookRec
{
  HANDLE hHook;
  char * HookStr;
} HookRec;
//UnhookAll();

HookRec * hooksrec=NULL;
DWORD hooksRecAlloced=0;
    
    
#undef HookEvent
#undef UnhookEvent

HANDLE MyHookEvent(char *EventID,MIRANDAHOOK HookProc)
{
  HookRec * hr=NULL;
  DWORD i;
  //1. Find free
  for (i=0;i<hooksRecAlloced;i++)
  {
    if (hooksrec[i].hHook==NULL) 
    {
      hr=&(hooksrec[i]);
      break;
    }
  }
  if (hr==NULL)
  {
    //2. Need realloc
    hooksrec=(HookRec*)mir_realloc(hooksrec,sizeof(HookRec)*(hooksRecAlloced+1));
    hr=&(hooksrec[hooksRecAlloced]);
    hooksRecAlloced++;
  }
  //3. Hook and rec
  hr->hHook=pluginLink->HookEvent(EventID,HookProc);
  hr->HookStr=NULL;
#ifdef _DEBUG
  if (hr->hHook) hr->HookStr=mir_strdup(EventID);
#endif
  //3. Hook and rec
  return hr->hHook;
}

int MyUnhookEvent(HANDLE hHook)
{
  DWORD i;
  //1. Find free
  for (i=0;i<hooksRecAlloced;i++)
  {
    if (hooksrec[i].hHook==hHook) 
    {
      pluginLink->UnhookEvent(hHook);
      hooksrec[i].hHook=NULL;
      if (hooksrec[i].HookStr) mir_free(hooksrec[i].HookStr);
      return 1;
    }
  }
	return 0;
}

int UnhookAll()
{
  DWORD i;
  TRACE("Unhooked Events:\n");
  for (i=0;i<hooksRecAlloced;i++)
  {
    if (hooksrec[i].hHook!=NULL) 
    {
      pluginLink->UnhookEvent(hooksrec[i].hHook);
      hooksrec[i].hHook=NULL;
      if (hooksrec[i].HookStr) 
      {
        TRACE(hooksrec[i].HookStr);
        TRACE("\n");
        mir_free(hooksrec[i].HookStr);
      }
    }
  }  
  mir_free(hooksrec);
  return 1;
}

#define HookEvent(a,b)  MyHookEvent(a,b)
#define UnhookEvent(a)  MyUnhookEvent(a)

 