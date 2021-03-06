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

#define GetAValue(argb)((BYTE)((argb)>>24))
//Include

#include "commonheaders.h" 
#include "CLUIFRAMES\cluiframes.h"
#include "SkinEngine.h"  
#include "m_skin_eng.h"
#include "shlwapi.h"
#include "mod_skin_selector.h"
#include "commonprototypes.h"
//Definition
#define ScaleBord 2

#define MAX_BUFF_SIZE 255*400
#define MAXSN_BUFF_SIZE 255*1000
#define TEST_SECTION DEFAULTSKINSECTION

int LOCK_UPDATING=0;
BYTE UseKeyColor=1;
DWORD KeyColor=RGB(255,0,255);
//type definition
typedef struct _sCurrentWindowImageData
{
  HDC hImageDC;
  HDC hBackDC;
  HDC hScreenDC;
  HBITMAP hImageDIB, hImageOld;
  HBITMAP hBackDIB, hBackOld;
  BYTE * hImageDIBByte;
  BYTE * hBackDIBByte;

  int Width,Height;

}sCurrentWindowImageData;



BOOL LayeredFlag=0;   //TO BE GLOBAL
//Global in module
//BOOL LayeredFlag=0;   //TO BE GLOBAL
BOOL LOCK_IMAGE_UPDATING=0;

BOOL (WINAPI *MyUpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
BOOL UPDATE_ALLREADY_QUEUED=0;
BOOL POST_WAS_CANCELED=0;
BYTE CURRENT_ALPHA=255;
DWORD MSG_COUNTER=0;
SKINOBJECTSLIST * CURRENTSKIN=NULL;
HWND DialogWnd;
char * szFileSave=NULL;
sCurrentWindowImageData * cachedWindow=NULL;
char **settingname;
int arrlen;
BOOL JustDrawNonFramedObjects=0;
SKINOBJECTSLIST glObjectList={0};


//Declaration
int AlphaTextOutServ(WPARAM w,LPARAM l);
int AlphaTextOut (HDC hDC, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format, DWORD ARGBcolor);
//int ImageList_ReplaceIcon_FixAlphaServ (WPARAM w,LPARAM l);
//int ImageList_AddIcon_FixAlphaServ(WPARAM w,LPARAM l);
//int FixAlphaServ (WPARAM w,LPARAM l);
int UpdateWindowImageRect(RECT * r);
BOOL DrawIconExServ(WPARAM w,LPARAM l);
BOOL DrawIconExS(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);

//BOOL DrawTextS(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format);
//BOOL TextOutS(HDC hdc, int x, int y, LPCTSTR lpString, int nCount);
HBITMAP CreateBitmap32(int cx, int cy);
HBITMAP CreateBitmap32Point(int cx, int cy, void ** bits);
int ValidateSingleFrameImage(wndFrame * Frame, BOOL SkipBkgBlitting);
int ValidateFrameImageProc(RECT * r);
int UpdateWindowImage();
BOOL FillRect255Alpha(HDC memdc,RECT *fr);
BOOL FillRgn255Alpha(HDC memdc,HRGN fr);

int FixAlpha(HIMAGELIST himl,HICON hicon, int res);
//int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);
//int ImageList_AddIcon_FixAlpha(HIMAGELIST himl,HICON hicon);
int DrawNonFramedObjects(BOOL Erase,RECT *r);


//Implementation
BOOL MyAlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction)
{
  if (!gdiPlusFail &&0) //Use gdi+ engine
  {
    HBITMAP hbmp=GetCurrentObject(hdcSrc,OBJ_BITMAP);
    DrawAvatarImageWithGDIp(hdcDest,nXOriginDest,nYOriginDest,nWidthDest,nHeightDest,hbmp,nXOriginSrc,nYOriginSrc,nWidthSrc,nHeightSrc,blendFunction.AlphaFormat?8:0);
    return 0;
  }
  return AlphaBlend(hdcDest,nXOriginDest,nYOriginDest,nWidthDest,nHeightDest,hdcSrc,nXOriginSrc,nYOriginSrc,nWidthSrc,nHeightSrc,blendFunction);
}

CRITICAL_SECTION skin_cs={0};
int LockSkin()
{
  EnterCriticalSection(&skin_cs);
  return 0;
}
int UnlockSkin()
{
  LeaveCriticalSection(&skin_cs);
  return 0;
}
int LoadSkinModule()
{
  InitializeCriticalSection(&skin_cs);
  MainModernMaskList=mir_alloc(sizeof(ModernMaskList));
  memset(MainModernMaskList,0,sizeof(ModernMaskList));   
  TRACE("LOAD SKIN MODULE\n");
  //init variables
  glObjectList.dwObjLPAlocated=0;
  glObjectList.dwObjLPReserved=0;
  glObjectList.Objects=NULL;
  // Initialize GDI+
  InitGdiPlus();
  //load decoder
  hImageDecoderModule=NULL;
  if (gdiPlusFail)
  {
    hImageDecoderModule = LoadLibrary(TEXT("ImgDecoder.dll"));
    if (hImageDecoderModule==NULL) 
    {
      char tDllPath[ MAX_PATH ];
      GetModuleFileNameA( g_hInst, tDllPath, sizeof( tDllPath ));
      {
        char* p = strrchr( tDllPath, '\\' );
        if ( p != NULL )
          strcpy( p+1, "ImgDecoder.dll" );
        else
        {
          strcpy( tDllPath, "ImgDecoder.dll" );
        }
      }

      hImageDecoderModule = LoadLibraryA(tDllPath);
    }
    if (hImageDecoderModule!=NULL) 
    {
      ImgNewDecoder=(pfnImgNewDecoder )GetProcAddress( hImageDecoderModule, "ImgNewDecoder");
      ImgDeleteDecoder=(pfnImgDeleteDecoder )GetProcAddress( hImageDecoderModule, "ImgDeleteDecoder");
      ImgNewDIBFromFile=(pfnImgNewDIBFromFile)GetProcAddress( hImageDecoderModule, "ImgNewDIBFromFile");
      ImgDeleteDIBSection=(pfnImgDeleteDIBSection)GetProcAddress( hImageDecoderModule, "ImgDeleteDIBSection");
      ImgGetHandle=(pfnImgGetHandle)GetProcAddress( hImageDecoderModule, "ImgGetHandle");
    }




  }
  //create services
  {       
    //  CreateServiceFunction(MS_SKIN_REGISTEROBJECT,Skin_RegisterObject);
    CreateServiceFunction(MS_SKIN_DRAWGLYPH,Skin_DrawGlyph);
    //    CreateServiceFunction(MS_SKIN_REGISTERDEFOBJECT,ServCreateGlyphedObjectDefExt);
    CreateServiceFunction(MS_SKINENG_REGISTERPAINTSUB,RegisterPaintSub);
    CreateServiceFunction(MS_SKINENG_UPTATEFRAMEIMAGE,UpdateFrameImage);
    CreateServiceFunction(MS_SKINENG_INVALIDATEFRAMEIMAGE,InvalidateFrameImage);

    CreateServiceFunction(MS_SKINENG_ALPHATEXTOUT,AlphaTextOutServ);
    //		CreateServiceFunction(MS_SKINENG_IL_REPLACEICONFIX,ImageList_ReplaceIcon_FixAlphaServ);
    //CreateServiceFunction(MS_SKINENG_IL_ADDICONFIX,ImageList_AddIcon_FixAlphaServ);
    //CreateServiceFunction(MS_SKINENG_IL_ALPHAFIX,FixAlphaServ);
    CreateServiceFunction(MS_SKINENG_DRAWICONEXFIX,DrawIconExServ);
  }
  //create event handle
  hEventServicesCreated=CreateHookableEvent(ME_SKIN_SERVICESCREATED);
  hSkinLoaded=HookEvent(ME_SKIN_SERVICESCREATED,OnSkinLoad);



  // if (DBGetContactSettingByte(NULL,"CLUI","StoreNotUsingElements",0))
  //     GetSkinFromDB(DEFAULTSKINSECTION,&glObjectList);

  //notify services created
  {
    int t=NotifyEventHooks(hEventServicesCreated,0,0);
    t=t;

  }

  return 1;
}

int UnloadSkinModule()
{

  //unload services
  DeleteCriticalSection(&skin_cs);
  GdiFlush();
  DestroyServiceFunction(MS_SKIN_REGISTEROBJECT);
  DestroyServiceFunction(MS_SKIN_DRAWGLYPH);
  DestroyHookableEvent(hEventServicesCreated);
  if (hImageDecoderModule) FreeLibrary(hImageDecoderModule);
  ShutdownGdiPlus();
  //free variables
  return 1;
}



BOOL IsRectOverlap(l1,t1,r1,b1,l2,t2,r2,b2)
{
  int h,v;
  h=0; v=0;
  if (l2>=l1 && l2<=r1) h=1;
  else if (r2>=l1 && r2<=r1) h=1;
  if (t2>=t1 && t2<=b1) v=1;
  else if (b2>=t1 && b2<=b1) v=1;
  return 1;
}
BOOL FillRgn255Alpha(HDC memdc,HRGN hrgn)
{
  RGNDATA * rdata;
  DWORD rgnsz;
  DWORD d;
  RECT * rect;
  rgnsz=GetRegionData(hrgn,0,NULL);
  rdata=(RGNDATA *) mir_alloc(rgnsz);
  GetRegionData(hrgn,rgnsz,rdata);
  rect=(RECT *)rdata->Buffer;
  for (d=0; d<rdata->rdh.nCount; d++)
  {
    FillRect255Alpha(memdc,&rect[d]);
  }
  mir_free(rdata);
  return TRUE;
}

BOOL FillRectAlpha(HDC memdc,RECT *fr,DWORD Color)
{
  int x,y;
  int sx,sy,ex,ey;
  int f=0;
  BYTE * bits;
  BITMAP bmp;
  HBITMAP hbmp=GetCurrentObject(memdc,OBJ_BITMAP);  
  GetObject(hbmp, sizeof(bmp),&bmp);
  sx=(fr->left>0)?fr->left:0;
  sy=(fr->top>0)?fr->top:0;
  ex=(fr->right<bmp.bmWidth)?fr->right:bmp.bmWidth;
  ey=(fr->bottom<bmp.bmHeight)?fr->bottom:bmp.bmHeight;
  if (!bmp.bmBits)
  {
    f=1;
    bits=mir_alloc(bmp.bmWidthBytes*bmp.bmHeight);
    GetBitmapBits(hbmp,bmp.bmWidthBytes*bmp.bmHeight,bits);
  }
  else
    bits=bmp.bmBits;
  for (y=sy;y<ey;y++)
    for (x=sx;x<ex;x++)   
    {
      DWORD *p;
      p=(DWORD*)((BYTE*)bits+(bmp.bmHeight-y-1)*bmp.bmWidthBytes);
      p+=x;
      ((BYTE*)p)[0]=((BYTE*)&Color)[2];
      ((BYTE*)p)[1]=((BYTE*)&Color)[1];
      ((BYTE*)p)[2]=((BYTE*)&Color)[0];
      ((BYTE*)p)[3]=((BYTE*)&Color)[3];
    }
    if (f)
    {
      SetBitmapBits(hbmp,bmp.bmWidthBytes*bmp.bmHeight,bits);        mir_free(bits);
    }
    // DeleteObject(hbmp);
    return 1;
}

