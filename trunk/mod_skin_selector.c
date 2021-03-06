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
*/

///// structures and services to manage modern skin objects (mask mechanism)

//#include "windows.h"
#include "commonheaders.h"
#include "mod_skin_selector.h"
#include "SkinEngine.h"  
#include "m_skin_eng.h"
extern LPSKINOBJECTDESCRIPTOR skin_FindObjectByName(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin);
extern int AddButton(HWND parent,char * ID,char * CommandService,char * StateDefService,char * HandeService,             int Left, int Top, int Right, int Bottom, DWORD AlignedTo,TCHAR * Hint,char * DBkey,char * TypeDef,int MinWidth, int MinHeight);
struct TList_ModernMask * MainModernMaskList=NULL;

/// IMPLEMENTATIONS
char * ModernMaskToString(TLO_MMask * mm, char * buf, UINT bufsize)
{
    int i=0;
    for (i=0; i<(int)mm->dwParamCnt;i++)
    {
        if (mm->pl_Params[i].bFlag)
        {
            if (i>0) _snprintf(buf,bufsize,"%s%%",buf);
            if (mm->pl_Params[i].bFlag &2)
                _snprintf(buf,bufsize,"%s=%s",mm->pl_Params[i].szName,mm->pl_Params[i].szValue);
            else
                _snprintf(buf,bufsize,"%s^%s",mm->pl_Params[i].szName,mm->pl_Params[i].szValue);
        }
        else break;
    }
    return buf;
}
int DeleteMask(TLO_MMask * mm)
{
  int i;
  if (!mm->pl_Params) return 0;
  for (i=0;i<(int)mm->dwParamCnt;i++)
  {
    if (mm->pl_Params[i].szName) mir_free(mm->pl_Params[i].szName);
    if (mm->pl_Params[i].szValue) mir_free(mm->pl_Params[i].szValue);
  }
  mir_free(mm->pl_Params);
  return 1;
}

BOOL WildComparei(char * name, char * mask)
{
  char * last='\0';
  for(;; mask++, name++)
  {
    if(*mask != '?' && (*mask&223) != (*name&223)) break;
    if(*name == '\0') return ((BOOL)!*mask);
  }
  if(*mask != '*') return FALSE;
  for(;; mask++, name++)
  {      
    while(*mask == '*')
    {    
      last = mask++;
      if(*mask == '\0') return ((BOOL)!*mask);   /* true */
    }
    if(*name == '\0') return ((BOOL)!*mask);      /* *mask == EOS */
    if(*mask != '?' && (*mask&223)  != (*name&223) ) name -= (size_t)(mask - last) - 1, mask = last;
  }
}

BOOL _inline WildCompare(char * name, char * mask, BYTE option)
    {
         char * last='\0';
         for(;; mask++, name++)
         {
                 if(*mask != '?' && *mask != *name) break;
                 if(*name == '\0') return ((BOOL)!*mask);
         }
         if(*mask != '*') return FALSE;
         for(;; mask++, name++)
         {      
                 while(*mask == '*')
                 {    
                         last = mask++;
                         if(*mask == '\0') return ((BOOL)!*mask);   /* true */
                 }
                 if(*name == '\0') return ((BOOL)!*mask);      /* *mask == EOS */
                 if(*mask != '?' && *mask != *name) name -= (size_t)(mask - last) - 1, mask = last;
         }
    }



DWORD mod_CalcHash(char * a)
{
    DWORD Val=0;
    BYTE N;
    DWORD k=mir_strlen(a);
    if (k<23) N=(BYTE)k; else N=23;
    while (N>0)
    {
        Val=Val<<1;
        Val^=((DWORD)*a++)-31;
        N--;
    }
    return Val;
}
int AddModernMaskToList(TLO_MMask * mm,  TList_ModernMask * mmTemplateList)
{
    if (!mmTemplateList || !mm) return -1;
	mmTemplateList->pl_Masks=mir_realloc(mmTemplateList->pl_Masks,sizeof(TLO_MMask)*(mmTemplateList->dwMaskCnt+1));
    memmove(&(mmTemplateList->pl_Masks[mmTemplateList->dwMaskCnt]),mm,sizeof(TLO_MMask));
    mmTemplateList->dwMaskCnt++;
    return mmTemplateList->dwMaskCnt-1;
}

