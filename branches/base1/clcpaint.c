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
#include "skinengine.h"
#include "commonprototypes.h"

#define HORIZONTAL_SPACE 4
#define EXTRA_CHECKBOX_SPACE 4
#define EXTRA_SPACE 2
#define SELECTION_BORDER 6

#define MIN_TEXT_WIDTH 20

extern int StartGDIPlus();
extern int TerminateGDIPlus();
extern BYTE gdiPlusFail;
extern int CallTest(HDC hdc, int x, int y, char * Text);
extern BOOL DrawIconExS(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);


//#include <gdiplus.h>
//#include <gdiplus.h>
tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint,HRGN rgn,  DWORD dFlags, void * CallBackData);
extern int hClcProtoCount;
extern int IsInMainWindow(HWND hwnd);
extern ClcProtoStatus *clcProto;
extern HIMAGELIST himlCListClc;
static BYTE divide3[765]={255};
int MetaIgnoreEmptyExtra;

extern struct AvatarOverlayIconConfig 
{
  char *name;
  char *description;
  int id;
  HICON icon;
} avatar_overlay_icons[ID_STATUS_OUTTOLUNCH - ID_STATUS_OFFLINE + 1];
//extern BOOL ImageList_DrawEx_New( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle);

//extern void DrawAvatarImageWithGDIp(HDC hDestDC,int x, int y, DWORD width, DWORD height, HBITMAP hbmp, int x1, int y1, DWORD width1, DWORD height1,DWORD flag);

BOOL IsForegroundWindow(HWND hwnd)
{
  HWND h;
  h=hwnd;
  while (h)
  {
    if (GetForegroundWindow()==h) return TRUE;
    h=GetParent(h);
  }
  return FALSE;

}

HFONT ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight)
{
  HFONT res;
  res=SelectObject(hdc,dat->fontInfo[id].hFont);
  SetTextColor(hdc,dat->fontInfo[id].colour);
  if(fontHeight) *fontHeight=dat->fontInfo[id].fontHeight;
  return res;
}

static void __inline SetHotTrackColour(HDC hdc,struct ClcData *dat)
{
  if(dat->gammaCorrection) {
    COLORREF oldCol,newCol;
    int oldLum,newLum;

    oldCol=GetTextColor(hdc);
    oldLum=(GetRValue(oldCol)*30+GetGValue(oldCol)*59+GetBValue(oldCol)*11)/100;
    newLum=(GetRValue(dat->hotTextColour)*30+GetGValue(dat->hotTextColour)*59+GetBValue(dat->hotTextColour)*11)/100;
    if(newLum==0) {
      SetTextColor(hdc,dat->hotTextColour);
      return;
    }
    if(newLum>=oldLum+20) {
      oldLum+=20;
      newCol=RGB(GetRValue(dat->hotTextColour)*oldLum/newLum,GetGValue(dat->hotTextColour)*oldLum/newLum,GetBValue(dat->hotTextColour)*oldLum/newLum);
    }
    else if(newLum<=oldLum) {
      int r,g,b;
      r=GetRValue(dat->hotTextColour)*oldLum/newLum;
      g=GetGValue(dat->hotTextColour)*oldLum/newLum;
      b=GetBValue(dat->hotTextColour)*oldLum/newLum;
      if(r>255) {
        g+=(r-255)*3/7;
        b+=(r-255)*3/7;
        r=255;
      }
      if(g>255) {
        r+=(g-255)*59/41;
        if(r>255) r=255;
        b+=(g-255)*59/41;
        g=255;
      }
      if(b>255) {
        r+=(b-255)*11/89;
        if(r>255) r=255;
        g+=(b-255)*11/89;
        if(g>255) g=255;
        b=255;
      }
      newCol=RGB(r,g,b);
    }
    else newCol=dat->hotTextColour;
    SetTextColor(hdc,newCol);
  }
  else
    SetTextColor(hdc,dat->hotTextColour);
}

static int GetStatusOnlineness(int status)
{
  switch(status) {
    case ID_STATUS_FREECHAT:   return 110;
    case ID_STATUS_ONLINE:     return 100;
    case ID_STATUS_OCCUPIED:   return 60;
    case ID_STATUS_ONTHEPHONE: return 50;
    case ID_STATUS_DND:        return 40;
    case ID_STATUS_AWAY:	   return 30;
    case ID_STATUS_OUTTOLUNCH: return 20;
    case ID_STATUS_NA:		   return 10;
    case ID_STATUS_INVISIBLE:  return 5;
  }
  return 0;
}

static int GetGeneralisedStatus(void)
{
  int i,status,thisStatus,statusOnlineness,thisOnlineness;

  status=ID_STATUS_OFFLINE;
  statusOnlineness=0;

  for (i=0;i<hClcProtoCount;i++) {
    thisStatus = clcProto[i].dwStatus;
    if(thisStatus==ID_STATUS_INVISIBLE) return ID_STATUS_INVISIBLE;
    thisOnlineness=GetStatusOnlineness(thisStatus);
    if(thisOnlineness>statusOnlineness) {
      status=thisStatus;
      statusOnlineness=thisOnlineness;
    }
  }
  return status;
}

int GetRealStatus(struct ClcContact * contact, int status) 
{
  int i;
  char *szProto=contact->proto;
  if (!szProto) return status;
  for (i=0;i<hClcProtoCount;i++) {
    if (!lstrcmpA(clcProto[i].szProto,szProto)) {
      return clcProto[i].dwStatus;
    }
  }
  return status;
}

int GetBasicFontID(struct ClcContact * contact) 
{
  switch (contact->type)
  {
  case CLCIT_GROUP:
    return FONTID_GROUPS;
    break;
  case CLCIT_INFO:
    if(contact->flags & CLCIIF_GROUPFONT) 
      return FONTID_GROUPS;
    else 
      return FONTID_CONTACTS;
    break;
  case CLCIT_DIVIDER:
    return FONTID_DIVIDERS;
    break;
  case CLCIT_CONTACT:
    if (contact->flags & CONTACTF_NOTONLIST)
      return FONTID_NOTONLIST;
    else if ( (contact->flags&CONTACTF_INVISTO 
      && GetRealStatus(contact, ID_STATUS_OFFLINE) != ID_STATUS_INVISIBLE)
      ||
      (contact->flags&CONTACTF_VISTO 
      && GetRealStatus(contact, ID_STATUS_OFFLINE) == ID_STATUS_INVISIBLE) ) 
    {
      // the contact is in the always visible list and the proto is invisible
      // the contact is in the always invisible and the proto is in any other mode
      return contact->flags & CONTACTF_ONLINE ? FONTID_INVIS : FONTID_OFFINVIS;
    }
    else
      return contact->flags & CONTACTF_ONLINE ? FONTID_CONTACTS : FONTID_OFFLINE;
    break;
  default:
    return FONTID_CONTACTS;
  }
}

static HMODULE  themeAPIHandle = NULL; // handle to uxtheme.dll
static HANDLE   (WINAPI *MyOpenThemeData)(HWND,LPCWSTR);
static HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);
static HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *);

#define MGPROC(x) GetProcAddress(themeAPIHandle,x)

RECT GetRectangleEx(struct ClcData *dat, RECT *row_rc, RECT *free_row_rc, int *left_pos, int *right_pos, BOOL left, int real_width, int width, int height, int horizontal_space, BOOL ignore_align)
{
  RECT rc = *free_row_rc;
  int width_tmp;
  if (left)
  {
    if (dat->row_align_left_items_to_left && !ignore_align)
      width_tmp = real_width;
    else
      width_tmp = width;
    rc.left += (width_tmp - real_width) >> 1;
    rc.right = rc.left + real_width;
    rc.top += (rc.bottom - rc.top - height) >> 1;
    rc.bottom = rc.top + height;
    *left_pos += width_tmp + horizontal_space;
    free_row_rc->left = row_rc->left + *left_pos;
  }
  else // if (!left)
  {
    if (dat->row_align_right_items_to_right && !ignore_align)
      width_tmp = real_width;
    else
      width_tmp = width;

    if (width_tmp > rc.right - rc.left)
    {
      rc.left = rc.right + 1;
    }
    else
    {
      rc.left = max(rc.left + horizontal_space, rc.right - width_tmp)  + ((width_tmp - real_width) >> 1);
      rc.right = min(rc.left + real_width, rc.right);
      rc.top += max(0, (rc.bottom - rc.top - height) >> 1);
      rc.bottom = min(rc.top + height, rc.bottom);

      *right_pos += min(width_tmp + horizontal_space, free_row_rc->right - free_row_rc->left);
      free_row_rc->right = row_rc->right - *right_pos;
    }
  }

  return rc;
}
RECT GetRectangle(struct ClcData *dat, RECT *row_rc, RECT *free_row_rc, int *left_pos, int *right_pos, BOOL left, int real_width, int width, int height, int horizontal_space)
{
  return GetRectangleEx(dat, row_rc, free_row_rc, left_pos, right_pos, left, real_width, width, height, horizontal_space, FALSE);
}
void _inline AppendChar(char * buf, UINT size, char * add)
{
  _snprintf(buf,size,"%s%s",buf,add);
}
void GetTextSize(SIZE *text_size, HDC hdcMem, RECT free_row_rc, TCHAR *szText, SortedList *plText, UINT uTextFormat)
{
  if (szText == NULL)
  {
    text_size->cy = 0;
    text_size->cx = 0;
  }
  else
  {
    RECT text_rc = free_row_rc;
    int free_width;
    int free_height;

    free_width = text_rc.right - text_rc.left;
    free_height = text_rc.bottom - text_rc.top;

    // Always need cy...
    text_size->cy = DrawTextS(hdcMem,szText,lstrlen(szText), &text_rc, DT_CALCRECT | uTextFormat);

    if (plText == NULL) 
    {
      text_size->cx = min(text_rc.right - text_rc.left + 2, free_width);
    }
    else
    {
      // See each item of list
      int i;

      text_size->cx = 0;

      for (i = 0; i < plText->realCount && text_size->cx < free_width; i++)
      {
        ClcContactTextPiece *piece = (ClcContactTextPiece *) plText->items[i];

        if (piece->type == TEXT_PIECE_TYPE_TEXT)
        {
          text_rc = free_row_rc;

          DrawTextS(hdcMem, &szText[piece->start_pos], piece->len, &text_rc, DT_CALCRECT | uTextFormat);
          text_size->cx = min(text_size->cx + text_rc.right - text_rc.left + 2, free_width);
        }
        else
        {
          double factor;

          if (piece->smiley_height > text_size->cy)
          {
            factor = text_size->cy / (double) piece->smiley_height;
          }
          else
          {
            factor = 1;
          }

          text_size->cx = min(text_size->cx + (LONG)(factor * piece->smiley_width), free_width);
        }
      }
    }

    // Just here, because the smiley has to use the full row height
    text_size->cy = min(text_size->cy, free_height);
  }
}