BOOL FillRect255Alpha(HDC memdc,RECT *fr)
{
  int x,y;
  int sx,sy,ex,ey;
  int f=0;
  BYTE * bits;
  BITMAP bmp;
  HBITMAP hbmp=GetCurrentObject(memdc,OBJ_BITMAP);  
  GetObject(hbmp, sizeof(bmp),&bmp);
  sx=(fr->left>0)?fr->left:0;
  sy=(fr->top>0)?fr->top:0;
  ex=(fr->right<bmp.bmWidth)?fr->right:bmp.bmWidth;
  ey=(fr->bottom<bmp.bmHeight)?fr->bottom:bmp.bmHeight;
  if (!bmp.bmBits)
  {
    f=1;
    bits=malloc(bmp.bmWidthBytes*bmp.bmHeight);
    GetBitmapBits(hbmp,bmp.bmWidthBytes*bmp.bmHeight,bits);
  }
  else
    bits=bmp.bmBits;
  for (y=sy;y<ey;y++)
    for (x=sx;x<ex;x++)
      (((BYTE*)bits)+(bmp.bmHeight-y-1)*bmp.bmWidthBytes+x*4)[3]=255;
  if (f)
  {
    SetBitmapBits(hbmp,bmp.bmWidthBytes*bmp.bmHeight,bits);    
    //free(bits);
  }
  // DeleteObject(hbmp);
  return 1;
}
BOOL SkFillRectangle(HDC hDest, HDC hSource, RECT * rFill, RECT * rGlyph, RECT * rClip, BYTE mode, BYTE drawMode, int depth)
{
  int destw=0, desth=0;
  int xstart=0, xmax=0;
  int ystart=0, ymax=0;
  BLENDFUNCTION bfa={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA }; 
  BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA }; 
  //     int res;
  //initializations
  // SetStretchBltMode(hDest, HALFTONE);
  if (mode==FM_STRETCH)
  {
    HDC mem2dc;
    HBITMAP mem2bmp, oldbmp;
    RECT wr;
    IntersectRect(&wr,rClip,rFill);
    if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;       
    if (drawMode!=2)
    {
      mem2dc=CreateCompatibleDC(hDest);
      mem2bmp=CreateBitmap32(wr.right-wr.left,wr.bottom-wr.top);
      oldbmp=SelectObject(mem2dc,mem2bmp);

    }

    if (drawMode==0 || drawMode==2)
    {
      if (drawMode==0)
      {
        //   SetStretchBltMode(mem2dc, HALFTONE);
        MyAlphaBlend(mem2dc,rFill->left-wr.left,rFill->top-wr.top,rFill->right-rFill->left,rFill->bottom-rFill->top,
          hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,rGlyph->bottom-rGlyph->top,bf);
        MyAlphaBlend(hDest,wr.left,wr.top,wr.right-wr.left, wr.bottom -wr.top,mem2dc,0,0,wr.right-wr.left, wr.bottom -wr.top,bf);
      }
      else 
      {
        MyAlphaBlend(hDest,rFill->left,rFill->top,rFill->right-rFill->left,rFill->bottom-rFill->top,
          hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,rGlyph->bottom-rGlyph->top,bf);

      }
    }
    else
    {
      //            BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
      MyAlphaBlend(mem2dc,rFill->left-wr.left,rFill->top-wr.top,rFill->right-rFill->left,rFill->bottom-rFill->top,
        hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,rGlyph->bottom-rGlyph->top,bf); 
      MyAlphaBlend(hDest,wr.left,wr.top,wr.right-wr.left, wr.bottom -wr.top,mem2dc,0,0,wr.right-wr.left, wr.bottom -wr.top,bf);
    }
    if (drawMode!=2)
    {
      SelectObject(mem2dc,oldbmp);
      DeleteObject(mem2bmp);
      DeleteDC(mem2dc);
    }
    return 1;
  }
  else if (mode==FM_TILE_VERT && (rGlyph->bottom-rGlyph->top>0)&& (rGlyph->right-rGlyph->left>0))
  {
    HDC mem2dc;
    HBITMAP mem2bmp,oldbmp;
    RECT wr;
    IntersectRect(&wr,rClip,rFill);
    if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;
    mem2dc=CreateCompatibleDC(hDest);
    //SetStretchBltMode(mem2dc, HALFTONE);
    mem2bmp=CreateBitmap32(wr.right-wr.left, rGlyph->bottom-rGlyph->top);
    oldbmp=SelectObject(mem2dc,mem2bmp);
    if (!oldbmp) 
      return 0;
    //MessageBoxA(NULL,"Tile bitmap not selected","ERROR", MB_OK);
    /// draw here
    {
      int  y=0, sy=0, maxy=0;
      int w=rFill->right-rFill->left;
      int h=rGlyph->bottom-rGlyph->top;
      if (h>0 && (wr.bottom-wr.top)*(wr.right-wr.left)!=0)
      {
        w=wr.right-wr.left;
        {
          //                   BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
          MyAlphaBlend(mem2dc,-(wr.left-rFill->left),0,rFill->right-rFill->left,h,hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,h,bf);
          //StretchBlt(mem2dc,-(wr.left-rFill->left),0,rFill->right-rFill->left,h,hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,h,SRCCOPY);
        }
        if (drawMode==0 || drawMode==2)
        {
          if (drawMode==0 )
          {

            int dy;
            dy=(wr.top-rFill->top)%h;
            if (dy>=0)
            {
              int ht;
              y=wr.top;
              ht=(y+h-dy<=wr.bottom)?(h-dy):(wr.bottom-wr.top);
              BitBlt(hDest,wr.left,y,w,ht,mem2dc,0,dy,SRCCOPY);
            }

            y=wr.top+h-dy;
            while (y<wr.bottom-h){
              BitBlt(hDest,wr.left,y,w,h,mem2dc,0,0,SRCCOPY);
              y+=h;
            }
            if (y<=wr.bottom)
              BitBlt(hDest,wr.left,y,w,wr.bottom-y, mem2dc,0,0,SRCCOPY);                                            

          }
          else 
          {  
            y=wr.top;
            while (y<wr.bottom-h)
            {
              //                             BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
              MyAlphaBlend(hDest,wr.left,y,w,h, mem2dc,0,0,w,h,bf);                    
              y+=h;
            }
            if (y<=wr.bottom)
            {
              //                           BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
              MyAlphaBlend(hDest,wr.left,y,w,wr.bottom-y, mem2dc,0,0,w,wr.bottom-y,bf);                                            
            }
          }

        }
        else
        {
          int dy;

          BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

          dy=(wr.top-rFill->top)%h;

          if (dy>=0)
          {
            int ht;
            y=wr.top;
            ht=(y+h-dy<=wr.bottom)?(h-dy):(wr.bottom-wr.top);
            MyAlphaBlend(hDest,wr.left,y,w,ht,mem2dc,0,dy,w,ht,bf);
          }

          y=wr.top+h-dy;
          while (y<wr.bottom-h)
          {
            MyAlphaBlend(hDest,wr.left,y,w,h,mem2dc,0,0,w,h,bf);
            y+=h;
          }
          if (y<=wr.bottom)
            MyAlphaBlend(hDest,wr.left,y,w,wr.bottom-y, mem2dc,0,0,w,wr.bottom-y,bf);
        }
      }
    }
    SelectObject(mem2dc,oldbmp);
    DeleteObject(mem2bmp);
    DeleteDC(mem2dc);
  }
  else if (mode==FM_TILE_HORZ && (rGlyph->right-rGlyph->left>0)&& (rGlyph->bottom-rGlyph->top>0)&&(rFill->bottom-rFill->top)>0 && (rFill->right-rFill->left)>0)
  {
    HDC mem2dc;
    RECT wr;
    HBITMAP mem2bmp,oldbmp;
    int w=rGlyph->right-rGlyph->left;
    int h=rFill->bottom-rFill->top;
    IntersectRect(&wr,rClip,rFill);
    if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;
    h=wr.bottom-wr.top;
    mem2dc=CreateCompatibleDC(hDest);

    mem2bmp=CreateBitmap32(w,h);
    oldbmp=SelectObject(mem2dc,mem2bmp);

    if (!oldbmp)
      return 0;
    /// draw here
    {
      int  x=0, sy=0, maxy=0;
      {
        //SetStretchBltMode(mem2dc, HALFTONE);
        //StretchBlt(mem2dc,0,0,w,h,hSource,rGlyph->left+(wr.left-rFill->left),rGlyph->top,w,h,SRCCOPY);

        //                    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
        MyAlphaBlend(mem2dc,0,-(wr.top-rFill->top),w,rFill->bottom-rFill->top,hSource,rGlyph->left,rGlyph->top,w,rGlyph->bottom-rGlyph->top,bf);
        if (drawMode==0 || drawMode==2)
        {
          if (drawMode==0)
          {

            int dx;
            dx=(wr.left-rFill->left)%w;
            if (dx>=0)
            {   
              int wt;
              x=wr.left;
              wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left);                                                   
              BitBlt(hDest,x,wr.top,wt,h,mem2dc,dx,0,SRCCOPY);
            }
            x=wr.left+w-dx;                        
            while (x<wr.right-w){
              BitBlt(hDest,x,wr.top,w,h,mem2dc,0,0,SRCCOPY);
              x+=w;
            }
            if (x<=wr.right);
            BitBlt(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,SRCCOPY);  
          }
          else 
          {
            int dx;
            dx=(wr.left-rFill->left)%w;
            x=wr.left-dx;
            while (x<wr.right-w){
              MyAlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
              x+=w;
            }
            if (x<=wr.right)
              MyAlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);
          }

        }
        else
        {
          BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
          int dx;
          dx=(wr.left-rFill->left)%w;
          if (dx>=0)
          {
            int wt;
            x=wr.left;
            wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left); 
            MyAlphaBlend(hDest,x,wr.top,wt,h,mem2dc,dx,0,wt,h,bf);
          }
          x=wr.left+w-dx;
          while (x<wr.right-w){
            MyAlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
            x+=w;
          }
          if (x<=wr.right)
            MyAlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);  

        }
      }
    }
    SelectObject(mem2dc,oldbmp);
    DeleteObject(mem2bmp);
    DeleteDC(mem2dc);
  }
  else if (mode==FM_TILE_BOTH && (rGlyph->right-rGlyph->left>0) && (rGlyph->bottom-rGlyph->top>0))
  {
    HDC mem2dc;
    int w=rGlyph->right-rGlyph->left;
    int  x=0, sy=0, maxy=0;
    int h=rFill->bottom-rFill->top;
    HBITMAP mem2bmp,oldbmp;
    RECT wr;
    IntersectRect(&wr,rClip,rFill);
    if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;
    mem2dc=CreateCompatibleDC(hDest);
    mem2bmp=CreateBitmap32(w,wr.bottom-wr.top);
    h=wr.bottom-wr.top;
    oldbmp=SelectObject(mem2dc,mem2bmp);
#ifdef _DEBUG
    if (!oldbmp) 
      (NULL,"Tile bitmap not selected","ERROR", MB_OK);
#endif
    /// draw here
    {

      //fill temp bitmap
      {
        int y;
        int dy;
        dy=(wr.top-rFill->top)%(rGlyph->bottom-rGlyph->top);
        y=-dy;
        while (y<wr.bottom-wr.top)
        {

          MyAlphaBlend(mem2dc,0,y,w,rGlyph->bottom-rGlyph->top, hSource,rGlyph->left,rGlyph->top,w,rGlyph->bottom-rGlyph->top,bf);                    
          y+=rGlyph->bottom-rGlyph->top;
        }

        //--    
        //end temp bitmap
        if (drawMode==0 || drawMode==2)
        {
          if (drawMode==0)
          {

            int dx;
            dx=(wr.left-rFill->left)%w;
            if (dx>=0)
            {   
              int wt;
              x=wr.left;
              wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left);                                                   
              BitBlt(hDest,x,wr.top,wt,h,mem2dc,dx,0,SRCCOPY);
            }
            x=wr.left+w-dx;                        
            while (x<wr.right-w){
              BitBlt(hDest,x,wr.top,w,h,mem2dc,0,0,SRCCOPY);
              x+=w;
            }
            if (x<=wr.right);
            BitBlt(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,SRCCOPY);  
          }
          else 
          {
            int dx;
            dx=(wr.left-rFill->left)%w;
            x=wr.left-dx;
            while (x<wr.right-w){
              MyAlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
              x+=w;
            }
            if (x<=wr.right)
              MyAlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);
          }

        }
        else
        {
          BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

          int dx;
          dx=(wr.left-rFill->left)%w;
          if (dx>=0)
          {
            int wt;
            x=wr.left;
            wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left); 
            MyAlphaBlend(hDest,x,wr.top,wt,h,mem2dc,dx,0,wt,h,bf);
          }
          x=wr.left+w-dx;
          while (x<wr.right-w){
            MyAlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
            x+=w;
          }
          if (x<=wr.right)
            MyAlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);  

        }
      }

    }
    SelectObject(mem2dc,oldbmp);
    DeleteObject(mem2bmp);
    DeleteDC(mem2dc);
  }
  return 1;

}
BOOL DeleteBitmap32(HBITMAP hBmpOsb)
{
  DIBSECTION ds;
  int res;
  GetObject(hBmpOsb,sizeof(ds),&ds);
  res=DeleteObject(hBmpOsb);
  if (!res) 
  {
    //     MessageBoxA(NULL,"IMAGE STILL SELECTED","IMAGE DELETING ERROR",MB_OK); 
    DebugBreak();
  }
  //        if(ds.dsBm.bmBits) free(ds.dsBm.bmBits);
  return TRUE;
}

HBITMAP CreateBitmap32(int cx, int cy)
{
  return CreateBitmap32Point(cx,cy,NULL);
}