int ClearMaskList(TList_ModernMask * mmTemplateList)
{
  	int i;
    if (!mmTemplateList) return -1;
    if (!mmTemplateList->pl_Masks) return -1;
    for(i=0; i<(int)mmTemplateList->dwMaskCnt; i++)
        DeleteMask(&(mmTemplateList->pl_Masks[i]));
	mir_free(mmTemplateList->pl_Masks);
    mmTemplateList->pl_Masks=NULL;
    mmTemplateList->dwMaskCnt=0;
    return 0;
}
int DeleteMaskByItID(DWORD mID,TList_ModernMask * mmTemplateList)
{
    if (!mmTemplateList) return -1;
    if (mID<0|| mID>=mmTemplateList->dwMaskCnt) return -1;
    if (mmTemplateList->dwMaskCnt==1)
    {
       DeleteMask(&(mmTemplateList->pl_Masks[0]));
       mir_free(mmTemplateList->pl_Masks);
       mmTemplateList->pl_Masks=NULL;
       mmTemplateList->dwMaskCnt;
    }
    else
    {
      TLO_MMask * newAlocation;
      DWORD i;
      DeleteMask(&(mmTemplateList->pl_Masks[mID]));
	  newAlocation=mir_alloc(sizeof(TLO_MMask)*mmTemplateList->dwMaskCnt-1);
      memmove(newAlocation,mmTemplateList->pl_Masks,sizeof(TLO_MMask)*(mID+1));
      for (i=mID; i<mmTemplateList->dwMaskCnt-1; i++)
      {
          newAlocation[i]=mmTemplateList->pl_Masks[i+1];
          newAlocation[i].dwMaskId=i;
      }
	  mir_free(mmTemplateList->pl_Masks);
      mmTemplateList->pl_Masks=newAlocation;
      mmTemplateList->dwMaskCnt--;
    }
    return mmTemplateList->dwMaskCnt;
}


