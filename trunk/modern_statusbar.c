#include "commonheaders.h"
#include "modern_statusbar.h"
#include "m_skin_eng.h"

HANDLE  hStatusBarShowToolTipEvent;
HANDLE  hStatusBarHideToolTipEvent;
BOOL tooltipshoing;
POINT lastpnt;

#define TM_STATUSBAR 23435234
#define TM_STATUSBARHIDE 23435235

extern int BehindEdgeSettings;
HWND hModernStatusBar=NULL;
HANDLE hFramehModernStatusBar=NULL;



#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

typedef struct
{
  BOOL sameWidth;
  RECT rectBorders;
  BYTE extraspace;
  BYTE Align;
  BYTE showProtoName;
  BYTE showStatusName;
  HFONT BarFont;
  DWORD fontColor;
  BYTE connectingIcon;
  BYTE TextEffectID;
  DWORD TextEffectColor1;
  DWORD TextEffectColor2;
  BYTE xStatusMode;     // 0-only main, 1-xStatus, 2-main as overlay
}StatusBarData;

typedef struct
{
  HICON icon;
  HICON extraIcon;
  int iconIndex;
  char * ProtoName;
  int ProtoStatus;
  char * ProtoStatusText;
  TCHAR * ProtoXStatus;
  int ProtoPos;
  int fullWidth;
  RECT protoRect;
  BOOL DoubleIcons;

} ProtoItemData;

ProtoItemData *ProtosData=NULL;
int allocedItemData=0;
StatusBarData sbdat={0};


char * ApendSubSetting(char * buf, int size, char *first, char *second)
{
  _snprintf(buf,size,"%sFont%s",first,second);
  return buf;
}

//LOGFONTA LoadLogFontFromDB(char * section, char * id, DWORD * color);

/*HFONT LoadFontFromDB(char * section, char * id, DWORD * color)
{
  LOGFONTA logfont=LoadLogFontFromDB(section, id, color);
  HFONT hfont=NULL;
  hfont=CreateFontIndirectA(&logfont);
  return hfont;
}

LOGFONTA LoadLogFontFromDB(char * section, char * id, DWORD * color)
{
  char * facename;
  char buf[255];
  LOGFONTA logfont;
  facename=DBGetStringA(NULL,section,ApendSubSetting(buf,sizeof(buf),id,"Name"));
  if (!facename) facename=mir_strdup("MS Shell Dlg");
  {

    BYTE style;
	BYTE len=min(MyStrLen(facename)+1,sizeof(logfont.lfFaceName));
    memset(&logfont,0,sizeof(logfont));

    memmove(logfont.lfFaceName,facename,len);
    style=(BYTE)DBGetContactSettingByte(NULL,section,ApendSubSetting(buf,sizeof(buf),id,"Sty"),0);
    logfont.lfWeight=style&DBFONTF_BOLD?FW_BOLD:FW_NORMAL;
    logfont.lfItalic=(style&DBFONTF_ITALIC)!=0;
    logfont.lfUnderline=(style&DBFONTF_UNDERLINE)!=0;
    logfont.lfCharSet=DBGetContactSettingByte(NULL,section,ApendSubSetting(buf,sizeof(buf),id,"Set"),DEFAULT_CHARSET);
    logfont.lfOutPrecision=OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
    logfont.lfQuality=DEFAULT_QUALITY;
    logfont.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
    {
      HDC hdc=CreateCompatibleDC(NULL);
      int h=(int)(BYTE)DBGetContactSettingByte(NULL,section,ApendSubSetting(buf,sizeof(buf),id,"Size"),10);
      logfont.lfHeight=(long)-MulDiv(h, GetDeviceCaps(hdc, LOGPIXELSY), 72);
      ModernDeleteDC(hdc);
    }
    *color=DBGetContactSettingDword(NULL,section,ApendSubSetting(buf,sizeof(buf),id,"Col"),0);
    mir_free(facename);
  }
  return logfont;
}
*/

extern int FindFrameID(HWND FrameHwnd);