HBITMAP CreateBitmap32Point(int cx, int cy, void ** bits)
{
  BITMAPINFO RGB32BitsBITMAPINFO; 
  UINT * ptPixels;
  HBITMAP DirectBitmap;

  ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
  RGB32BitsBITMAPINFO.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  RGB32BitsBITMAPINFO.bmiHeader.biWidth=cx;//bm.bmWidth;
  RGB32BitsBITMAPINFO.bmiHeader.biHeight=cy;//bm.bmHeight;
  RGB32BitsBITMAPINFO.bmiHeader.biPlanes=1;
  RGB32BitsBITMAPINFO.bmiHeader.biBitCount=32;
  // pointer used for direct Bitmap pixels access


  DirectBitmap = CreateDIBSection(NULL, 
    (BITMAPINFO *)&RGB32BitsBITMAPINFO, 
    DIB_RGB_COLORS,
    (void **)&ptPixels, 
    NULL, 0);
  if (bits!=NULL) *bits=ptPixels;
  return DirectBitmap;
}
int DrawSkinObject(SKINDRAWREQUEST * preq, GLYPHOBJECT * pobj)
{
  HDC memdc=NULL, glyphdc=NULL;
  int k=0;
  HBITMAP membmp=0,oldbmp=0,oldglyph=0;
  BYTE Is32Bit=0;
  RECT PRect;
  POINT mode2offset={0};
  int depth=0; 
  int mode=0; //0-FastDraw, 1-DirectAlphaDraw, 2-BufferedAlphaDraw

  if (!(preq && pobj)) return -1;
  if (!pobj->hGlyph && ((pobj->Style&7) ==ST_IMAGE ||(pobj->Style&7) ==ST_SOLARIZE)) return 0;
  // Determine painting mode
  depth=GetDeviceCaps(preq->hDC,BITSPIXEL);
  depth=depth<16?16:depth;
  {
    BITMAP bm={0};
    GetObject(pobj->hGlyph,sizeof(BITMAP),&bm);
    Is32Bit=bm.bmBitsPixel==32;
  }
  if ((!Is32Bit && pobj->dwAlpha==255)&& pobj->Style!=ST_BRUSH) mode=0;
  else if (pobj->dwAlpha==255 && pobj->Style!=ST_BRUSH) mode=1;
  else mode=2;
  // End painting mode

  //force mode

  if(preq->rcClipRect.bottom-preq->rcClipRect.top*preq->rcClipRect.right-preq->rcClipRect.left==0)
    preq->rcClipRect=preq->rcDestRect;
  IntersectRect(&PRect,&preq->rcDestRect,&preq->rcClipRect);
  if (mode==2)
  {   
    memdc=CreateCompatibleDC(preq->hDC);
    membmp=CreateBitmap32(PRect.right-PRect.left,PRect.bottom-PRect.top);
    oldbmp=SelectObject(memdc,membmp);
    if (oldbmp==NULL) 
    {
      if (mode==2)
      {
        SelectObject(memdc,oldbmp);
        DeleteDC(memdc);
        DeleteObject(membmp);
      }
      return 0;
    }
  } 

  if (mode!=2) memdc=preq->hDC;
  {
    if (pobj->hGlyph)
    {
      glyphdc=CreateCompatibleDC(preq->hDC);
      oldglyph=SelectObject(glyphdc,pobj->hGlyph);
    }
    // Drawing
    {    
      BITMAP bmp;
      RECT rFill, rGlyph, rClip;
      if ((pobj->Style&7) ==ST_BRUSH)
      {
        HBRUSH br=CreateSolidBrush(pobj->dwColor);
        RECT fr;
        if (mode==2)
        {
          SetRect(&fr,0,0,PRect.right-PRect.left,PRect.bottom-PRect.top);
          FillRect(memdc,&fr,br);
          FillRect255Alpha(memdc,&fr);
          // FillRectAlpha(memdc,&fr,pobj->dwColor|0xFF000000);
        }
        else
        {
          fr=PRect;
          // SetRect(&fr,0,0,PRect.right-PRect.left,PRect.bottom-PRect.top);
          FillRect(preq->hDC,&fr,br);
        }
        DeleteObject(br);
        k=-1;
      }
      else
      {
        if (mode==2)  
        {
          mode2offset.x=PRect.left;
          mode2offset.y=PRect.top;
          OffsetRect(&PRect,-mode2offset.x,-mode2offset.y);
        }


        GetObject(pobj->hGlyph,sizeof(BITMAP),&bmp);
        rClip=(preq->rcClipRect);

        {
          // Draw center...
          if (1)
          {
            rFill.top=preq->rcDestRect.top+pobj->dwTop;
            rFill.bottom=preq->rcDestRect.bottom-pobj->dwBottom; 
            rFill.left=preq->rcDestRect.left+pobj->dwLeft;
            rFill.right=preq->rcDestRect.right-pobj->dwRight;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);

            rGlyph.top=pobj->dwTop;
            rGlyph.left=pobj->dwLeft;
            rGlyph.right=bmp.bmWidth-pobj->dwRight;
            rGlyph.bottom=bmp.bmHeight-pobj->dwBottom;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode,mode,depth);
          }

          // Draw top side...
          if(1)
          {
            rFill.top=preq->rcDestRect.top;
            rFill.bottom=preq->rcDestRect.top+pobj->dwTop; 
            rFill.left=preq->rcDestRect.left+pobj->dwLeft;
            rFill.right=preq->rcDestRect.right-pobj->dwRight;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);

            rGlyph.top=0;
            rGlyph.left=pobj->dwLeft;
            rGlyph.right=bmp.bmWidth-pobj->dwRight;
            rGlyph.bottom=pobj->dwTop;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_HORZ,mode,depth);
          }
          // Draw bottom side...
          if(1)
          {
            rFill.top=preq->rcDestRect.bottom-pobj->dwBottom;
            rFill.bottom=preq->rcDestRect.bottom; 
            rFill.left=preq->rcDestRect.left+pobj->dwLeft;
            rFill.right=preq->rcDestRect.right-pobj->dwRight;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


            rGlyph.top=bmp.bmHeight-pobj->dwBottom;
            rGlyph.left=pobj->dwLeft;
            rGlyph.right=bmp.bmWidth-pobj->dwRight;
            rGlyph.bottom=bmp.bmHeight;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_HORZ,mode,depth);
          }
          // Draw left side...
          if(1)
          {
            rFill.top=preq->rcDestRect.top+pobj->dwTop;
            rFill.bottom=preq->rcDestRect.bottom-pobj->dwBottom; 
            rFill.left=preq->rcDestRect.left;
            rFill.right=preq->rcDestRect.left+pobj->dwLeft;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


            rGlyph.top=pobj->dwTop;
            rGlyph.left=0;
            rGlyph.right=pobj->dwLeft;
            rGlyph.bottom=bmp.bmHeight-pobj->dwBottom;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_VERT,mode,depth);
          }

          // Draw right side...
          if(1)
          {
            rFill.top=preq->rcDestRect.top+pobj->dwTop;
            rFill.bottom=preq->rcDestRect.bottom-pobj->dwBottom; 
            rFill.left=preq->rcDestRect.right-pobj->dwRight;
            rFill.right=preq->rcDestRect.right;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


            rGlyph.top=pobj->dwTop;
            rGlyph.left=bmp.bmWidth-pobj->dwRight;
            rGlyph.right=bmp.bmWidth;
            rGlyph.bottom=bmp.bmHeight-pobj->dwBottom;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_VERT,mode,depth);
          }


          // Draw Top-Left corner...
          if(1)
          {
            rFill.top=preq->rcDestRect.top;
            rFill.bottom=preq->rcDestRect.top+pobj->dwTop; 
            rFill.left=preq->rcDestRect.left;
            rFill.right=preq->rcDestRect.left+pobj->dwLeft;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


            rGlyph.top=0;
            rGlyph.left=0;
            rGlyph.right=pobj->dwLeft;
            rGlyph.bottom=pobj->dwTop;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
          }
          // Draw Top-Right corner...
          if(1)
          {
            rFill.top=preq->rcDestRect.top;
            rFill.bottom=preq->rcDestRect.top+pobj->dwTop; 
            rFill.left=preq->rcDestRect.right-pobj->dwRight;
            rFill.right=preq->rcDestRect.right;

            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


            rGlyph.top=0;
            rGlyph.left=bmp.bmWidth-pobj->dwRight;
            rGlyph.right=bmp.bmWidth;
            rGlyph.bottom=pobj->dwTop;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
          }

          // Draw Bottom-Left corner...
          if(1)
          {
            rFill.top=preq->rcDestRect.bottom-pobj->dwBottom;
            rFill.bottom=preq->rcDestRect.bottom; 
            rFill.left=preq->rcDestRect.left;
            rFill.right=preq->rcDestRect.left+pobj->dwLeft;


            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


            rGlyph.left=0;
            rGlyph.right=pobj->dwLeft; 
            rGlyph.top=bmp.bmHeight-pobj->dwBottom;
            rGlyph.bottom=bmp.bmHeight;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
          }
          // Draw Bottom-Right corner...
          if(1)
          {
            rFill.top=preq->rcDestRect.bottom-pobj->dwBottom;
            rFill.bottom=preq->rcDestRect.bottom;
            rFill.left=preq->rcDestRect.right-pobj->dwRight;
            rFill.right=preq->rcDestRect.right;


            if (mode==2)
              OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);

            rGlyph.left=bmp.bmWidth-pobj->dwRight;
            rGlyph.right=bmp.bmWidth;
            rGlyph.top=bmp.bmHeight-pobj->dwBottom;
            rGlyph.bottom=bmp.bmHeight;

            k+=SkFillRectangle(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
          }
        }

      }

      if ((k>0 || k==-1) && mode==2)
      {
        BITMAP bm={0};
        GetObject(pobj->hGlyph,sizeof(BITMAP),&bm);               
        {
          BLENDFUNCTION bf={AC_SRC_OVER, 0, /*(bm.bmBitsPixel==32)?255:*/pobj->dwAlpha, (bm.bmBitsPixel==32 && pobj->Style!=ST_BRUSH)?AC_SRC_ALPHA:0};
          if (mode==2)
            OffsetRect(&PRect,mode2offset.x,mode2offset.y);
          MyAlphaBlend( preq->hDC,PRect.left,PRect.top,PRect.right-PRect.left,PRect.bottom-PRect.top, 
            memdc,0,0,PRect.right-PRect.left,PRect.bottom-PRect.top,bf);
        }                 
      }
    }
    //free GDI resources
    {

      if (oldglyph) SelectObject(glyphdc,oldglyph);
      if (glyphdc) DeleteDC(glyphdc);
    }    
    if (mode==2)
    {
      SelectObject(memdc,oldbmp);
      DeleteDC(memdc);
      DeleteObject(membmp);
    }

  }  
  return 0;
}



int AddObjectDescriptorToSkinObjectList (LPSKINOBJECTDESCRIPTOR lpDescr, SKINOBJECTSLIST* Skin)
{
  SKINOBJECTSLIST *sk;
  if (Skin) sk=Skin; else sk=&glObjectList;
  if (boolstrcmpi(lpDescr->szObjectID,"_HEADER_")) return 0;
  {//check if new object allready presents.
    DWORD i=0;
    for (i=0; i<sk->dwObjLPAlocated;i++)
      if (!MyStrCmp(sk->Objects[i].szObjectID,lpDescr->szObjectID)) return 0;
  }
  if (sk->dwObjLPAlocated+1>sk->dwObjLPReserved)
  { // Realocated list to add space for new object

    sk->Objects=mir_realloc(sk->Objects,sizeof(SKINOBJECTDESCRIPTOR)*(sk->dwObjLPReserved+1)/*alloc step*/);
    sk->dwObjLPReserved++; 
  }
  { //filling new objects field
    sk->Objects[sk->dwObjLPAlocated].bType=lpDescr->bType;
    sk->Objects[sk->dwObjLPAlocated].Data=NULL;
    sk->Objects[sk->dwObjLPAlocated].szObjectID=mir_strdup(lpDescr->szObjectID);
    //  sk->Objects[sk->dwObjLPAlocated].szObjectName=mir_strdup(lpDescr->szObjectName);
    if (lpDescr->Data!=NULL)
    {   //Copy defaults values
      switch (lpDescr->bType) 
      {
      case OT_GLYPHOBJECT:
        {   
          GLYPHOBJECT * obdat;
          GLYPHOBJECT * gl=(GLYPHOBJECT*)lpDescr->Data;
          sk->Objects[sk->dwObjLPAlocated].Data=mir_alloc(sizeof(GLYPHOBJECT));
          obdat=(GLYPHOBJECT*)sk->Objects[sk->dwObjLPAlocated].Data;
          memcpy(obdat,gl,sizeof(GLYPHOBJECT));
          if (gl->szFileName!=NULL)                    
          {
            obdat->szFileName=mir_strdup(gl->szFileName);
            mir_free(gl->szFileName);
          }
          else
            obdat->szFileName=NULL;
          obdat->hGlyph=NULL;
          break;
        }
      }

    }
  }
  sk->dwObjLPAlocated++;
  return 1;
}
LPSKINOBJECTDESCRIPTOR FindObject(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin)
{
  // DWORD i;
  SKINOBJECTSLIST* sk;
  sk=(Skin==NULL)?(&glObjectList):Skin;
  return FindObjectByRequest((char *)szName,sk->MaskList);

  /*  for (i=0; i<sk->dwObjLPAlocated; i++)
  {
  if (sk->Objects[i].bType==objType || objType==OT_ANY)
  {
  if (!MyStrCmp(sk->Objects[i].szObjectID,szName))
  return &(sk->Objects[i]);
  }
  }
  return NULL;
  */
}

LPSKINOBJECTDESCRIPTOR FindObjectByName(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin)
{
  DWORD i;
  SKINOBJECTSLIST* sk;
  sk=(Skin==NULL)?(&glObjectList):Skin;
  for (i=0; i<sk->dwObjLPAlocated; i++)
  {
    if (sk->Objects[i].bType==objType || objType==OT_ANY)
    {
      if (!MyStrCmp(sk->Objects[i].szObjectID,szName))
        return &(sk->Objects[i]);
    }
  }
  return NULL;
}

int Skin_DrawGlyph(WPARAM wParam,LPARAM lParam)
{
  LPSKINDRAWREQUEST preq;
  LPSKINOBJECTDESCRIPTOR pgl;
  LPGLYPHOBJECT gl;
  char buf[255];
  int res;
  if (!wParam) return -1;
  LockSkin();
  preq=(LPSKINDRAWREQUEST)wParam;   
  strncpy(buf,preq->szObjectID,sizeof(buf));   
  do
  {
    pgl=FindObject(buf, OT_GLYPHOBJECT,(SKINOBJECTSLIST*)lParam);
    if (pgl==NULL) {UnlockSkin(); return -1;}
    if (pgl->Data==NULL){UnlockSkin(); return -1;}
    gl= (LPGLYPHOBJECT)pgl->Data;
    if ((gl->Style&7) ==ST_SKIP) {UnlockSkin(); return ST_SKIP;}
    if ((gl->Style&7) ==ST_PARENT) 
    {
      int i;
      for (i=MyStrLen(buf); i>0; i--)
        if (buf[i]=='/')  {buf[i]='\0'; break;}
        if (i==0) {UnlockSkin(); return -1;}
    }
  }while ((gl->Style&7) ==ST_PARENT);
  {
    if (gl->hGlyph==NULL && ((gl->Style&7) ==ST_IMAGE || (gl->Style&7) ==ST_SOLARIZE))
      if (gl->szFileName) 
        gl->hGlyph=LoadGlyphImage(gl->szFileName);
  }
  res=DrawSkinObject(preq,gl);
  UnlockSkin();
  return res;
}


void PreMultiplyChanells(HBITMAP hbmp,BYTE Mult)
{
  BITMAP bmp;     

  BYTE * pBitmapBits;
  DWORD Len;
  int bh,bw,y,x;

  GetObject(hbmp, sizeof(BITMAP), (LPSTR)&bmp);
  bh=bmp.bmHeight;
  bw=bmp.bmWidth;
  Len=bh*bw*4;
  pBitmapBits=(LPBYTE)malloc(Len);
  GetBitmapBits(hbmp,Len,pBitmapBits);
  for (y=0; y<bh; ++y)
  {
    BYTE *pPixel= pBitmapBits + bw * 4 * y;

    for (x=0; x<bw ; ++x)
    {
      if (Mult)
      {
        pPixel[0]= pPixel[0]*pPixel[3]/255;
        pPixel[1]= pPixel[1]*pPixel[3]/255;
        pPixel[2]= pPixel[2]*pPixel[3]/255;
      }
      else
      {
        pPixel[3]=255;
      }
      pPixel+= 4;
    }
  }
  Len=SetBitmapBits(hbmp,Len,pBitmapBits);
  free (pBitmapBits);
  return;
}

char * StrConcat(char * dest, char * first, char * delim, char * second, DWORD maxSize)
{
  strncpy(dest,first,maxSize);
  strncat(dest,delim,maxSize);
  strncat(dest,second,maxSize);
  return dest;
}
HBITMAP intLoadGlyphImageByImageDecoder(char * szFileName);
extern HBITMAP intLoadGlyphImageByGDIPlus(char *szFileName);

HBITMAP intLoadGlyphImage(char * szFileName)
{
  if (!gdiPlusFail) return intLoadGlyphImageByGDIPlus(szFileName);
  else return intLoadGlyphImageByImageDecoder(szFileName);
}
HBITMAP intLoadGlyphImageByImageDecoder(char * szFileName)
{
  // Loading image from file by imgdecoder...    
  HBITMAP hBitmap=NULL;
  char ext[5];
  BYTE f=0;
  LPBYTE pBitmapBits;
  LPVOID pImg= NULL;
  LPVOID m_pImgDecoder;
  BITMAP bmpInfo;
  {
    int l;
    l=MyStrLen(szFileName);
    memcpy(ext,szFileName +(l-4),5);   
  }
  if (!PathFileExistsA(szFileName)) return NULL;

  if (hImageDecoderModule==NULL || !boolstrcmpi(ext,".png"))
    hBitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)szFileName);
  else
  {
    f=1;
    ImgNewDecoder(&m_pImgDecoder);
    if (!ImgNewDIBFromFile(m_pImgDecoder, szFileName, &pImg))	
    {
      ImgGetHandle(pImg, &hBitmap, (LPVOID *)&pBitmapBits);
      ImgDeleteDecoder(m_pImgDecoder);
    }
  }
  if (hBitmap)
  {

    GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
    if (bmpInfo.bmBitsPixel == 32)	
      PreMultiplyChanells(hBitmap,f);
    else
    {
      HDC dc24,dc32;
      HBITMAP hBitmap32,obmp24,obmp32;
      dc32=CreateCompatibleDC(NULL);
      dc24=CreateCompatibleDC(NULL);
      hBitmap32=CreateBitmap32(bmpInfo.bmWidth,bmpInfo.bmHeight);
      obmp24=SelectObject(dc24,hBitmap);
      obmp32=SelectObject(dc32,hBitmap32);
      BitBlt(dc32,0,0,bmpInfo.bmWidth,bmpInfo.bmHeight,dc24,0,0,SRCCOPY);
      SelectObject(dc24,obmp24);
      SelectObject(dc32,obmp32);
      DeleteDC(dc24);
      DeleteDC(dc32);
      DeleteObject(hBitmap);
      hBitmap=hBitmap32;
      PreMultiplyChanells(hBitmap,0);
    }

  }
  return hBitmap; 
}