int ExchangeMasksByID(DWORD mID1, DWORD mID2, TList_ModernMask * mmTemplateList)
{
    if (!mmTemplateList) return 0;
    if (mID1<0|| mID1>=mmTemplateList->dwMaskCnt) return 0;
    if (mID2<0|| mID2>=mmTemplateList->dwMaskCnt) return 0;
    if (mID1==mID2) return 0;
    {
        TLO_MMask mm;
        mm=mmTemplateList->pl_Masks[mID1];
        mmTemplateList->pl_Masks[mID1]=mmTemplateList->pl_Masks[mID2];
        mmTemplateList->pl_Masks[mID2]=mm;
    }
    return 1;
}
int SortMaskList(TList_ModernMask * mmList)
{
        DWORD pos=1;
        if (mmList->dwMaskCnt<2) return 0;
        do {
                if(mmList->pl_Masks[pos].dwMaskId<mmList->pl_Masks[pos-1].dwMaskId)
                {
                        ExchangeMasksByID(pos, pos-1, mmList);
                        pos--;
                        if (pos<1)
                                pos=1;
                }
                else
                        pos++;
        } while(pos<mmList->dwMaskCnt);

        return 1;
}
int ParseToModernMask(TLO_MMask * mm, char * szText)
{
    //TODO
    if (!mm || !szText) return -1;
    else
    {
        UINT textLen=mir_strlen(szText);
        BYTE curParam=0;
        TLO_MaskParam param={0};
        UINT currentPos=0;
        UINT startPos=0;
        while (currentPos<textLen)
        {
            //find next single ','
            while (currentPos<textLen) 
            {
                if (szText[currentPos]==',')
                    if  (currentPos<textLen-1)
                        if (szText[currentPos+1]==',') currentPos++;
                        else break;
                currentPos++;
            }
            //parse chars between startPos and currentPos
            {   
                //Get param name
                if (startPos!=0) 
                {
                    //search '=' sign or '^'
                    UINT keyPos=startPos;
                    while (keyPos<currentPos-1 && !(szText[keyPos]=='=' ||szText[keyPos]=='^')) keyPos++;
                    if (szText[keyPos]=='=') param.bFlag=1;
                    else if (szText[keyPos]=='^') param.bFlag=3;
                    //szText[keyPos]='/0';
                    {
                        DWORD k;
                        char v[MAXVALUE];
                        k=keyPos-startPos;
                        if (k>MAXVALUE-1) k=MAXVALUE-1;
                        strncpy(v,szText+startPos,k);
                        v[k]='\0';
                        param.dwId=mod_CalcHash(v);
                        param.szName=mir_strdup(v);
                        startPos=keyPos+1;
                    }
                }
                else //ParamName='Module'
                {
                    param.bFlag=1;
                    param.dwId=mod_CalcHash("Module");
                    param.szName=mir_strdup("Module");
                }
                //szText[currentPos]='/0';
                {
                  int k;
				  int m;
                  char v[MAXVALUE]={0};
                  k=currentPos-startPos;
                  if (k>MAXVALUE-1) k=MAXVALUE-1;
				          m=min((UINT)k,mir_strlen(szText)-startPos+1);
                  strncpy(v,&(szText[startPos]),k);
                  param.szValue=mir_strdup(v);
                }
                param.dwValueHash=mod_CalcHash(param.szValue);
                {   // if Value don't contain '*' or '?' count add flag
                    UINT i=0;
                    BOOL f=4;
                    while (param.szValue[i]!='\0' && i<(UINT)mir_strlen(param.szValue)+1)
                        if (param.szValue[i]=='*' || param.szValue[i]=='?') {f=0; break;} else i++;
                    param.bFlag|=f;
                }
                startPos=currentPos+1;
                currentPos++;
                {//Adding New Parameter;
                  if (curParam>=mm->dwParamCnt)
                  {
					mm->pl_Params=mir_realloc(mm->pl_Params,(mm->dwParamCnt+1)*sizeof(TLO_MaskParam));
					mm->dwParamCnt++;                    
                  }
                  memmove(&(mm->pl_Params[curParam]),&param,sizeof(TLO_MaskParam));
                  curParam++;
                  memset(&param,0,sizeof(TLO_MaskParam));
            }
        }
    }
    }
    return 0;
};

BOOL CompareModernMask(TLO_MMask * mmValue,TLO_MMask * mmTemplate)
{
    //TODO
    BOOL res=TRUE;
    BOOL exit=FALSE;
    BYTE pVal=0, pTemp=0;
    while (pTemp<mmTemplate->dwParamCnt && pVal<mmValue->dwParamCnt && !exit) 
    {
      // find pTemp parameter in mValue
      DWORD vh, ph;
      BOOL finded=0;
      TLO_MaskParam p=mmTemplate->pl_Params[pTemp];
      ph=p.dwId;      
      vh=p.dwValueHash;
      pVal=0;
      if (p.bFlag&4)  //compare by hash     
          while (pVal<mmValue->dwParamCnt && mmValue->pl_Params[pVal].bFlag !=0)
          {
             if (mmValue->pl_Params[pVal].dwId==ph)
             {
                 if (mmValue->pl_Params[pVal].dwValueHash==vh){finded=1; break;}
                 else {finded=0; break;}
             }   
            pVal++;
          }
      else
          while (mmValue->pl_Params[pVal].bFlag!=0)
          {
             if (mmValue->pl_Params[pVal].dwId==ph)
             {
                 if (WildCompare(mmValue->pl_Params[pVal].szValue,p.szValue,0)){finded=1; break;}
                 else {finded=0; break;}
             }
             pVal++;
          }
       if (!((finded && !(p.bFlag&2)) || (!finded && (p.bFlag&2))))
           {res=FALSE; break;}
      pTemp++;
    }
    return res;
};