int LoadStatusBarData()
{
  sbdat.showStatusName=DBGetContactSettingByte(NULL,"CLUI","SBarShow",7)&4;
  sbdat.showProtoName=DBGetContactSettingByte(NULL,"CLUI","SBarShow",7)&2;
  sbdat.rectBorders.left=DBGetContactSettingDword(NULL,"CLUI","LeftOffset",0);
  sbdat.rectBorders.right=DBGetContactSettingDword(NULL,"CLUI","RightOffset",0);
  sbdat.extraspace=(BYTE)DBGetContactSettingDword(NULL,"CLUI","SpaceBetween",0);
  sbdat.xStatusMode=(BYTE)(DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",6));
  sbdat.Align=DBGetContactSettingByte(NULL,"CLUI","Align",0);
  sbdat.sameWidth=DBGetContactSettingByte(NULL,"CLUI","EqualSections",0);
  sbdat.connectingIcon=DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1);
  if (sbdat.BarFont) DeleteObject(sbdat.BarFont);
  sbdat.BarFont=NULL;//LoadFontFromDB("ModernData","StatusBar",&sbdat.fontColor);
  {
    int vis=DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1);
    int frameopt;
    int frameID=FindFrameID(hModernStatusBar);
    frameopt=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,frameID),0);
    frameopt=frameopt & (~F_VISIBLE);
    if(vis)
    {
      ShowWindow(hModernStatusBar,SW_SHOW);
      frameopt|=F_VISIBLE;
    }
    else
    {
      ShowWindow(hModernStatusBar,SW_HIDE);
    };
    CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,frameID),frameopt);
  }
  sbdat.TextEffectID=DBGetContactSettingByte(NULL,"StatusBar","TextEffectID",0xFF);
  sbdat.TextEffectColor1=DBGetContactSettingDword(NULL,"StatusBar","TextEffectColor1",0);
  sbdat.TextEffectColor2=DBGetContactSettingDword(NULL,"StatusBar","TextEffectColor2",0);
  SendMessage(pcli->hwndContactList,WM_SIZE,0,0);
  return 1;
}