HBITMAP LoadGlyphImage(char * szfileName)
{
  // try to find image in loaded
  DWORD i;HBITMAP hbmp;
  char szFileName [MAX_PATH];
  char fn[MAX_PATH];
  {
    _snprintf(fn,sizeof(fn),"%s\\%s",glObjectList.SkinPlace,szfileName);
    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)fn, (LPARAM)&szFileName);
  }
  //CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)szfileName,(LPARAM) &szFileName);
  for (i=0; i<glLoadedImagesCount; i++)
  {
    if (boolstrcmpi(glLoadedImages[i].szFileName,szFileName))
    {
      glLoadedImages[i].dwLoadedTimes++;
      return glLoadedImages[i].hGlyph;
    }
  }
  {
    // load new image
    hbmp=intLoadGlyphImage(szFileName);
    if (hbmp==NULL) return NULL;
    // add to loaded list
    if (glLoadedImagesCount+1>glLoadedImagesAlocated)
    {
      glLoadedImages=mir_realloc(glLoadedImages,sizeof(GLYPHIMAGE)*(glLoadedImagesCount+1));
      if (glLoadedImages) glLoadedImagesAlocated++;
      else return NULL;
    }
    glLoadedImages[glLoadedImagesCount].dwLoadedTimes=1;
    glLoadedImages[glLoadedImagesCount].hGlyph=hbmp;
    glLoadedImages[glLoadedImagesCount].szFileName=mir_strdup(szFileName);
    glLoadedImagesCount++;
  }
  return hbmp;
}
int UnloadGlyphImage(HBITMAP hbmp)
{
  DWORD i;
  for (i=0; i<glLoadedImagesCount; i++)
  {
    if (hbmp==glLoadedImages[i].hGlyph)
    {
      glLoadedImages[i].dwLoadedTimes--;
      if (glLoadedImages[i].dwLoadedTimes==0)
      {
        LPGLYPHIMAGE gl=glLoadedImages;
        if (gl->szFileName)mir_free(gl->szFileName);
        memcpy(&(glLoadedImages[i]),&(glLoadedImages[i+1]),sizeof(GLYPHIMAGE)*(glLoadedImagesCount-i-1));
        glLoadedImagesCount--;
        DeleteObject(hbmp);
      }
      return 0;
    }

  }
  DeleteObject(hbmp);
  return 0;
}

CRITICAL_SECTION skin;
int UnloadSkin(SKINOBJECTSLIST * Skin)
{   

  DWORD i;
  InitializeCriticalSection(&skin);
  EnterCriticalSection(&skin);
  ClearMaskList(Skin->MaskList);

  if (Skin->SkinPlace) mir_free(Skin->SkinPlace);
  DeleteButtons();
  if (Skin->dwObjLPAlocated==0) return 0;
  for (i=0; i<Skin->dwObjLPAlocated; i++)
  {
    switch(Skin->Objects[i].bType)
    {
    case OT_GLYPHOBJECT:
      {
        GLYPHOBJECT * dt;
        dt=(GLYPHOBJECT*)Skin->Objects[i].Data;
        if (dt->hGlyph) UnloadGlyphImage(dt->hGlyph);
        dt->hGlyph=NULL;
        if (dt->szFileName) mir_free(dt->szFileName);
        mir_free(dt);
      }
      break;
    } 
    if (Skin->Objects[i].szObjectID) mir_free(Skin->Objects[i].szObjectID); 

  }
  mir_free(Skin->Objects);
  Skin->dwObjLPAlocated=0;
  Skin->dwObjLPReserved=0;
  LeaveCriticalSection(&skin);
  DeleteCriticalSection(&skin);
  return 0;
}

int EnumGetProc (const char *szSetting,LPARAM lParam)
{   
  if (WildCompare((char *)szSetting,"$*",0))
  {
    char * value;
    value=DBGetString(NULL,SKIN,szSetting);
    RegisterObjectByParce((char *)szSetting,value);
    mir_free(value);
  }
  else if (WildCompare((char *)szSetting,"#*",0))
  {
    char * value;
    value=DBGetString(NULL,SKIN,szSetting);
    RegisterButtonByParce((char *)szSetting,value);
    mir_free(value);
  }
  return 1;
}

// Getting skin objects and masks from DB
int GetSkinFromDB(char * szSection, SKINOBJECTSLIST * Skin)
{
  if (Skin==NULL) return 0;
  UnloadSkin(Skin);
  Skin->MaskList=mir_alloc(sizeof(ModernMaskList));
  memset(Skin->MaskList,0,sizeof(ModernMaskList));
  Skin->SkinPlace=DBGetString(NULL,SKIN,"SkinFolder");
  if (!Skin->SkinPlace) 
  {
    char buf[255];
    DWORD col;
    Skin->SkinPlace=mir_strdup("\\Skin\\Default");
    //main window part
    col=GetSysColor(COLOR_3DFACE);
    _snprintf(buf,sizeof(buf),"Glyph,Solid,%d,%d,%d,%d",GetRValue(col),GetGValue(col),GetBValue(col),255);
    RegisterObjectByParce("$DefaultSkinObj",buf);

    //selection part
    col=GetSysColor(COLOR_HIGHLIGHT);
    _snprintf(buf,sizeof(buf),"Glyph,Solid,%d,%d,%d,%d",GetRValue(col),GetGValue(col),GetBValue(col),255);
    RegisterObjectByParce("$DefSkinSelObj",buf);

    AddStrModernMaskToList("CL,ID=Selection","$DefSkinSelObj",Skin->MaskList,Skin);	
    AddStrModernMaskToList("*,ID^Ovl,ID^Row,ID^Hot*","$DefaultSkinObj",Skin->MaskList,Skin);	
    return 1;
  }
  //Load objects
  {
    DBCONTACTENUMSETTINGS dbces;
    CURRENTSKIN=Skin;
    dbces.pfnEnumProc=EnumGetProc;
    dbces.szModule=SKIN;
    dbces.ofsSettings=0;
    CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);
    CURRENTSKIN=NULL;
  }
  //Load Masks
  {
    DWORD n=0;
    while (TRUE)
    {
      char buf[260];
      DWORD Param=0;
      char * Mask=NULL;
      char * value=NULL;
      _snprintf(buf,sizeof(buf),"@%d",n);
      if (value=DBGetString(NULL,SKIN,buf))
      {
        int i=0;
        for (i=0; i<MyStrLen(value); i++)
          if (value[i]==':') break;
        if (i<MyStrLen(value))
        {
          char * Obj, *Mask;
          int res;
          Mask=value+i+1;
          Obj=mir_alloc(i+1);
          strncpy(Obj,value,i);
          Obj[i]='\0';
          res=AddStrModernMaskToList(Mask,Obj,Skin->MaskList,Skin);
          mir_free(Obj);
        }
        mir_free(value);
        value=NULL;
        n++;
      }
      else break;
    }
  }
  return 0;
}

//surrogate to be called from outside
void LoadSkinFromDB(void) 
{ 
  GetSkinFromDB(SKIN,&glObjectList); 
  UseKeyColor=DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1);
  KeyColor=DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255));
}


int GetSkinFolder(char * szFileName, char * t2)
{
  char *buf;   
  char *b2;

  b2=mir_strdup(szFileName);
  buf=b2+MyStrLen(b2);
  while (buf>b2 && *buf!='.') {buf--;}
  *buf='\0';
  strcpy(t2,b2);

  {
    char custom_folder[MAX_PATH];
    char cus[MAX_PATH];
    char *b3;
    strcpy(custom_folder,t2);
    b3=custom_folder+MyStrLen(custom_folder);
    while (b3>custom_folder && *b3!='\\') {b3--;}
    *b3='\0';

    GetPrivateProfileStringA("Skin_Description_Section","SkinFolder","",cus,sizeof(custom_folder),szFileName);
    if (MyStrLen(cus)>0)
      _snprintf(t2,MAX_PATH,"%s\\%s",custom_folder,cus);
  }   	
  mir_free(b2);
  CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)t2, (LPARAM)t2);
  return 0;
}
//Load data from ini file
int LoadSkinFromIniFile(char * szFileName)
{
  char bsn[MAXSN_BUFF_SIZE];
  char * Buff;

  int i=0;
  int f=0;
  int ReadingSection=0;
  char AllowedSection[260];
  int AllowedAll=0;
  char t2[MAX_PATH];
  char t3[MAX_PATH];

  DWORD retu=GetPrivateProfileSectionNamesA(bsn,MAXSN_BUFF_SIZE,szFileName);
  DeleteAllSettingInSection("ModernSkin");
  GetSkinFolder(szFileName,t2);
  DBWriteContactSettingString(NULL,SKIN,"SkinFolder",t2);
  CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFileName, (LPARAM)t3);
  DBWriteContactSettingString(NULL,SKIN,"SkinFile",t3);
  Buff=bsn;
  AllowedSection[0]=0;
  do         
  {
    f=MyStrLen(Buff);
    if (f>0 && !boolstrcmpi(Buff,"Skin_Description_Section"))
    {
      char b3[MAX_BUFF_SIZE];
      DWORD ret=0;
      ret=GetPrivateProfileSectionA(Buff,b3,MAX_BUFF_SIZE,szFileName);
      if (ret>MAX_BUFF_SIZE-3) continue;
      if (ret==0) continue;
      {
        DWORD p=0;
        char *s1;
        char *s2;
        char *s3;
        {
          DWORD t;
          BOOL LOCK=FALSE;
          for (t=0; t<ret-1;t++)
          {
            if (b3[t]=='\0') LOCK=FALSE;
            if (b3[t]=='=' && !LOCK) 
            {
              b3[t]='\0';
              LOCK=TRUE;
            }
          }
        }
        do
        {
          s1=b3+p;

          s2=s1+MyStrLen(s1)+1;
          switch (s2[0])
          {
          case 'b':
            {
              BYTE P;
              //                            char ba[255];
              s3=s2+1;
              P=(BYTE)atoi(s3);
              DBWriteContactSettingByte(NULL,Buff,s1,P);
            }
            break;
          case 'w':
            {
              WORD P;
              //                           char ba[255];
              s3=s2+1;
              P=(WORD)atoi(s3);
              DBWriteContactSettingWord(NULL,Buff,s1,P);
            }break;
          case 'd':
            {
              DWORD P;

              s3=s2+1;
              P=(DWORD)atoi(s3);
              DBWriteContactSettingDword(NULL,Buff,s1,P);
            }break;
          case 's':
            {
              //                          char ba[255];
              char bb[255];
              s3=s2+1;
              strncpy(bb,s3,sizeof(bb));
              DBWriteContactSettingString(NULL,Buff,s1,s3);
            }break;
          case 'f': //file
            {
              //                         char ba[255];
              char bb[255];

              s3=s2+1;
              {
                char fn[MAX_PATH];
                int pp, i;
                pp=-1;
                CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFileName, (LPARAM)fn);
                {
                  for (i=0; i<MyStrLen(fn); i++)  if (fn[i]=='.') pp=i;
                  if (pp!=-1)
                  {
                    fn[pp]='\0';
                  }
                }                      
                sprintf(bb,"%s\\%s",fn,s3);
                DBWriteContactSettingString(NULL,Buff,s1,bb);
              }
            }break;
          }
          p=p+MyStrLen(s1)+MyStrLen(s2)+2;
        } while (p<ret);

      }
    }
    Buff+=MyStrLen(Buff)+1;
  }while (((DWORD)Buff-(DWORD)bsn)<retu);
  return 0;
}


int EnumDeletionProc (const char *szSetting,LPARAM lParam)
{

  if (szSetting==NULL){return(1);};
  arrlen++;
  settingname=(char **)realloc(settingname,arrlen*sizeof(char *));
  settingname[arrlen-1]=strdup(szSetting);
  return(1);
};
int DeleteAllSettingInSection(char * SectionName)
{
  DBCONTACTENUMSETTINGS dbces;
  arrlen=0;
  settingname=NULL;
  dbces.pfnEnumProc=EnumDeletionProc;
  dbces.szModule=SectionName;
  dbces.ofsSettings=0;

  CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

  //delete all settings
  if (arrlen==0){return(0);};
  {
    int i;
    for (i=0;i<arrlen;i++)
    {
      DBDeleteContactSetting(0,SectionName,settingname[i]);
      free(settingname[i]);
    };
    free(settingname);
    settingname=NULL;
    arrlen=0;    
  };
  return(0);
};


BOOL TextOutSA(HDC hdc, int x, int y, char * lpString, int nCount)
{
#ifdef UNICODE
	TCHAR *buf=mir_alloc((2+nCount)*sizeof(TCHAR));
	BOOL res;
	MultiByteToWideChar(CP_ACP, 0, lpString, -1, buf, (2+nCount)*sizeof(TCHAR)); 
	res=TextOutS(hdc,x,y,buf,nCount);
	mir_free(buf);
	return res;
#else
	return TextOutS(hdc,x,y,lpString,nCount);
#endif
}

BOOL TextOutS(HDC hdc, int x, int y, LPCTSTR lpString, int nCount)
{
  int ta;
  SIZE sz;
  RECT rc={0};
  if (!gdiPlusFail &&0) ///text via gdi+
  {
    TextOutWithGDIp(hdc,x,y,lpString,nCount);
    return 0;
  }
  else

  {
    // return TextOut(hdc, x,y,lpString,nCount);
    GetTextExtentPoint32(hdc,lpString,nCount,&sz);
    ta=GetTextAlign(hdc);
    SetRect(&rc,x,y,x+sz.cx,y+sz.cy);
    DrawTextS(hdc,lpString,nCount,&rc,DT_NOCLIP|DT_SINGLELINE|DT_LEFT);
  }
  return 1;
}