BOOL CompareStrWithModernMask(char * szValue,TLO_MMask * mmTemplate)
{
    TLO_MMask mmValue={0};
  int res;
  if (!ParseToModernMask(&mmValue, szValue))
  {
       res=CompareModernMask(&mmValue,mmTemplate);
       DeleteMask(&mmValue);
       return res;
  }
  else return 0;
};
//AddingMask
int AddStrModernMaskToList(DWORD maskID, char * szStr, char * objectName,  TList_ModernMask * mmTemplateList, void * pObjectList)
{
    TLO_MMask mm={0};
    if (!szStr || !mmTemplateList) return -1;
    if (ParseToModernMask(&mm,szStr)) return -1;
    mm.pObject=(void*) skin_FindObjectByName(objectName, OT_ANY, (SKINOBJECTSLIST*) pObjectList);
        mm.dwMaskId=maskID;
    return AddModernMaskToList(&mm,mmTemplateList);    
}



//Searching
TLO_MMask *  FindMaskByStr(char * szValue,TList_ModernMask * mmTemplateList)
{
    //TODO
    return NULL;
}
SKINOBJECTDESCRIPTOR *  skin_FindObjectByRequest(char * szValue,TList_ModernMask * mmTemplateList)
{
    //TODO
    TLO_MMask mm={0};
    SKINOBJECTDESCRIPTOR * res=NULL;
    DWORD i=0;
    if (!mmTemplateList)    
        if (glObjectList.MaskList)
            mmTemplateList=glObjectList.MaskList;
		else return NULL;
    if (!mmTemplateList) return NULL;
	if (IsBadReadPtr(mmTemplateList,sizeof(TList_ModernMask)))return NULL;
    ParseToModernMask(&mm,szValue);
    while (i<mmTemplateList->dwMaskCnt)
    {
        if (CompareModernMask(&mm,&(mmTemplateList->pl_Masks[i])))
        {

            res=(SKINOBJECTDESCRIPTOR*) mmTemplateList->pl_Masks[i].pObject; 
            DeleteMask(&mm);
            return res;
        }
        i++;
    }
    DeleteMask(&mm);
    return NULL;
}

TCHAR * GetParamNT(char * string, TCHAR * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces)
{
#ifdef UNICODE
	char *ansibuf=mir_alloc(buflen/sizeof(TCHAR));
	GetParamN(string, ansibuf, buflen/sizeof(TCHAR), paramN, Delim, SkipSpaces);
	MultiByteToWideChar(CP_UTF8,0,ansibuf,-1,buf,buflen);
	mir_free(ansibuf);
	return buf;
#else
	return GetParamN(string, buf, buflen, paramN, Delim, SkipSpaces);
#endif
}

char * GetParamN(char * string, char * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces)
{
    int i=0;
    DWORD start=0;
    DWORD end=0;
    DWORD CurentCount=0;
    DWORD len;
    while (i<mir_strlen(string))
    {
        if (string[i]==Delim)
        {
            if (CurentCount==paramN) break;
            start=i+1;
            CurentCount++;     
        }
        i++;
    }
    if (CurentCount==paramN)
    {
        if (SkipSpaces)
        { //remove spaces
          while (string[start]==' ' && (int)start<mir_strlen(string))
            start++;
          while (i>1 && string[i-1]==' ' && i>(int)start)
            i--;
        }
        len=((int)(i-start)<buflen)?i-start:buflen;
        strncpy(buf,string+start,len);
        buf[len]='\0';
    }
    else buf[0]='\0';
    return buf;
}

