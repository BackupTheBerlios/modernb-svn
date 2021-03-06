/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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

Created by Anton Senko aka ZORG , tweaked by Artem Shpynov aka FYR

*/

#define _CPPCODE


#include <malloc.h>
#include <tchar.h>

#ifdef _DEBUG
#ifndef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>
#endif
#endif

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <math.h>
#include "resource.h"


extern "C"
{
  /*struct MM_INTERFACE memoryManagerInterface;
  struct PLUGINLINK * pluginLink;
  #include "newpluginapi.h"
  #include "m_database.h"
  #include "m_system.h"  
  char * DBGetStringA(HANDLE, const char*, const char*);
	#include "forkthread.h"
	#include "win2k.h"
  */
  #include "commonheaders.h"
}

#include "modern_row.h"

//Futher declaration
extern "C" BOOL rowParse(ROWCELL* &cell, ROWCELL* parent, char *tbuf, int &hbuf, int &sequence, ROWCELL* *RowTabAccess );
extern "C" void rowSizeWithReposition(ROWCELL* &root, int width);
void rowCalculateMinSize(ROWCELL* cell);
void rowEqualize(ROWCELL* cell);
void rowResetEmptyRects(ROWCELL* cell);
extern "C" void rowDeleteTree(ROWCELL* cell);
////////



//extern ROWCELL * gl_RowRoot;	// ��������� �� �������� ��� <contact> � �������
//ROWOBJECTS RowTA;	// ���������, ����� ������� �������������� ������ � ��������� ��������.
					// ����������� ��� ���������� ��-� RowParce. ������� �������� ��� rowParserGetParam


										// ����������� ��� ���������� ��-� RowParce. ������� �������� ��� rowParserGetParam

char *tmplbuf;		// ����� ��� �������� ������� � ��������� ����

/*
#define mir_free(ptr) { mir_free_proxy(ptr); ptr=NULL; }

int mir_free_proxy(void *ptr)
{
	if (ptr==NULL||IsBadCodePtr((FARPROC)ptr))
	{
		DebugBreak();
		return 0;
	}
    memoryManagerInterface.mmi_free(ptr);
	return 0;

}
*/
#ifndef TEST_SOURCE
	extern "C" 
#endif
ROWCELL *cppInitModernRow(ROWCELL	** tabAccess)
{
	int fsize;
	int seq=0;
  ROWCELL * RowRoot=NULL; 
	FILE * hFile;
	int i=0;
	if (!DBGetContactSettingByte(NULL,"ModernData","UseAdvancedRowLayout",0)) return NULL;
  tmplbuf=NULL;
    if (DBGetContactSettingByte(NULL,"ModernData","UseAdvancedRowLayout",0)==1)
	tmplbuf=DBGetStringA(NULL,"ModernData","RowTemplate");
  if (tmplbuf) 
  {
      rowParse(RowRoot, RowRoot, tmplbuf, i, seq,tabAccess);
      mir_free(tmplbuf);
      return RowRoot;
  }
  if (hFile = fopen("template.txt", "rb"))
  {  
	  fsize = _filelength(_fileno(hFile));
	  tmplbuf = (char*)malloc(fsize+1);
	  ZeroMemory(tmplbuf, fsize+1);
 
	  for (i=0; i<fsize; i++) tmplbuf[i] = getc(hFile);
		  tmplbuf[i] = 0;
		  i = 0;     
		  rowParse(RowRoot, RowRoot, tmplbuf, i, seq,tabAccess);
      DBWriteContactSettingString(NULL,"ModernData","RowTemplate",tmplbuf);
      free(tmplbuf);
		  fclose(hFile);
		  return RowRoot;
  }
  return NULL;

}

extern "C" void cppDeleteTree(ROWCELL	* RowRoot)
{
	ROWCELL *rc=RowRoot;
	rowDeleteTree(rc);
}

extern "C" int cppCalculateRowHeight(ROWCELL	*RowRoot)
{
  RowRoot->h=0;
  RowRoot->w=0;
  rowResetEmptyRects(RowRoot);
  rowCalculateMinSize(RowRoot);
  rowEqualize(RowRoot);
  if (RowRoot) return RowRoot->r.bottom;
  return 0;
}
extern "C" void cppCalculateRowItemsPos(ROWCELL	*RowRoot, int width)
{
  rowSizeWithReposition(RowRoot, width); 
}