int AlphaTextOutServ(WPARAM w,LPARAM l)
{
  if (!w) return 0;
  {
    AlphaTextOutParams ap=*(AlphaTextOutParams*)w;
    return AlphaTextOut(ap.hDC,ap.lpString,ap.nCount,ap.lpRect,ap.format,ap.ARGBcolor);
  }
}
BYTE weight[256];
BYTE weight2[256];
BYTE weightfilled=0;
#include "math.h"
int AlphaTextOut (HDC hDC, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format, DWORD ARGBcolor)
{
  HBITMAP destBitmap;
  SIZE sz, fsize;
  SIZE wsize={0};
  HDC memdc;
  HBITMAP hbmp,holdbmp;
  HDC bufDC;
  HBITMAP bufbmp,bufoldbmp;
  BITMAP bmpdata;
  BYTE * destBits;
  BOOL noDIB=0;
  BOOL is16bit=0;
  BYTE * bits;
  BYTE * bufbits;
  HFONT hfnt, holdfnt;
  int drx=0;
  int dry=0;
  int dtx=0;
  int dty=0;
  RECT workRect;
  workRect=*lpRect;
  if (!weightfilled)
  {
    int i;
    for(i=0;i<256;i++)
    {
      double f;
      double gamma=(double)DBGetContactSettingDword(NULL,"ModernData","AlphaTextOutGamma1",700)/1000;
      double f2;
      double gamma2=(double)DBGetContactSettingDword(NULL,"ModernData","AlphaTextOutGamma2",700)/1000;

      f=(double)i/255;
      f=pow(f,(1/gamma));
      f2=(double)i/255;
      f2=pow(f2,(1/gamma2));
      weight[i]=(BYTE)(255*f);
      weight2[i]=(BYTE)(255*f);
    }
    weightfilled=1;
  }
  if (!lpString) return 0;
  if (nCount==-1) nCount=lstrlen(lpString);
  // retrieve destination bitmap bits
  {
    destBitmap=(HBITMAP)GetCurrentObject(hDC,OBJ_BITMAP);
    GetObject(destBitmap, sizeof(BITMAP),&bmpdata);
    if (bmpdata.bmBits==NULL)
    {
      noDIB=1;
      destBits=(BYTE*)malloc(bmpdata.bmHeight*bmpdata.bmWidthBytes);
      GetBitmapBits(destBitmap,bmpdata.bmHeight*bmpdata.bmWidthBytes,destBits);
    }
    else 
      destBits=bmpdata.bmBits;
    is16bit=(bmpdata.bmBitsPixel)!=32;
  }
  // Create DC
  {
    memdc=CreateCompatibleDC(hDC);
    hfnt=(HFONT)GetCurrentObject(hDC,OBJ_FONT);
    SetBkColor(memdc,0);
    SetTextColor(memdc,RGB(255,255,255));
    holdfnt=(HFONT)SelectObject(memdc,hfnt);
  }

  // Calc Sizes
  {
    //Calc full text size
    GetTextExtentPoint32(memdc,lpString,nCount,&sz);
    sz.cx+=2;
    fsize=sz;
    // Calc buffer size
    if (workRect.left<0) workRect.left=0;
    if (workRect.top<0) workRect.top=0;
    if (workRect.right>bmpdata.bmWidth) workRect.right=bmpdata.bmWidth;
    if (workRect.bottom>bmpdata.bmHeight) workRect.bottom=bmpdata.bmHeight;
    if (workRect.right<workRect.left) workRect.right=workRect.left;
    if (workRect.bottom<workRect.top) workRect.bottom=workRect.top;
    sz.cx=min(workRect.right-workRect.left,sz.cx);
    sz.cy=min(workRect.bottom-workRect.top,sz.cy);	
  }
  if (sz.cx>0 && sz.cy>0)
  {
    //Create text bitmap
    {
      hbmp=CreateBitmap32Point(sz.cx,sz.cy,(void**)&bits);
      holdbmp=SelectObject(memdc,hbmp);

      bufDC=CreateCompatibleDC(hDC);
      bufbmp=CreateBitmap32Point(sz.cx,sz.cy,(void**)&bufbits);
      bufoldbmp=SelectObject(bufDC,bufbmp);
      BitBlt(bufDC,0,0,sz.cx,sz.cy,hDC,workRect.left,workRect.top,SRCCOPY);
    }
    //Calc text draw offsets
    {
      //horizontal (drx)
      if (format&ADT_RIGHT)
        drx=(lpRect->right-lpRect->left-fsize.cx);
      else if (format&ADT_HCENTER)
        drx=(lpRect->right-lpRect->left-fsize.cx)/2;
      else drx=0; 
      if (lpRect->left<workRect.left) drx+=lpRect->left-workRect.left;
      //vertical (dry)
      if (format&ADT_BOTTOM)
        dry=(lpRect->bottom-lpRect->top-fsize.cy);
      else if (format&ADT_VCENTER)
        dry=(lpRect->bottom-lpRect->top-fsize.cy)/2;
      else dry=0; 
      if (lpRect->top<workRect.top) dry+=lpRect->top-workRect.top;	
    }
    //Draw text on temp bitmap
    {
      UINT al=GetTextAlign(hDC);
      if (format&DT_RTLREADING)
        al|=TA_RTLREADING;
      SetTextAlign(hDC,al);
      TextOut(memdc,drx,dry,lpString,nCount);
    }
    //UpdateAlphaCannel
    {
      DWORD x,y;
      DWORD width=sz.cx;
      DWORD heigh=sz.cy;
      BYTE * ScanLine;
      BYTE * BufScanLine;
      BYTE * pix;
      BYTE * bufpix;

      BYTE r=GetRValue(ARGBcolor);
      BYTE g=GetGValue(ARGBcolor);
      BYTE b=GetBValue(ARGBcolor);
      for (y=0; y<heigh;y++)
      {
        int a=y*(width<<2);
        ScanLine=bits+a;
        BufScanLine=bufbits+a;
        for(x=0; x<width; x++)
        {
          BYTE bx,rx,gx,mx;
          pix=ScanLine+x*4;
          bufpix=BufScanLine+(x<<2);
          
		  bx=weight2[pix[0]];
          gx=weight2[pix[1]];
          rx=weight2[pix[2]];

		  bx=(weight[bx]*(255-b)+bx*(b))/255;
		  gx=(weight[gx]*(255-g)+gx*(g))/255;
		  rx=(weight[rx]*(255-r)+rx*(r))/255;
          
		  mx=(BYTE)(max(max(bx,rx),gx));
          
		  if (1) 
          {
            bx=(bx<mx)?(BYTE)(((WORD)bx*7+(WORD)mx)>>3):bx;
            rx=(rx<mx)?(BYTE)(((WORD)rx*7+(WORD)mx)>>3):rx;
            gx=(gx<mx)?(BYTE)(((WORD)gx*7+(WORD)mx)>>3):gx;
			// reduce boldeness at white fonts
          }
		  if (mx)                                      
          {
			short rrx,grx,brx;
			BYTE axx=bufpix[3];
			BYTE nx;
			nx=weight[mx];
			{


				//Normalize components	to alpha level
				bx=(nx*(255-axx)+bx*axx)/255;
				gx=(nx*(255-axx)+gx*axx)/255;
				rx=(nx*(255-axx)+rx*axx)/255;
				mx=(nx*(255-axx)+mx*axx)/255;
			}
			{
				brx=(short)((b-bufpix[0])*bx/255);
				grx=(short)((g-bufpix[1])*gx/255);
				rrx=(short)((r-bufpix[2])*rx/255);
				bufpix[0]+=brx;
				bufpix[1]+=grx;
				bufpix[2]+=rrx;
				bufpix[3]=(BYTE)(mx+(BYTE)(255-mx)*bufpix[3]/255);
			}
		  }
        }
      }

    }
    //Blend to destination
    {
      BitBlt(hDC,workRect.left,workRect.top,sz.cx,sz.cy,bufDC,0,0,SRCCOPY);
    }
    //free resources
    {
      SelectObject(memdc,holdbmp);
      DeleteObject(hbmp);
      SelectObject(bufDC,bufoldbmp);
      DeleteObject(bufbmp);
      DeleteDC(bufDC);
    }	
  }
  SelectObject(memdc,holdfnt);
  DeleteDC(memdc);
  return 0;
}
//int AlphaTextOutOld (HDC hDC, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format, DWORD ARGBcolor)
//{
//	SIZE sz;
//	SIZE wsize={0};
//	HDC memdc;
//	HBITMAP hbmp,holdbmp;
//	HBITMAP destBitmap;
//	BITMAP bmpdata;
//	BYTE * destBits;
//	BOOL noDIB=0;
//	BOOL is16bit=0;
//	BYTE * bits;
//
//
//	HFONT hfnt, holdfnt;
//	int dx,dy;
//
//
//	//inializations
//	if (!lpString) return 0;
//	if (nCount==-1) nCount=MyStrLen(lpString);
//
//	// retrieve destination bitmap bits
//	{
//		destBitmap=(HBITMAP)GetCurrentObject(hDC,OBJ_BITMAP);
//		GetObject(destBitmap, sizeof(BITMAP),&bmpdata);
//		if (bmpdata.bmBits==NULL)
//		{
//			noDIB=1;
//			destBits=(BYTE*)malloc(bmpdata.bmHeight*bmpdata.bmWidthBytes);
//			GetBitmapBits(destBitmap,bmpdata.bmHeight*bmpdata.bmWidthBytes,destBits);
//		}
//		else 
//			destBits=bmpdata.bmBits;
//	}
//	is16bit=(bmpdata.bmBitsPixel)!=32;
//
//	//DC creation
//	memdc=CreateCompatibleDC(hDC);
//	hfnt=(HFONT)GetCurrentObject(hDC,OBJ_FONT);
//	SetBkColor(memdc,0);
//	SetTextColor(memdc,RGB(255,255,255));
//	holdfnt=(HFONT)SelectObject(memdc,hfnt);
//	//calc sizes
//	GetTextExtentPoint32A(memdc,lpString,nCount,&sz);
//	sz.cx+=2;
//	if (sz.cx>lpRect->right-lpRect->left)
//	{
//		wsize.cx=lpRect->right-lpRect->left;
//		//if (format&ADT_RIGHT)
//		//    dx=-(sz.cx-lpRect->right+lpRect->left);
//		//else if (format&ADT_HCENTER)
//		//    dx=((lpRect->right-lpRect->left)-sz.cx)/2; 
//		//else dx=0;
//	}
//	else
//	{
//		wsize.cx=sz.cx;
//	}
//	if (format&ADT_RIGHT)
//		dx=lpRect->right-lpRect->left-sz.cx;
//	else if (format&ADT_HCENTER)
//		dx=((lpRect->right-lpRect->left)-sz.cx)/2;
//	else dx=0;  
//
//	if (sz.cy>lpRect->bottom-lpRect->top)
//		wsize.cy=lpRect->bottom-lpRect->top;
//	else
//		wsize.cy=sz.cy;  
//	if (lpRect->top<0) {wsize.cy+=lpRect->top; dy=lpRect->top;}
//	else dy=0;
//
//	if (format&ADT_BOTTOM)
//		dy=lpRect->bottom-lpRect->top-sz.cy;
//	else if (format&ADT_VCENTER)
//		dy=((lpRect->bottom-lpRect->top)-sz.cy)/2;
//
//	// drawing
//	hbmp=CreateBitmap32Point(wsize.cx,wsize.cy,(void**)&bits);
//	holdbmp=SelectObject(memdc,hbmp);
//	//BitBlt(memdc,0,0,wsize.cx,wsize.cy,hDC,lpRect->left,lpRect->top,SRCCOPY);
//	TextOut(memdc,dx<=0?dx:0,dy<=0?dy:0,lpString,nCount);
//	//TextOut(hDC,lpRect->left,lpRect->top,lpString,nCount);
//
//	// ok. now perform per pixel transfering from temp to destination 
//	{
//		int x,y;
//		int sx,sy,mx,my,ddx,ddy;
//		BYTE cr,cg,cb,ca;
//		DWORD w,h;
//		DWORD destH, destWB;  //avoiding use struct in cycles
//		BYTE * destB, * sourB;
//		w=wsize.cx; h=wsize.cy;
//		destH=bmpdata.bmHeight;
//		destWB=bmpdata.bmWidthBytes;
//		cb=*(((BYTE*)&ARGBcolor)+0);
//		cg=*(((BYTE*)&ARGBcolor)+1);
//		cr=*(((BYTE*)&ARGBcolor)+2);
//		ca=*(((BYTE*)&ARGBcolor)+3);
//
//		ddx=lpRect->left+(dx>0?dx:0);
//		sx=0;//((lpRect->left)<0)?(-lpRect->left):0;
//		if (ddx<0) {sx+=0/*-ddx*/; ddx=-ddx;}
//
//		ddy=lpRect->top+(dy>0?dy:0);
//		sy=0;//((lpRect->top)<0)?(-lpRect->top):0;
//		if (ddy<0) {sy+=0/*-ddy*/; ddy=0;}
//
//
//		if (ddx+wsize.cx>bmpdata.bmWidth) mx=bmpdata.bmWidth-ddx;
//		else mx=wsize.cx;
//
//		if (ddy+wsize.cy>bmpdata.bmHeight) my=bmpdata.bmHeight-ddy;
//		else my=wsize.cy;    
//
//		my=min(my,wsize.cy);
//		mx=min(mx,wsize.cx);
//		for (y=sy;y<my;y++)
//		{
//			if (noDIB) destB=destBits+(y+ddy)*destWB;
//			else destB=destBits+(destH-(y+ddy)-1)*destWB;
//			sourB=bits+(h-y-1)*(w*4);
//			for (x=sx;x<mx;x++)  
//			{
//				if (!is16bit) 
//				{
//					BYTE sr,sg,sb,sa;
//					BYTE tr,tg,tb,ta,taf;
//					BYTE * psx=(sourB+(x)*4);
//					tr=*psx;
//					tg=*(psx+1);
//					tb=*(psx+2);   
//					ta=(BYTE)(((DWORD)((DWORD)tr+(DWORD)tg+(DWORD)tb))/3);
//					taf=255-ta;
//
//					if (ta!=0)
//					{
//						BYTE * pdx=(destB+(x+ddx)*4);
//						sr=*pdx;
//						sg=*(pdx+1);
//						sb=*(pdx+2);
//						sa=*(pdx+3);
//						sr=(cr*tr+(255-tr)*sr)/255;
//						sg=(cg*tg+(255-tg)*sg)/255;
//						sb=(cb*tb+(255-tb)*sb)/255;
//						sa=ta+taf*sa/255;
//						pdx[0]=sr;
//						pdx[1]=sg;
//						pdx[2]=sb;
//						pdx[3]=sa;
//					}
//				}
//			}
//		}
//	}
//	if (noDIB)
//	{
//		SetBitmapBits(destBitmap,bmpdata.bmHeight*bmpdata.bmWidthBytes,destBits);
//		free(destBits);
//	}
//	SelectObject(memdc,holdfnt);
//	SelectObject(memdc,holdbmp);
//	DeleteObject(hbmp);
//	DeleteDC(memdc);
//	return 0; 
//}
//
//
//
//BOOL DrawTextSA(HDC hdc, LPCSTR lpString, int nCount, RECT * lpRect, UINT format);

//BOOL DrawTextSW(HDC hdc, LPCWSTR lpString, int nCount, RECT * lpRect, UINT format);

BOOL DrawTextSA(HDC hdc, char * lpString, int nCount, RECT * lpRect, UINT format)
{
#ifdef UNICODE
	TCHAR *buf=mir_alloc((2+nCount)*sizeof(TCHAR));
	BOOL res;
	MultiByteToWideChar(CP_ACP, 0, lpString, -1, buf, (2+nCount)*sizeof(TCHAR)); 
	res=DrawTextS(hdc,buf,nCount,lpRect,format);
	mir_free(buf);
	return res;
#else
	return DrawTextS(hdc,lpString,nCount,lpRect,format);
#endif
}


BOOL DrawTextS(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format)
{
  DWORD form=0, color=0;
  RECT r=*lpRect;
  OffsetRect(&r,1,1);
  if (format&DT_CALCRECT) return DrawText(hdc,lpString,nCount,lpRect,format);

  //form|=(format&DT_VCENTER)?ADT_VCENTER:0;
  //form|=(format&DT_BOTTOM)?ADT_BOTTOM:0;
  //form|=(format&DT_RIGHT)?ADT_RIGHT:0;
  //form|=(format&DT_CENTER)?ADT_HCENTER:0;
  form=format;
  color=GetTextColor(hdc);
  if (!gdiPlusFail &&0) ///text via gdi+
  {
    TextOutWithGDIp(hdc,lpRect->left,lpRect->top,lpString,nCount);
    return 0;
  }
//  AlphaTextOut(hdc,lpString,nCount,&r,form,0);
  return AlphaTextOut(hdc,lpString,nCount,lpRect,form,color);

  /*    int i;
  DWORD tick;
  char buf[255];
  if (format&DT_CALCRECT&& nCount!=10)
  return DrawTextSA(hdc,lpString,nCount,lpRect,format);
  {
  tick=GetTickCount();
  for (i=0;i<100;i++)
  AlphaTextOut(hdc,lpString,nCount,lpRect,0,RGB(255,128,64));
  //DrawTextSA(hdc,lpString,nCount,lpRect,format);
  tick=GetTickCount()-tick;
  MAX=tick>MAX?tick:MAX;
  MIN=(tick<MIN || MIN==0)?tick:MIN;
  SUM+=(tick);
  count++;
  if (count>50)
  {
  sprintf(buf,"%d, %d-%d........\n",SUM/count,MIN,MAX);
  SUM=0;
  //    MAX=0; MIN=-1;
  count=0;
  {
  HDC wnd=GetDC(NULL);
  RECT r;
  SetRect(&r,0,0,300,20);
  DrawText(wnd,buf,-1,&r,0);
  ReleaseDC(NULL,wnd);
  } 
  }
  tick=0;
  }
  //return AlphaTextOut(hdc,lpString,nCount,lpRect,0,RGB(255,128,64));
  */
}


