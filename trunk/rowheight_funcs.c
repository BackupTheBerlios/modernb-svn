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

Created by Pescuma

*/
#include "commonheaders.h"
#include "rowheight_funcs.h"


extern int GetRealStatus(struct ClcContact * contact, int status);
extern int GetBasicFontID(struct ClcContact * contact);

/*
*
*/
#include "modern_row.h"
ROWCELL	* gl_RowTabAccess[TC_ELEMENTSCOUNT+1]={0};	// ������, ����� ������� �������������� ������ � ��������� ��������.
ROWCELL * gl_RowRoot;
extern void GetTextSize(SIZE *text_size, HDC hdcMem, RECT free_row_rc, TCHAR *szText, SortedList *plText, UINT uTextFormat, int smiley_height);
extern HFONT ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight);

void InitModernRow()
{
  gl_RowRoot=cppInitModernRow(gl_RowTabAccess);
  return;
}

SIZE GetAvatarSize(int imageWidth, int imageHeight, int maxWidth, int maxHeight)
{
  float scalefactor=0;
  SIZE sz={0};
  if (imageWidth==0 || imageHeight==0) return sz;
  if (maxWidth==0) maxWidth=maxHeight;
  scalefactor=min((float)maxWidth/imageWidth,(float)maxHeight/imageHeight); 
  sz.cx=(LONG)(imageWidth*scalefactor);
  sz.cy=(LONG)(imageHeight*scalefactor);
  return sz;
}

int ModernCalcRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item)
{
  BYTE i=0;
  int res=0;
  int height=0;
  BOOL hasAvatar=FALSE;
  DWORD style=GetWindowLong(hwnd,GWL_STYLE);
  if (!RowHeights_Alloc(dat, item + 1))
    return -1;
  if (!pcli->hwndContactTree) return 0;
  //if (!gl_RowRoot) InitModernRow();
  if (dat->hWnd!=pcli->hwndContactTree || !gl_RowRoot || contact->type == CLCIT_GROUP) 
  {
    int tmp=0;
    tmp = dat->fontModernInfo[GetBasicFontID(contact)].fontHeight;
    if (dat->text_replace_smileys && dat->first_line_draw_smileys && !dat->text_resize_smileys)
      tmp = max(tmp, contact->iTextMaxSmileyHeight);
    if (contact->type == CLCIT_GROUP)
    {
      char * szCounts = pcli->pfnGetGroupCountsText(dat, contact);
      // Has the count?
      if(szCounts && szCounts[0])
        tmp=max(tmp,dat->fontModernInfo[contact->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS].fontHeight);
    }
    tmp=max(tmp, ICON_HEIGHT);       
    tmp=max(tmp,dat->row_min_heigh);
    tmp+=dat->row_border*2;
    tmp+=(contact->type == CLCIT_GROUP && 
      contact->groupId==0 && 
      contact->group->parent->cl.items[0]!=contact) ?dat->row_before_group_space:0;
    if (item!=-1) dat->row_heights[item] = tmp;
    return tmp;
  }
  hasAvatar=(dat->use_avatar_service && contact->avatar_data != NULL) ||(!dat->use_avatar_service && contact->avatar_pos != AVATAR_POS_DONT_HAVE);
  while (gl_RowTabAccess[i]!=NULL)
  {
    if (gl_RowTabAccess[i]->type!=TC_SPACE)
    {
      gl_RowTabAccess[i]->h=0;
      gl_RowTabAccess[i]->w=0;
      SetRect(&(gl_RowTabAccess[i]->r),0,0,0,0);
      switch (gl_RowTabAccess[i]->type)
      {
      case TC_TEXT1:
        {
          int tmp=0;
          tmp = dat->fontModernInfo[GetBasicFontID(contact)].fontHeight;
          if (dat->text_replace_smileys && dat->first_line_draw_smileys && !dat->text_resize_smileys)
            tmp = max(tmp, contact->iTextMaxSmileyHeight);
          if (item==-1)
          {
            //calculate text width here
            SIZE size={0};
            RECT dummyRect={0,0,1024,tmp};
            HDC hdc=CreateCompatibleDC(NULL);
            ChangeToFont(hdc,dat,GetBasicFontID(contact),NULL);
            GetTextSize(&size,hdc,dummyRect,contact->szText,contact->plText,0, dat->text_resize_smileys ? 0 : contact->iTextMaxSmileyHeight);
            if (contact->type==CLCIT_GROUP)
            {
              char * szCounts = pcli->pfnGetGroupCountsText(dat, contact);
              if(szCounts && strlen(szCounts)>0) 
              {
                RECT count_rc={0};
                // calc width and height
                ChangeToFont(hdc,dat,contact->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
                DrawTextS(hdc,_T(" "),1,&count_rc,DT_CALCRECT | DT_NOPREFIX);
                size.cx +=count_rc.right-count_rc.left;
                count_rc.right=0;
                count_rc.left=0;
                DrawTextSA(hdc,szCounts,lstrlenA(szCounts),&count_rc,DT_CALCRECT);
                size.cx +=count_rc.right-count_rc.left;
                tmp = max(tmp,count_rc.bottom-count_rc.top);
              }
            }
            gl_RowTabAccess[i]->w=size.cx;
			SelectObject(hdc,GetStockObject(DEFAULT_GUI_FONT));
			DeleteDC(hdc);
          }
          gl_RowTabAccess[i]->h=tmp;

          break;
        }
      case TC_TEXT2:
        {
          int tmp=0;
          if (dat->second_line_show && contact->szSecondLineText)
          {
            tmp = dat->fontModernInfo[FONTID_SECONDLINE].fontHeight;
            if (dat->text_replace_smileys && dat->second_line_draw_smileys && !dat->text_resize_smileys)
              tmp = max(tmp, contact->iSecondLineMaxSmileyHeight);
            if (item==-1)
            {
              //calculate text width here

              SIZE size={0};
              RECT dummyRect={0,0,1024,tmp};
              HDC hdc=CreateCompatibleDC(NULL);
              ChangeToFont(hdc,dat,FONTID_SECONDLINE,NULL);
              GetTextSize(&size,hdc,dummyRect,contact->szSecondLineText,contact->plSecondLineText,0, dat->text_resize_smileys ? 0 : contact->iSecondLineMaxSmileyHeight);
              gl_RowTabAccess[i]->w=size.cx;
              SelectObject(hdc,GetStockObject(DEFAULT_GUI_FONT));
			  DeleteDC(hdc);
            }
          }
          gl_RowTabAccess[i]->h=tmp;
          break;
        }
      case TC_TEXT3:
        {
          int tmp=0;
          if (dat->third_line_show && contact->szThirdLineText)
          {
            tmp = dat->fontModernInfo[FONTID_THIRDLINE].fontHeight;
            if (dat->text_replace_smileys && dat->third_line_draw_smileys && !dat->text_resize_smileys)
              tmp = max(tmp, contact->iThirdLineMaxSmileyHeight);
            if (item==-1)
            {
              //calculate text width here
              SIZE size={0};
              RECT dummyRect={0,0,1024,tmp};
              HDC hdc=CreateCompatibleDC(NULL);
              ChangeToFont(hdc,dat,FONTID_THIRDLINE,NULL);
              GetTextSize(&size,hdc,dummyRect,contact->szThirdLineText,contact->plThirdLineText,0, dat->text_resize_smileys ? 0 : contact->iThirdLineMaxSmileyHeight);
              gl_RowTabAccess[i]->w=size.cx;
              SelectObject(hdc,GetStockObject(DEFAULT_GUI_FONT));
			  DeleteDC(hdc);
            }
          }
          gl_RowTabAccess[i]->h=tmp;			    
          break;
        }
      case TC_STATUS:
        {
          if (
            (contact->type == CLCIT_GROUP && !dat->row_hide_group_icon)||
            (	contact->type == CLCIT_CONTACT && contact->iImage != -1 &&
            !(dat->icon_hide_on_avatar && dat->avatars_show	&& (hasAvatar|| (!hasAvatar && dat->icon_draw_on_avatar_space && contact->iImage != -1)) && !contact->image_is_special))
            )
          {
            gl_RowTabAccess[i]->h=ICON_HEIGHT;
            gl_RowTabAccess[i]->w=ICON_HEIGHT;
          }
          break;
        }
      case TC_AVATAR:
        {
          if (dat->avatars_show && 
            contact->type == CLCIT_CONTACT && 
            (hasAvatar || (dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space && contact->iImage != -1)	)
            )
          {
            SIZE sz={0};
            int iW=0, iH=0;
            if (dat->use_avatar_service)         
            {
              if (contact->avatar_data)
              {
                iH=contact->avatar_data->bmHeight;
                iW=contact->avatar_data->bmWidth;
              }
            }
            else if (dat->avatar_cache.nodes) 
            {
              iW = dat->avatar_cache.nodes[contact->avatar_pos].width;
              iH = dat->avatar_cache.nodes[contact->avatar_pos].height;
            }					
            sz=GetAvatarSize(iW,iH,dat->avatars_maxwidth_size,dat->avatars_maxheight_size);
            if ((sz.cx==0 || sz.cy==0) &&  dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space && contact->iImage != -1)
            { sz.cx=ICON_HEIGHT; sz.cy=ICON_HEIGHT; }

            gl_RowTabAccess[i]->h=sz.cy;
            gl_RowTabAccess[i]->w=sz.cx;
          }

          break;
        }
      case TC_EXTRA:
        {
          // Draw extra icons

          if (contact->type == CLCIT_CONTACT && 
            (!contact->isSubcontact || DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0) == 0 && dat->extraColumnsCount > 0))
          {
            BOOL hasExtra=FALSE;
            int k;
            int width=0;
            for (k=0; k<dat->extraColumnsCount; k++)
              if (contact->iExtraImage[k]!=255 || !dat->MetaIgnoreEmptyExtra) 
              {
                hasExtra=TRUE;
                if (item!=-1) break;
                width+=(width>0)?dat->extraColumnSpacing:(dat->extraColumnSpacing-2);
              }
              if (hasExtra) 
              {
                gl_RowTabAccess[i]->h=ICON_HEIGHT;
                gl_RowTabAccess[i]->w=width;
              }
          }
          break;
        }
      case TC_TIME:
        {
          if (contact->type == CLCIT_CONTACT && dat->contact_time_show && contact->timezone != -1 && 
            (!dat->contact_time_show_only_if_different || contact->timediff != 0))
          {
            gl_RowTabAccess[i]->h=dat->fontModernInfo[FONTID_CONTACT_TIME].fontHeight;
            if (item==-1)
            {

              DBTIMETOSTRINGT dbtts;
              time_t contact_time;
              TCHAR szResult[80];

              contact_time = time(NULL) - contact->timediff;
              szResult[0] = '\0';

              dbtts.szDest = szResult;
              dbtts.cbDest = sizeof(szResult);
              dbtts.szFormat = _T("t");
              CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);

              if (szResult[0] != '\0')
              {
                SIZE text_size={0};
                RECT rc={0};
                // Select font
                HDC hdc=CreateCompatibleDC(NULL);
                ChangeToFont(hdc,dat,FONTID_CONTACT_TIME,NULL);

                // Get text size
                text_size.cy = DrawTextS(hdc, szResult, lstrlen(szResult), &rc, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
                SelectObject(hdc,GetStockObject(DEFAULT_GUI_FONT));
				DeleteDC(hdc);
                text_size.cx = rc.right - rc.left;
                gl_RowTabAccess[i]->w=text_size.cx;

              }
            }
          }				  
          break;
        }
      }
    }
    i++;
  }

  height=cppCalculateRowHeight(gl_RowRoot);
  height+=dat->row_border*2;
  height=max(height,dat->row_min_heigh);
  if (item!=-1) dat->row_heights[item] = height;
  return height;
}




