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
#include "commonprototypes.h"
#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

HWND hCLUIwnd=NULL;
extern HWND hwndContactList,hwndContactTree,hwndStatus;
LOGFONTA LoadLogFontFromDB(char * section, char * id, DWORD * color);
extern HMENU hMenuMain;
extern BOOL IsOnDesktop;
extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
static BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//extern hFrameHelperStatusBar;
extern void ReAssignExtraIcons();
extern int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
extern int UseOwnerDrawStatusBar;
//extern int OnStatusBarBackgroundChange();
extern BOOL SmoothAnimation;
extern int SetParentForContainers(HWND parent);
static UINT expertOnlyControls[]={IDC_BRINGTOFRONT, IDC_AUTOSIZE,IDC_STATIC21,IDC_MAXSIZEHEIGHT,IDC_MAXSIZESPIN,IDC_STATIC22,IDC_AUTOSIZEUPWARD,IDC_SHOWMAINMENU,IDC_SHOWCAPTION,IDC_CLIENTDRAG};
extern BYTE UseKeyColor;
extern DWORD KeyColor;
int CluiOptInit(WPARAM wParam,LPARAM lParam)
{
  OPTIONSDIALOGPAGE odp;

  ZeroMemory(&odp,sizeof(odp));
  odp.cbSize=sizeof(odp);
  odp.position=0;
  odp.hInstance=g_hInst;
  odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLUI);
  odp.pszTitle=Translate("Window");
  odp.pszGroup=Translate("Contact List");
  odp.pfnDlgProc=DlgProcCluiOpts;
  odp.flags=ODPF_BOLDGROUPS;
  odp.nIDBottomSimpleControl=IDC_STWINDOWGROUP;
  odp.expertOnlyControls=expertOnlyControls;
  odp.nExpertOnlyControls=sizeof(expertOnlyControls)/sizeof(expertOnlyControls[0]);
  CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
  odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_SBAR);
  odp.pszTitle=Translate("Status Bar");
  odp.pfnDlgProc=DlgProcSBarOpts;
  odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
  odp.nIDBottomSimpleControl=0;
  odp.nExpertOnlyControls=0;
  odp.expertOnlyControls=NULL;
  CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
  return 0;
}

static BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
	hCLUIwnd=hwndDlg;  
    TranslateDialogDefault(hwndDlg);
    CheckDlgButton(hwndDlg, IDC_BRINGTOFRONT, DBGetContactSettingByte(NULL,"CList","BringToFront",SETTING_BRINGTOFRONT_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    //CheckDlgButton(hwndDlg, IDC_TOOLWND, DBGetContactSettingByte(NULL,"CList","ToolWindow",SETTING_TOOLWINDOW_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    //CheckDlgButton(hwndDlg, IDC_MIN2TRAY, DBGetContactSettingByte(NULL,"CList","Min2Tray",SETTING_MIN2TRAY_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);

    //CheckDlgButton(hwndDlg, IDC_BORDER, DBGetContactSettingByte(NULL,"CList","ThinBorder",0) ? BST_CHECKED : BST_UNCHECKED);
    //CheckDlgButton(hwndDlg, IDC_NOBORDERWND, DBGetContactSettingByte(NULL,"CList","NoBorder",0) ? BST_CHECKED : BST_UNCHECKED);


    //if(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND)) EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
    //CheckDlgButton(hwndDlg, IDC_SHOWCAPTION, DBGetContactSettingByte(NULL,"CLUI","ShowCaption",SETTING_SHOWCAPTION_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    //CheckDlgButton(hwndDlg, IDC_SHOWMAINMENU, DBGetContactSettingByte(NULL,"CLUI","ShowMainMenu",SETTING_SHOWMAINMENU_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_CLIENTDRAG, DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",SETTING_CLIENTDRAG_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_DRAGTOSCROLL, DBGetContactSettingByte(NULL,"CLUI","DragToScroll",0) ? BST_CHECKED : BST_UNCHECKED);
    //EnableWindow(GetDlgItem(hwndDlg,IDC_CLIENTDRAG),!IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
    //if(!IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) {
    //	EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
    //	EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),FALSE);
    //	EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),FALSE);
    //}
    // if (IsDlgButtonChecked(hwndDlg,IDC_BORDER) || IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND))
    //{
    //            EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
    //EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),FALSE);
    //EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),FALSE);
    //            EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),FALSE);				
    //        }

    CheckDlgButton(hwndDlg, IDC_FADEINOUT, DBGetContactSettingByte(NULL,"CLUI","FadeInOut",1) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_AUTOSIZE, DBGetContactSettingByte(NULL,"CLUI","AutoSize",0) ? BST_CHECKED : BST_UNCHECKED);			
    CheckDlgButton(hwndDlg, IDC_LOCKSIZING, DBGetContactSettingByte(NULL,"CLUI","LockSize",0) ? BST_CHECKED : BST_UNCHECKED);			
    //CheckDlgButton(hwndDlg, IDC_DROPSHADOW, DBGetContactSettingByte(NULL,"CList","WindowShadow",0) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_ONDESKTOP, DBGetContactSettingByte(NULL,"CList","OnDesktop", 0) ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(hwndDlg,IDC_MAXSIZESPIN,UDM_SETRANGE,0,MAKELONG(100,0));

    SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_SETRANGE,0,MAKELONG(10,0));
    SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_SETRANGE,0,MAKELONG(10,0));
    SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_SETPOS,0,DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1));
    SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_SETPOS,0,DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1));

    SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
    SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
    SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
    SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));

    SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0));
    SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0));
    SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0));
    SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0));
    /*
    nRect.left+=DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0); //$$ Left BORDER SIZE
    nRect.right-=DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0); //$$ Left BORDER SIZE
    nRect.top+=DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0); //$$ TOP border
    nRect.bottom-=DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0); //$$ BOTTOM border
    */

    SendDlgItemMessage(hwndDlg,IDC_MAXSIZESPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",75));
    CheckDlgButton(hwndDlg, IDC_AUTOSIZEUPWARD, DBGetContactSettingByte(NULL,"CLUI","AutoSizeUpward",0) ? BST_CHECKED : BST_UNCHECKED);


    CheckDlgButton(hwndDlg, IDC_CHECKKEYCOLOR, DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1) ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
    SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));

    CheckDlgButton(hwndDlg, IDC_AUTOHIDE, DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETRANGE,0,MAKELONG(900,1));
    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),0));
    EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
    EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC01),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));

    {
      int i, item;
      char *hidemode[]={"Hide to tray", "Behind left edge", "Behind right edge"};
      for (i=0; i<sizeof(hidemode)/sizeof(char*); i++) {
        item=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_ADDSTRING,0,(LPARAM)Translate(hidemode[i]));
        SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_SETITEMDATA,item,(LPARAM)0);
        SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_SETCURSEL,DBGetContactSettingByte(NULL,"ModernData","HideBehind",0),0);
      }
    }


    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_SETRANGE,0,MAKELONG(600,0));
    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","ShowDelay",3),0));

    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_SETRANGE,0,MAKELONG(600,0));
    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","HideDelay",3),0));

    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_SETRANGE,0,MAKELONG(50,1));
    SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","HideBehindBorderSize",1),0));

    {
      int mode=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDELAY),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN2),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN3),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN4),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY2),mode!=0);
    }

    if(!IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE)) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC21),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC22),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZEHEIGHT),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZESPIN),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_AUTOSIZEUPWARD),FALSE);
    }


    {	DBVARIANT dbv;
    char *s;
    char szUin[20];

    if(!DBGetContactSetting(NULL,"CList","TitleText",&dbv))
      s=mir_strdup(dbv.pszVal);
    else
      s=mir_strdup(MIRANDANAME);

    dbv.pszVal=s;

    SetDlgItemTextA(hwndDlg,IDC_TITLETEXT,dbv.pszVal);
    if (dbv.pszVal) mir_free(dbv.pszVal);
    DBFreeVariant(&dbv);
    //if (s) mir_free(s);
    SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)MIRANDANAME);
    sprintf(szUin,"%u",DBGetContactSettingDword(NULL,"ICQ","UIN",0));
    SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)szUin);

    if(!DBGetContactSetting(NULL,"ICQ","Nick",&dbv)) {
      SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
      mir_free(dbv.pszVal);
      DBFreeVariant(&dbv);
      dbv.pszVal=NULL;
    }
    if(!DBGetContactSetting(NULL,"ICQ","FirstName",&dbv)) {
      SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
      mir_free(dbv.pszVal);
      DBFreeVariant(&dbv);
      dbv.pszVal=NULL;
    }
    if(!DBGetContactSetting(NULL,"ICQ","e-mail",&dbv)) {
      SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
      mir_free(dbv.pszVal);
      DBFreeVariant(&dbv);
      dbv.pszVal=NULL;
    }
    }
    if(!IsWinVer2000Plus()) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_FADEINOUT),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENT),FALSE);
      //EnableWindow(GetDlgItem(hwndDlg,IDC_DROPSHADOW),FALSE);
    }
    else CheckDlgButton(hwndDlg,IDC_TRANSPARENT,DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT)?BST_CHECKED:BST_UNCHECKED);
    if(!IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT)) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC11),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC12),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSACTIVE),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSINACTIVE),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_ACTIVEPERC),FALSE);
      EnableWindow(GetDlgItem(hwndDlg,IDC_INACTIVEPERC),FALSE);
    }
    SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_SETRANGE,FALSE,MAKELONG(1,255));
    SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_SETRANGE,FALSE,MAKELONG(1,255));
    SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_SETPOS,TRUE,DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT));
    SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_SETPOS,TRUE,DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT));
    SendMessage(hwndDlg,WM_HSCROLL,0x12345678,0);

    {//===EXTRA Icons
      CheckDlgButton(hwndDlg, IDC_EXTRA_PROTO, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_PROTO",1) ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hwndDlg, IDC_EXTRA_CLIENT, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_CLIENT",1) ? BST_CHECKED : BST_UNCHECKED);


      CheckDlgButton(hwndDlg, IDC_EXTRA_EMAIL, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_EMAIL",1) ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hwndDlg, IDC_EXTRA_CELLULAR, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_SMS",1) ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hwndDlg, IDC_EXTRA_ADV1, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_ADV1",1) ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hwndDlg, IDC_EXTRA_ADV2, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_ADV2",1) ? BST_CHECKED : BST_UNCHECKED);			
      CheckDlgButton(hwndDlg, IDC_EXTRA_WEB, DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_WEB",1) ? BST_CHECKED : BST_UNCHECKED);

    };


    return TRUE;

  case WM_COMMAND:
    if(LOWORD(wParam)==IDC_AUTOHIDE) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC01),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
    }
    else if(LOWORD(wParam)==IDC_DRAGTOSCROLL && IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG)) {
      CheckDlgButton(hwndDlg,IDC_CLIENTDRAG,FALSE);
    }
    else if(LOWORD(wParam)==IDC_CLIENTDRAG && IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL)) {
      CheckDlgButton(hwndDlg,IDC_DRAGTOSCROLL,FALSE);
    }
    else if(LOWORD(wParam)==IDC_TRANSPARENT) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC11),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC12),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
      EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSACTIVE),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
      EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSINACTIVE),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
      EnableWindow(GetDlgItem(hwndDlg,IDC_ACTIVEPERC),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
      EnableWindow(GetDlgItem(hwndDlg,IDC_INACTIVEPERC),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
    }

    else if(LOWORD(wParam)==IDC_ONDESKTOP && IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP)) {
      CheckDlgButton(hwndDlg, IDC_ONTOP, BST_UNCHECKED);    
    }
    else if(LOWORD(wParam)==IDC_ONTOP && IsDlgButtonChecked(hwndDlg,IDC_ONTOP)) {
      CheckDlgButton(hwndDlg, IDC_ONDESKTOP, BST_UNCHECKED);    
    }
    else if(LOWORD(wParam)==IDC_AUTOSIZE) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC21),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
      EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC22),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
      EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZEHEIGHT),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
      EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
      EnableWindow(GetDlgItem(hwndDlg,IDC_AUTOSIZEUPWARD),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
    }
    else if(LOWORD(wParam)==IDC_TOOLWND) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),!IsDlgButtonChecked(hwndDlg,IDC_TOOLWND));
    }
    else if(LOWORD(wParam)==IDC_SHOWCAPTION) {
      EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
      EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),!IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
      EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
    }
    else if(LOWORD(wParam)==IDC_NOBORDERWND ||LOWORD(wParam)==IDC_BORDER)
    {
      EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
      EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) && !(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
      EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));             
      if (LOWORD(wParam)==IDC_BORDER) CheckDlgButton(hwndDlg, IDC_NOBORDERWND,BST_UNCHECKED);
      else CheckDlgButton(hwndDlg, IDC_BORDER,BST_UNCHECKED); 

    }
    else if (LOWORD(wParam)==IDC_HIDEMETHOD)
    {
      int mode=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDELAY),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN2),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN3),mode!=0);
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN4),mode!=0);     
      EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY2),mode!=0);
    }

    else if (LOWORD(wParam)==IDC_CHECKKEYCOLOR) 
    {
      EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
      SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));
    }
    if ((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_HIDEDELAY2 ||LOWORD(wParam)==IDC_HIDEDELAY ||LOWORD(wParam)==IDC_SHOWDELAY ||LOWORD(wParam)==IDC_TITLETEXT || LOWORD(wParam)==IDC_MAXSIZEHEIGHT || LOWORD(wParam)==IDC_FRAMESGAP || LOWORD(wParam)==IDC_CAPTIONSGAP ||
      LOWORD(wParam)==IDC_LEFTMARGIN || LOWORD(wParam)==IDC_RIGHTMARGIN|| LOWORD(wParam)==IDC_TOPMARGIN || LOWORD(wParam)==IDC_BOTTOMMARGIN) &&
      (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
      return 0;

    // Enable apply button
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
    break;

  case WM_HSCROLL:
    {	char str[10];
    sprintf(str,"%d%%",100*SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_GETPOS,0,0)/255);
    SetDlgItemTextA(hwndDlg,IDC_INACTIVEPERC,str);
    sprintf(str,"%d%%",100*SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_GETPOS,0,0)/255);
    SetDlgItemTextA(hwndDlg,IDC_ACTIVEPERC,str);
    }
    if(wParam!=0x12345678) SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
    break;
  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
      //
      DBWriteContactSettingByte(NULL,"CLUI","LeftClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
      DBWriteContactSettingByte(NULL,"CLUI","RightClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));
      DBWriteContactSettingByte(NULL,"CLUI","TopClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_GETPOS,0,0));
      DBWriteContactSettingByte(NULL,"CLUI","BottomClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_GETPOS,0,0));




      DBWriteContactSettingByte(NULL,"ModernData","HideBehind",(BYTE)SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0));  
      DBWriteContactSettingWord(NULL,"ModernData","ShowDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_GETPOS,0,0));
      DBWriteContactSettingWord(NULL,"ModernData","HideDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_GETPOS,0,0));
      DBWriteContactSettingWord(NULL,"ModernData","HideBehindBorderSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_GETPOS,0,0));


      //      CheckDlgButton(hwndDlg, IDC_CHECKKEYCOLOR, DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1) ? BST_CHECKED : BST_UNCHECKED);
      //    EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
      //    SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));

      DBWriteContactSettingByte(NULL,"ModernSettings","UseKeyColor",IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR)?1:0);
      DBWriteContactSettingDword(NULL,"ModernSettings","KeyColor",SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_GETCOLOUR,0,0));

      UseKeyColor=DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1);
      KeyColor=DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255));

      DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));
      DBWriteContactSettingByte(NULL,"CList","OnTop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONTOP));
      SetWindowPos(hwndContactList, IsDlgButtonChecked(hwndDlg,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
      DBWriteContactSettingByte(NULL,"CLUI","DragToScroll",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
      //DBWriteContactSettingByte(NULL,"CList","ToolWindow",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_TOOLWND));
      DBWriteContactSettingByte(NULL,"CList","BringToFront",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BRINGTOFRONT));
      ////if(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND)) {
      //	// Window must be hidden to dynamically remove the taskbar button.
      //	// See http://msdn.microsoft.com/library/en-us/shellcc/platform/shell/programmersguide/shell_int/shell_int_programming/taskbar.asp
      //	WINDOWPLACEMENT p;
      //	p.length = sizeof(p);
      //	GetWindowPlacement(hwndContactList,&p);
      //	ShowWindow(hwndContactList,SW_HIDE);
      //	SetWindowLong(hwndContactList,GWL_EXSTYLE,GetWindowLong(hwndContactList,GWL_EXSTYLE)|WS_EX_TOOLWINDOW/*|WS_EX_WINDOWEDGE*/);
      //	SetWindowPlacement(hwndContactList,&p);
      //}
      //else
      //	SetWindowLong(hwndContactList,GWL_EXSTYLE,GetWindowLong(hwndContactList,GWL_EXSTYLE)&~WS_EX_TOOLWINDOW);

      if (IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP)) 
      {
        //SetWindowPos(hwndContactList,HWND_BOTTOM,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
        HWND hProgMan=FindWindow(TEXT("Progman"),NULL);
        if (IsWindow(hProgMan)) 
        {
          SetParent(hwndContactList,hProgMan);
          SetParentForContainers(hProgMan);
          IsOnDesktop=1;
        }
      } 
      else 
      {
        SetParent(hwndContactList,NULL);
        SetParentForContainers(NULL);
        IsOnDesktop=0;
      }


      //DBWriteContactSettingByte(NULL,"CLUI","ShowCaption",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
      //DBWriteContactSettingByte(NULL,"CLUI","ShowMainMenu",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWMAINMENU));
      DBWriteContactSettingByte(NULL,"CLUI","ClientAreaDrag",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG));


      //if(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))
      //	SetWindowLong(hwndContactList,GWL_STYLE,GetWindowLong(hwndContactList,GWL_STYLE)|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
      //else
      //	SetWindowLong(hwndContactList,GWL_STYLE,GetWindowLong(hwndContactList,GWL_STYLE)&~(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX));

      //DBWriteContactSettingByte(NULL,"CList","ThinBorder",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BORDER));
      //DBWriteContactSettingByte(NULL,"CList","NoBorder",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND));

      //if (1||(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BORDER) || (BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND))
      //{
      //DWORD style=0;
      //DWORD styleEx=0;
      //if (DBGetContactSettingByte(NULL,"CList","ThinBorder",0) || (DBGetContactSettingByte(NULL,"CList","NoBorder",0)))
      //{
      //    style=WS_CLIPCHILDREN|(DBGetContactSettingByte(NULL,"CList","ThinBorder",0)?WS_BORDER:0);
      //    styleEx=WS_EX_TOOLWINDOW;
      //} 
      //else if (DBGetContactSettingByte(NULL,"CLUI","ShowCaption",SETTING_SHOWCAPTION_DEFAULT) && DBGetContactSettingByte(NULL,"CList","ToolWindow",SETTING_TOOLWINDOW_DEFAULT))
      //{
      //    styleEx=WS_EX_TOOLWINDOW/*|WS_EX_WINDOWEDGE*/;
      //    style=WS_CAPTION|WS_MINIMIZEBOX|WS_POPUPWINDOW|WS_CLIPCHILDREN|WS_THICKFRAME;
      //    
      //}
      //else if (DBGetContactSettingByte(NULL,"CLUI","ShowCaption",SETTING_SHOWCAPTION_DEFAULT))
      //{
      //    style=WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_POPUPWINDOW|WS_CLIPCHILDREN|WS_THICKFRAME;
      //}
      //else 
      //{
      //    style=WS_POPUPWINDOW|WS_CLIPCHILDREN|WS_THICKFRAME;
      //    styleEx=WS_EX_TOOLWINDOW/*|WS_EX_WINDOWEDGE*/;
      //}
      //    ShowWindow(hwndContactList,SW_HIDE);
      //    SetWindowLong(hwndContactList,GWL_EXSTYLE,styleEx);
      //    SetWindowLong(hwndContactList,GWL_STYLE,style);
      //    ShowWindow(hwndContactList,SW_SHOW); 
      //}

      //               if(!IsDlgButtonChecked(hwndDlg,IDC_SHOWMAINMENU)) SetMenu(hwndContactList,NULL);
      //else SetMenu(hwndContactList,hMenuMain);

      //			SetWindowPos(hwndContactList,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
      //			RedrawWindow(hwndContactList,NULL,NULL,RDW_FRAME|RDW_INVALIDATE);

      //			DBWriteContactSettingByte(NULL,"CList","Min2Tray",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_MIN2TRAY));
      //			if(IsIconic(hwndContactList) && !IsDlgButtonChecked(hwndDlg,IDC_TOOLWND))
      //				ShowWindow(hwndContactList,IsDlgButtonChecked(hwndDlg,IDC_MIN2TRAY)?SW_HIDE:SW_SHOW);

      //{	char title[256];
      //	GetDlgItemText(hwndDlg,IDC_TITLETEXT,title,sizeof(title));
      //	DBWriteContactSettingString(NULL,"CList","TitleText",title);
      //	SetWindowText(hwndContactList,title);
      //}
      DBWriteContactSettingByte(NULL,"CLUI","FadeInOut",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FADEINOUT));
      SmoothAnimation=(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FADEINOUT);

      DBWriteContactSettingByte(NULL,"CLUI","AutoSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
      DBWriteContactSettingByte(NULL,"CLUI","LockSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_LOCKSIZING));
      DBWriteContactSettingByte(NULL,"CLUI","MaxSizeHeight",(BYTE)GetDlgItemInt(hwndDlg,IDC_MAXSIZEHEIGHT,NULL,FALSE));               
      {
        int i1;
        int i2;
        i1=SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_GETPOS,0,0);
        i2=SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_GETPOS,0,0);   

        DBWriteContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",(DWORD)i1);
        DBWriteContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",(DWORD)i2);
        CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
      }
      DBWriteContactSettingByte(NULL,"CLUI","AutoSizeUpward",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZEUPWARD));
      DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
      DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));

      DBWriteContactSettingByte(NULL,"CList","Transparent",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
      DBWriteContactSettingByte(NULL,"CList","Alpha",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_GETPOS,0,0));
      DBWriteContactSettingByte(NULL,"CList","AutoAlpha",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_GETPOS,0,0));
      //DBWriteContactSettingByte(NULL,"CList","WindowShadow",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DROPSHADOW));
      DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));



      //if(IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT))	{
      //	SetWindowLong(hwndContactList, GWL_EXSTYLE, GetWindowLong(hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);
      //	if(MySetLayeredWindowAttributes) MySetLayeredWindowAttributes(hwndContactList, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT), LWA_ALPHA);
      //}
      //else {
      //	SetWindowLong(hwndContactList, GWL_EXSTYLE, GetWindowLong(hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
      //}
      SendMessage(hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
      {
        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_PROTO",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_PROTO));
        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_CLIENT",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_CLIENT));						

        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_EMAIL",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_EMAIL));
        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_SMS",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_CELLULAR));
        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_ADV1",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_ADV1));
        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_ADV2",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_ADV2));
        DBWriteContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_WEB",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EXTRA_WEB));
        //SetAllExtraIcons()	
        ReAssignExtraIcons();

      };			
      ReloadCLUIOptions();
      ShowHide(0,1);

      return TRUE;
    }
    break;
  }
  return FALSE;
}