//ProtocolData;
int NewStatusPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData)
{
  return ModernDrawStatusBar(hWnd,hDC);
}
extern HFONT TitleBarFont;
int ModernDrawStatusBar(HWND hwnd, HDC hDC)
{
  if (hwnd==(HWND)-1) return 0;
  if (GetParent(hwnd)==pcli->hwndContactList)
    return ModernDrawStatusBarWorker(hwnd,hDC);
  else
    cliInvalidateRect(hwnd,NULL,FALSE);
  return 0;
}
extern HICON GetMainStatusOverlay(int STATUS);
extern HFONT ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight);
int ModernDrawStatusBarWorker(HWND hWnd, HDC hDC)
{
  RECT rc;
  HFONT hOldFont;
  int iconY, textY;
  int spaceWidth;
  int maxwidth=0;
  int xstatus=0;
  GetClientRect(hWnd,&rc);
  SkinDrawGlyph(hDC,&rc,&rc,"Main,ID=StatusBar"); //TBD
  hOldFont=ChangeToFont(hDC,NULL,FONTID_STATUSBAR_PROTONAME,NULL);
//  hOldFont=SelectObject(hDC,sbdat.BarFont);
//  SetTextColor(hDC,sbdat.fontColor);
  {
	  SIZE textSize={0};
    GetTextExtentPoint32A(hDC," ",1,&textSize);
    spaceWidth=textSize.cx;
    textY=rc.top+((rc.bottom-rc.top-textSize.cy)>>1);
  }
  iconY=rc.top+((rc.bottom-rc.top-GetSystemMetrics(SM_CXSMICON))>>1);

  {
    int visProtoCount=0;
    int protoCount;
    int SumWidth=0;
    int rectwidth=0;
    int aligndx=0;
    int * ProtoWidth=NULL;
    int i,j,po=0;
	int protcnt=0;
    // Count visible protos
    PROTOCOLDESCRIPTOR **proto;
    CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
    if (allocedItemData && ProtosData)
    {
      int k;

      for (k=0; k<allocedItemData; k++)
      {
		if(ProtosData[k].ProtoXStatus) mir_free (ProtosData[k].ProtoXStatus);
        if(ProtosData[k].ProtoName) mir_free (ProtosData[k].ProtoName);
        if(ProtosData[k].ProtoStatusText) mir_free (ProtosData[k].ProtoStatusText);
      }
      mir_free(ProtosData);
      ProtosData=NULL;
      allocedItemData=0;
    }
    ProtosData=mir_alloc(sizeof(ProtoItemData)*protoCount);
    memset(ProtosData,0,sizeof(ProtoItemData)*protoCount);
    protcnt=(int)DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);

	for (j=0; j<protcnt; j++)
	{
		int vis;
		i=GetProtoIndexByPos(proto,protoCount,j);
		if (i==-1) 
			vis=FALSE;
		else
			vis=GetProtocolVisibility(proto[i]->szName);
		if (!vis) continue;
	    ProtosData[visProtoCount].ProtoName=mir_strdup(proto[i]->szName);
        ProtosData[visProtoCount].ProtoStatus=CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0);
		ProtosData[visProtoCount].ProtoStatusText=mir_strdup((char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,(WPARAM)ProtosData[visProtoCount].ProtoStatus,0));
	    ProtosData[visProtoCount].ProtoPos=visProtoCount;
	    visProtoCount++;
        allocedItemData++;
    }
    //sort protocols
    //if (visProtoCount>1)
    //{
    //  ProtoItemData b;
    //  int k;
    //  for (k=1; k<visProtoCount; k++)
    //    if (ProtosData[k].ProtoPos<ProtosData[k-1].ProtoPos)
    //    {
    //      b=ProtosData[k];
    //      ProtosData[k]=ProtosData[k-1];
    //      ProtosData[k-1]=b;
    //      k--;
    //    }
    //}


    // Calculate rects
    {
	  DWORD sw;
	  rectwidth=rc.right-rc.left-sbdat.rectBorders.left-sbdat.rectBorders.right;
	  if (visProtoCount>1) sw=(rectwidth-(sbdat.extraspace*(visProtoCount-1)))/visProtoCount;
	  else sw=rectwidth;
      if (ProtoWidth) mir_free(ProtoWidth);
      ProtoWidth=mir_alloc(sizeof(DWORD)*visProtoCount);
      for (i=0; i<visProtoCount; i++)
      {
        SIZE textSize;
        DWORD w=GetSystemMetrics(SM_CXSMICON)+1;
		ProtosData[i].extraIcon=NULL;
		if ((sbdat.xStatusMode&8) && ProtosData[i].ProtoStatus>ID_STATUS_OFFLINE)	
		{
				char str[MAXMODULELABELLENGTH];
				strcpy(str,ProtosData[i].ProtoName);
				strcat(str,"/GetXStatus");
				if (ServiceExists(str))
				{
					char * dbTitle="XStatusName";
					char * dbTitle2=NULL;
					xstatus=CallProtoService(ProtosData[i].ProtoName,"/GetXStatus",(WPARAM)&dbTitle,(LPARAM)&dbTitle2);
					if (dbTitle && xstatus)
					{
						DBVARIANT dbv={0};
						if (!DBGetContactSettingTString(NULL,ProtosData[i].ProtoName,dbTitle,&dbv))
						{
							ProtosData[i].ProtoXStatus=mir_tstrdup(dbv.ptszVal);
							//mir_free(dbv.ptszVal);
							DBFreeVariant(&dbv);
						}
					}
				}
		}
		if ((sbdat.xStatusMode&3))
		{
			if (ProtosData[i].ProtoStatus>ID_STATUS_OFFLINE)
			{
				char str[MAXMODULELABELLENGTH];
				strcpy(str,ProtosData[i].ProtoName);
				strcat(str,"/GetXStatusIcon");
				if (ServiceExists(str))
					ProtosData[i].extraIcon=(HICON)CallService(str,0,0);
				if (ProtosData[i].extraIcon && (sbdat.xStatusMode&3)==3)
					w+=GetSystemMetrics(SM_CXSMICON)+1;

			}
		}
		if (sbdat.showProtoName)
		{
			GetTextExtentPoint32A(hDC,ProtosData[i].ProtoName,lstrlenA(ProtosData[i].ProtoName),&textSize);
			w+=textSize.cx+3+spaceWidth;
		}
		if (sbdat.showStatusName)
		{
			GetTextExtentPoint32A(hDC,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText),&textSize);
			w+=textSize.cx+spaceWidth+3;
			
		}
		if ((sbdat.xStatusMode&8) && ProtosData[i].ProtoXStatus)
		{
			GetTextExtentPoint32(hDC,ProtosData[i].ProtoXStatus,lstrlen(ProtosData[i].ProtoXStatus),&textSize);
			w+=textSize.cx+spaceWidth+3;
		}
		ProtosData[i].fullWidth=w;
		if (sbdat.sameWidth)
		{
			ProtoWidth[i]=sw;
			SumWidth+=w;
		}
		else
		{
			ProtoWidth[i]=w;
			SumWidth+=w;
		}
      }
      // Reposition rects
      for(i=0; i<visProtoCount; i++)
        if (ProtoWidth[i]>maxwidth) maxwidth=ProtoWidth[i];

      if (sbdat.sameWidth)
      {
        for (i=0; i<visProtoCount; i++)
          ProtoWidth[i]=maxwidth;
        SumWidth=maxwidth*visProtoCount;
      }
      SumWidth+=(visProtoCount-1)*(sbdat.extraspace+1);


      if (SumWidth>rectwidth)
      {
        float f=(float)rectwidth/SumWidth;
        //dx=(int)(0.5+(float)dx/visProtoCount);
        //SumWidth-=dx*visProtoCount;
		SumWidth=0;
		for (i=0; i<visProtoCount; i++)
		{
          ProtoWidth[i]=(int)((float)ProtoWidth[i]*f);
		      SumWidth+=ProtoWidth[i];
		}
    SumWidth+=(visProtoCount-1)*(sbdat.extraspace+1);
      }
    }
    if (sbdat.Align==1) //center
      aligndx=(rectwidth-SumWidth)>>1;
    else if (sbdat.Align==2) //right
      aligndx=(rectwidth-SumWidth);
    // Draw in rects
  	//SelectEffect(hDC,sbdat.TextEffectID,sbdat.TextEffectColor1,sbdat.TextEffectColor2);
    {
      RECT r=rc;
      r.top+=sbdat.rectBorders.top;
      r.bottom-=sbdat.rectBorders.bottom;
      r.left+=sbdat.rectBorders.left+aligndx;
      for (i=0; i< visProtoCount; i++)
      {

        HRGN rgn;
        int x=r.left;
        HICON hIcon=NULL;
		HICON hxIcon=NULL;
        BOOL NeedDestroy=FALSE;
		if (ProtosData[i].ProtoStatus>ID_STATUS_OFFLINE && ((sbdat.xStatusMode)&3)>0)
		{
			char str[MAXMODULELABELLENGTH];
			strcpy(str,ProtosData[i].ProtoName);
			strcat(str,"/GetXStatusIcon");
			if (ServiceExists(str))
			{
				hxIcon=ProtosData[i].extraIcon;
				if (hxIcon)
				{
					if (((sbdat.xStatusMode)&3)==2)
					{
						hIcon=GetMainStatusOverlay(ProtosData[i].ProtoStatus);
					}
					else if (((sbdat.xStatusMode)&3)==1)
					{
						hIcon=hxIcon;
						NeedDestroy=TRUE;
						hxIcon=NULL;
					}			
			
				}
			}
		}
		if (hIcon==NULL && (hxIcon==NULL || (((sbdat.xStatusMode)&3)==3)))
		{
			if (hIcon==NULL && (sbdat.connectingIcon==1) && ProtosData[i].ProtoStatus>=ID_STATUS_CONNECTING&&ProtosData[i].ProtoStatus<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
			{
				hIcon=(HICON)GetConnectingIconService((WPARAM)ProtosData[i].ProtoName,0);
				if (hIcon) NeedDestroy=TRUE;
				else LoadSkinnedProtoIcon(ProtosData[i].ProtoName,ProtosData[i].ProtoStatus);
			}
			else hIcon=LoadSkinnedProtoIcon(ProtosData[i].ProtoName,ProtosData[i].ProtoStatus);
		}
        
		r.right=r.left+ProtoWidth[i];
        rgn=CreateRectRgn(r.left,r.top,r.right,r.bottom);
		//
		{
			if (sbdat.sameWidth)
			{
				int fw=ProtosData[i].fullWidth;
				int rw=r.right-r.left;
				if (sbdat.Align==1)
				{
					x=r.left+((rw-fw)/2);
				}
				else if (sbdat.Align==2)
				{
					x=r.left+((rw-fw));
				}
				else x=r.left;
			}
		}
		//
        ProtosData[i].protoRect=r;
        SelectClipRgn(hDC,rgn);
		ProtosData[i].DoubleIcons=FALSE;
		if ((sbdat.xStatusMode&3)==3)
		{
			if (hIcon) mod_DrawIconEx_helper(hDC,x,iconY,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
			if (hxIcon) 
			{
				mod_DrawIconEx_helper(hDC,x+GetSystemMetrics(SM_CXSMICON)+1,iconY,hxIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
				x+=GetSystemMetrics(SM_CXSMICON)+1;
			}
			ProtosData[i].DoubleIcons=hIcon&&hxIcon;
		}
		else
		{
			if (hxIcon) mod_DrawIconEx_helper(hDC,x,iconY,hxIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
			if (hIcon) mod_DrawIconEx_helper(hDC,x,iconY,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL|((hxIcon&&(sbdat.xStatusMode&4))?(192<<24):0));
		}
		if (hxIcon) DestroyIcon(hxIcon);
        if (NeedDestroy) DestroyIcon(hIcon);
        x+=GetSystemMetrics(SM_CXSMICON)+1;
        if (sbdat.showProtoName)
        {
          SIZE textSize;
		  RECT rt=r;
		  rt.left=x+(spaceWidth>>1);
		  rt.top=textY;
		  mod_DrawTextA(hDC,ProtosData[i].ProtoName,lstrlenA(ProtosData[i].ProtoName),&rt,0);
          //TextOutS(hDC,x,textY,ProtosData[i].ProtoName,lstrlenA(ProtosData[i].ProtoName));
          if (sbdat.showStatusName || ((sbdat.xStatusMode&8) && ProtosData[i].ProtoXStatus))
          {
            GetTextExtentPoint32A(hDC,ProtosData[i].ProtoName,lstrlenA(ProtosData[i].ProtoName),&textSize);
            x+=textSize.cx+3;
          }
		}
		if (sbdat.showStatusName)
		{
			SIZE textSize;
			RECT rt=r;
			rt.left=x+(spaceWidth>>1);
			rt.top=textY;
			mod_DrawTextA(hDC,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText),&rt,0);
			if ((sbdat.xStatusMode&8) && ProtosData[i].ProtoXStatus)
			{
				GetTextExtentPoint32A(hDC,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText),&textSize);
				x+=textSize.cx+3;
			}
			//TextOutS(hDC,x,textY,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText));
		}
		if ((sbdat.xStatusMode&8) && ProtosData[i].ProtoXStatus)
		{
			RECT rt=r;
			rt.left=x+(spaceWidth>>1);
			rt.top=textY;
			mod_DrawText(hDC,ProtosData[i].ProtoXStatus,lstrlen(ProtosData[i].ProtoXStatus),&rt,0);
			//TextOutS(hDC,x,textY,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText));
		}

        r.left=r.right+sbdat.extraspace;
        //SelectClipRgn(hDC,NULL);
        DeleteObject(rgn);

      }
    }
    if (ProtoWidth) mir_free(ProtoWidth);
  }

  SelectObject(hDC,hOldFont);

  return 0;
}