/*
*
*/




BOOL RowHeights_Initialize(struct	ClcData	*dat)
{
  dat->max_row_height	=	0;
  dat->row_heights_size	=	0;
  dat->row_heights_allocated = 0;
  dat->row_heights = NULL;

  return TRUE;
}

void RowHeights_Free(struct ClcData *dat)
{
  if (dat->row_heights != NULL) 
  {
    free(dat->row_heights);
    dat->row_heights = NULL;
  }

  dat->row_heights_allocated = 0;
  dat->row_heights_size = 0;
}

void RowHeights_Clear(struct ClcData *dat)
{
  dat->row_heights_size = 0;
}


BOOL RowHeights_Alloc(struct ClcData *dat, int size)
{
  if (size > dat->row_heights_size)
  {
    if (size > dat->row_heights_allocated)
    {
      int size_grow = size;

      size_grow += 100 - (size_grow % 100);

      if (dat->row_heights != NULL)
      {
        int *tmp = (int *) realloc((void *)dat->row_heights, sizeof(int) * size_grow);

        if (tmp == NULL)
        {
          TRACE("Out of memory: realloc returned NULL (RowHeights_Alloc)");
          RowHeights_Free(dat);
          return FALSE;
        }

        dat->row_heights = tmp;
        memset(dat->row_heights+(dat->row_heights_allocated),0,sizeof(int) * (size_grow-dat->row_heights_allocated));
      }
      else
      {
        dat->row_heights = (int *) malloc(sizeof(int) * size_grow);

        if (dat->row_heights == NULL)
        {
          TRACE("Out of memory: alloc returned NULL (RowHeights_Alloc)");
          RowHeights_Free(dat);
          return FALSE;
        }
        memset(dat->row_heights,0,sizeof(int) * size_grow);
      }

      dat->row_heights_allocated = size_grow;
    }

    dat->row_heights_size = size;
  }

  return TRUE;
}