// rowAddCell
// �������� ����������� ���-�� ���. ������ ��� ��������� ROWCELL
// � ��������� �� � ������� �������� ��������
// link - ���� child ��� next, ������������ ��������� ROWCELL
// cont - ��� ����������: ������, ������� ��� �������� ����
//
//
extern "C" const ROWCELL * rowAddCell(ROWCELL* &link, int cont)
{
	link = (ROWCELL*)malloc(sizeof(ROWCELL));
	ZeroMemory(link, sizeof(ROWCELL));
	link->cont=cont;
	return link;
}

// rowDeleteTree
// ����������� ������ ������� ������� �������� ��������
// cell - ����� ��������� ���� ������ �������� ��������
//
//
extern "C" void rowDeleteTree(ROWCELL* cell)
{
	if (!cell) return;
	if (cell->child)
		rowDeleteTree((ROWCELL*)(cell->child));
	if (cell->next) 
		rowDeleteTree((ROWCELL*)(cell->next));
	free(cell);	
	cell = NULL;
	return;
}

// rowParserGetNextWord
// �������� �� ������ ������ (������ ���������) ��������� �����.
// ������ ��������� ������������������ ��������, ������������ �������: SP, <, >, ;, TAB, CR, LF
// ������� �� ; � �� ����� ������ ��������� ������������.
// NOTE: ������ ���������� �� ������ �������� ��� ��������� �� � ModernCL, 
// � �� ����, ��� ���� ����� ������������
// tbuf - ��������� �� ����� ���������� ����� �������
// hbuf - ��������� ������
//
//
char * rowParserGetNextWord(char *tbuf, int &hbuf)
{
	static char buf[256];
	char ch;

	int j=-1;

	ZeroMemory(buf, 256);

	while(tbuf[hbuf]!=0)
	{
		ch = tbuf[hbuf];

		// Remark found
		if (ch==';')					
		{
			if (j>=0) return buf;

			while (tbuf[hbuf]!=10 && tbuf[hbuf]!=13) hbuf++;
		}
	
		// Tag-bracers found
		if (!(ch == '>' && j<0)) //not single '>' found 
		{
			if ( (ch=='<' || ch=='>') && j>=0) 
			{
				if (ch == '>')
				{
					if (buf[0]=='/' || buf[0]=='<')  buf[++j] = ch;
					hbuf++;
				}
				return buf;
			}

			if (ch == ' ' || ch == 9 || ch == 10 || ch == 13 || ch==';' || ch == '>') 
			{
				if (ch == '>') 
				{
					buf[++j] = ch;
					hbuf++;
				}

				if (j>=0) return buf;	// Word is selected
			}
			else
				buf[++j] = ch;
		}
		hbuf++;
	}
	return NULL;
}