BOOL ImageList_DrawEx_New( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle)
{
  HDC imDC;
  HBITMAP oldBmp, imBmp, newbmp=NULL;
  BITMAP imbt,immaskbt;
  BYTE * imbits;
  BYTE * imimagbits;
  BYTE * immaskbits;
  DWORD cx,cy,icy;
  BYTE *t1, *t2, *t3;
  IMAGEINFO imi;
  //lockimagelist
  BYTE hasmask=FALSE;
  BYTE hasalpha=FALSE;
  //if (1) return ImageList_DrawEx(himl,i,hdcDst,x,y,dx,dy,rgbBk,rgbFg,fStyle);
  //CRITICAL_SECTION cs;
  //InitializeCriticalSection(&cs);
  //EnterCriticalSection(&cs);

  ImageList_GetImageInfo(himl,i,&imi);
  GetObject(imi.hbmImage,sizeof(BITMAP),&imbt);
  GetObject(imi.hbmMask,sizeof(BITMAP),&immaskbt);
  cy=imbt.bmHeight;

  if (imbt.bmBitsPixel!=32)
  {
	  BOOL res=ImageList_DrawEx(himl,i,hdcDst,x,y,dx,dy,rgbBk,rgbFg,fStyle);
	  return res;
	  /*
	  HDC tempdc=CreateCompatibleDC(hdcDst);
	  HBITMAP holdbmp, newbmp=CreateBitmap32Point(imi.rcImage.right-imi.rcImage.left,imi.rcImage.bottom-imi.rcImage.top,&imimagbits); 
	  holdbmp=SelectOject(tempdc,newbmp);
	  ImageList_DrawEx(himl,i,hdcDst,x,y,dx,dy,rgbBk,rgbFg,fStyle);		  	
	  */
  }
  else
  {
	if (imbt.bmBits==NULL)
	{
		imimagbits=(BYTE*)malloc(cy*imbt.bmWidthBytes);
		GetBitmapBits(imi.hbmImage,cy*imbt.bmWidthBytes,(void*)imimagbits);
	}
	else imimagbits=imbt.bmBits;
  } 

  if (immaskbt.bmBits==NULL)
  {
    immaskbits=(BYTE*)malloc(cy*immaskbt.bmWidthBytes);
    GetBitmapBits(imi.hbmMask,cy*immaskbt.bmWidthBytes,(void*)immaskbits);
  }
  else immaskbits=immaskbt.bmBits;
  icy=imi.rcImage.bottom-imi.rcImage.top;
  cx=imi.rcImage.right-imi.rcImage.left;
  imDC=CreateCompatibleDC(hdcDst);
  imBmp=CreateBitmap32Point(cx,icy,&imbits);
  oldBmp=SelectObject(imDC,imBmp);
  {
    int x; int y;
    int bottom,right,top,h,left;
    int hs,he;
    //	int hs2,he2;
    int mwb,mwb2,mwb3;
    mwb=immaskbt.bmWidthBytes;
    mwb2=imbt.bmWidthBytes;
    mwb3=16*4;
    bottom=imi.rcImage.bottom;
    right=imi.rcImage.right;   
    top=imi.rcImage.top;
    h=imbt.bmHeight;
    left=imi.rcImage.left;
    hs=imi.rcImage.left*immaskbt.bmBitsPixel/8;
    he=hs+(imi.rcImage.right-imi.rcImage.left)*immaskbt.bmBitsPixel/8;


    for (y=top;(y<bottom)&&!hasmask; y++)
    { 
      t1=immaskbits+y*mwb;
      for (x=hs; (x<he)&&!hasmask; x++)
        hasmask|=(*(t1+x)!=0);
    }
    //hs2=imi.rcImage.left*imbt.bmBitsPixel/8;
    //he2=hs2+(imi.rcImage.right-imi.rcImage.left)*imbt.bmBitsPixel/8;
    for (y=top;(y<bottom)&&!hasalpha; y++)
    {
      t1=imimagbits+(cy-y-1)*mwb2;
      for (x=imi.rcImage.left; (x<right)&&!hasalpha; x++)
		  hasalpha|=(*(t1+(x<<2)+3)!=0);
    }

    for (y=0; y<(int)icy; y++)
    {
      t1=imimagbits+(h-y-1-top)*mwb2;
      t2=imbits+(icy-y-1)*mwb3;
      t3=immaskbits+(y+top)*mwb;
      for (x=0; x<(int)cx; x++)
      {
        DWORD * src, *dest;               
        BYTE mask=((1<<(7-x%8))&(*(t3+((x+left)>>3))))!=0;
        src=(DWORD*)(t1+((x+left)<<2));
        dest=(DWORD*)(t2+(x<<2));
        if (hasalpha && !hasmask)
          *dest=*src;
        else
        {
          if (mask)
            *dest=0;
          else
          {
            BYTE a;
            a=((BYTE*)src)[3]>0?((BYTE*)src)[3]:255;
            ((BYTE*)dest)[3]=a;
            ((BYTE*)dest)[0]=((BYTE*)src)[0];//*a/255;
            ((BYTE*)dest)[1]=((BYTE*)src)[1];//*a/255;
            ((BYTE*)dest)[2]=((BYTE*)src)[2];//*a/255;
            dest=dest;
          }
        }
      }
    }
  }
 // LeaveCriticalSection(&cs);
 // DeleteCriticalSection(&cs);
  {
    BLENDFUNCTION bf={AC_SRC_OVER, 0, (fStyle&ILD_BLEND25)?64:(fStyle&ILD_BLEND50)?128:255, AC_SRC_ALPHA };
    MyAlphaBlend(hdcDst,x,y,cx,icy,imDC,0,0,cx,icy,bf);
  }
  if (immaskbt.bmBits==NULL) free(immaskbits);
  if (imbt.bmBits==NULL) free(imimagbits);
  SelectObject(imDC,oldBmp);
  DeleteObject(imBmp);
  //DeleteObject(imi.hbmImage);
  //DeleteObject(imi.hbmMask);
  DeleteDC(imDC);
  //unlock it;
  return 0;
};