// Calc and store max row height
int RowHeights_GetMaxRowHeight(struct ClcData *dat, HWND hwnd)
{
  int max_height = 0, i, tmp;
  DWORD style=GetWindowLong(hwnd,GWL_STYLE);

  if (!dat->text_ignore_size_for_row_height)
  {
    int contact_fonts[] = {FONTID_CONTACTS, FONTID_INVIS, FONTID_OFFLINE, FONTID_NOTONLIST, FONTID_OFFINVIS, 
      FONTID_AWAY,FONTID_DND, FONTID_NA, FONTID_OCCUPIED, FONTID_CHAT, FONTID_INVISIBLE, 
      FONTID_PHONE, FONTID_LUNCH};
    int other_fonts[] = {FONTID_OPENGROUPS, FONTID_OPENGROUPCOUNTS,FONTID_CLOSEDGROUPS, FONTID_CLOSEDGROUPCOUNTS, FONTID_DIVIDERS, FONTID_CONTACT_TIME};

    // Get contact font size
    tmp = 0;
    for (i = 0 ; i < MAX_REGS(contact_fonts) ; i++)
    {
      if (tmp < dat->fontModernInfo[contact_fonts[i]].fontHeight)
        tmp = dat->fontModernInfo[contact_fonts[i]].fontHeight;
    }
    if (dat->text_replace_smileys && dat->first_line_draw_smileys && !dat->text_resize_smileys)
    {
      tmp = max(tmp, dat->text_smiley_height);
    }
    max_height += tmp; 

    if (dat->second_line_show)
    {
      tmp = dat->fontModernInfo[FONTID_SECONDLINE].fontHeight;
      if (dat->text_replace_smileys && dat->second_line_draw_smileys && !dat->text_resize_smileys)
      {
        tmp = max(tmp, dat->text_smiley_height);
      }
      max_height += dat->second_line_top_space + tmp; 
    }

    if (dat->third_line_show)
    {
      tmp = dat->fontModernInfo[FONTID_THIRDLINE].fontHeight;
      if (dat->text_replace_smileys && dat->third_line_draw_smileys && !dat->text_resize_smileys)
      {
        tmp = max(tmp, dat->text_smiley_height);
      }
      max_height += dat->third_line_top_space + tmp; 
    }

    // Get other font sizes
    for (i = 0 ; i < MAX_REGS(other_fonts) ; i++)
    {
      if (max_height < dat->fontModernInfo[other_fonts[i]].fontHeight)
        max_height = dat->fontModernInfo[other_fonts[i]].fontHeight;
    }
  }

  // Avatar size
  if (dat->avatars_show && !dat->avatars_ignore_size_for_row_height)
  {
    max_height = max(max_height, dat->avatars_maxheight_size);
  }

  // Checkbox size
  if (style&CLS_CHECKBOXES || style&CLS_GROUPCHECKBOXES)
  {
    max_height = max(max_height, dat->checkboxSize);
  }

  // Icon size
  if (!dat->icon_ignore_size_for_row_height)
  {
    max_height = max(max_height, ICON_HEIGHT);
  }

  max_height += 2 * dat->row_border;

  // Min size
  max_height = max(max_height, dat->row_min_heigh);

  dat->max_row_height = max_height;

  return max_height;
}