//Parse DB string and add buttons
int RegisterButtonByParce(char * ObjectName, char * Params)
{
    char buf [255];
    int res;
    GetParamN(Params,buf, sizeof(buf),0,',',0);
   // if (boolstrcmpi("Push",buf)
    {   //Push type
        char buf2[20]={0};
        char pServiceName[255]={0};
        char pStatusServiceName[255]={0};
        int Left, Top,Right,Bottom;
        int MinWidth, MinHeight;
        char TL[9]={0};         
        TCHAR Hint[250]={0};
        char Section[250]={0};
        char Type[250]={0};

		DWORD alingnto;
		int a=((int)mir_bool_strcmpi(buf,"Switch"))*2;

        GetParamN(Params,pServiceName, sizeof(pServiceName),1,',',0);
       // if (a) GetParamN(Params,pStatusServiceName, sizeof(pStatusServiceName),a+1,',',0);
        Left=atoi(GetParamN(Params,buf2, sizeof(buf2),a+2,',',0));
        Top=atoi(GetParamN(Params,buf2, sizeof(buf2),a+3,',',0));
        Right=atoi(GetParamN(Params,buf2, sizeof(buf2),a+4,',',0));
        Bottom=atoi(GetParamN(Params,buf2, sizeof(buf2),a+5,',',0));
        GetParamN(Params,TL, sizeof(TL),a+6,',',0);  

        MinWidth=atoi(GetParamN(Params,buf2, sizeof(buf2),a+7,',',0));
        MinHeight=atoi(GetParamN(Params,buf2, sizeof(buf2),a+8,',',0));
        GetParamNT(Params,Hint, sizeof(Hint),a+9,',',0);
        if (a)
        {
          GetParamN(Params,Section, sizeof(Section),2,',',0);
          GetParamN(Params,Type, sizeof(Type),3,',',0);
        }
		alingnto=   (TL[0]=='R')
             +2*(TL[0]=='C')
             +4*(TL[1]=='B')
             +8*(TL[1]=='C')
             +16*(TL[2]=='R')
             +32*(TL[2]=='C')
             +64*(TL[3]=='B')
             +128*(TL[3]=='C')
             +256*(TL[4]=='I');
        if (a) res=AddButton(pcli->hwndContactList,ObjectName+1,pServiceName,pStatusServiceName,"\0",Left,Top,Right,Bottom,alingnto,Hint,Section,Type,MinWidth,MinHeight);
        else res=AddButton(pcli->hwndContactList,ObjectName+1,pServiceName,pStatusServiceName,"\0",Left,Top,Right,Bottom,alingnto,Hint,NULL,NULL,MinWidth,MinHeight);
    }
return res;
}