// rowParserGetParam
// ���� � �������������� ����� � �������, ����������� ����� ������
// cell - ��������� �� ������� ���������������� ��������� �������
// tbuf - ��������� �� ����� ���������� ����� �������
// hbuf - ��������� ������
//
//
void rowParserGetParam(ROWCELL* &cell, char *tbuf, int &hbuf)
{
  	char * word=rowParserGetNextWord(tbuf, hbuf);
  	int param=0;
  	
	     if (!_strnicmp(word, "avatar",     strlen(word))) param = TC_AVATAR; 
	else if (!_strnicmp(word, "text1",      strlen(word))) param = TC_TEXT1; 
	else if (!_strnicmp(word, "text2",      strlen(word))) param = TC_TEXT2; 
	else if (!_strnicmp(word, "text3",      strlen(word))) param = TC_TEXT3; 
	else if (!_strnicmp(word, "status",     strlen(word))) param = TC_STATUS; 
	else if (!_strnicmp(word, "extra",      strlen(word))) param = TC_EXTRA; 
	else if (!_strnicmp(word, "extra1",     strlen(word))) param = TC_EXTRA1;
	else if (!_strnicmp(word, "extra2",     strlen(word))) param = TC_EXTRA2;
	else if (!_strnicmp(word, "extra3",     strlen(word))) param = TC_EXTRA3;
	else if (!_strnicmp(word, "extra4",     strlen(word))) param = TC_EXTRA4;
	else if (!_strnicmp(word, "extra5",     strlen(word))) param = TC_EXTRA5;
	else if (!_strnicmp(word, "extra6",     strlen(word))) param = TC_EXTRA6;
	else if (!_strnicmp(word, "extra7",     strlen(word))) param = TC_EXTRA7;
	else if (!_strnicmp(word, "extra8",     strlen(word))) param = TC_EXTRA8;
	else if (!_strnicmp(word, "extra9",     strlen(word))) param = TC_EXTRA9;
	else if (!_strnicmp(word, "time",       strlen(word))) param = TC_TIME;
	else if (!_strnicmp(word, "space",      strlen(word))) param = TC_SPACE;
	else if (!_strnicmp(word, "fspace",      strlen(word))) param = TC_FIXED;
	
	else if (!_strnicmp(word, "left",       strlen(word))) param = TC_LEFT;
	else if (!_strnicmp(word, "top",        strlen(word))) param = TC_TOP;
	else if (!_strnicmp(word, "vcenter",    strlen(word))) param = TC_VCENTER;
	else if (!_strnicmp(word, "hcenter",    strlen(word))) param = TC_HCENTER;
	else if (!_strnicmp(word, "right",      strlen(word))) param = TC_RIGHT;
	else if (!_strnicmp(word, "bottom",     strlen(word))) param = TC_BOTTOM;

	else if (!_strnicmp(word, "layer",      strlen(word))) cell->layer = TRUE;

	else if (!_strnicmp(word, "width",      strlen(word))) param = TC_WIDTH;
	else if (!_strnicmp(word, "height",     strlen(word))) param = TC_HEIGHT;
	
	else 
	{
		hbuf-=(int)strlen(word);
		return;
	}
	
	if (param>TC_TEXT3 && param != TC_SPACE) cell->hasfixed = 1;

	switch (param)
	{
	case TC_TEXT1:
	case TC_TEXT2:
	case TC_TEXT3:
  case TC_SPACE:
		cell->sizing = 1;
	case TC_STATUS:
	case TC_AVATAR:
	case TC_EXTRA:
	case TC_EXTRA1:
	case TC_EXTRA2:
	case TC_EXTRA3:
	case TC_EXTRA4:
	case TC_EXTRA5:
	case TC_EXTRA6:
	case TC_EXTRA7:
	case TC_EXTRA8:
	case TC_EXTRA9:
	case TC_TIME:
	case TC_FIXED:
	
		cell->type = param;
		break;

	case TC_HCENTER:
	case TC_RIGHT:
		cell->halign = param;
		break;

	case TC_VCENTER:
	case TC_BOTTOM:
		cell->valign = param;
		break;

	case TC_WIDTH:
		word = rowParserGetNextWord(tbuf, hbuf);
		param = atoi(word);
		cell->w = param;
		break;
	
	case TC_HEIGHT:
		word = rowParserGetNextWord(tbuf, hbuf);
		param = atoi(word);
		cell->h = param;
	}

	rowParserGetParam(cell, tbuf, hbuf);
	return;
}
// rowParse
// ���� � ������� ���� <contact>, <tr> � <tc>, � ��������� ��������������� ����
// � ������ �������� ��������
// cell - ���� child ��� next ������������� ����������
// parent - ��������� �� ������������ ���������
// tbuf - ��������� �� ����� ���������� ����� �������
// hbuf - ��������� ������
// sequence - ����� �������� 0, ��� ����������� ����������
//
extern "C" BOOL rowParse(ROWCELL* &cell, ROWCELL* parent, char *tbuf, int &hbuf, int &sequence, ROWCELL* *RowTabAccess )
{
	char * word;
	word = rowParserGetNextWord(tbuf, hbuf);
	int cont;
		
	if      (!_strnicmp(word, "<tr",   strlen(word)) ||!_strnicmp(word, "<tr>",   strlen(word))) cont = TC_ROW; 
	else if (!_strnicmp(word, "<tc",   strlen(word)) ||!_strnicmp(word, "<tc>",   strlen(word))) cont = TC_COL; 
	else if (!_strnicmp(word, "/>",     strlen(word))||
			 !_strnicmp(word, "</tr>",  strlen(word))||
			 !_strnicmp(word, "</tc>",  strlen(word))) return TRUE;
	else return FALSE;
	
	rowAddCell(cell, cont);
	rowParserGetParam(cell, tbuf, hbuf);
	if (cell->type != 0 && cell->type !=TC_SPACE && cell->type !=TC_FIXED) 
		RowTabAccess[sequence++] = cell;
	
	if (!rowParse(cell->child, cell, tbuf, hbuf, sequence,RowTabAccess)) 
		return FALSE;

	if (!parent)
	{
		RowTabAccess[sequence] = NULL;
		return TRUE;
	}
	
	if (!rowParse(cell->next, parent, tbuf, hbuf, sequence,RowTabAccess))
			return FALSE;
		
	parent->sizing |= cell->sizing;
  parent->hasfixed|=cell->hasfixed;
	return TRUE;
}