extern TMO_IntMenuItem * GetMenuItemByGlobalID(int globalMenuID);
extern MenuProto * menusProto;
extern int AllocedProtos;
LRESULT CALLBACK ModernStatusProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  switch (msg)
  {
  case WM_DESTROY:
    if (allocedItemData && ProtosData)
    {
      int k;

      for (k=0; k<allocedItemData; k++)
      {
        if(ProtosData[k].ProtoName) mir_free (ProtosData[k].ProtoName);
        if(ProtosData[k].ProtoStatusText) mir_free (ProtosData[k].ProtoStatusText);
      }
      mir_free(ProtosData);
      ProtosData=NULL;
      allocedItemData=0;
    }
    break;
  case WM_SIZE:
	  if (!LayeredFlag)
		  InvalidateRect(hwnd,NULL,FALSE);
	  return DefWindowProc(hwnd, msg, wParam, lParam);
  case WM_ERASEBKGND:
	  return 0;
  case WM_PAINT:
    if (GetParent(hwnd)==pcli->hwndContactList && LayeredFlag)
      InvalidateFrameImage((WPARAM)hwnd,0);
    else if (GetParent(hwnd)==pcli->hwndContactList && !LayeredFlag)
	{
		HDC hdc, hdc2;
		HBITMAP hbmp,hbmpo;
		RECT rc={0};
		GetClientRect(hwnd,&rc);
		rc.right++;
		rc.bottom++;
		hdc = GetDC(hwnd);
		hdc2=CreateCompatibleDC(hdc);
		hbmp=CreateBitmap32(rc.right,rc.bottom);
		hbmpo=SelectObject(hdc2,hbmp);
		BltBackImage(hwnd,hdc2,&rc);
		ModernDrawStatusBarWorker(hwnd,hdc2);
		BitBlt(hdc,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
			hdc2,rc.left,rc.top,SRCCOPY);
		SelectObject(hdc2,hbmpo);
		DeleteObject(hbmp);
		mod_DeleteDC(hdc2);
		{
			HFONT hf=GetStockObject(DEFAULT_GUI_FONT);
			SelectObject(hdc,hf);
		}
		ReleaseDC(hwnd,hdc);
		ValidateRect(hwnd,NULL);
	}
	else
    {
      HDC hdc, hdc2;
      HBITMAP hbmp, hbmpo;
      RECT rc;
      PAINTSTRUCT ps;
      HBRUSH br=GetSysColorBrush(COLOR_3DFACE);
      GetClientRect(hwnd,&rc);
      hdc=BeginPaint(hwnd,&ps);
      hdc2=CreateCompatibleDC(hdc);
      hbmp=CreateBitmap32(rc.right,rc.bottom);
      hbmpo=SelectObject(hdc2,hbmp);
      FillRect(hdc2,&ps.rcPaint,br);
      ModernDrawStatusBarWorker(hwnd,hdc2);
      //BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
      //  hdc2,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
      BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
        hdc2,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
      SelectObject(hdc2,hbmpo);
      DeleteObject(hbmp);
      mod_DeleteDC(hdc2);
      ps.fErase=FALSE;
      EndPaint(hwnd,&ps);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);

   case WM_GETMINMAXINFO:{
			RECT rct;
			GetWindowRect(hwnd,&rct);
			memset((LPMINMAXINFO)lParam,0,sizeof(MINMAXINFO));
			((LPMINMAXINFO)lParam)->ptMinTrackSize.x=16;
			((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=1600;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
			return(0);
	}

  case WM_SHOWWINDOW:
    {
      int res;
	  int ID;
      if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
					tooltipshoing=FALSE;
				};
	  ID=FindFrameID(hwnd);
	  if (ID)
	  {
		res=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS,ID),0);
		if (res>=0)	DBWriteContactSettingByte(0,"CLUI","ShowSBar",(BYTE)(wParam/*(res&F_VISIBLE)*/?1:0));
	  }
    }
    break;
  case WM_TIMER:
    {
      if (wParam==TM_STATUSBARHIDE)
      {
        KillTimer(hwnd,TM_STATUSBARHIDE);
        if (tooltipshoing)
        {
          NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
          tooltipshoing=FALSE;
        };
      }
      else if (wParam==TM_STATUSBAR)
      {
        POINT pt;
        KillTimer(hwnd,TM_STATUSBAR);
        GetCursorPos(&pt);
        if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
        {
          int i;
          RECT rc;
          ScreenToClient(hwnd,&pt);
          for (i=0; i<allocedItemData; i++)
          {
            rc=ProtosData[i].protoRect;
            if(PtInRect(&rc,pt))
            {
              NotifyEventHooks(hStatusBarShowToolTipEvent,(WPARAM)ProtosData[i].ProtoName,0);
              SetTimer(hwnd,TM_STATUSBARHIDE,DBGetContactSettingWord(NULL,"CLUIFrames","HideToolTipTime",5000),0);
              tooltipshoing=TRUE;
              return 0;
            }
          }
          return 0;
        }
      }
      return 0;
    };
  case WM_SETCURSOR:
    {
      if (BehindEdgeSettings) UpdateTimer(0);
      {
        POINT pt;
        GetCursorPos(&pt);
        SendMessage(GetParent(hwnd),msg,wParam,lParam);
        if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
        {
          return(TestCursorOnBorders());
        };
        lastpnt=pt;
        if (tooltipshoing){
          KillTimer(hwnd,TM_STATUSBARHIDE);
          NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
          tooltipshoing=FALSE;
        };
        KillTimer(hwnd,TM_STATUSBAR);
        SetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750),0);

        return(TestCursorOnBorders());
      };

    };
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
    {
      RECT rc;
      POINT pt;
      int i;
	  BOOL showXStatusMenu=FALSE;
      pt.x=(short)LOWORD(lParam);
      pt.y=(short)HIWORD(lParam);
      KillTimer(hwnd,TM_STATUSBARHIDE);

      if (tooltipshoing){
        NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
      };
      tooltipshoing=FALSE;
      for (i=0; i<allocedItemData; i++)
      {
		RECT rc1;
		BOOL isOnExtra=FALSE;
        rc=ProtosData[i].protoRect;
		rc1=rc;
		rc1.left=rc.left+16;
		rc1.right=rc1.left+16;
		if (PtInRect(&rc,pt) && PtInRect(&rc1,pt)&&ProtosData[i].DoubleIcons)
			isOnExtra=TRUE;
        if(PtInRect(&rc,pt))
        {
          HMENU hMenu=NULL;
		  SHORT a=GetKeyState(VK_CONTROL);
		  if (msg==WM_MBUTTONDOWN || (a&0x8000) || isOnExtra)	
		  {
			  showXStatusMenu=TRUE;
			  hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
			  if(allocedItemData>=1 && menusProto && i<AllocedProtos)// && GetSubMenu(hMenu,i)) hMenu=GetSubMenu(hMenu,i);
				{			
					TMO_IntMenuItem * it=GetMenuItemByGlobalID((int)menusProto[i].menuID);
					hMenu=it->hSubMenu;
				}
				if (hMenu)
				{
					HMENU tm=hMenu;
					hMenu=GetSubMenu(tm,0);	
					if (!hMenu) hMenu=GetSubMenu(tm,1);	
				}
		  }        
		  if (!hMenu)
		  {
			  if (msg==WM_RBUTTONDOWN)
			  {
				if( DBGetContactSettingByte(NULL,"CLUI","SBarRightClk",0))
					hMenu=(HMENU)CallService(MS_CLIST_MENUGETMAIN,0,0);
				else
					hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
			  }
			  else
			  {
				hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);		
				if(allocedItemData>1 && menusProto && i<AllocedProtos)// && GetSubMenu(hMenu,i)) hMenu=GetSubMenu(hMenu,i);
				{			
					TMO_IntMenuItem * it=GetMenuItemByGlobalID((int)menusProto[i].menuID);
					hMenu=it->hSubMenu;
				}
			}
		  }
          ClientToScreen(hwnd,&pt);
          {
            HWND parent=GetParent(hwnd);
            if (parent!=pcli->hwndContactList) parent=GetParent(parent);
            TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,0,parent,NULL);
          }
          return 0;
        }
      }
    }
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