void DrawTextSSmiley(HDC hdcMem, RECT free_rc, SIZE text_size, TCHAR *szText, int len, SortedList *plText, UINT uTextFormat)
{
  if (szText == NULL)
  {
    return;
  }

  if (plText == NULL) 
  {
    // Draw text
    DrawTextS(hdcMem,szText, len, &free_rc, uTextFormat);
  }
  else
  {
    // Draw list
    int i;
    int pos_x = 0;
    int row_height;
    RECT tmp_rc = free_rc;

    if (uTextFormat & DT_RIGHT)
      i = plText->realCount - 1;
    else
      i = 0;

    // Get real height of the line
    row_height = DrawTextS(hdcMem, TEXT("A"), 1, &tmp_rc, DT_CALCRECT | uTextFormat);

    // Just draw ellipsis
    if (free_rc.right <= free_rc.left)
    {
      DrawTextS(hdcMem, TEXT("..."), 3, &free_rc, uTextFormat & ~DT_END_ELLIPSIS);
    }
    else
    {
      // Draw text and smileys
      for (; i < plText->realCount && i >= 0 && pos_x < text_size.cx && len > 0; i += (uTextFormat & DT_RIGHT ? -1 : 1))
      {
        ClcContactTextPiece *piece = (ClcContactTextPiece *) plText->items[i];
        RECT text_rc = free_rc;

        if (uTextFormat & DT_RIGHT)
          text_rc.right -= pos_x;
        else
          text_rc.left += pos_x;

        if (piece->type == TEXT_PIECE_TYPE_TEXT)
        {
          tmp_rc = text_rc;
          tmp_rc.right += 50;
          DrawTextS(hdcMem, &szText[piece->start_pos], min(len, piece->len), &tmp_rc, DT_CALCRECT | (uTextFormat & ~DT_END_ELLIPSIS));
          pos_x += tmp_rc.right - tmp_rc.left + 2;

          DrawTextS(hdcMem, &szText[piece->start_pos], min(len, piece->len), &text_rc, uTextFormat);
          len -= piece->len;
        }
        else
        {
          double factor;

          if (len < piece->len)
          {
            len = 0;
          }
          else
          {
            if (piece->smiley_height > row_height)
            {
              factor = row_height / (double) piece->smiley_height;
            }
            else
            {
              factor = 1;
            }

            if (uTextFormat & DT_RIGHT)
              text_rc.left = max(text_rc.right - (LONG) (piece->smiley_width * factor), text_rc.left);

            if ((LONG)(piece->smiley_width * factor) <= text_rc.right - text_rc.left)
            {
              text_rc.top += (row_height - (LONG)(piece->smiley_height * factor)) >> 1;

              DrawIconExS(hdcMem, text_rc.left, text_rc.top, piece->smiley, 
                (LONG)(piece->smiley_width * factor), (LONG)(piece->smiley_height * factor), 0, NULL, DI_NORMAL); 
            }
            else
            {
              DrawTextS(hdcMem, TEXT("..."), 3, &text_rc, uTextFormat);
            }

            pos_x += (LONG)(piece->smiley_width * factor);
          }
        }
      }
    }
  }
}



#define BUFSIZE 255
#define BUF2SIZE 7
_inline char * GetCLCContactRowBackObject(struct ClcGroup * group, struct ClcContact * Drawing, int indent, int index, BOOL selected, BOOL hottrack,struct ClcData * dat)
{
  char buf[BUFSIZE];
  char b2[BUF2SIZE];
  _snprintf(buf,BUFSIZE,"CL,ID=Row");
  switch (Drawing->type)
  {
  case CLCIT_GROUP:
    {
      AppendChar(buf,BUFSIZE,",Type=Group");
      AppendChar(buf,BUFSIZE,",Open=");
      AppendChar(buf,BUFSIZE,(Drawing && Drawing->group && Drawing->group->expanded)?"True":"False");
    }
    break;
  case CLCIT_CONTACT:
    {
      struct ClcContact * mCont=Drawing;
      if (Drawing->isSubcontact)
      {
        AppendChar(buf,BUFSIZE,",Type=SubContact");
        if (Drawing->isSubcontact==1 && Drawing->subcontacts->SubAllocated==1) AppendChar(buf,BUFSIZE,",SubPos=First-Single");
        else if (Drawing->isSubcontact==1) AppendChar(buf,BUFSIZE,",SubPos=First");    
        else if (Drawing->isSubcontact==Drawing->subcontacts->SubAllocated) AppendChar(buf,BUFSIZE,",SubPos=Last");
        else AppendChar(buf,BUFSIZE,",SubPos=Middle");
        mCont=Drawing->subcontacts;
      }
      else if (Drawing->SubAllocated)
      {
        AppendChar(buf,BUFSIZE,",Type=MetaContact");
        AppendChar(buf,BUFSIZE,",Open=");
        AppendChar(buf,BUFSIZE,(Drawing->SubExpanded)?"True":"False");
      }
      else AppendChar(buf,BUFSIZE,",Type=Contact");
      AppendChar(buf,BUFSIZE,",Protocol=");	
      AppendChar(buf,BUFSIZE,Drawing->proto);	
      AppendChar(buf,BUFSIZE,",Status=");	
      switch(Drawing->status)
      {
        // case ID_STATUS_CONNECTING: AppendChar(buf,BUFSIZE,"CONNECTING"); break;
      case ID_STATUS_ONLINE: AppendChar(buf,BUFSIZE,"ONLINE"); break;
      case ID_STATUS_AWAY: AppendChar(buf,BUFSIZE,"AWAY");  break;
      case ID_STATUS_DND:	AppendChar(buf,BUFSIZE,"DND"); break;
      case ID_STATUS_NA: AppendChar(buf,BUFSIZE,"NA"); break;
      case ID_STATUS_OCCUPIED: AppendChar(buf,BUFSIZE,"OCCUPIED"); break;
      case ID_STATUS_FREECHAT: AppendChar(buf,BUFSIZE,"FREECHAT"); break;
      case ID_STATUS_INVISIBLE:  AppendChar(buf,BUFSIZE,"INVISIBLE"); break;
      case ID_STATUS_OUTTOLUNCH: AppendChar(buf,BUFSIZE,"OUTTOLUNCH"); break;
      case ID_STATUS_ONTHEPHONE: AppendChar(buf,BUFSIZE,"ONTHEPHONE"); break;
      case ID_STATUS_IDLE:  AppendChar(buf,BUFSIZE,"IDLE");  break;
      default:	AppendChar(buf,BUFSIZE,"OFFLINE");
      }
      AppendChar(buf,BUFSIZE,",HasAvatar=");
      if (dat->avatars_show  && Drawing->avatar_data != NULL) AppendChar(buf,BUFSIZE,"True");
      else AppendChar(buf,BUFSIZE,"False");
      break;    
    }
  case CLCIT_DIVIDER:
    {
      AppendChar(buf,BUFSIZE,",Type=Divider");
    }
    break;
  case CLCIT_INFO:
    {
      AppendChar(buf,BUFSIZE,",Type=Info");
    }
    break;
  }
  if (group->scanIndex==0 && group->contactCount==1) AppendChar(buf,BUFSIZE,",GroupPos=First-Single");
  else if (group->scanIndex==0) AppendChar(buf,BUFSIZE,",GroupPos=First");
  else if (group->scanIndex+1==group->contactCount) AppendChar(buf,BUFSIZE,",GroupPos=Last");
  else AppendChar(buf,BUFSIZE,",GroupPos=Mid");

  AppendChar(buf,BUFSIZE,",Selected=");
  AppendChar(buf,BUFSIZE,selected?"True":"False");
  AppendChar(buf,BUFSIZE,",Hot=");
  AppendChar(buf,BUFSIZE,hottrack?"True":"False");
  AppendChar(buf,BUFSIZE,",Odd=");    
  AppendChar(buf,BUFSIZE,(index&1)?"True":"False");
  AppendChar(buf,BUFSIZE,",Indent=");
  AppendChar(buf,BUFSIZE,itoa(indent,b2,BUF2SIZE));
  AppendChar(buf,BUFSIZE,",Index=");
  AppendChar(buf,BUFSIZE,itoa(index,b2,BUF2SIZE));
  {
    TCHAR * b2=mir_strdupT(Drawing->szText);
    int i,m;
    m=lstrlen(b2);	
    for (i=0; i<m;i++)
      if (b2[i]==TEXT(',')) b2[i]=TEXT('.');
    //TODO b2 to UTF8
    AppendChar(buf,BUFSIZE,",Name=");
    AppendChar(buf,BUFSIZE,b2);			
    mir_free(b2);
  }
  if (group->parent)
  {
    TCHAR * b2=mir_strdupT(group->parent->contact->szText);
    int i,m;
    m=lstrlen(b2);	
    for (i=0; i<m;i++)
      if (b2[i]==TEXT(',')) b2[i]=TEXT('.');
    //TODO b2 to UTF8
    AppendChar(buf,BUFSIZE,",Group=");
    AppendChar(buf,BUFSIZE,b2);		
    mir_free(b2);
  }
  return mir_strdup(buf);  
}
#undef BUFSIZE
#undef BUF2SIZE
tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData)
{
  struct ClcData* dat;
  dat=(struct ClcData*)GetWindowLong(hWnd,0);
  if (!dat) return 0;
  PaintClc(hWnd,dat,hDC,rcPaint);
  return NULL;
}