// Calc and store row height for all itens in the list
void RowHeights_CalcRowHeights(struct ClcData *dat, HWND hwnd)
{
  int indent, subident, subindex, line_num;
  struct ClcContact *Drawing;
  struct ClcGroup *group;

  // Draw lines
  group=&dat->list;
  group->scanIndex=0;
  indent=0;
  subindex=-1;
  line_num = -1;

  RowHeights_Clear(dat);

  while(TRUE)
  {
    if (subindex==-1)
    {
      if (group->scanIndex==group->cl.count) 
      {
        group=group->parent;
        indent--;
        if(group==NULL) break;	// Finished list
        group->scanIndex++;
        continue;
      }

      // Get item to draw
      Drawing = group->cl.items[group->scanIndex];
      subident = 0;
    }
    else
    {
      // Get item to draw
      Drawing = &(group->cl.items[group->scanIndex]->subcontacts[subindex]);
      subident = dat->subIndent;
    }

    line_num++;

    // Calc row height
    if (!gl_RowRoot) RowHeights_GetRowHeight(dat, hwnd, Drawing, line_num);
    else ModernCalcRowHeight(dat, hwnd, Drawing, line_num);

    //increment by subcontacts
    if (group->cl.items[group->scanIndex]->subcontacts!=NULL && group->cl.items[group->scanIndex]->type!=CLCIT_GROUP)
    {
      if (group->cl.items[group->scanIndex]->SubExpanded && dat->expandMeta)
      {
        if (subindex<group->cl.items[group->scanIndex]->SubAllocated-1)
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
      if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP && group->cl.items[group->scanIndex]->group->expanded) 
      {
        group=group->cl.items[group->scanIndex]->group;
        indent++;
        group->scanIndex=0;
        subindex=-1;
        continue;
      }
      group->scanIndex++;
    }
  }
}