LOGFONTA lf;

static BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    TranslateDialogDefault(hwndDlg);
    CheckDlgButton(hwndDlg, IDC_SHOWSBAR, DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_USECONNECTINGICON, DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1) ? BST_CHECKED : BST_UNCHECKED);
    {
      BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);
      CheckDlgButton(hwndDlg, IDC_SHOWICON, showOpts&1 ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hwndDlg, IDC_SHOWPROTO, showOpts&2 ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hwndDlg, IDC_SHOWSTATUS, showOpts&4 ? BST_CHECKED : BST_UNCHECKED);
    }
    CheckDlgButton(hwndDlg, IDC_RIGHTSTATUS, DBGetContactSettingByte(NULL,"CLUI","SBarRightClk",0) ? BST_UNCHECKED : BST_CHECKED);
    CheckDlgButton(hwndDlg, IDC_RIGHTMIRANDA, !IsDlgButtonChecked(hwndDlg,IDC_RIGHTSTATUS) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_EQUALSECTIONS, DBGetContactSettingByte(NULL,"CLUI","EqualSections",0) ? BST_CHECKED : BST_UNCHECKED);
        
    SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_SETRANGE,0,MAKELONG(50,0));
    SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","LeftOffset",0),0));

    SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_SETRANGE,0,MAKELONG(50,0));
    SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","RightOffset",0),0));

    SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_SETRANGE,0,MAKELONG(50,0));
    SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","SpaceBetween",0),2));

    {
      int i, item;
      char *align[]={"Left", "Center", "Right"};
      for (i=0; i<sizeof(align)/sizeof(char*); i++) {
        item=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)Translate(align[i]));
        SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CLUI","Align",0),0);
      }
    }
    {
      DWORD color;
      lf=LoadLogFontFromDB("ModernData","StatusBar", &color);
      SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETCOLOUR,0,color);
    }
    {
      int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWICON),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWPROTO),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWSTATUS),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTSTATUS),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTMIRANDA),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_EQUALSECTIONS),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_USECONNECTINGICON),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_USEOWNERDRAW),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN2),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON2),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN3),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON3),en);
	  EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO2),en);
	  EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),en);
    }
    return TRUE;
  case WM_COMMAND:
    if(LOWORD(wParam)==IDC_BUTTON1)  
    { 
      if (HIWORD(wParam)==BN_CLICKED)
      {
          CHOOSEFONTA fnt;
          memset(&fnt,0,sizeof(CHOOSEFONTA));
          fnt.lStructSize=sizeof(CHOOSEFONTA);
          fnt.hwndOwner=hwndDlg;
          fnt.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT;
          fnt.lpLogFont=&lf;
          ChooseFontA(&fnt);
		      SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          return 0;
      } 
    }
	else if(LOWORD(wParam)==IDC_COLOUR ||(LOWORD(wParam)==IDC_COMBO2 && HIWORD(wParam)==CBN_SELCHANGE)) SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
    else if(LOWORD(wParam)==IDC_SHOWSBAR) {
      int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWICON),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWPROTO),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWSTATUS),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTSTATUS),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTMIRANDA),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_EQUALSECTIONS),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_USECONNECTINGICON),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_USEOWNERDRAW),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN2),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON2),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN3),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON3),en);
      EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO2),en);
	  EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),en);
	  EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1),en);
      SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	  
    }
    else if ((LOWORD(wParam)==IDC_OFFSETICON||LOWORD(wParam)==IDC_OFFSETICON2||LOWORD(wParam)==IDC_OFFSETICON3) && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap 
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
    break;
  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
      {
        int frameopt;

        DBWriteContactSettingByte(NULL,"CLUI","ShowSBar",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR));
        DBWriteContactSettingByte(NULL,"CLUI","SBarShow",(BYTE)((IsDlgButtonChecked(hwndDlg,IDC_SHOWICON)?1:0)|(IsDlgButtonChecked(hwndDlg,IDC_SHOWPROTO)?2:0)|(IsDlgButtonChecked(hwndDlg,IDC_SHOWSTATUS)?4:0)));
        DBWriteContactSettingByte(NULL,"CLUI","SBarRightClk",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_RIGHTMIRANDA));
        DBWriteContactSettingByte(NULL,"CLUI","EqualSections",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EQUALSECTIONS));
        DBWriteContactSettingByte(NULL,"CLUI","UseConnectingIcon",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USECONNECTINGICON));
        DBWriteContactSettingDword(NULL,"CLUI","LeftOffset",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_GETPOS,0,0));
        DBWriteContactSettingDword(NULL,"CLUI","RightOffset",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_GETPOS,0,0));
        DBWriteContactSettingDword(NULL,"CLUI","SpaceBetween",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_GETPOS,0,0));
        DBWriteContactSettingByte(NULL,"CLUI","Align",(BYTE)SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0));

        {
          //store lf to db.
          BYTE style=0;
		  long sz;
		  if(lf.lfHeight<0) 
		  {
        int a;
			  HFONT hFont=CreateFontIndirectA(&lf);
			  HDC hdc=GetDC(NULL);
        a=-MulDiv(lf.lfHeight,72,GetDeviceCaps(hdc, LOGPIXELSY));
			  ReleaseDC(NULL,hdc);
			  sz=a;
		  }
		  else sz=lf.lfHeight;

		  style|=lf.lfWeight==FW_BOLD?DBFONTF_BOLD:0;
		  style|=lf.lfItalic?DBFONTF_ITALIC:0;
		  style|=lf.lfUnderline?DBFONTF_UNDERLINE:0;
		  DBWriteContactSettingByte(NULL,"ModernData","StatusBarFontSty",style);
		  DBWriteContactSettingByte(NULL,"ModernData","StatusBarFontSet",lf.lfCharSet);
		  DBWriteContactSettingByte(NULL,"ModernData","StatusBarFontSize",(BYTE)sz);
		  DBWriteContactSettingDword(NULL,"ModernData","StatusBarFontCol",(DWORD)SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
		  DBWriteContactSettingString(NULL,"ModernData","StatusBarFontName",lf.lfFaceName);              
        }

        DBWriteContactSettingDword(NULL,"ModernData","StatusBarFontCol",SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
    

        if(IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR)) ShowWindow(hwndStatus,SW_SHOW);
        else ShowWindow(hwndStatus,SW_HIDE);
        frameopt=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,hwndStatus),0);
        frameopt=frameopt & (~F_VISIBLE);		
        if(IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR)) 
        {
          ShowWindow(hwndStatus,SW_SHOW);
          frameopt|=F_VISIBLE;
        }
        else 
        {
          ShowWindow(hwndStatus,SW_HIDE);
        };
        DBWriteContactSettingByte(NULL,"CLUI","ShowSBar",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR));
        CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,hwndStatus),frameopt);
        SendMessage(hwndContactList,WM_SIZE,0,0);
        LoadStatusBarData();
        CluiProtocolStatusChanged(0,0);	
        return TRUE;
      }
    }
    break;
  }
  return FALSE;
}