void rowResetEmptyRects(ROWCELL* cell)
{
  if (!cell) return;
  if (cell->type==0) 
  {
    SetRect(&(cell->r),0,0,0,0);
    cell->full_width=0;
    cell->fixed_width=0;    
  }
  rowResetEmptyRects(cell->child);
  rowResetEmptyRects(cell->next);
}

// rowCalculateMinSize
// ���������� ����������� �������� ������� ���������� ������ �������� ��������
// ��� ��-� ������! ������ ���������� ��������������� ����� rowPositioning
// cell - ��������� �� �������� ���� ������ �������� ��������
// NOTE: ����� ������� rowCalculateMinSize ���������� ��������� ���� w � h ��������� RowTA, ��� ������� ��������
//
void rowCalculateMinSize(ROWCELL* cell)
{
	ROWCELL* curchild=NULL;
	int w=0,h=0;
	int wl=0, hl=0;
  int fullWidth=0;
	if (!cell) return;

	cell->r.left	= 0;
	cell->r.top		= 0;

	if (cell->type < TC_TEXT1 || cell->type > TC_TEXT3 && cell->type!=TC_SPACE)
		cell->r.right	= cell->w;
	else 
		cell->r.right	= 0;

	cell->r.bottom	= cell->h;

	rowCalculateMinSize(cell->child);
	rowCalculateMinSize(cell->next);

	if (!(curchild = cell->child)) return;

	if (cell->cont == TC_ROW) 
	{
		do 
		{
			h = max(h, curchild->r.bottom);
		
			if (curchild->layer)
      {
				//w = max(w, curchild->r.right);
				wl += curchild->r.right;
				fullWidth=max(fullWidth,max(curchild->full_width,curchild->w));
      }
      else 
      {
				w += curchild->r.right;
				fullWidth+=max(curchild->full_width,curchild->w);
      }
		}
		while (curchild = curchild->next);
	}

	if (cell->cont == TC_COL) 
	{
		while (curchild)
		{
			w = max(w, curchild->r.right);
      fullWidth=max(fullWidth,max(curchild->full_width,curchild->w));

			if (curchild->layer)
			{
				hl = curchild->r.bottom;
//				h = max(h, curchild->r.bottom);
			}
			else
				h += curchild->r.bottom;

			curchild = curchild->next;
		}
	}
	
	cell->r.right  = max(max(w, cell->r.right),wl);
    cell->r.bottom = max(max(h, cell->r.bottom),hl);
    cell->full_width = max(fullWidth,cell->full_width);
	cell->fixed_width = max(cell->fixed_width,cell->r.right); 
	return;
}

// void rowEqualise(ROWCELL* cell)
// 
// ���������� ������ ����� ������ �����, � ������ ����� ������ �������
// cell - ��������� �� �������� ���� ������ �������� ��������
//
void rowEqualize(ROWCELL* cell)
{
	ROWCELL* curchild=NULL;
	if (!cell) return;
	rowEqualize(cell->child);
	rowEqualize(cell->next);

	if (!(curchild = cell->child)) return;

	if (cell->cont == TC_ROW)
	{
		do 
		{
			if (curchild->layer) continue;
			curchild->r.bottom = cell->r.bottom;
		}
		while (curchild = curchild->next);
	}

	if (cell->cont == TC_COL)
	{
		do 
		{
			if (curchild->layer) continue;
			curchild->r.right = cell->r.right;
		}
		while (curchild = curchild->next);
	}
	

	//rowEqualize(cell->child);
	//rowEqualize(cell->next);
}

// void rowPlacing(pttCell cell, pttCell parent)
//
// ������������� ������� ������ �������� � ��� ����������
// cell - ��������� �� ��������� ���������
//
void rowPlacing(pROWCELL cell)
{
	if (cell->type == 0) return;

	switch(cell->type)
	{
		case TC_TEXT1:
		case TC_TEXT2:
		case TC_TEXT3:
		case TC_SPACE:
			cell->r.right += cell->r.left;
			break;
		default:
		{
			switch(cell->halign)
			{	
				case TC_LEFT:
					break;
				case TC_HCENTER:
					cell->r.left += (cell->r.right - cell->w)/2;
					break;
				case TC_RIGHT:
					cell->r.left += cell->r.right - cell->w;
			}
			cell->r.right = cell->r.left + cell->w;
		}
	}
			
	switch(cell->valign)
	{
		case TC_TOP:
			break;
		case TC_VCENTER:
			cell->r.top += (cell->r.bottom - cell->h)/2;
			break;
		case TC_BOTTOM:
			cell->r.top += cell->r.bottom - cell->h;
	}	
	cell->r.bottom = cell->r.top + cell->h;
}

