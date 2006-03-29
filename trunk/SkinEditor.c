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
#include "clist.h"
#include "m_clc.h"
#include "SkinEngine.h"
#include "io.h"
#include "commonprototypes.h"

#define TreeView_InsertItemA(hwnd, lpis) \
	(HTREEITEM)SendMessageA((hwnd), TVM_INSERTITEMA, 0, (LPARAM)(LPTV_INSERTSTRUCTA)(lpis))

#define TreeView_GetItemA(hwnd, pitem) \
	(BOOL)SendMessageA((hwnd), TVM_GETITEMA, 0, (LPARAM)(TV_ITEM *)(pitem))

typedef struct 
{
	char * szName;
	char * szPath;
	char * szValue;
	char * szTempValue;
} OPT_OBJECT_DATA;

static char *gl_Mask=NULL;
HWND gl_Dlg=NULL;
int  gl_controlID=0;


HTREEITEM FindChild(HWND hTree, HTREEITEM Parent, char * Caption)
{
  HTREEITEM res=NULL, tmp=NULL;
  if (Parent) 
    tmp=TreeView_GetChild(hTree,Parent);
  else 
	tmp=TreeView_GetRoot(hTree);
  while (tmp)
  {
    TVITEMA tvi;
    char buf[255];
    tvi.hItem=tmp;
    tvi.mask=TVIF_TEXT|TVIF_HANDLE;
    tvi.pszText=(LPSTR)&buf;
    tvi.cchTextMax=254;
    TreeView_GetItemA(hTree,&tvi);
    if (boolstrcmpi(Caption,tvi.pszText))
      return tmp;
    tmp=TreeView_GetNextSibling(hTree,tmp);
  }
  return tmp;
}

int TreeAddObject(HWND hwndDlg, int ID, OPT_OBJECT_DATA * data)
{
	HTREEITEM rootItem=NULL;
	HTREEITEM cItem=NULL;
	char * path=data->szPath?mir_strdup(data->szPath):mir_strdup((data->szName)+2);
	char * ptr=path;
	char * ptrE=path;
	BOOL ext=FALSE;
	do 
	{
		
		while (*ptrE!='/' && *ptrE!='\0') ptrE++;
		if (*ptrE=='/')
		{
			*ptrE='\0';
			ptrE++;
			// find item if not - create;
			{
				cItem=FindChild(GetDlgItem(hwndDlg,ID),rootItem,ptr);
				if (!cItem) // not found - create node
				{
					TVINSERTSTRUCTA tvis;
					tvis.hParent=rootItem;
					tvis.hInsertAfter=TVI_SORT;
					tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_PARAM;
					tvis.item.pszText=ptr;
					tvis.item.lParam=(LPARAM)NULL;
					cItem=TreeView_InsertItemA(GetDlgItem(hwndDlg,ID),&tvis);
					
				}	
				rootItem=cItem;
			}
			ptr=ptrE;
		}
		else ext=TRUE;
	}while (!ext);
	//Insert item node
	{
		TVINSERTSTRUCTA tvis;
		tvis.hParent=rootItem;
		tvis.hInsertAfter=TVI_SORT;
		tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_PARAM;
		tvis.item.pszText=ptr;
		tvis.item.lParam=(LPARAM)data;
		TreeView_InsertItemA(GetDlgItem(hwndDlg,ID),&tvis);
	}
	mir_free(path);
	return 0;
}
int EnumSkinObjectsInBase(const char *szSetting,LPARAM lParam)
{
	if (WildCompare((char *)szSetting,gl_Mask,0))
	{
		char * value;
		char *desc;
		char *descKey;
		descKey=mir_strdup(szSetting);
		descKey[0]='%';
		value=DBGetStringA(NULL,SKIN,szSetting);
		desc=DBGetStringA(NULL,SKIN,descKey);
		if (WildCompare(value,"?lyph*",0))
		{
			OPT_OBJECT_DATA * a=mir_alloc(sizeof(OPT_OBJECT_DATA));
			a->szPath=desc;
			a->szName=mir_strdup(szSetting);
			a->szValue=value;
			a->szTempValue=NULL;
			TreeAddObject(gl_Dlg,gl_controlID,a);
		}
		else
		{
			if (value) mir_free(value);
			if (desc) mir_free(desc);
		}
		mir_free(descKey);		
	}
	return 1;
}

int FillObjectTree(HWND hwndDlg, int ObjectTreeID, char * wildmask)
{
	DBCONTACTENUMSETTINGS dbces;
	gl_Dlg=hwndDlg;
	gl_controlID=ObjectTreeID;
	gl_Mask=wildmask;
	dbces.pfnEnumProc=EnumSkinObjectsInBase;
	dbces.szModule=SKIN;
	dbces.ofsSettings=0;
	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);
	return 0;
}

static BOOL CALLBACK DlgSkinEditorOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY: 
		{
			break;
		}

	case WM_INITDIALOG:
		{ 			
			TranslateDialogDefault(hwndDlg);
			FillObjectTree(hwndDlg,IDC_OBJECT_TREE,"$%*");
			break;
		}

	case WM_COMMAND:	
		{	
			//switch (LOWORD(wParam))
			//{
			//
			//}
			//check (LOWORD(wParam))
			//SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) 
		{
		case IDC_OBJECT_TREE:
				{
					//Save existed object
					//Change to new object
					NMTREEVIEWA * nmtv = (NMTREEVIEWA *) lParam;
					if (!nmtv) return 0;
					if (nmtv->hdr.code==TVN_SELCHANGEDA)
					{
						if (nmtv->itemNew.lParam)
						{
							OPT_OBJECT_DATA * data=(OPT_OBJECT_DATA*)nmtv->itemNew.lParam;
							char buf[255];
							_snprintf(buf,sizeof(buf),"%s=%s",data->szName, data->szValue);
							SendDlgItemMessageA(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)buf);
						}
						else
							SendDlgItemMessageA(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)"");

					}
					return 0;
				}

		case 0:
			switch (((LPNMHDR)lParam)->code)
			{
			case PSN_APPLY:
				{
					return TRUE;
				}
				break;
			}
			break;
		}
	}
	return FALSE;
}

int SkinEditorOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-1000000000;
	odp.hInstance=g_hInst;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_SKINEDITOR);
	odp.pszGroup=Translate("Customize");
	odp.pszTitle=Translate("Modify Skin");
	odp.pfnDlgProc=DlgSkinEditorOpts;
	odp.flags=ODPF_BOLDGROUPS;
	//	odp.nIDBottomSimpleControl=IDC_STCLISTGROUP;
	//	odp.expertOnlyControls=expertOnlyControls;
	//	odp.nExpertOnlyControls=sizeof(expertOnlyControls)/sizeof(expertOnlyControls[0]);
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}