int CreateModernStatusBar(HWND parent)
{

  WNDCLASS wndclass={0};
  TCHAR pluginname[]=TEXT("ModernStatusBar");
  int h=GetSystemMetrics(SM_CYSMICON)+2;
  if (GetClassInfo(g_hInst,pluginname,&wndclass) ==0)
  {
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = ModernStatusProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = pluginname;
    RegisterClass(&wndclass);
  }
  hModernStatusBar=CreateWindow(pluginname,pluginname,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
    0,0,0,h,parent,NULL,g_hInst,NULL);
  // register frame
  {
    CLISTFrame Frame;
    memset(&Frame,0,sizeof(Frame));
    Frame.cbSize=sizeof(CLISTFrame);
    Frame.hWnd=hModernStatusBar;
    Frame.align=alBottom;
    Frame.hIcon=LoadSkinnedIcon (SKINICON_OTHER_MIRANDA);
    Frame.Flags=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER|F_NO_SUBCONTAINER;
    Frame.height=h;
    Frame.name=(Translate("Status Bar"));
    hFramehModernStatusBar=(HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
    CallService(MS_SKINENG_REGISTERPAINTSUB,(WPARAM)Frame.hWnd,(LPARAM)NewStatusPaintCallbackProc); //$$$$$ register sub for frame
  }

  LoadStatusBarData();
  hStatusBarShowToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
  hStatusBarHideToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_HIDE_TOOLTIP);
  CluiProtocolStatusChanged(0,0);
  CallService(MS_CLIST_FRAMES_UPDATEFRAME,-1,0);
  return (int)hModernStatusBar;

};