// void ttTLProc(pROWCELL cell, pROWCELL parent)
//
// ������������� ��������� ���������, ������ �������������
// cell - ��������� �� ��������� ���������
// parent - ��������� �� ������������ ���������
//
void rowLayerProc(pROWCELL cell, pROWCELL parent)
{
	if (cell->sizing)
	{
		cell->r.left = parent->r.left;
		//cell->r.right += cell->r.left;
	}
	else
	{
		switch(cell->halign)
		{
			case TC_LEFT:
				cell->r.left = parent->r.left;
				break;
			case TC_HCENTER:
				cell->r.left = parent->r.left + (parent->r.right - cell->r.right)/2;
				break;
			case TC_RIGHT:
				cell->r.left = parent->r.left + parent->r.right - cell->r.right;
		}
	}

	switch(cell->valign)
	{
		case TC_TOP:
			cell->r.top = parent->r.top;
			break;
		case TC_VCENTER:
			cell->r.top = parent->r.top + (parent->r.bottom - cell->r.bottom)/2;
			break;
		case TC_BOTTOM:
			cell->r.top = parent->r.top + parent->r.bottom - cell->r.bottom;
			break;
	}
}

// void rowPositioning(pROWCELL cell, int &dist)
//
// ��������� �������������� ��������� ��������, �������� ������������ � ����������
// cell - ��������� �� �������� ���� ������ �������� ��������
// dist - ����� ������ ��������
//
void rowPositioning(pROWCELL cell, int &dist)
{
	ROWCELL* curchild = NULL;

	int x = cell->r.left;
	int y = cell->r.top;

	int h = cell->r.bottom;
	int w = dist;

	int valign = 0;
	int halign = 0;

	int r = 0;
	int size = 0;
	int cw = 0;
  int fixedsized=0;
  int autosized=0;
	int dummy = 0;
	
	// �������������� ����������� ������ dist
	if (w < cell->r.right && (cell->type < TC_TEXT1 || cell->type > TC_TEXT3 && cell->type!=TC_SPACE) || !cell->sizing) 
		dist = w = cell->r.right;

	cell->r.right= dist;
    dummy=dist;
	if (!(curchild = cell->child)) 
	{
		rowPlacing(cell);
		return;
	}

	// ���������������� ����������� � ������
	if (cell->cont == TC_ROW)
	{
    fixedsized=cell->fixed_width;
		while (curchild)
		{
			// ���������� layer �� ������ ������ �� ���������������� ����������� tc
			if (curchild->layer)
			{
				curchild = curchild->next;
				continue;
			}

			cw += curchild->r.right;
			
			if (curchild->sizing) 
			{
				autosized+=max(curchild->w,curchild->full_width);
				r++;
			}
			else 
				size += curchild->r.right;

			curchild = curchild->next;
		}
			
		w -= size;
    fixedsized-=size;

		if (r == 0)
		{
			switch(cell->halign)
			{
				case TC_HCENTER:
					x += (dist - cw)/2;// - 1;
					break;
				case TC_RIGHT:
					x += dist - cw; 
					break;
			}
		}
		

		curchild = cell->child;
			
		size = 0;
		while(curchild)
		{		
			if (curchild->layer)
			{
				//int dummy = 0;
				rowLayerProc(curchild, cell);
				rowPositioning(curchild, dummy);
			}
			else
			{
				curchild->r.top = cell->r.top;
				curchild->r.left = x;		
				

				w -= size;
				if (curchild->sizing) 
				{
          if ((0&!curchild->fitwidth) || r>1)  //���� ��������� -�������� � �������������
          {
            if (curchild->hasfixed)
                fixedsized-=curchild->fixed_width;
            switch (cell->halign)
						{
						case TC_RIGHT:
               size=(w-fixedsized)-(autosized-max(curchild->full_width,curchild->w));
               break;
            case TC_LEFT:
               size=min(w-fixedsized,max(curchild->full_width,curchild->w));         
               break;
            case TC_HCENTER:
              if (autosized) {size=max(curchild->full_width,curchild->w)*w/autosized; break;}
						default:
							size = w / r;
            }
            autosized-=(max(curchild->full_width,curchild->w));
            if (autosized<0) autosized=0;
            if (size<0) size=0;
          }
					else size = w;
          /*  ���� ��������� ��� �������� ������ �� ������������ ���������� �� ������ ����
           if (0 &&!curchild->fitwidth)
            if(size>max(curchild->full_width,curchild->w))
              size=max(curchild->full_width,curchild->w);
           */
          r--;
				}
				else 
					size = curchild->r.right;

				rowPositioning(curchild, size);
				x += size;
			
				if (!curchild->sizing) 
					size=0;
			}

			curchild = curchild->next;
		}
	}

	// ���������������� ����������� � �������
	if (cell->cont == TC_COL)
	{
		while (curchild)
		{
			// ���������� layer �� ������ ������ �� ���������������� ����������� tr
			if (curchild->layer)
			{
				curchild = curchild->next;
				continue;
			}

			size += curchild->r.bottom;
			curchild = curchild->next;
		}

		if (h > size)
		{
			switch(cell->valign)
			{
				case TC_VCENTER:
					y += (h - size) / 2; 
					break;
				case TC_BOTTOM:
					y += (h - size);
					break;
			}
		}
			
		curchild = cell->child;
		while(curchild)
		{
			if (curchild->layer)
			{
				rowLayerProc(curchild, cell);
				rowPositioning(curchild, dummy);
			}
			else
			{
				curchild->r.top = y;
				y += curchild->r.bottom;

				curchild->r.left  = cell->r.left;
				curchild->r.right = dist;

				rowPositioning(curchild, size);

			}
			
			curchild = curchild->next;
		}
	}

	rowPlacing(cell);

}