void InternalPaintRowItems(HWND hwnd, HDC hdcMem, struct ClcData *dat, struct ClcContact *Drawing,RECT row_rc, RECT free_row_rc, int left_pos, int right_pos, int selected,int hottrack, RECT *rcPaint)
{
  int item, item_iterator, item_text;
  BOOL left = TRUE;
  int text_left_pos = free_row_rc.right + 1;

  // Now let's check what whe have to draw
  for ( item_iterator = 0 ; item_iterator < NUM_ITEM_TYPE && free_row_rc.left < free_row_rc.right ; item_iterator++ )
  {
    if (left)
      item = item_iterator;
    else
      item = NUM_ITEM_TYPE - (item_iterator - item_text);

    switch(dat->row_items[item])
    {
    case ITEM_AVATAR: ///////////////////////////////////////////////////////////////////////////////////////////////////
		{
        RECT rc, real_rc;
        int round_radius;
        int max_width;
        int width;
        int height;

        if (!dat->avatars_show || Drawing->type != CLCIT_CONTACT)
          break;

        if (dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space)
          max_width = max(dat->iconXSpace, dat->avatars_size);
        else
          max_width = dat->avatars_size;

        // Has to draw?
        if ((dat->use_avatar_service && (Drawing->avatar_data == NULL))
		  || (dat->use_avatar_service && Drawing->avatar_data &&(Drawing->avatar_data->bmHeight*Drawing->avatar_data->bmWidth==0))
          || (!dat->use_avatar_service && Drawing->avatar_pos == AVATAR_POS_DONT_HAVE))
        {
          // Don't have to draw avatar

          // Has to draw icon instead?
          if (dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space 
            /*&& !Drawing->image_is_special*/ && Drawing->iImage != -1)
          {
            // Draw icon here!
            int iImage = Drawing->iImage;
            COLORREF colourFg;
            int mode;
            RECT rc;

            // Make rectangle
            rc = GetRectangleEx(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
              left, dat->iconXSpace, max_width, ICON_HEIGHT, HORIZONTAL_SPACE, TRUE);

            if (rc.left < rc.right)
            {
              // Store pos
              Drawing->pos_icon = rc;

              // Get data to draw
              if(hottrack) 
              {
                colourFg=dat->hotTextColour;
                mode=ILD_NORMAL;
              }
              else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              {
                colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; 
                mode=ILD_BLEND50;
              }
              else
              {
                colourFg=dat->selBkColour;
                mode=ILD_NORMAL;
              }

              if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
                GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
              {
                mode=ILD_SELECTED;
              }

              ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, 
                rc.left, rc.top,
                0,0,CLR_NONE,colourFg,mode);
            }
          }
          else if (!dat->icon_hide_on_avatar)
          {
            // Has to keep the empty space?? ?? why if we don't has avatar???
            if (left)
            {
              if (!dat->row_align_left_items_to_left )
              {
                // Make rectangle
                rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                  left, max_width, max_width, dat->avatars_size, HORIZONTAL_SPACE);

                // Store pos
                Drawing->pos_avatar = rc;;
              }
            }
            else // if (!left)
            {
              if (!dat->row_align_right_items_to_right)
              {
                // Make rectangle
                rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                  left, max_width, max_width, dat->avatars_size, HORIZONTAL_SPACE);

                if (rc.left < rc.right)
                {
                  // Store pos
                  Drawing->pos_avatar = rc;
                }
              }
            }
          }

          break;
        }

        // Has to draw!

        // Get size
        if (dat->use_avatar_service)
        {
          // Make bounds -> keep aspect radio
          // Clipping width and height
          width = dat->avatars_size;
          height = dat->avatars_size;
          

          if (height * Drawing->avatar_data->bmWidth / Drawing->avatar_data->bmHeight <= width)
          {
            width = height * Drawing->avatar_data->bmWidth / Drawing->avatar_data->bmHeight;
          }
          else
          {
            height = width * Drawing->avatar_data->bmHeight / Drawing->avatar_data->bmWidth;					
          }
        }
        else
        {
          width = dat->avatar_cache.nodes[Drawing->avatar_pos].width;
          height = dat->avatar_cache.nodes[Drawing->avatar_pos].height;
        }

        // Make rectangle
        rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
          left, width, max_width, height, HORIZONTAL_SPACE);
        real_rc = rc;
        //real_rc.top++;
        //real_rc.bottom++;
        if (real_rc.bottom - real_rc.top < height)
        {
          real_rc.top -= (height - real_rc.bottom + real_rc.top) >> 1;
          real_rc.bottom = real_rc.top + height;
        }

        if (rc.left < rc.right)
        {
          HRGN rgn;
          // Store pos
          Drawing->pos_avatar = rc;

          // Round corners
          if (dat->avatars_round_corners)
          {
            if (dat->avatars_use_custom_corner_size)
              round_radius = dat->avatars_custom_corner_size;
            else
              round_radius = min(width, height) / 5;
          }
          else
          {
            round_radius = 0;
          } 
          if (dat->avatars_draw_border)
          {
            HBRUSH hBrush=CreateSolidBrush(dat->avatars_border_color);
            HBRUSH hOldBrush = SelectObject(hdcMem, hBrush);
            HRGN rgn2;
            rgn=CreateRoundRectRgn(real_rc.left, real_rc.top, real_rc.right+1, real_rc.bottom+1, round_radius<<1, round_radius<<1);           			
            rgn2=CreateRoundRectRgn(real_rc.left+1, real_rc.top+1, real_rc.right, real_rc.bottom, round_radius<<1, round_radius<<1);           			
            CombineRgn(rgn2,rgn,rgn2,RGN_DIFF);
           // FrameRgn(hdcMem,rgn,hBrush,1,1);
            FillRgn(hdcMem,rgn2,hBrush);
            FillRgn255Alpha(hdcMem,rgn2);
            SelectObject(hdcMem, hOldBrush);
            DeleteObject(hBrush);
            DeleteObject(rgn);
            DeleteObject(rgn2);
          }
          if (dat->avatars_round_corners || dat->avatars_draw_border)
          {
            int k=dat->avatars_draw_border?1:0;
            rgn=CreateRoundRectRgn(real_rc.left+k, real_rc.top+k, real_rc.right+1-k, real_rc.bottom+1-k, round_radius * 2, round_radius * 2);           
            ExtSelectClipRgn(hdcMem, rgn, RGN_AND);
          }

          // Draw avatar
          if (dat->use_avatar_service)
          {
            int w=width;
            int h=height;
            if (!gdiPlusFail) //Use gdi+ engine
            {
              DrawAvatarImageWithGDIp(hdcMem, rc.left, rc.top, w, h,Drawing->avatar_data->hbmPic,0,0,Drawing->avatar_data->bmWidth,Drawing->avatar_data->bmHeight,Drawing->avatar_data->dwFlags);
            }
            else
            {
              if (!(Drawing->avatar_data->dwFlags&AVS_PREMULTIPLIED))
              {
                HDC hdcTmp = CreateCompatibleDC(hdcMem);
                RECT r={0,0,w,h};
                HDC hdcTmp2 = CreateCompatibleDC(hdcMem);
                HBITMAP bmo=SelectObject(hdcTmp,Drawing->avatar_data->hbmPic);
                HBITMAP b2=CreateBitmap32(w,h);
                HBITMAP bmo2=SelectObject(hdcTmp2,b2);
                SetStretchBltMode(hdcTmp,  HALFTONE);
                SetStretchBltMode(hdcTmp2,  HALFTONE);
                StretchBlt(hdcTmp2, 0, 0, w, h,
                  hdcTmp, 0, 0, Drawing->avatar_data->bmWidth, Drawing->avatar_data->bmHeight, 
                  SRCCOPY);

                FillRect255Alpha(hdcTmp2,&r);
                BitBlt(hdcMem, rc.left, rc.top, w, h,hdcTmp2,0,0,SRCCOPY);
                SelectObject(hdcTmp2,bmo2);
                SelectObject(hdcTmp,bmo);
                DeleteDC(hdcTmp);
                DeleteDC(hdcTmp2);
                DeleteObject(b2);
              }
              else {
                BLENDFUNCTION bf={AC_SRC_OVER, 0,255, AC_SRC_ALPHA };
                HDC hdcTempAv = CreateCompatibleDC(hdcMem);                 
                HBITMAP hbmTempAvOld;
                hbmTempAvOld = SelectObject(hdcTempAv,Drawing->avatar_data->hbmPic);
                AlphaBlend(hdcMem, rc.left, rc.top, w, h, hdcTempAv, 0, 0,Drawing->avatar_data->bmWidth,Drawing->avatar_data->bmHeight, bf);
                SelectObject(hdcTempAv, hbmTempAvOld);
                DeleteDC(hdcTempAv);
              }
            }
          }
          else
          {
            ImageArray_DrawImage(&dat->avatar_cache, Drawing->avatar_pos, hdcMem, real_rc.left, real_rc.top);
          }
          // Restore region
          if (dat->avatars_round_corners || dat->avatars_draw_border)
          {
            DeleteObject(rgn);
          }
          rgn = CreateRectRgn(row_rc.left, row_rc.top, row_rc.right, row_rc.bottom);
          SelectClipRgn(hdcMem, rgn);
          DeleteObject(rgn);


          // Draw borders

          //TODO fix overlays
          // Draw overlay
          if (dat->avatars_draw_overlay && dat->avatars_size >= ICON_HEIGHT + (dat->avatars_draw_border ? 2 : 0)
            && Drawing->status - ID_STATUS_OFFLINE < MAX_REGS(avatar_overlay_icons))
          {
            real_rc.top = real_rc.bottom - ICON_HEIGHT;
            real_rc.left = real_rc.right - ICON_HEIGHT;

            if (dat->avatars_draw_border)
            {
              real_rc.top--;
              real_rc.left--;
            }
//#ifdef _DEBUG
            {
                pdisplayNameCacheEntry pdnce = GetDisplayNameCacheEntry((HANDLE)Drawing->hContact);
                if (Drawing->status!=pdnce->status)
                {
                  TRACE("!!!! OVERLAY ERROR : Statuses is not equal\n");
                  Drawing->status=pdnce->status;
                }
            }
//#endif
            DrawIconExS(hdcMem, real_rc.left, real_rc.top, avatar_overlay_icons[Drawing->status - ID_STATUS_OFFLINE].icon, 
              ICON_HEIGHT, ICON_HEIGHT, 0, NULL, DI_NORMAL); 
          }
        }

        break;
      }
    case ITEM_ICON: /////////////////////////////////////////////////////////////////////////////////////////////////////
      {
        RECT rc;
        int iImage = -1;
        BOOL has_avatar = dat->avatars_show &&( (dat->use_avatar_service && Drawing->avatar_data != NULL) ||
          (!dat->use_avatar_service && Drawing->avatar_pos != AVATAR_POS_DONT_HAVE) );

        if (Drawing->type == CLCIT_CONTACT && dat->avatars_show &&(
          (has_avatar && dat->icon_hide_on_avatar && !Drawing->image_is_special)
          || ((!has_avatar && dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space) //&& !Drawing->image_is_special
          )


          ))
        {
          // Don't have to draw, but has to keep the empty space?  why?????
          if (!dat->icon_draw_on_avatar_space && Drawing->image_is_special &&
            (
            (left && !dat->row_align_left_items_to_left) 
            || (!left && !dat->row_align_right_items_to_right) 
            ))
          {
            rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
              left, dat->iconXSpace, dat->iconXSpace, ICON_HEIGHT, HORIZONTAL_SPACE);

            if (rc.left < rc.right)
            {
              // Store pos
              Drawing->pos_icon = rc;
            }
          }
          break;
        }

        // Get image
        if (Drawing->type == CLCIT_GROUP)
        {
          if (!dat->row_hide_group_icon) iImage = Drawing->group->expanded ? IMAGE_GROUPOPEN : IMAGE_GROUPSHUT;
          else iImage=-1;
        }
        else if (Drawing->type == CLCIT_CONTACT)
          iImage = Drawing->iImage;

        // Has image to draw?
        if (iImage != -1) 
        {
          COLORREF colourFg;
          int mode;

          // Make rectangle
          rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
            left, dat->iconXSpace, dat->iconXSpace, ICON_HEIGHT, HORIZONTAL_SPACE);

          if (rc.left < rc.right)
          {
            // Store pos
            Drawing->pos_icon = rc;

            // Get data to draw
            if(hottrack) 
            {
              colourFg=dat->hotTextColour;
              mode=ILD_NORMAL;
            }
            else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
            {
              colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; 
              mode=ILD_BLEND50;
            }
            else
            {
              colourFg=dat->selBkColour;
              mode=ILD_NORMAL;
            }

            if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
              GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
            {
              mode=ILD_SELECTED;
            }

            ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, 
              rc.left, rc.top,
              0,0,CLR_NONE,colourFg,mode);
          }
        }
        break;
      }
    case ITEM_TEXT: /////////////////////////////////////////////////////////////////////////////////////////////////////
      {
        // Store init text position:
        text_left_pos = left_pos;

        left_pos += MIN_TEXT_WIDTH;
        free_row_rc.left = row_rc.left + left_pos;

        item_text = item;
        left = FALSE;
        break;
      }
    case ITEM_EXTRA_ICONS: //////////////////////////////////////////////////////////////////////////////////////////////
      {
        // Draw extra icons
        if (!Drawing->isSubcontact || DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0) == 0 && dat->extraColumnsCount > 0)
        {
          int BlendedInActiveState = DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
          int BlendValue = DBGetContactSettingByte(NULL,"CLC","Blend25%",1) ? ILD_BLEND25 : ILD_BLEND50;
          int iImage;
          int count = 0;
          RECT rc;

          for( (left ? (iImage = 0) : (iImage = dat->extraColumnsCount-1)) ; 
            (left ? (iImage < dat->extraColumnsCount) : (iImage >= 0)) ; 
            iImage += (left ? 1 : -1) ) 
          {
            COLORREF colourFg=dat->selBkColour;
            int mode=BlendedInActiveState?BlendValue:ILD_NORMAL;

            if(Drawing->iExtraImage[iImage] == 0xFF) 
            {
              /* need to be removed to allow skip empty icon option if align to left is turned off
              if ((left && !dat->row_align_left_items_to_left) 
              || (!left && !dat->row_align_right_items_to_right))
              */
              if (!dat->MetaIgnoreEmptyExtra)
              {
                rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                  left, dat->extraColumnSpacing, dat->extraColumnSpacing, ICON_HEIGHT, 0);

                if (rc.left < rc.right)
                {
                  // Store pos
                  Drawing->pos_extra[iImage] = rc;
                  count++;
                }
              }
              continue;
            }

            if(selected) 
            {
              mode=BlendedInActiveState?ILD_NORMAL:ILD_SELECTED;
            }
            else if(hottrack) 
            {
              mode=BlendedInActiveState?ILD_NORMAL:ILD_FOCUS; 
              colourFg=dat->hotTextColour;
            }
            else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
            {
              colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; 
              mode=BlendValue;
            }

            rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
              left, dat->extraColumnSpacing, dat->extraColumnSpacing, ICON_HEIGHT, 0);

            if (rc.left < rc.right)
            {
              // Store pos
              Drawing->pos_extra[iImage] = rc;

              count++;

              ImageList_DrawEx_New(dat->himlExtraColumns,Drawing->iExtraImage[iImage],hdcMem,
                rc.left, rc.top,0,0,CLR_NONE,colourFg,mode);
            }
          }

          // Add extra space
          if (count > 0)
          {
            GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
              left, HORIZONTAL_SPACE, HORIZONTAL_SPACE, ICON_HEIGHT, 0);
          }
        }
        break;
      }
    }
  }

  if (text_left_pos < free_row_rc.right)
  {
    // Draw text
    RECT text_rc;
    SIZE text_size = {0};
    SIZE second_line_text_size = {0};
    SIZE third_line_text_size = {0};
    SIZE space_size = {0};
    SIZE counts_size = {0};
    TCHAR *szCounts = NULL;//mir_strdupT(TEXT(""));
    int free_width;
    int free_height;
    UINT uTextFormat = DT_NOPREFIX | DT_SINGLELINE | (dat->text_rtl ? (DT_RIGHT | DT_RTLREADING) : 0);
    free_row_rc.left = text_left_pos;
    free_width = free_row_rc.right - free_row_rc.left;
    free_height = free_row_rc.bottom - free_row_rc.top;

    // Select font
    ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);

    // Get text size
    text_rc = free_row_rc;

    GetTextSize(&text_size, hdcMem, free_row_rc, Drawing->szText, Drawing->plText, uTextFormat);

    /*
    text_size.cy = DrawTextS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText), &text_rc, 
    DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE | (dat->text_rtl ? DT_RIGHT : 0));
    text_size.cy = min(text_size.cy, free_height);
    text_size.cx = min(text_rc.right - text_rc.left + 1, free_width);
    */

    // Get rect
    text_rc = free_row_rc;

    text_rc.top += (free_height - text_size.cy) >> 1;
    text_rc.bottom = text_rc.top + text_size.cy;

    if (dat->text_rtl)
      text_rc.left = free_row_rc.right - text_size.cx;
    else
      text_rc.right = free_row_rc.left + text_size.cx;

    // If group, can have the size of count
    if (Drawing->type == CLCIT_GROUP)
    {
      int full_text_width=text_size.cx;

      // Group conts?
      szCounts = GetGroupCountsText(dat, Drawing);

      // Has to draw the count?
      if(szCounts) 
      {
        RECT space_rc = free_row_rc;
        RECT counts_rc = free_row_rc;
        int text_width;

        // Get widths
        DrawTextS(hdcMem,TEXT(" "),1,&space_rc,DT_CALCRECT | DT_NOPREFIX);

        ChangeToFont(hdcMem,dat,FONTID_GROUPCOUNTS,NULL);
        DrawTextS(hdcMem,szCounts,lstrlen(szCounts),&counts_rc,DT_CALCRECT | uTextFormat);
        
        counts_size.cx = counts_rc.right - counts_rc.left;
        counts_size.cy = min(counts_rc.bottom - counts_rc.top, free_height);
        space_size.cx = space_rc.right - space_rc.left;
        space_size.cy = space_rc.bottom - space_rc.top;
        text_width = free_row_rc.right - free_row_rc.left - space_size.cx - counts_size.cx;

        if (text_width > 4)
        {
          text_size.cx = min(text_width, text_size.cx);
          text_width = text_size.cx + space_size.cx + counts_size.cx;
        }
        else
        {
          text_width = text_size.cx;
          space_size.cx = 0;
          space_size.cy = 0;
          counts_size.cx = 0;
          counts_size.cy = 0;
        }

        // Get rect
        text_rc.top = free_row_rc.top + ((free_height - max(text_size.cy, counts_size.cy)) >> 1);
        text_rc.bottom = text_rc.top + max(text_size.cy, counts_size.cy);

        if (dat->text_rtl)
          text_rc.left = free_row_rc.right - text_width;
        else
          text_rc.right = free_row_rc.left + text_width;
        full_text_width=text_width;
        ChangeToFont(hdcMem,dat,FONTID_GROUPS,NULL);
      }

      if (dat->row_align_group_mode==1) //center
      {
        int x;
        x=free_row_rc.left+((free_row_rc.right-free_row_rc.left-full_text_width)>>1);
        text_rc.left=x;
        text_rc.right=x+full_text_width;
      }
      else if (dat->row_align_group_mode==2) //right
      {
        text_rc.left=free_row_rc.right-full_text_width;
        text_rc.right=free_row_rc.right;
      }
      else //left
      {
        text_rc.left=free_row_rc.left;
        text_rc.right=free_row_rc.left+full_text_width;
      }
    }
    else if (Drawing->type == CLCIT_CONTACT)
    {
      int tmp;

      if (dat->second_line_show && Drawing->szSecondLineText!= NULL 
        && free_height > text_size.cy + dat->second_line_top_space)
      {
        //RECT rc_tmp = free_row_rc;

        ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);

        // Get sizes
        GetTextSize(&second_line_text_size, hdcMem, free_row_rc, Drawing->szSecondLineText, Drawing->plSecondLineText, 
          uTextFormat);

        //second_line_text_size.cy = DrawTextS(hdcMem,Drawing->szSecondLineText,lstrlenA(Drawing->szSecondLineText),
        //									&rc_tmp, DT_CALCRECT | uTextFormat);

        //second_line_text_size.cy = min(second_line_text_size.cy, free_height - text_size.cy - dat->second_line_top_space);
        //second_line_text_size.cx = min(rc_tmp.right - rc_tmp.left + 1, free_width);

        // Get rect
        tmp = min(free_height, ((text_rc.bottom - text_rc.top) + dat->second_line_top_space + second_line_text_size.cy));

        text_rc.top = free_row_rc.top + ((free_height - tmp) >> 1);
        text_rc.bottom = text_rc.top + tmp;

        tmp = max(second_line_text_size.cx, text_rc.right - text_rc.left);
        if (dat->text_rtl)
          text_rc.left = free_row_rc.right - tmp;
        else
          text_rc.right = free_row_rc.left + tmp;
      }

      if (dat->third_line_show && Drawing->szThirdLineText!= NULL
        && free_height > text_size.cy + dat->third_line_top_space + third_line_text_size.cy + dat->third_line_top_space)
      {
        //RECT rc_tmp = free_row_rc;

        ChangeToFont(hdcMem,dat,FONTID_THIRDLINE,NULL);

        // Get sizes
        GetTextSize(&third_line_text_size, hdcMem, free_row_rc, Drawing->szThirdLineText, Drawing->plThirdLineText, 
          uTextFormat);

        //third_line_text_size.cy = DrawTextS(hdcMem,Drawing->szThirdLineText,lstrlenA(Drawing->szThirdLineText),
        //									&rc_tmp, DT_CALCRECT | uTextFormat);

        //third_line_text_size.cy = min(third_line_text_size.cy, free_height - text_size.cy - dat->third_line_top_space);
        //third_line_text_size.cx = min(rc_tmp.right - rc_tmp.left + 1, free_width);

        // Get rect
        tmp = min(free_height, ((text_rc.bottom - text_rc.top) + dat->third_line_top_space + third_line_text_size.cy));

        text_rc.top = free_row_rc.top + ((free_height - tmp) >> 1);
        text_rc.bottom = text_rc.top + tmp;

        tmp = max(third_line_text_size.cx, text_rc.right - text_rc.left);
        if (dat->text_rtl)
          text_rc.left = free_row_rc.right - tmp;
        else
          text_rc.right = free_row_rc.left + tmp;
      }

      ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);
    }

    // Store pos
    Drawing->pos_label = text_rc;
    Drawing->pos_full_first_row=free_row_rc;

    //if (dat->text_rtl)
    //	Drawing->pos_label.left = max(Drawing->pos_label.left - SELECTION_BORDER, free_row_rc.left);
    //else
    //	Drawing->pos_label.right = max(Drawing->pos_label.right + SELECTION_BORDER, free_row_rc.right);

    // Selection background
    if (selected || hottrack)
    {
      if (dat->HiLightMode == 0) // Default
      {
        RECT rc = text_rc;

        rc.top = max(rc.top - SELECTION_BORDER, row_rc.top);
        rc.bottom = min(rc.bottom + SELECTION_BORDER, row_rc.bottom);

        if (dat->text_rtl)
          rc.left = max(rc.left - SELECTION_BORDER, free_row_rc.left);
        else
          rc.right = min(rc.right + SELECTION_BORDER, free_row_rc.right);

        if (item > 0)
          rc.left -= HORIZONTAL_SPACE / 2;

        if (item < NUM_ITEM_TYPE - 1)
          rc.right += HORIZONTAL_SPACE / 2;

        if (selected)
          SkinDrawGlyph(hdcMem,&rc,rcPaint,"Contact List/Selection");
        else if(hottrack)
          SkinDrawGlyph(hdcMem,&rc,rcPaint,"Contact List/HotTracking");
      }

      // Set color
      if (selected)
        SetTextColor(hdcMem,dat->selTextColour);
      else if(hottrack)
        SetHotTrackColour(hdcMem,dat);
    }

    // Draw text
    uTextFormat = uTextFormat | DT_END_ELLIPSIS;

    switch (Drawing->type)
    {
    case CLCIT_DIVIDER:
      {
        RECT trc = free_row_rc;
        RECT rc = free_row_rc;
        rc.top += (rc.bottom - rc.top) >> 1; 
        rc.bottom = rc.top + 2;
        rc.right = rc.left + ((rc.right - rc.left - text_size.cx)>>1) - 3;
        DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);

        trc.left = rc.right + 3;
        if (text_size.cy < trc.bottom - trc.top)
        {
          trc.top += (trc.bottom - trc.top - text_size.cy) >> 1;
          trc.bottom = trc.top + text_size.cy;
        }
        DrawTextS(hdcMem,Drawing->szText,lstrlen(Drawing->szText),&trc, uTextFormat);

        rc.left = rc.right + 6 + text_size.cx;
        rc.right = free_row_rc.right;
        DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
        break;
      }
    case CLCIT_GROUP:
      {
        RECT rc = text_rc;

      
      // Get text rectangle :
        if (dat->text_rtl)
          rc.left = rc.right - text_size.cx;
        else
          rc.right = rc.left + text_size.cx;


        if (text_size.cy < rc.bottom - rc.top)
        {
          rc.top += (rc.bottom - rc.top - text_size.cy) >> 1;
          rc.bottom = rc.top + text_size.cy;
        }

        // Draw text
        DrawTextS(hdcMem,Drawing->szText,lstrlen(Drawing->szText),&rc,uTextFormat);

        if(selected && dat->szQuickSearch[0] != '\0') 
        {
          SetTextColor(hdcMem, dat->quickSearchColour);
          DrawTextS(hdcMem,Drawing->szText,lstrlen(dat->szQuickSearch),&rc,uTextFormat);
        }

        // Has to draw the count?
        if(counts_size.cx > 0) 
        {
          RECT counts_rc = text_rc;

          if (dat->text_rtl) 
            counts_rc.right = text_rc.left + counts_size.cx+2;
          else
            counts_rc.left = text_rc.right - counts_size.cx;

          if (counts_size.cy < counts_rc.bottom - counts_rc.top)
          {
            counts_rc.top += (counts_rc.bottom - counts_rc.top - counts_size.cy) >> 1;
            counts_rc.bottom = counts_rc.top + counts_size.cy;
          }

          ChangeToFont(hdcMem,dat,FONTID_GROUPCOUNTS,NULL);
          if (selected)
            SetTextColor(hdcMem,dat->selTextColour);
          else if(hottrack)
            SetHotTrackColour(hdcMem,dat);

          // Draw counts
          DrawTextS(hdcMem,szCounts,lstrlen(szCounts),&counts_rc,uTextFormat);
          if (szCounts) mir_free(szCounts);
        }

        // Update free
        if (dat->exStyle&CLS_EX_LINEWITHGROUPS) 
        {
          if (dat->text_rtl)
            free_row_rc.right -= text_rc.right - text_rc.left;
          else
            free_row_rc.left += text_rc.right - text_rc.left;

          if (free_row_rc.right > free_row_rc.left + 6)
          {
            RECT rc = free_row_rc;
            rc.top += (rc.bottom - rc.top) >> 1;
            rc.bottom = rc.top + 2;
            if (dat->text_rtl)
              rc.right -= 6;
            else
              rc.left += 6;

            DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
          }
        }
        break;
      }
    case CLCIT_CONTACT:
      {
        RECT free_rc = text_rc;

        if (text_size.cx > 0 && free_rc.bottom > free_rc.top)
        {
          RECT rc = free_rc;
          rc.bottom = min(rc.bottom, rc.top + text_size.cy);

          if (text_size.cx < rc.right - rc.left)
          {
            if (dat->text_rtl)
              rc.left = rc.right - text_size.cx;
            else
              rc.right = rc.left + text_size.cx;
          }

          DrawTextSSmiley(hdcMem, rc, text_size, Drawing->szText, lstrlen(Drawing->szText), Drawing->plText, uTextFormat);

          //DrawTextS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&rc,uTextFormat);

          if(selected && dat->szQuickSearch[0] != '\0') 
          {
            SetTextColor(hdcMem, dat->quickSearchColour);
            DrawTextSSmiley(hdcMem, rc, text_size, Drawing->szText, lstrlen(dat->szQuickSearch), Drawing->plText, uTextFormat);
          }
          free_rc.top = rc.bottom;
        }

        if (second_line_text_size.cx > 0 && free_rc.bottom > free_rc.top)
        {
          free_rc.top += dat->second_line_top_space;

          if (free_rc.bottom > free_rc.top)
          {
            RECT rc = free_rc;
            rc.bottom = min(rc.bottom, rc.top + second_line_text_size.cy);

            if (second_line_text_size.cx < rc.right - rc.left)
            {
              if (dat->text_rtl)
                rc.left = rc.right - second_line_text_size.cx;
              else
                rc.right = rc.left + second_line_text_size.cx;
            }

            ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);
            DrawTextSSmiley(hdcMem, rc, second_line_text_size, Drawing->szSecondLineText, MyStrLen(Drawing->szSecondLineText), 
              Drawing->plSecondLineText, uTextFormat);

            free_rc.top = rc.bottom;
          }
        }

        if (third_line_text_size.cx > 0 && free_rc.bottom > free_rc.top)
        {
          free_rc.top += dat->third_line_top_space;

          if (free_rc.bottom > free_rc.top)
          {
            RECT rc = free_rc;
            rc.bottom = min(rc.bottom, rc.top + third_line_text_size.cy);

            if (third_line_text_size.cx < rc.right - rc.left)
            {
              if (dat->text_rtl)
                rc.left = rc.right - third_line_text_size.cx;
              else
                rc.right = rc.left + third_line_text_size.cx;
            }

            ChangeToFont(hdcMem,dat,FONTID_THIRDLINE,NULL);
            //DrawTextS(hdcMem,Drawing->szThirdLineText,lstrlenA(Drawing->szThirdLineText),&rc,uTextFormat);
            DrawTextSSmiley(hdcMem, rc, third_line_text_size, Drawing->szThirdLineText, lstrlen(Drawing->szThirdLineText), 
              Drawing->plThirdLineText, uTextFormat);

            free_rc.top = rc.bottom;
          }
        }
        break;
      }
    default: // CLCIT_INFO
      {
        DrawTextS(hdcMem,Drawing->szText,lstrlen(Drawing->szText),&text_rc,uTextFormat);

        if(selected && dat->szQuickSearch[0] != '\0') 
        {
          SetTextColor(hdcMem, dat->quickSearchColour);
          DrawTextS(hdcMem,Drawing->szText,lstrlen(dat->szQuickSearch),&text_rc,uTextFormat);
        }
      }
    }
  }
}


void InternalPaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint)
{
  HDC hdcMem;
  HBITMAP oldbmp;
  RECT clRect;
  HFONT hdcMemOldFont;
  int y,indent,subident, subindex, line_num;
  struct ClcContact *Drawing;
  struct ClcGroup *group;
  HBITMAP hBmpOsb;
  DWORD style=GetWindowLong(hwnd,GWL_STYLE);
  int status=GetGeneralisedStatus();
  int grey=0; //,groupCountsFontTopShift;
  HBRUSH hBrushAlternateGrey=NULL;
  BOOL NotInMain=!IsInMainWindow(hwnd);
  // yes I know about GetSysColorBrush()
  COLORREF tmpbkcolour = style&CLS_CONTACTLIST ? ( /*dat->useWindowsColours ? GetSysColor(COLOR_3DFACE) :*/ dat->bkColour ) : dat->bkColour;
  int old_stretch_mode;
  int old_bk_mode;

  /*
  #ifdef _DEBUG
  DWORD time_ini, time_end;
  char time_diff[128];
  time_ini = GetTickCount();
  #endif
  */

  if(dat->greyoutFlags&ClcStatusToPf2(status) || style&WS_DISABLED) grey=1;
  else if(GetFocus()!=hwnd && dat->greyoutFlags&GREYF_UNFOCUS) grey=1;
  GetClientRect(hwnd,&clRect);
  if(rcPaint==NULL) rcPaint=&clRect;
  if(IsRectEmpty(rcPaint)) return;
  y=-dat->yScroll;
  if (!(NotInMain || dat->force_in_dialog))
	hdcMem=hdc;
  else
	  hdcMem=CreateCompatibleDC(hdc);
  hdcMemOldFont=GetCurrentObject(hdcMem,OBJ_FONT);
  if (NotInMain || dat->force_in_dialog)
  {
	  hBmpOsb=CreateBitmap32(clRect.right,clRect.bottom);//,1,GetDeviceCaps(hdc,BITSPIXEL),NULL);
	  oldbmp=(HBITMAP)  SelectObject(hdcMem,hBmpOsb);
  }
  if(style&CLS_GREYALTERNATE)
    hBrushAlternateGrey = CreateSolidBrush(GetNearestColor(hdcMem,RGB(GetRValue(tmpbkcolour)-10,GetGValue(tmpbkcolour)-10,GetBValue(tmpbkcolour)-10)));

  // Set some draw states
  old_bk_mode = SetBkMode(hdcMem,TRANSPARENT);
  {
    POINT org;
    GetBrushOrgEx(hdcMem, &org);
    old_stretch_mode = SetStretchBltMode(hdcMem, HALFTONE);
    SetBrushOrgEx(hdcMem, org.x, org.y, NULL);
  }

  //// Draw parent background
  //   {
  //       HWND parent;
  //       parent=(HWND)CallService(MS_CLUI_GETHWND,0,0);
  //       if (GetParent(hwnd)!=parent)
  //           NotInMain=1;

  //       if (NotInMain) 
  //		SkinDrawGlyph(hdcMem,&clRect,rcPaint,"Main Window/Backgrnd");
  //       else  
  //		SkinDrawWindowBack(hwnd,hdcMem,rcPaint,"Main Window/Backgrnd");
  //   }

  // Draw background
  if (NotInMain || dat->force_in_dialog)
  {
	  FillRect(hdcMem,rcPaint,GetSysColorBrush(COLOR_BTNFACE));
	  FillRect255Alpha(hdcMem,rcPaint);
  }
  else
    SkinDrawGlyph(hdcMem,&clRect,rcPaint,"CL,ID=Background");

  // Draw lines
  group=&dat->list;
  group->scanIndex=0;
  indent=0;
  subindex=-1;
  line_num = -1;
  if (!dat->row_heights ) 
  {
	  if (NotInMain || dat->force_in_dialog)
	  {
		  SelectObject(hdcMem,oldbmp);
		  DeleteObject(hBmpOsb);
		  DeleteDC(hdcMem);
	  }
    return;
  }
  //EnterCriticalSection(&(dat->lockitemCS));
  while(y < rcPaint->bottom)
  {
    if (subindex==-1)
    {
      if (group->scanIndex==group->contactCount) 
      {
        group=group->parent;
        indent--;
        if(group==NULL) break;	// Finished list
        group->scanIndex++;
        continue;
      }
    }

    line_num++;

    // Draw line, if needed
    if (y > rcPaint->top - dat->row_heights[line_num]) 
    {
      //-        int iImage;
      int selected;
      int hottrack;
      /*-        SIZE textSize, secondLineSize;
      int space_width = 0;
      int counts_width = 0;
      int counts_height = 0;
      char *szCounts = "";
      -*/
      int left_pos;
      int right_pos;
      int free_row_height;
      RECT row_rc;
      RECT free_row_rc;
      char * request=NULL;
      RECT rc;

      // Get item to draw
      if (subindex==-1)
      {
        Drawing = &(group->contact[group->scanIndex]);
        subident = 0;
      }
      else
      {
        Drawing = &(group->contact[group->scanIndex].subcontacts[subindex]);
        subident = dat->subIndent;
      }
      if (request) mir_free(request);

      // Something to draw?
      if (!Drawing)    
      {

        SelectObject(hdcMem,oldbmp);
        DeleteObject(hBmpOsb);
        DeleteDC(hdcMem);
        //LeaveCriticalSection(&(dat->lockitemCS));
        return;
      };
      if (IsBadCodePtr((FARPROC)Drawing))    
      {
        SelectObject(hdcMem,oldbmp);
        DeleteObject(hBmpOsb);
        DeleteDC(hdcMem);
        //LeaveCriticalSection(&(dat->lockitemCS));
        return;
      };
      // Calc row height
      RowHeights_GetRowHeight(dat, hwnd, Drawing, line_num);
      /*-        {
      secondLineSize.cy=0;
      //Adjust text sizes
      if (Drawing->type==CLCIT_CONTACT && dat->show_status_msg && Drawing->szStatusMsg[0]!='\0')
      {
      int h1=0;
      int h2=0;    
      SIZE sz;        
      int FontID=0;
      FontID=GetBasicFontID(Drawing);
      ChangeToFont(hdcMem,dat,FontID,NULL);
      // Get sizes
      GetTextExtentPoint32A(hdcMem,"A",1,&sz);
      h1=sz.cy;
      ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);
      GetTextExtentPoint32A(hdcMem,"A",1,&sz);
      h2=sz.cy;
      dat->row_heights[line_num]=max(dat->row_heights[line_num],h1+h2+ROW_SPACE_BEETWEEN_LINES+dat->row_border*2);
      secondLineSize.cy = h2;
      }

      }
      -*/
      // Init settings
      selected = ((line_num==dat->selection) && (dat->showSelAlways || dat->exStyle&CLS_EX_SHOWSELALWAYS || IsForegroundWindow(hwnd)) && Drawing->type!=CLCIT_DIVIDER);
      hottrack = dat->exStyle&CLS_EX_TRACKSELECT && Drawing->type!=CLCIT_DIVIDER && dat->iHotTrack==line_num;
      left_pos = clRect.left + dat->leftMargin + indent * dat->groupIndent + subident;
      right_pos = dat->rightMargin;	// Border

      SetRect(&row_rc, clRect.left, y, clRect.right, y + dat->row_heights[line_num]);
      free_row_rc = row_rc;
      free_row_rc.left += left_pos;
      free_row_rc.right -= right_pos;
      free_row_rc.top += dat->row_border;
      free_row_rc.bottom -= dat->row_border;
      free_row_height = free_row_rc.bottom - free_row_rc.top;

      {
        HRGN rgn = CreateRectRgn(row_rc.left, row_rc.top, row_rc.right, row_rc.bottom);
        SelectClipRgn(hdcMem, rgn);
        DeleteObject(rgn);
      }

      // Store pos
      Drawing->pos_indent = free_row_rc.left;
      ZeroMemory(&Drawing->pos_check, sizeof(Drawing->pos_check));
      ZeroMemory(&Drawing->pos_avatar, sizeof(Drawing->pos_avatar));
      ZeroMemory(&Drawing->pos_icon, sizeof(Drawing->pos_icon));
      ZeroMemory(&Drawing->pos_label, sizeof(Drawing->pos_label));
      ZeroMemory(&Drawing->pos_full_first_row, sizeof(Drawing->pos_full_first_row));
      ZeroMemory(&Drawing->pos_extra, sizeof(Drawing->pos_extra));


      // **** Draw Background 

      // Alternating grey
      if (style&CLS_GREYALTERNATE && line_num&1) 
      {
        SkinDrawGlyph(hdcMem,&row_rc,rcPaint,"CL,ID=GreyAlternate");
      }
      // Row background
      if (!dat->force_in_dialog)
      {   //Build request string
        request=GetCLCContactRowBackObject(group,Drawing,indent,line_num,selected,hottrack,dat);
        //TRACE(request);
        //TRACE("\n");
        SkinDrawGlyph(hdcMem,&row_rc,rcPaint,request);
        //mir_free(request);
      }
      if (selected || hottrack)
      { 
        // Selection background (only if hole line - full/less)
        if (dat->HiLightMode == 1) // Full  or default
        {
          if (selected)
            SkinDrawGlyph(hdcMem,&row_rc,rcPaint,"CL,ID=Selection");
          if(hottrack)
            SkinDrawGlyph(hdcMem,&row_rc,rcPaint,"CL,ID=HotTracking");
        }
        else if (dat->HiLightMode == 2) // Less
        {
          if (selected)
            SkinDrawGlyph(hdcMem,&row_rc,rcPaint,"CL,ID=Selection");      //instead of free_row_rc
          if(hottrack)
            SkinDrawGlyph(hdcMem,&row_rc,rcPaint,"CL,ID=HotTracking");
        }
      }

      // **** Checkboxes
      if((style&CLS_CHECKBOXES && Drawing->type==CLCIT_CONTACT) ||
        (style&CLS_GROUPCHECKBOXES && Drawing->type==CLCIT_GROUP) ||
        (Drawing->type==CLCIT_INFO && Drawing->flags&CLCIIF_CHECKBOX))
      {
        //RECT rc;
        HANDLE hTheme = NULL;

        // THEME
        if (IsWinVerXPPlus()) 
        {
          if (!themeAPIHandle) 
          {
            themeAPIHandle = GetModuleHandle(TEXT("uxtheme"));
            if (themeAPIHandle) {
              MyOpenThemeData = (HANDLE (WINAPI *)(HWND,LPCWSTR))MGPROC("OpenThemeData");
              MyCloseThemeData = (HRESULT (WINAPI *)(HANDLE))MGPROC("CloseThemeData");
              MyDrawThemeBackground = (HRESULT (WINAPI *)(HANDLE,HDC,int,int,const RECT *,const RECT *))MGPROC("DrawThemeBackground");
            }
          }
          // Make sure all of these methods are valid (i would hope either all or none work)
          if (MyOpenThemeData
            &&MyCloseThemeData
            &&MyDrawThemeBackground) {
              hTheme = MyOpenThemeData(hwnd,L"BUTTON");
            }
        }

        rc = free_row_rc;
        rc.right = rc.left + dat->checkboxSize;
        rc.top += (rc.bottom - rc.top - dat->checkboxSize) >> 1;
        rc.bottom = rc.top + dat->checkboxSize;

        if (hTheme) {
          MyDrawThemeBackground(hTheme, hdcMem, BP_CHECKBOX, Drawing->flags&CONTACTF_CHECKED?(hottrack?CBS_CHECKEDHOT:CBS_CHECKEDNORMAL):(hottrack?CBS_UNCHECKEDHOT:CBS_UNCHECKEDNORMAL), &rc, &rc);
        }
        else DrawFrameControl(hdcMem,&rc,DFC_BUTTON,DFCS_BUTTONCHECK|DFCS_FLAT|(Drawing->flags&CONTACTF_CHECKED?DFCS_CHECKED:0)|(hottrack?DFCS_HOT:0));
        if (hTheme&&MyCloseThemeData) {
          MyCloseThemeData(hTheme);
          //hTheme = NULL;
        }

        left_pos += dat->checkboxSize + EXTRA_CHECKBOX_SPACE + HORIZONTAL_SPACE;
        free_row_rc.left = row_rc.left + left_pos;

        // Store pos
        Drawing->pos_check = rc;
      }
      /* ------------------------
      else if (!dat->avatars_align_left && !dat->show_icon_only_if_no_avatar)
      {
      // Don't draw, but has to keep empty space
      left_pos += dat->avatar_size + HORIZONTAL_SPACE + EXTRA_SPACE;
      free_row_rc.left = row_rc.left + left_pos;

      // Store pos
      Drawing->pos_avatar = free_row_rc.left;
      }    
      }


      // Draw icon
      iImage = -1;

      if (Drawing->type == CLCIT_GROUP)
      iImage = Drawing->group->expanded ? IMAGE_GROUPOPEN : IMAGE_GROUPSHUT;
      else if (Drawing->type == CLCIT_CONTACT 
      && !(dat->show_icon_only_if_no_avatar && dat->show_avatars 
      && ( (dat->use_avatar_service && Drawing->avatar_data != NULL) ||
      (!dat->use_avatar_service && Drawing->avatar_pos != AVATAR_POS_DONT_HAVE)
      )
      && !Drawing->image_is_special))
      iImage = Drawing->iImage;

      if (iImage != -1) 
      {
      COLORREF colourFg;
      int mode;
      RECT rc = free_row_rc;
      int width;

      if(hottrack) 
      {
      colourFg=dat->hotTextColour;
      mode=ILD_NORMAL;
      }
      else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
      {
      colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; 
      mode=ILD_BLEND50;
      }
      else
      {
      colourFg=dat->selBkColour;
      mode=ILD_NORMAL;
      }

      if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
      GetRealStatus(&group->contact[group->scanIndex],ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
      {
      mode=ILD_SELECTED;
      }

      if (Drawing->type == CLCIT_CONTACT && dat->show_icon_only_if_no_avatar && !dat->avatars_align_left)
      {
      width = max(dat->iconXSpace, dat->avatar_size);

      rc.left += (width - dat->iconXSpace) >> 1;
      }
      else
      {
      width = dat->iconXSpace;
      }

      ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, 
      rc.left, rc.top+((free_row_height-ICON_HEIGHT)>>1),
      0,0,CLR_NONE,colourFg,mode);

      left_pos += width + HORIZONTAL_SPACE + EXTRA_SPACE;

      free_row_rc.left = row_rc.left + left_pos;

      // Store pos
      Drawing->pos_icon = free_row_rc.left;
      }


      // Draw extra icons
      if ((free_row_rc.right - free_row_rc.left > MIN_TEXT_SIZE_TO_DRAW_EXTRA_ICONS + HORIZONTAL_SPACE + dat->extraColumnSpacing) && 
      (!Drawing->isSubcontact || DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0)==0))
      {
      int c = dat->extraColumnsCount;
      int BlendedInActiveState = DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
      int BlendValue = DBGetContactSettingByte(NULL,"CLC","Blend25%",1) ? ILD_BLEND25 : ILD_BLEND50;
      int last_pos = -1;

      for(iImage = dat->extraColumnsCount-1 ; iImage >= 0 ; iImage--) 
      {
      COLORREF colourFg=dat->selBkColour;
      int mode=BlendedInActiveState?BlendValue:ILD_NORMAL;

      if(Drawing->iExtraImage[iImage]==0xFF) 
      continue;

      if(selected) 
      {
      mode=BlendedInActiveState?ILD_NORMAL:ILD_SELECTED;
      }
      else if(hottrack) 
      {
      mode=BlendedInActiveState?ILD_NORMAL:ILD_FOCUS; 
      colourFg=dat->hotTextColour;
      }
      else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
      {
      colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; 
      mode=BlendValue;
      }

      if (dat->MetaIgnoreEmptyExtra) 
      c--; 
      else 
      c=iImage;

      {
      int x = free_row_rc.right - dat->extraColumnSpacing * (dat->extraColumnsCount-c);

      if (x >= free_row_rc.left + MIN_TEXT_SIZE_TO_DRAW_EXTRA_ICONS + HORIZONTAL_SPACE)
      {
      ImageList_DrawEx_New(dat->himlExtraColumns,Drawing->iExtraImage[iImage],hdcMem,
      x, free_row_rc.top+((free_row_height-ICON_HEIGHT)>>1),0,0,CLR_NONE,colourFg,mode);

      last_pos = x;
      }
      }
      }
      if (last_pos != -1)
      {
      right_pos = row_rc.right - last_pos + HORIZONTAL_SPACE;
      free_row_rc.right = row_rc.right - right_pos;

      // Store pos
      Drawing->pos_extra = free_row_rc.right;
      }
      }

      // Select font
      ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);

      // Get text size
      {
      RECT rc = free_row_rc;

      textSize.cy = DrawTextSS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&rc,DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
      if (textSize.cy > free_row_height)
      textSize.cy = free_row_height;

      textSize.cx = rc.right - rc.left;
      if (textSize.cx > free_row_rc.right - free_row_rc.left)
      textSize.cx = free_row_rc.right - free_row_rc.left;
      }
      secondLineSize.cx = 0;
      // secondLineSize.cy = 0;

      // If group, can have the size of count
      if (Drawing->type == CLCIT_GROUP)
      {
      // Group conts?
      szCounts = GetGroupCountsText(dat,&group->contact[group->scanIndex]);

      // Has to draw the count?
      if(szCounts[0]) 
      {
      RECT space_rc = free_row_rc;
      RECT counts_rc = free_row_rc;
      int text_width;

      // Get widths
      DrawTextSS(hdcMem," ",1,&space_rc,DT_CALCRECT | DT_NOPREFIX);

      ChangeToFont(hdcMem,dat,FONTID_GROUPCOUNTS,NULL);
      DrawTextSS(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);

      counts_width = counts_rc.right - counts_rc.left;
      counts_height = counts_rc.bottom - counts_rc.top;
      space_width = space_rc.right - space_rc.left;
      text_width = free_row_rc.right - free_row_rc.left - space_width - counts_width;

      if (text_width > 4)
      {
      if (text_width > textSize.cx)
      {
      text_width = textSize.cx;
      }
      textSize.cx = text_width + space_width + counts_width;
      }
      else
      {
      space_width = 0;
      counts_width = 0;
      }

      ChangeToFont(hdcMem,dat,FONTID_GROUPS,NULL);
      }
      }
      else if (Drawing->type == CLCIT_CONTACT)
      {
      if (dat->show_status_msg && free_row_height > textSize.cy + ROW_SPACE_BEETWEEN_LINES && Drawing->szStatusMsg[0] != '\0')
      {
      RECT rc = free_row_rc;

      //    ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);

      // Get sizes
      //    secondLineSize.cy = DrawTextSS(hdcMem,Drawing->szStatusMsg,lstrlenA(Drawing->szStatusMsg),&rc, 
      //      DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);

      if (secondLineSize.cy > free_row_height - textSize.cy - ROW_SPACE_BEETWEEN_LINES)
      secondLineSize.cy = free_row_height - textSize.cy - ROW_SPACE_BEETWEEN_LINES;

      secondLineSize.cx = rc.right - rc.left;
      if (secondLineSize.cx > free_row_rc.right - free_row_rc.left)
      secondLineSize.cx = free_row_rc.right - free_row_rc.left;


      ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);
      }
      }


      // Store pos
      Drawing->pos_label = min(free_row_rc.left + textSize.cx + 5, free_row_rc.right);

      // Selection background (only if text size)
      if (selected || hottrack)
      {
      if (dat->HiLightMode == 0) // Default
      {
      RECT text_rc = free_row_rc;

      if (secondLineSize.cy > 0)
      {
      int height = textSize.cy + 4 + secondLineSize.cy;

      if (height < text_rc.bottom - text_rc.top)
      {
      text_rc.top += (text_rc.bottom - text_rc.top - height) >> 1;
      }
      text_rc.bottom = text_rc.top + textSize.cy + ROW_SPACE_BEETWEEN_LINES / 2;
      }
      else
      {
      if (textSize.cy < text_rc.bottom - text_rc.top)
      {
      text_rc.top += (text_rc.bottom - text_rc.top - textSize.cy) >> 1;
      text_rc.bottom = text_rc.top + textSize.cy;
      }
      text_rc.bottom = min(text_rc.bottom + SELECTION_BORDER, row_rc.bottom);
      }

      text_rc.top = max(text_rc.top - SELECTION_BORDER, row_rc.top);

      text_rc.right = min(text_rc.left + textSize.cx + SELECTION_BORDER, text_rc.right);
      text_rc.left = max(0, text_rc.left - min(HORIZONTAL_SPACE, SELECTION_BORDER));

      if (selected)
      SkinDrawGlyph(hdcMem,&text_rc,rcPaint,"CL,ID=Selection");
      else if(hottrack)
      SkinDrawGlyph(hdcMem,&text_rc,rcPaint,"CL,ID=Hot");
      }

      // Set color
      if (selected)
      SetTextColor(hdcMem,dat->selTextColour);
      else if(hottrack)
      SetHotTrackColour(hdcMem,dat);
      }

      // Draw text
      switch (Drawing->type)
      {
      case CLCIT_DIVIDER:
      {
      RECT trc = free_row_rc;
      RECT rc = free_row_rc;
      rc.top += free_row_height>>1; 
      rc.bottom=rc.top+2;
      rc.right = rc.left + ((rc.right-rc.left-textSize.cx)>>1) - 3;
      //DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);

      trc.left = rc.right + 3;
      if (textSize.cy < trc.bottom - trc.top)
      {
      trc.top += (trc.bottom - trc.top - textSize.cy) >> 1;
      trc.bottom = trc.top + textSize.cy;
      }
      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&trc,DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);

      rc.left = rc.right + 6 + textSize.cx;
      rc.right = free_row_rc.right;
      //DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
      break;
      }
      case CLCIT_GROUP:
      {
      RECT text_rc = free_row_rc;

      // Get text rectangle
      textSize.cx -= counts_width + space_width;

      if (textSize.cx < text_rc.right - text_rc.left)
      {
      text_rc.right = text_rc.left + textSize.cx;
      }
      if (textSize.cy < text_rc.bottom - text_rc.top)
      {
      text_rc.top += (text_rc.bottom - text_rc.top - textSize.cy) >> 1;
      text_rc.bottom = text_rc.top + textSize.cy;
      }

      // Draw text
      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&text_rc,DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);

      if(selected && dat->szQuickSearch[0] != '\0') 
      {
      SetTextColor(hdcMem, dat->quickSearchColour);
      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(dat->szQuickSearch),&text_rc,DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
      }

      // Has to draw the count?
      if(szCounts[0] && counts_width > 0) 
      {
      RECT counts_rc = free_row_rc;

      counts_rc.left = text_rc.right + space_width;
      counts_rc.right = counts_rc.left + counts_width;

      if (counts_height < counts_rc.bottom - counts_rc.top)
      {
      counts_rc.top += (counts_rc.bottom - counts_rc.top - counts_height) >> 1;
      counts_rc.bottom = counts_rc.top + counts_height;
      }

      ChangeToFont(hdcMem,dat,FONTID_GROUPCOUNTS,NULL);
      if (selected)
      SetTextColor(hdcMem,dat->selTextColour);
      else if(hottrack)
      SetHotTrackColour(hdcMem,dat);

      // Draw counts
      DrawTextSS(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,DT_NOPREFIX | DT_SINGLELINE);
      }

      // Update free
      left_pos += text_rc.right - text_rc.left + space_width + counts_width;
      free_row_rc.left = row_rc.left + left_pos;

      if (dat->exStyle&CLS_EX_LINEWITHGROUPS) 
      {
      if (free_row_rc.right > free_row_rc.left + 6)
      {
      RECT rc = free_row_rc;
      rc.top += (rc.bottom - rc.top) >> 1;
      rc.bottom = rc.top + 2;
      rc.left += 6;

      //DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
      }
      }
      break;
      }
      case CLCIT_CONTACT:
      {
      RECT text_rc = free_row_rc;
      RECT second_line_rc = free_row_rc;

      if (secondLineSize.cy > 0)
      {
      int height = textSize.cy + ROW_SPACE_BEETWEEN_LINES + secondLineSize.cy;

      if (height < text_rc.bottom - text_rc.top)
      {
      text_rc.top += (text_rc.bottom - text_rc.top - height) >> 1;
      }
      text_rc.bottom = text_rc.top + textSize.cy;

      second_line_rc.top = text_rc.bottom + ROW_SPACE_BEETWEEN_LINES;
      second_line_rc.bottom = second_line_rc.top + secondLineSize.cy;
      }
      else
      {
      if (textSize.cy < text_rc.bottom - text_rc.top)
      {
      text_rc.top += (text_rc.bottom - text_rc.top - textSize.cy) >> 1;
      text_rc.bottom = text_rc.top + textSize.cy;
      }
      }

      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&text_rc,DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);

      if(selected && dat->szQuickSearch[0] != '\0') 
      {
      SetTextColor(hdcMem, dat->quickSearchColour);
      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(dat->szQuickSearch),&text_rc,DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
      }

      if (secondLineSize.cy > 0)
      {
      ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);

      // Selection background (only if hole line - full/less)
      if (selected && (dat->HiLightMode == 1 || dat->HiLightMode == 2))
      SetTextColor(hdcMem, dat->selTextColour);

      DrawTextSS(hdcMem,Drawing->szStatusMsg,lstrlenA(Drawing->szStatusMsg),&second_line_rc,DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
      }
      break;
      }
      default: // CLCIT_INFO
      {
      RECT text_rc = free_row_rc;

      if (textSize.cy < text_rc.bottom - text_rc.top)
      {
      text_rc.top += (text_rc.bottom - text_rc.top - textSize.cy) >> 1;
      text_rc.bottom = text_rc.top + textSize.cy;
      }

      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&text_rc,DT_END_ELLIPSIS | DT_NOPREFIX);

      if(selected && dat->szQuickSearch[0] != '\0') 
      {
      SetTextColor(hdcMem, dat->quickSearchColour);
      DrawTextSS(hdcMem,Drawing->szText,lstrlenA(dat->szQuickSearch),&text_rc,DT_END_ELLIPSIS | DT_NOPREFIX);
      }
      }
      }  

      --------------------------*/  

      InternalPaintRowItems(hwnd, hdcMem, dat, Drawing, row_rc, free_row_rc, left_pos, right_pos, selected, hottrack, rcPaint);
      if (request && !dat->force_in_dialog)
      {
        request[6]='O';
        request[7]='v';
        request[8]='l';
        SkinDrawGlyph(hdcMem,&row_rc,rcPaint,request);
        mir_free(request);
        request=NULL;
      }
    }


    // if (y > rcPaint->top - dat->row_heights[line_num] && y < rcPaint->bottom) 
    y += dat->row_heights[line_num];

    //increment by subcontacts
    if (group->contact && group->contact[group->scanIndex].subcontacts!=NULL && group->contact[group->scanIndex].type!=CLCIT_GROUP)
    {
      if (group->contact[group->scanIndex].SubExpanded && dat->expandMeta)
      {
        if (subindex<group->contact[group->scanIndex].SubAllocated-1)
        {
          subindex++;
        }
        else
        {
          subindex=-1;
        }
      }
    }

    if(subindex==-1)
    {
      if(group->contact[group->scanIndex].type==CLCIT_GROUP && group->contact[group->scanIndex].group->expanded) 
      {
        group=group->contact[group->scanIndex].group;
        indent++;
        group->scanIndex=0;
        subindex=-1;
        continue;
      }
      group->scanIndex++;
    }
  }

  //LeaveCriticalSection(&(dat->lockitemCS));
  SelectClipRgn(hdcMem, NULL);
  if(dat->iInsertionMark!=-1) {	//insertion mark
    HBRUSH hBrush,hoBrush;
    POINT pts[8];
    HRGN hRgn;

    pts[0].x=dat->leftMargin; pts[0].y= RowHeights_GetItemTopY(dat, dat->iInsertionMark) - dat->yScroll - 4;
    pts[1].x=pts[0].x+2;      pts[1].y=pts[0].y+3;
    pts[2].x=clRect.right-4;  pts[2].y=pts[1].y;
    pts[3].x=clRect.right-1;  pts[3].y=pts[0].y-1;
    pts[4].x=pts[3].x;        pts[4].y=pts[0].y+7;
    pts[5].x=pts[2].x+1;      pts[5].y=pts[1].y+2;
    pts[6].x=pts[1].x;        pts[6].y=pts[5].y;
    pts[7].x=pts[0].x;        pts[7].y=pts[4].y;
    hRgn=CreatePolygonRgn(pts,sizeof(pts)/sizeof(pts[0]),ALTERNATE);
    hBrush=CreateSolidBrush(dat->fontInfo[FONTID_CONTACTS].colour);
    hoBrush=(HBRUSH)SelectObject(hdcMem,hBrush);
    FillRgn(hdcMem,hRgn,hBrush);
    SelectObject(hdcMem,hoBrush);
    DeleteObject(hBrush);
  }
  if(!grey)
  {
	if (NotInMain || dat->force_in_dialog)
	{
		BitBlt(hdc,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,hdcMem,rcPaint->left,rcPaint->top,SRCCOPY);
	}
  }
  if(hBrushAlternateGrey) DeleteObject(hBrushAlternateGrey);
  if(grey) {
    PBYTE bits;
    BITMAPINFOHEADER bmih={0};
    int i;
    int greyRed,greyGreen,greyBlue;
    COLORREF greyColour;
    bmih.biBitCount=32;
    bmih.biSize=sizeof(bmih);
    bmih.biCompression=BI_RGB;
    bmih.biHeight=-clRect.bottom;
    bmih.biPlanes=1;
    bmih.biWidth=clRect.right;
    bits=(PBYTE)mir_alloc(4*bmih.biWidth*-bmih.biHeight);
    GetDIBits(hdc,hBmpOsb,0,clRect.bottom,bits,(BITMAPINFO*)&bmih,DIB_RGB_COLORS);
    greyColour=GetSysColor(COLOR_3DFACE);
    greyRed=GetRValue(greyColour)*2;
    greyGreen=GetGValue(greyColour)*2;
    greyBlue=GetBValue(greyColour)*2;
    if(divide3[0]==255) {
      for(i=0;i<sizeof(divide3)/sizeof(divide3[0]);i++) divide3[i]=(i+1)/3;
    }
    for(i=4*clRect.right*clRect.bottom-4;i>=0;i-=4) {
      bits[i]=divide3[bits[i]+greyBlue];
      bits[i+1]=divide3[bits[i+1]+greyGreen];
      bits[i+2]=divide3[bits[i+2]+greyRed];
    }
    SetDIBitsToDevice(hdc,0,0,clRect.right,clRect.bottom,0,0,0,clRect.bottom,bits,(BITMAPINFO*)&bmih,DIB_RGB_COLORS);
    mir_free(bits);
  }

  // Restore some draw states
  if (old_bk_mode != TRANSPARENT)
    SetBkMode(hdcMem, old_bk_mode);

  if (old_stretch_mode != HALFTONE)
    SetStretchBltMode(hdcMem, old_stretch_mode);
  SelectObject(hdcMem,hdcMemOldFont);
  if (NotInMain || dat->force_in_dialog)
  {
   SelectObject(hdcMem,oldbmp);
   DeleteObject(hBmpOsb);
   DeleteDC(hdcMem);
  }
  
  

}


void PaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint)
{

  if (SED.cbSize==sizeof(SED)&&SED.PaintClc!=NULL)
  {
    SED.PaintClc(hwnd,dat,hdc,rcPaint,hClcProtoCount,clcProto,himlCListClc);
    return;
  }
  InternalPaintClc(hwnd,dat,hdc,rcPaint);
}