//int ImageList_ReplaceIcon_FixAlphaServ (WPARAM w,LPARAM l)
//{
//	ImageListFixParam * pa=(ImageListFixParam*)w;
//	if (!pa) return -1;
//	return ImageList_ReplaceIcon_FixAlpha(pa->himl,pa->index,pa->hicon);
//}
//int ImageList_ReplaceIcon_FixAlpha(HIMAGELIST himl, int i, HICON hicon)
//{
//	int res=ImageList_ReplaceIcon(himl,i,hicon);
////	if (res>=0) FixAlpha(himl,hicon,res);
//	return res;
//}
//int ImageList_AddIcon_FixAlphaServ(WPARAM w,LPARAM l)
//{
//	ImageListFixParam * pa=(ImageListFixParam*)w;
//	if (!pa) return -1;
//	return ImageList_AddIcon_FixAlpha(pa->himl,pa->hicon);
//}
//int ImageList_AddIcon_FixAlpha(HIMAGELIST himl,HICON hicon)
//{
//	int res=ImageList_AddIcon(himl,hicon);
////	if (res>=0) FixAlpha(himl,hicon,res);
//	return res;
//}
//int FixAlphaServ (WPARAM w,LPARAM l)
//{
//	ImageListFixParam * pa=(ImageListFixParam*)w;
//	if (!pa) return -1;
//	return FixAlpha(pa->himl,pa->hicon,pa->index);
//}
//int FixAlpha(HIMAGELIST himl,HICON hicon, int res)
//{
//	{//Fix alpha channel
//		IMAGEINFO imi;
//		//return 0;
//		ImageList_GetImageInfo(himl,res,&imi);
//		{
//			BITMAP color,mask;
//			BYTE *mbi;
//			GetObject(imi.hbmImage,sizeof(color),&color);
//			GetObject(imi.hbmMask ,sizeof(mask),&mask);
//			mbi=mir_alloc(mask.bmWidthBytes*mask.bmHeight);
//			GetBitmapBits(imi.hbmMask, mask.bmWidthBytes*mask.bmHeight,mbi);
//			if (color.bmBitsPixel !=32) return 0;
//			{
//				int x,y;
//				BOOL wAlpha=0;
//				BOOL noMask=1;
//				{
//					for (y=imi.rcImage.top;y<imi.rcImage.bottom;y++)
//					{
//						for (x=imi.rcImage.left; x<imi.rcImage.right; x++)
//						{
//							BYTE* byte=((BYTE*) color.bmBits)+(color.bmHeight-y-1)*color.bmWidthBytes+x*4;
//							wAlpha|=(byte[3]>0);
//							if (wAlpha&& noMask) break;
//						}
//						if (wAlpha&& noMask) break;
//					}
//				}
//				if (!wAlpha && noMask)
//				{
//					for (y=imi.rcImage.top;y<imi.rcImage.bottom;y++)
//						for (x=imi.rcImage.left; x<imi.rcImage.right; x++)
//						{
//							BYTE* byte=((BYTE*) color.bmBits)+(color.bmHeight-y-1)*color.bmWidthBytes+x*4;
//							BYTE m,f,k;
//							{
//								BYTE* byte2=mbi+(y)*mask.bmWidthBytes+x*mask.bmBitsPixel/8;
//								m=*byte2;
//								k=1<<(7-x%8);
//								f=(m&k)==0;
//								byte[3]=f?255:0;
//							}
//						}
//				}   
//				mir_free(mbi);
//			}
//		}
//	}
//
//	return res;   
//}
//
//int FixAlphaOld(HIMAGELIST himl,HICON hicon, int res)
//{
//	{//Fix alpha channel
//		IMAGEINFO imi;
//		//return 0;
//		ImageList_GetImageInfo(himl,res,&imi);
//		{
//			BITMAP color,mask;
//			BYTE *mbi;
//			GetObject(imi.hbmImage,sizeof(color),&color);
//			GetObject(imi.hbmMask ,sizeof(mask),&mask);
//			mbi=mir_alloc(mask.bmWidthBytes*mask.bmHeight);
//			GetBitmapBits(imi.hbmMask, mask.bmWidthBytes*mask.bmHeight,mbi);
//			if (color.bmBitsPixel !=32) return 0;
//			{
//				int x,y;
//				BOOL wAlpha=0;
//				BOOL noMask=1;
//				{
//					for (y=imi.rcImage.top;y<imi.rcImage.bottom;y++)
//					{
//						for (x=imi.rcImage.left; x<imi.rcImage.right; x++)
//						{
//							BYTE* byte=((BYTE*) color.bmBits)+(color.bmHeight-y-1)*color.bmWidthBytes+x*4;
//							wAlpha|=(byte[3]>0);
//							if (wAlpha&& noMask) break;
//						}
//						if (wAlpha&& noMask) break;
//					}
//				}
//				if (!wAlpha && noMask)
//				{
//					for (y=imi.rcImage.top;y<imi.rcImage.bottom;y++)
//						for (x=imi.rcImage.left; x<imi.rcImage.right; x++)
//						{
//							BYTE* byte=((BYTE*) color.bmBits)+(color.bmHeight-y-1)*color.bmWidthBytes+x*4;
//							BYTE m,f,k;
//							{
//								BYTE* byte2=mbi+(y)*mask.bmWidthBytes+x*mask.bmBitsPixel/8;
//								m=*byte2;
//								k=1<<(7-x%8);
//								f=(m&k)==0;
//								byte[3]=f?255:0;
//							}
//						}
//				}   
//				mir_free(mbi);
//			}
//		}
//	}
//
//	return res;   
//}
//
BOOL DrawIconExServ(WPARAM w,LPARAM l)
{
  DrawIconFixParam *p=(DrawIconFixParam*)w;
  if (!p) return 0;
  return DrawIconExS(p->hdc,p->xLeft,p->yTop,p->hIcon,p->cxWidth,p->cyWidth,p->istepIfAniCur,p->hbrFlickerFreeDraw,p->diFlags);
}
BOOL DrawIconExS(HDC hdcDst,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{

  ICONINFO ici;


  HDC imDC;
  HBITMAP oldBmp, imBmp,tBmp;
  BITMAP imbt,immaskbt;
  BYTE * imbits;
  BYTE * imimagbits;
  BYTE * immaskbits;
  DWORD cx,cy,icy;
  BYTE *t1, *t2, *t3;

  //lockimagelist
  BYTE hasmask=FALSE;
  BYTE no32bit=FALSE;
  BYTE noMirrorMask=FALSE;
  BYTE hasalpha=FALSE;
  CRITICAL_SECTION cs;
  InitializeCriticalSection(&cs);
  EnterCriticalSection(&cs);
  //return DrawIconEx(hdc,xLeft,yTop,hIcon,cxWidth,cyWidth,istepIfAniCur,hbrFlickerFreeDraw,DI_NORMAL);
  if (!GetIconInfo(hIcon,&ici)) 
  {
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
    return 0;
  }

  GetObject(ici.hbmColor,sizeof(BITMAP),&imbt);
  if (imbt.bmWidth*imbt.bmHeight==0)
  {
	  DeleteObject(ici.hbmColor);
	  DeleteObject(ici.hbmMask);
	  LeaveCriticalSection(&cs);
	  DeleteCriticalSection(&cs);
	  return 0;
  }
  GetObject(ici.hbmMask,sizeof(BITMAP),&immaskbt);
  cy=imbt.bmHeight;

  if (imbt.bmBitsPixel!=32)
  {
    HDC tempDC1;
    HBITMAP otBmp;
    no32bit=TRUE;
    tempDC1=CreateCompatibleDC(hdcDst);
    tBmp=CreateBitmap32(imbt.bmWidth,imbt.bmHeight);
	if (tBmp) 
	{
		GetObject(tBmp,sizeof(BITMAP),&imbt);
		otBmp=SelectObject(tempDC1,tBmp);
		DrawIconEx(tempDC1,0,0,hIcon,imbt.bmWidth,imbt.bmHeight,istepIfAniCur,hbrFlickerFreeDraw,DI_IMAGE);   
		noMirrorMask=TRUE;
	}
	SelectObject(tempDC1,otBmp);
    DeleteDC(tempDC1);
  }

  if (imbt.bmBits==NULL)
  {
    imimagbits=(BYTE*)malloc(cy*imbt.bmWidthBytes);
    GetBitmapBits(ici.hbmColor,cy*imbt.bmWidthBytes,(void*)imimagbits);
  }
  else imimagbits=imbt.bmBits;


  if (immaskbt.bmBits==NULL)
  {
    immaskbits=(BYTE*)malloc(cy*immaskbt.bmWidthBytes);
    GetBitmapBits(ici.hbmMask,cy*immaskbt.bmWidthBytes,(void*)immaskbits);
  }
  else immaskbits=immaskbt.bmBits;
  icy=imbt.bmHeight;
  cx=imbt.bmWidth;
  imDC=CreateCompatibleDC(hdcDst);
  imBmp=CreateBitmap32Point(cx,icy,&imbits);
  oldBmp=SelectObject(imDC,imBmp);
  if (imbits!=NULL && imimagbits!=NULL && immaskbits!=NULL)
  {
    int x; int y;
    int bottom,right,top,h;
    int mwb,mwb2;
    mwb=immaskbt.bmWidthBytes;
    mwb2=imbt.bmWidthBytes;
    bottom=icy;
    right=cx;   
    top=0;
    h=icy;
    for (y=top;(y<bottom)&&!hasmask; y++)
    { 
      t1=immaskbits+y*mwb;
      for (x=0; (x<mwb)&&!hasmask; x++)
        hasmask|=(*(t1+x)!=0);
    }

    for (y=top;(y<bottom)&&!hasalpha; y++)
    {
      t1=imimagbits+(cy-y-1)*mwb2;
      for (x=0; (x<right)&&!hasalpha; x++)
        hasalpha|=(*(t1+(x<<2)+3)!=0);
    }

    for (y=0; y<(int)icy; y++)
    {
      t1=imimagbits+(h-y-1-top)*mwb2;
      t2=imbits+(!no32bit?y:(icy-y-1))*mwb2;
      t3=immaskbits+(noMirrorMask?y:(h-y-1-top))*mwb;
      for (x=0; x<right; x++)
      {
        DWORD * src, *dest;               
        BYTE mask=((1<<(7-x%8))&(*(t3+(x>>3))))!=0;
        src=(DWORD*)(t1+(x<<2));
        dest=(DWORD*)(t2+(x<<2));
        if (hasalpha && !hasmask)
          *dest=*src;
        else
        {
          if (mask)
			  // TODO: ADD verification about validity
            *dest=0;  

          else
          {
            //*dest=*src;
            BYTE a;
            a=((BYTE*)src)[3]>0?((BYTE*)src)[3]:255;
            ((BYTE*)dest)[3]=a;
            ((BYTE*)dest)[0]=((BYTE*)src)[0]*a/255;
            ((BYTE*)dest)[1]=((BYTE*)src)[1]*a/255;
            ((BYTE*)dest)[2]=((BYTE*)src)[2]*a/255;
            dest=dest;
          }
        }
      }
    }
  }

  LeaveCriticalSection(&cs);
  DeleteCriticalSection(&cs);
  {
    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    MyAlphaBlend(hdcDst,xLeft,yTop,cxWidth, cyWidth, imDC,0,0, cx,icy,bf);
  }

  if (immaskbt.bmBits==NULL) free(immaskbits);
  if (imbt.bmBits==NULL) free(imimagbits);
  SelectObject(imDC,oldBmp);
  DeleteObject(imBmp);
  if(no32bit)DeleteObject(tBmp);
  DeleteObject(ici.hbmColor);
  DeleteObject(ici.hbmMask);
  DeleteDC(imDC);
  //unlock it;


  /*GetObject(ici.hbmColor,sizeof(color),&color);
  GetObject(ici.hbmMask,sizeof(mask),&mask);
  memDC=CreateCompatibleDC(NULL);
  hbitmap=CreateBitmap32(color.bmWidth,color.bmHeight);
  if (!hbitmap) return FALSE;
  GetObject(hbitmap,sizeof(bits),&bits);
  bbit=bits.bmBits;
  cbit=mir_alloc(color.bmWidthBytes*color.bmHeight);
  mbit=mir_alloc(mask.bmWidthBytes*mask.bmHeight);

  ob=SelectObject(memDC,hbitmap);
  GetBitmapBits(ici.hbmColor, color.bmWidthBytes*color.bmHeight,cbit);
  GetBitmapBits(ici.hbmMask, mask.bmWidthBytes*mask.bmHeight,mbit);
  {  int x,y;
  DWORD offset1;
  DWORD offset2;
  for (y=0; y<color.bmHeight;y++)
  {
  offset1=(y)*color.bmWidthBytes;
  offset2=(bits.bmHeight-y-1)*bits.bmWidthBytes;
  for (x=0; x<color.bmWidth; x++)
  {
  BYTE *byte;
  DWORD *a,*b;
  BYTE m,f,k;
  a=(DWORD*)(bbit+offset2+x*4);
  b=(DWORD*)(cbit+offset1+x*4);
  *a=*b;
  byte=bbit+offset2+x*4;
  if (byte[3]==0)
  {
  BYTE* byte2=mbit+(y)*mask.bmWidthBytes+x*mask.bmBitsPixel/8;
  m=*byte2;
  k=1<<(7-x%8);
  f=(m&k)!=0;
  byte[3]=f?0:255;
  if (f) *a=0;
  }

  if (byte[3]!=255)
  {
  byte[0]=byte[0]*byte[3]/255;
  byte[1]=byte[1]*byte[3]/255;
  byte[2]=byte[2]*byte[3]/255;
  }

  }
  }


  }
  {
  BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
  MyAlphaBlend(hdc,xLeft,yTop,cxWidth, cyWidth, memDC,0,0, color.bmWidth,color.bmHeight,bf);
  }
  {
  int res;

  SelectObject(memDC,ob);
  res=DeleteDC(memDC);
  res=DeleteObject(hbitmap);
  DeleteObject(ici.hbmColor);
  DeleteObject(ici.hbmMask);
  if (!res)
  res=res;

  if(cbit) mir_free(cbit);
  if(mbit) mir_free(mbit);
  }*/

  return 1;// DrawIconExS(hdc,xLeft,yTop,hIcon,cxWidth,cyWidth,istepIfAniCur,hbrFlickerFreeDraw,diFlags);
}

int RegisterPaintSub(WPARAM wParam, LPARAM lParam)
{
  if (!wParam) return 0;
  {
    wndFrame *frm=FindFrameByItsHWND((HWND)wParam);
    if (!frm) return 0;
    if (lParam)
      frm->PaintCallbackProc=(tPaintCallbackProc)lParam;
    else
      frm->PaintCallbackProc=NULL;
    return 1;
  }
}
BYTE TempLockUpdate=0;
int PrepeareImageButDontUpdateIt(RECT * r)
{
  TempLockUpdate=1;
  DrawNonFramedObjects(TRUE,r);
  ValidateFrameImageProc(r);
  TempLockUpdate=0;
  return 0;
}

int RedrawCompleteWindow()
{   
  DrawNonFramedObjects(TRUE,0);
  InvalidateFrameImage(0,0);   
  return 0;
}
// Request to repaint frame or change/drop callback data
// wParam = hWnd of called frame
// lParam = pointer to sPaintRequest (or NULL to redraw all)
// return 2 - already queued, data updated, 1-have been queued, 0 - failure

int UpdateFrameImage(WPARAM wParam, LPARAM lParam)           // Immideately recall paint routines for frame and refresh image
{
  RECT wnd;
  wndFrame *frm;
  BOOL NoCancelPost=0;
  BOOL IsAnyQueued=0;
  if (!ON_EDGE_SIZING)
    GetWindowRect(hwndContactList,&wnd);
  else
    wnd=ON_EDGE_SIZING_POS;
  //   GetWindowRect(hwndContactList,&wnd);
  //#ifdef _DEBUG
  //    {
  //        char deb[100];
  //        sprintf(deb,"%d : --- UPDATE FRAME IMAGE ---\n",MSG_COUNTER++);
  //        TRACE(deb);
  //    }
  //#endif
  //Check validity If not ok ->ValidateFrameImageProc
  if (cachedWindow==NULL) ValidateFrameImageProc(&wnd);
  else if (cachedWindow->Width!=wnd.right-wnd.left || cachedWindow->Height!=wnd.bottom-wnd.top) ValidateFrameImageProc(&wnd);
  else if (wParam==0) ValidateFrameImageProc(&wnd);
  else // all Ok Update Single Frame
  {
    frm=FindFrameByItsHWND((HWND)wParam);
    if (!frm)  ValidateFrameImageProc(&wnd);
    // Validate frame, update window image and remove it from queue
    else 
    {
      if(frm->UpdateRgn)
      {
        DeleteObject(frm->UpdateRgn);
        frm->UpdateRgn=0;
      }
      ValidateSingleFrameImage(frm,0);
      UpdateWindowImage();
      NoCancelPost=1;
      //-- Remove frame from queue
      if (UPDATE_ALLREADY_QUEUED)
      {
        int i;
        frm->bQueued=0;
        for(i=0;i<nFramescount;i++)
          if(IsAnyQueued|=Frames[i].bQueued) break;
      }
    }
  }       
  if ((!NoCancelPost || !IsAnyQueued) && UPDATE_ALLREADY_QUEUED) // no any queued updating cancel post or need to cancel post
  {
    UPDATE_ALLREADY_QUEUED=0;
    POST_WAS_CANCELED=1;
  }
  return 1;   
}
int InvalidateFrameImage(WPARAM wParam, LPARAM lParam)       // Post request for updating
{

  //#ifdef _DEBUG
  //    {
  //        char deb[100];
  //        sprintf(deb,"%d : --- INVALIDATE FRAME IMAGE ---",MSG_COUNTER++);
  //        TRACE(deb);
  //    }
  //#endif
  if (wParam)
  {
    wndFrame *frm=FindFrameByItsHWND((HWND)wParam);
    sPaintRequest * pr=(sPaintRequest*)lParam;
    if (frm) 
    {
      if (frm->PaintCallbackProc!=NULL)
      {
        frm->PaintData=(sPaintRequest *)pr;
        frm->bQueued=1;
        if (pr)
        {
          HRGN r2;
          if (!IsRectEmpty(&pr->rcUpdate))
            r2=CreateRectRgn(pr->rcUpdate.left,pr->rcUpdate.top,pr->rcUpdate.right,pr->rcUpdate.bottom);
          else
          {
            RECT r;
            GetClientRect(frm->hWnd,&r);
            r2=CreateRectRgn(r.left,r.top,r.right,r.bottom);
          }
          if(!frm->UpdateRgn)
          {
            frm->UpdateRgn=CreateRectRgn(0,0,1,1);
            CombineRgn(frm->UpdateRgn,r2,0,RGN_COPY);                                            
          }
          else CombineRgn(frm->UpdateRgn,frm->UpdateRgn,r2,RGN_OR);
          DeleteObject(r2);
        }   

      }
    }      
    else
    {
      QueueAllFramesUpdating(1);
    }
  }
  else QueueAllFramesUpdating(1);
  if (!UPDATE_ALLREADY_QUEUED||POST_WAS_CANCELED)
    if (PostMessage(hwndContactList,UM_UPDATE,0,0))
    {            
      UPDATE_ALLREADY_QUEUED=1;
      POST_WAS_CANCELED=0;
      {
        //TRACE("  message is POSTED");
      }
    }
    //TRACE("\n");
    return 1;
}


int ValidateSingleFrameImage(wndFrame * Frame, BOOL SkipBkgBlitting)                              // Calling frame paint proc
{

  //#ifdef _DEBUG
  //    {
  //        char deb[100];
  //        sprintf(deb,"%d : ValidateSingleFrameImage\n",MSG_COUNTER++);
  //        TRACE(deb);
  //    }
  //#endif

  if (!cachedWindow) { TRACE("ValidateSingleFrameImage calling without cached\n"); return 0;}
  if (!Frame->PaintCallbackProc)  { TRACE("ValidateSingleFrameImage calling without FrameProc\n"); return 0;}
  { // if ok update image 
    HDC hdc;
    HBITMAP o,n;
    RECT rcPaint,wnd;
    RECT ru={0};
    int w,h,x,y;
    int w1,h1,x1,y1;

    SizingGetWindowRect(hwndContactList,&wnd);
    rcPaint=Frame->wndSize;
    //OffsetRect(&rcPaint,wnd.left,wnd.top);
    //GetWindowRect(Frame->hWnd,&rcPaint);
    {
      int dx,dy,bx,by;
      if (ON_EDGE_SIZING)
      {
        dx=rcPaint.left-wnd.left;
        dy=rcPaint.top-wnd.top;
        bx=rcPaint.right-wnd.right;
        by=rcPaint.bottom-wnd.bottom;
        wnd=ON_EDGE_SIZING_POS;
        rcPaint.left=wnd.left+dx;
        rcPaint.top=wnd.top+dy;
        rcPaint.right=wnd.right+bx;
        rcPaint.bottom=wnd.bottom+by;
      }
    }
    //OffsetRect(&rcPaint,-wnd.left,-wnd.top);
    w=rcPaint.right-rcPaint.left;
    h=rcPaint.bottom-rcPaint.top;
    x=rcPaint.left;
    y=rcPaint.top;
    hdc=CreateCompatibleDC(cachedWindow->hImageDC);
    n=CreateBitmap32(w,h);
    o=SelectObject(hdc,n);
    {
      HRGN rgnUpdate=0;

      if (Frame->UpdateRgn && !SkipBkgBlitting)
      {

        rgnUpdate=Frame->UpdateRgn;
        GetRgnBox(rgnUpdate,&ru);
        {
          RECT rc;
          GetClientRect(Frame->hWnd,&rc);
          if (ru.top<0) ru.top=0;
          if (ru.left<0) ru.left=0;
          if (ru.right>rc.right) ru.right=rc.right;
          if (ru.bottom>rc.bottom) ru.bottom=rc.bottom;
        } 
        if (!IsRectEmpty(&ru))
        {
          x1=ru.left;
          y1=ru.top;
          w1=ru.right-ru.left;
          h1=ru.bottom-ru.top;
        }
        else
        {x1=0; y1=0; w1=w; h1=h;}
        // copy image at hdc
        if (SkipBkgBlitting)  //image already at foreground
        {
          BitBlt(hdc,x1,y1,w1,h1,cachedWindow->hImageDC,x+x1,y+y1,SRCCOPY);  
        }
        else
        {
          BitBlt(hdc,x1,y1,w1,h1,cachedWindow->hBackDC,x+x1,y+y1,SRCCOPY);  
        }
        Frame->PaintCallbackProc(Frame->hWnd,hdc,&ru,rgnUpdate, Frame->dwFlags,Frame->PaintData);
      }
      else
      {
        RECT r;
        GetClientRect(Frame->hWnd,&r);
        rgnUpdate=CreateRectRgn(r.left,r.top,r.right,r.bottom); 
        ru=r;
        if (!IsRectEmpty(&ru))
        {
          x1=ru.left;
          y1=ru.top;
          w1=ru.right-ru.left;
          h1=ru.bottom-ru.top;
        }
        else
        {x1=0; y1=0; w1=w; h1=h;}
        // copy image at hdc
        if (SkipBkgBlitting)  //image already at foreground
        {
          BitBlt(hdc,x1,y1,w1,h1,cachedWindow->hImageDC,x+x1,y+y1,SRCCOPY);  
        }
        else
        {
          BitBlt(hdc,x1,y1,w1,h1,cachedWindow->hBackDC,x+x1,y+y1,SRCCOPY);  
        }
        Frame->PaintCallbackProc(Frame->hWnd,hdc,&r,rgnUpdate, Frame->dwFlags,Frame->PaintData);
        ru=r;
      }
      DeleteObject(rgnUpdate);
      Frame->UpdateRgn=0;
    }
    if (!IsRectEmpty(&ru))
    {
      x1=ru.left;
      y1=ru.top;
      w1=ru.right-ru.left;
      h1=ru.bottom-ru.top;
    }
    else
    {x1=0; y1=0; w1=w; h1=h;}
  /*  if (!SkipBkgBlitting)
    {
      BitBlt(cachedWindow->hImageDC,x+x1,y+y1,w1,h1,cachedWindow->hBackDC,x+x1,y+y1,SRCCOPY);
    }

  */  
    {
      //BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
      BitBlt(cachedWindow->hImageDC,x+x1,y+y1,w1,h1,hdc,x1,y1,SRCCOPY);
      //BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
      //MyAlphaBlend(cachedWindow->hImageDC,x+x1,y+y1,w1,h1,hdc,x1,y1,w1,h1,bf);  
    }
    //StartGDIPlus();
    {
      if (GetWindowLong(Frame->hWnd,GWL_STYLE)&WS_VSCROLL)
      {
        //Draw vertical scroll bar
        //
        RECT rThumb;
        RECT rUpBtn;
        RECT rDnBtn;
        RECT rLine;
        int dx,dy;
        SCROLLBARINFO si={0};
        si.cbSize=sizeof(SCROLLBARINFO);
        GetScrollBarInfo(Frame->hWnd,OBJID_VSCROLL,&si);
        rLine=(si.rcScrollBar);
        rUpBtn=rLine;
        rDnBtn=rLine;
        rThumb=rLine;
        rUpBtn.bottom=rUpBtn.top+si.dxyLineButton;
        rDnBtn.top=rDnBtn.bottom-si.dxyLineButton;
        rThumb.top=rLine.top+si.xyThumbTop;
        rThumb.bottom=rLine.top+si.xyThumbBottom;
        {
          dx=Frame->wndSize.right-rLine.right;
          dy=-rLine.top+Frame->wndSize.top;
        }
        OffsetRect(&rLine,dx,dy);
        OffsetRect(&rUpBtn,dx,dy);
        OffsetRect(&rDnBtn,dx,dy);
        OffsetRect(&rThumb,dx,dy);
        BitBlt(cachedWindow->hImageDC,rLine.left,rLine.top,rLine.right-rLine.left,rLine.bottom-rLine.top,cachedWindow->hBackDC,rLine.left,rLine.top,SRCCOPY);
        {
          char req[255];
          _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=Back",Frame->name);
          SkinDrawGlyph(cachedWindow->hImageDC,&rLine,&rLine,req);
          _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=Thumb",Frame->name);
          SkinDrawGlyph(cachedWindow->hImageDC,&rThumb,&rThumb,req);
          _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=UpLineButton",Frame->name);
          SkinDrawGlyph(cachedWindow->hImageDC,&rUpBtn,&rUpBtn,req);
          _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=DownLineButton",Frame->name);
          SkinDrawGlyph(cachedWindow->hImageDC,&rDnBtn,&rDnBtn,req);
        }
      }

    }
    //StartGDIPlus();
    //CallTest(cachedWindow->hImageDC, 0, 0, "Test case");
    //TerminateGDIPlus();

    SelectObject(hdc,o);
    DeleteObject(n);
    DeleteDC(hdc);
  }
  return 1;
}
extern int TitleBarH;
extern int GapBetweenTitlebar;
int DrawNonFramedObjects(BOOL Erase,RECT *r)
{
  RECT w,wnd;
  if (r) w=*r;
  else SizingGetWindowRect(hwndContactList,&w);

  if (cachedWindow==NULL)
    return ValidateFrameImageProc(&w);

  wnd=w;
  OffsetRect(&w, -w.left, -w.top);
  if (Erase)
  {
    HBITMAP hb2;
    hb2=CreateBitmap32(cachedWindow->Width,cachedWindow->Height); 
    SelectObject(cachedWindow->hBackDC,hb2);
    DeleteObject(cachedWindow->hBackDIB);
    cachedWindow->hBackDIB=hb2;
  }

  SkinDrawGlyph(cachedWindow->hBackDC,&w,&w,"Main,ID=Background");
  //--Draw frames captions
  {
    int i;
    for(i=0;i<nFramescount;i++)
      if (Frames[i].TitleBar.ShowTitleBar && Frames[i].visible && !Frames[i].floating)
      {
        RECT rc;
        SetRect(&rc,Frames[i].wndSize.left,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar,Frames[i].wndSize.right,Frames[i].wndSize.top-GapBetweenTitlebar);
        //GetWindowRect(Frames[i].TitleBar.hwnd,&rc);
        //OffsetRect(&rc,-wnd.left,-wnd.top);
        DrawTitleBar(cachedWindow->hBackDC,rc,Frames[i].id);
      }
  }
  LOCK_UPDATING=1;

  JustDrawNonFramedObjects=1;
  return 0;
}
int ValidateFrameImageProc(RECT * r)                                // Calling queued frame paint procs and refresh image
{
  RECT wnd;
  BOOL IsNewCache=0;
  BOOL IsForceAllPainting=0;

  //#ifdef _DEBUG
  //    {
  //        char deb[100];
  //        sprintf(deb,"%d : VALIDATING _____ FrameImageProc\n",MSG_COUNTER++);
  //        TRACE(deb);
  //    }
  //#endif
  if (r) wnd=*r;
  else GetWindowRect(hwndContactList,&wnd);
  LOCK_UPDATING=1;
  //-- Check cached.
  if (cachedWindow==NULL)
  {
    //-- Create New Cache
    {
      cachedWindow=(sCurrentWindowImageData*)mir_alloc(sizeof(sCurrentWindowImageData));
      cachedWindow->hScreenDC=GetDC(NULL);
      cachedWindow->hBackDC=CreateCompatibleDC(cachedWindow->hScreenDC);
      cachedWindow->hImageDC=CreateCompatibleDC(cachedWindow->hScreenDC);
      cachedWindow->Width=wnd.right-wnd.left;
      cachedWindow->Height=wnd.bottom-wnd.top;
      cachedWindow->hImageDIB=CreateBitmap32Point(cachedWindow->Width,cachedWindow->Height,&(cachedWindow->hImageDIBByte));
      cachedWindow->hBackDIB=CreateBitmap32Point(cachedWindow->Width,cachedWindow->Height,&(cachedWindow->hBackDIBByte));
      cachedWindow->hImageOld=SelectObject(cachedWindow->hImageDC,cachedWindow->hImageDIB);
      cachedWindow->hBackOld=SelectObject(cachedWindow->hBackDC,cachedWindow->hBackDIB);
    }
    IsNewCache=1;
  }   
  if (cachedWindow->Width!=wnd.right-wnd.left || cachedWindow->Height!=wnd.bottom-wnd.top)
  {
    HBITMAP hb1,hb2;
    cachedWindow->Width=wnd.right-wnd.left;
    cachedWindow->Height=wnd.bottom-wnd.top;
    hb1=CreateBitmap32Point(cachedWindow->Width,cachedWindow->Height,&(cachedWindow->hImageDIBByte));
    hb2=CreateBitmap32Point(cachedWindow->Width,cachedWindow->Height,&(cachedWindow->hImageDIBByte)); 
    SelectObject(cachedWindow->hImageDC,hb1);
    SelectObject(cachedWindow->hBackDC,hb2);
    DeleteObject(cachedWindow->hImageDIB);
    DeleteObject(cachedWindow->hBackDIB);
    cachedWindow->hImageDIB=hb1;
    cachedWindow->hBackDIB=hb2;
    IsNewCache=1;
  }
  if (IsNewCache)
  {
    //-- Draw Back, frame captions, non-client area
    //#ifdef _DEBUG
    //    {
    //        char deb[100];
    //        sprintf(deb,"%d : Draw Back, frame captions, non-client area to ImageFrameImageProc\n",MSG_COUNTER++);
    //        TRACE(deb);
    //    }
    //#endif
    DrawNonFramedObjects(0,&wnd);       
    IsForceAllPainting=1;
  }
  if (JustDrawNonFramedObjects)
  {
    IsForceAllPainting=1;
    JustDrawNonFramedObjects=0;
  }
  if (IsForceAllPainting) 
  { 
    //BitBlt whole back to Image
    //#ifdef _DEBUG
    //    {
    //        char deb[100];
    //        sprintf(deb,"%d : BitBlt whole back to ImageFrameImageProc\n",MSG_COUNTER++);
    //        TRACE(deb);
    //    }
    //#endif

    BitBlt(cachedWindow->hImageDC,0,0,cachedWindow->Width,cachedWindow->Height,cachedWindow->hBackDC,0,0,SRCCOPY);
    QueueAllFramesUpdating(1);
  }
  //-- Validating frames
  { 
    int i;
    for(i=0;i<nFramescount;i++)
      if (Frames[i].PaintCallbackProc && Frames[i].visible)
        if (Frames[i].bQueued || IsForceAllPainting)
          ValidateSingleFrameImage(&Frames[i],IsForceAllPainting);
  }
  LOCK_UPDATING=1;
  RedrawButtons();
  LOCK_UPDATING=0;
  if (!TempLockUpdate)  UpdateWindowImageRect(&wnd);
  //-- Clear queue
  {
    QueueAllFramesUpdating(0);
    UPDATE_ALLREADY_QUEUED=0;
    POST_WAS_CANCELED=0;
  }
  return 1;
}

int UpdateWindowImage()
{
  RECT r;
  GetWindowRect(hwndContactList,&r);
  return UpdateWindowImageRect(&r);
}

BOOL NeedToBeFullRepaint=0;
int UpdateWindowImageRect(RECT * r)                                     // Update window with current image and 
{
  //if not validity -> ValidateImageProc
  //else Update using current alpha
  RECT wnd=*r;

  if (cachedWindow==NULL) return ValidateFrameImageProc(&wnd); 
  if (cachedWindow->Width!=wnd.right-wnd.left || cachedWindow->Height!=wnd.bottom-wnd.top) return ValidateFrameImageProc(&wnd);
  if (NeedToBeFullRepaint) 
  {
    NeedToBeFullRepaint=0; 
    return ValidateFrameImageProc(&wnd);
  }
  JustUpdateWindowImageRect(&wnd);
  return 0;
}
int JustUpdateWindowImage()
{
  RECT r;
  GetWindowRect(hwndContactList,&r);
  return JustUpdateWindowImageRect(&r);
}
int JustUpdateWindowImageRect(RECT * rty)
//Update window image
{
  BLENDFUNCTION bf={AC_SRC_OVER, 0,CURRENT_ALPHA, AC_SRC_ALPHA };
  POINT dest={0}, src={0};
  int res;
  RECT wnd=*rty;

  RECT rect;
  SIZE sz={0};
  LOCK_IMAGE_UPDATING=1;
  if (!hwndContactList) return 0;
  rect=wnd;
  dest.x=rect.left;
  dest.y=rect.top;
  sz.cx=rect.right-rect.left;
  sz.cy=rect.bottom-rect.top;
  //#ifdef _DEBUG
  //{
  //        char b[250];
  //        sprintf(b,"Update in rect=%d,%d  - %d,%d\n",dest.x,dest.y,rect.right,rect.bottom);
  //        TRACE(b);
  //    }
  //#endif
  if (MyUpdateLayeredWindow && LayeredFlag)
  {
    if (!(GetWindowLong(hwndContactList, GWL_EXSTYLE)&WS_EX_LAYERED))
      SetWindowLong(hwndContactList,GWL_EXSTYLE, GetWindowLong(hwndContactList, GWL_EXSTYLE) |WS_EX_LAYERED);
    SetAlpha(CURRENT_ALPHA);

    res=MyUpdateLayeredWindow(hwndContactList,cachedWindow->hScreenDC,&dest,&sz,cachedWindow->hImageDC,&src,0,&bf,ULW_ALPHA);
  }
  else
  {
    //		PAINTSTRUCT ps;
    HDC hdcWin;
    //		HBITMAP bmp;
    RECT rc;
    HRGN rgn;	
    //
    {
      if ((GetWindowLong(hwndContactList, GWL_EXSTYLE)&WS_EX_LAYERED))
        SetWindowLong(hwndContactList,GWL_EXSTYLE, GetWindowLong(hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    }
    TRACE("DRAWING\n");

    hdcWin=GetWindowDC(hwndContactList);//DCEx(hwndContactList,rgn,DCX_VALIDATE);		
    {
      GetClientRect(hwndContactList,&rc);
      rgn=CreateRectRgn(rc.left,rc.top,rc.right,rc.bottom);
      {
        int i;
        for(i=0;i<nFramescount;i++)
          if (Frames[i].PaintCallbackProc && Frames[i].visible)
          {
            HRGN rgn2;
            RECT clr;
            GetClientRect(Frames[i].hWnd,&clr);
            OffsetRect(&clr,Frames[i].wndSize.left,Frames[i].wndSize.top);
            rgn2=CreateRectRgn(Frames[i].wndSize.left,Frames[i].wndSize.top,Frames[i].wndSize.right,Frames[i].wndSize.bottom);
            CombineRgn(rgn,rgn,rgn2,RGN_DIFF);
            DeleteObject(rgn2);
            rgn2=CreateRectRgn(clr.left,clr.top,clr.right,clr.bottom);						
            CombineRgn(rgn,rgn,rgn2,RGN_OR);
            DeleteObject(rgn2);
          }			
      }
    }
    {

      SelectClipRgn(hdcWin,rgn);
      res=BitBlt(hdcWin,0,0,sz.cx,sz.cy,cachedWindow->hImageDC,0,0,SRCCOPY);

    }
    DeleteObject(rgn);
    ReleaseDC(hwndContactList,hdcWin);
  }
  if (!res &&0)
  {

    char szBuf[80]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 


    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );

    sprintf(szBuf,  "UPDATE LAYERED WINDOW  failed with error %d: %s\n",  dw, lpMsgBuf); 

    TRACE(szBuf);
    MessageBoxA(NULL, szBuf, "UPDATE LAYERED WINDOW FAILURE", MB_OK); 
    DebugBreak();

  }

  LOCK_IMAGE_UPDATING=0;
  return 0;
}

int SkinDrawImageAt(HDC hdc, RECT *rc)
{
  BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
  BitBlt(cachedWindow->hImageDC,rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,cachedWindow->hBackDC,rc->left,rc->top,SRCCOPY);
  MyAlphaBlend(cachedWindow->hImageDC,rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,hdc,0,0,rc->right-rc->left,rc->bottom-rc->top,bf);  
  if (!LOCK_UPDATING) UpdateWindowImage();
  return 0;
}

HBITMAP GetCurrentWindowImage(){ return cachedWindow->hImageDIB;}