// void rowSizeWithReposition(ROWCELL* &root, int width)
//
// ���������� ������� � ���������������� ��������� �������
// ����� ������� ���������� ��������� ��������� RowTA
//
extern "C" void rowSizeWithReposition(ROWCELL* &root, int width)
{
	root->h = 0;
	root->w = 0;
	rowCalculateMinSize(root);
	rowEqualize(root);
	rowPositioning(root, width);
	root->h = root->r.bottom;
	root->w = root->r.right;
}

#undef _CPPCODE

/*

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PARSER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_PARSER);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_PARSER);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= (LPCTSTR)IDC_PARSER;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, 
					   szTitle, 
					   WS_OVERLAPPEDWINDOW,
					   CW_USEDEFAULT, 
					   0, 
					   310, 
					   250, 
					   NULL, 
					   NULL, 
					   hInstance, 
					   NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


void DrawRectangles(HDC hdc, ROWCELL* cell)
{	
	char buf[10];

	if (cell->type)
	{
		Rectangle(hdc, cell->r.left, cell->r.top, cell->r.right, cell->r.bottom);	
		_snprintf(buf, 10, "%d", cell->type);
		DrawText(hdc, (LPCSTR)&buf, 1, &(cell->r), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	if (cell->child) DrawRectangles(hdc, cell->child);
	if (cell->next)  DrawRectangles(hdc, cell->next);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int fsize;
	int seq=0;

	FILE * hFile;

	switch (message) 
	{
	case WM_CREATE:
		hFile = fopen("template.txt", "rb");
		fsize = _filelength(_fileno(hFile));
		tmplbuf = (char*)malloc(fsize);
		ZeroMemory(tmplbuf, fsize);
	
		for (int i=0; i<fsize; i++) 
			tmplbuf[i] = getc(hFile);

		tmplbuf[i] = 0;

		//MessageBox(0, tmplbuf, "", MB_OK);

		i = 0;
		rowParse(RowRoot, RowRoot, tmplbuf, i, seq);
		fclose(hFile);
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	
	case WM_SIZE:
		RECT r;
		GetClientRect(hWnd, &r);
		
		{
		DrawTime = GeRowickCount();
		for (int i=0; i<1000; i++)
			rowSizeWithReposition(RowRoot, r.right);

		DrawTime = GeRowickCount() - DrawTime;
		
		
			char buf[20];
			_snprintf(buf, 10, "%d", DrawTime);
			SendMessage(hWnd, WM_SERowEXT, (WPARAM)0, (LPARAM)&buf);
		}

		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		DrawRectangles(hdc, RowRoot);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
*/