// Calc and store row height
int RowHeights_GetRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item)
{
  int height = 0;

  if (gl_RowRoot)
    return ModernCalcRowHeight(dat, hwnd, contact, item);
  else
  {

    DWORD style=GetWindowLong(hwnd,GWL_STYLE);
    //TODO replace futher code with new rowheight definition
    int tmp;
    if (!RowHeights_Alloc(dat, item + 1))
      return -1;

    if (dat->row_variable_height)
    {
      if (!dat->text_ignore_size_for_row_height)
      {
        tmp = dat->fontModernInfo[GetBasicFontID(contact)].fontHeight;
        if (dat->text_replace_smileys && dat->first_line_draw_smileys && !dat->text_resize_smileys)
        {
          tmp = max(tmp, contact->iTextMaxSmileyHeight);
        }
        height += tmp;

        if (dat->second_line_show && contact->szSecondLineText)
        {
          tmp = dat->fontModernInfo[FONTID_SECONDLINE].fontHeight;
          if (dat->text_replace_smileys && dat->second_line_draw_smileys && !dat->text_resize_smileys)
          {
            tmp = max(tmp, contact->iSecondLineMaxSmileyHeight);
          }
          height += dat->second_line_top_space + tmp;
        }

        if (dat->third_line_show && contact->szThirdLineText)
        {
          tmp = dat->fontModernInfo[FONTID_THIRDLINE].fontHeight;
          if (dat->text_replace_smileys && dat->third_line_draw_smileys && !dat->text_resize_smileys)
          {
            tmp = max(tmp, contact->iThirdLineMaxSmileyHeight);
          }
          height += dat->third_line_top_space + tmp;
        }
      }

      // Avatar size
      if (dat->avatars_show && !dat->avatars_ignore_size_for_row_height && 
        contact->type == CLCIT_CONTACT && 
        (
        (dat->use_avatar_service && contact->avatar_data != NULL) ||
        (!dat->use_avatar_service && contact->avatar_pos != AVATAR_POS_DONT_HAVE)
        )
        )
      {
        height = max(height, dat->avatars_maxheight_size);
      }

      // Checkbox size
      if((style&CLS_CHECKBOXES && contact->type==CLCIT_CONTACT) ||
        (style&CLS_GROUPCHECKBOXES && contact->type==CLCIT_GROUP) ||
        (contact->type==CLCIT_INFO && contact->flags&CLCIIF_CHECKBOX))
      {
        height = max(height, dat->checkboxSize);
      }

      // Icon size
      if (!dat->icon_ignore_size_for_row_height)
      {
        if (contact->type == CLCIT_GROUP 
          || (contact->type == CLCIT_CONTACT && contact->iImage != -1 
          && !(dat->icon_hide_on_avatar && dat->avatars_show
          && ( (dat->use_avatar_service && contact->avatar_data != NULL) ||
          (!dat->use_avatar_service && contact->avatar_pos != AVATAR_POS_DONT_HAVE)
          )
          && !contact->image_is_special)))
        {
          height = max(height, ICON_HEIGHT);
        }
      }

      height += 2 * dat->row_border;

      // Min size
      height = max(height, dat->row_min_heigh);
    }
    else
    {
      height = dat->max_row_height;
    }

    dat->row_heights[item] = height;

    return height;
  }
}


// Calc item top Y (using stored data)
int RowHeights_GetItemTopY(struct ClcData *dat, int item)
{
  int i;
  int y = 0;

  if (item >= dat->row_heights_size)
    return -1;

  for (i = 0 ; i < item ; i++)
  {
    y += dat->row_heights[i];
  }

  return y;
}


// Calc item bottom Y (using stored data)
int RowHeights_GetItemBottomY(struct ClcData *dat, int item)
{
  int i;
  int y = 0;

  if (item >= dat->row_heights_size)
    return -1;

  for (i = 0 ; i <= item ; i++)
  {
    y += dat->row_heights[i];
  }

  return y;
}


// Calc total height of rows (using stored data)
int RowHeights_GetTotalHeight(struct ClcData *dat)
{
  int i;
  int y = 0;

  for (i = 0 ; i < dat->row_heights_size ; i++)
  {
    y += dat->row_heights[i];
  }

  return y;
}

// Return the line that pos_y is at or -1 (using stored data)
int RowHeights_HitTest(struct ClcData *dat, int pos_y)
{
  int i;
  int y = 0;

  if (pos_y < 0)
    return -1;

  for (i = 0 ; i < dat->row_heights_size ; i++)
  {
    y += dat->row_heights[i];

    if (pos_y < y)
      return i;
  }

  return -1;
}

int RowHeights_GetHeight(struct ClcData *dat, int item)
{	
  if ( item >= dat->row_heights_size)
    return dat->max_row_height;

  return dat->row_heights[ item ];
}