//Parse DB string and add object
// Params is:
// Glyph,None
// Glyph,Solid,<ColorR>,<ColorG>,<ColorB>,<Alpha>
// Glyph,Image,Filename,(TileBoth|TileVert|TileHor|StretchBoth),<MarginLeft>,<MarginTop>,<MarginRight>,<MarginBottom>,<Alpha>
int RegisterObjectByParce(char * ObjectName, char * Params)
{
 if (!ObjectName || !Params) return 0;
 {
     int res=0;
     SKINOBJECTDESCRIPTOR obj={0};
     char buf[250];
     obj.szObjectID=mir_strdup(ObjectName);
     GetParamN(Params,buf, sizeof(buf),0,',',0);
     if (mir_bool_strcmpi(buf,"Glyph"))
         obj.bType=OT_GLYPHOBJECT;
     else if (mir_bool_strcmpi(buf,"Font"))
         obj.bType=OT_FONTOBJECT;

     switch (obj.bType)
     {
     case OT_GLYPHOBJECT:
         {
             GLYPHOBJECT gl={0};
             GetParamN(Params,buf, sizeof(buf),1,',',0);
             if (mir_bool_strcmpi(buf,"Solid"))
             {
                 //Solid
                 int r,g,b;
                 gl.Style=ST_BRUSH;
                 r=atoi(GetParamN(Params,buf, sizeof(buf),2,',',0));
                 g=atoi(GetParamN(Params,buf, sizeof(buf),3,',',0));
                 b=atoi(GetParamN(Params,buf, sizeof(buf),4,',',0));
                 gl.dwAlpha=atoi(GetParamN(Params,buf, sizeof(buf),5,',',0));
                 gl.dwColor=RGB(r,g,b);
             }
             else if (mir_bool_strcmpi(buf,"Image"))
             {
                 //Image
				         gl.Style=ST_IMAGE;
                 gl.szFileName=mir_strdup(GetParamN(Params,buf, sizeof(buf),2,',',0));                
                 gl.dwLeft=atoi(GetParamN(Params,buf, sizeof(buf),4,',',0));
                 gl.dwTop=atoi(GetParamN(Params,buf, sizeof(buf),5,',',0));
                 gl.dwRight=atoi(GetParamN(Params,buf, sizeof(buf),6,',',0));
                 gl.dwBottom=atoi(GetParamN(Params,buf, sizeof(buf),7,',',0));
                 gl.dwAlpha =atoi(GetParamN(Params,buf, sizeof(buf),8,',',0));
                 GetParamN(Params,buf, sizeof(buf),3,',',0);
                 if (mir_bool_strcmpi(buf,"TileBoth")) gl.FitMode=FM_TILE_BOTH;
                 else if (mir_bool_strcmpi(buf,"TileVert")) gl.FitMode=FM_TILE_VERT;
                 else if (mir_bool_strcmpi(buf,"TileHorz")) gl.FitMode=FM_TILE_HORZ;
                 else gl.FitMode=0;                
             }
             else if (mir_bool_strcmpi(buf,"Fragment"))
             {
                 //Image
				         gl.Style=ST_FRAGMENT;
                 gl.szFileName=mir_strdup(GetParamN(Params,buf, sizeof(buf),2,',',0));
                 
                 gl.clipArea.x=atoi(GetParamN(Params,buf, sizeof(buf),3,',',0));
                 gl.clipArea.y=atoi(GetParamN(Params,buf, sizeof(buf),4,',',0));
                 gl.szclipArea.cx=atoi(GetParamN(Params,buf, sizeof(buf),5,',',0));
                 gl.szclipArea.cy=atoi(GetParamN(Params,buf, sizeof(buf),6,',',0));

                 gl.dwLeft=atoi(GetParamN(Params,buf, sizeof(buf),8,',',0));
                 gl.dwTop=atoi(GetParamN(Params,buf, sizeof(buf),9,',',0));
                 gl.dwRight=atoi(GetParamN(Params,buf, sizeof(buf),10,',',0));
                 gl.dwBottom=atoi(GetParamN(Params,buf, sizeof(buf),11,',',0));
                 gl.dwAlpha =atoi(GetParamN(Params,buf, sizeof(buf),12,',',0));
                 GetParamN(Params,buf, sizeof(buf),7,',',0);
                 if (mir_bool_strcmpi(buf,"TileBoth")) gl.FitMode=FM_TILE_BOTH;
                 else if (mir_bool_strcmpi(buf,"TileVert")) gl.FitMode=FM_TILE_VERT;
                 else if (mir_bool_strcmpi(buf,"TileHorz")) gl.FitMode=FM_TILE_HORZ;
                 else gl.FitMode=0;                
             }
             else 
             {
                 //None
                 gl.Style=ST_SKIP;
             }
             obj.Data=&gl;
             res=AddObjectDescriptorToSkinObjectList(&obj,NULL);
             mir_free(obj.szObjectID);
             if (gl.szFileName) mir_free(gl.szFileName);
             return res;
         }
         break;
     }
 }
 return 0;
}