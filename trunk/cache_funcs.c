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
#include "cache_funcs.h"

typedef struct _AskChain {
  HANDLE ContactRequest;
  struct AskChain *Next;
}AskChain;

extern int CopySkipUnPrintableChars(char *to, char * buf, DWORD size);

AskChain * FirstChain=NULL;
AskChain * LastChain=NULL;
BOOL LockChainAddition=0;
BOOL LockChainDeletion=0;
DWORD RequestTick=0;

int AddHandleToChain(HANDLE hContact)
{
  AskChain * workChain;
  while (LockChainAddition) 
  {
    SleepEx(0,TRUE);
    if (Miranda_Terminated()) return 0;
  }
  LockChainDeletion=1;
  {
    //check that handle is present
     AskChain * wChain;
     wChain=FirstChain;
     if (wChain)
     {
       do {
         if (wChain->ContactRequest==hContact)
         {
           LockChainDeletion=0;
           return 0;
         }
       } while(wChain=(AskChain *)wChain->Next);
     }
  }
  if (!FirstChain)  
  {
    FirstChain=mir_alloc(sizeof(AskChain));
    workChain=FirstChain;

  }
  else 
  {
    LastChain->Next=mir_alloc(sizeof(AskChain));
    workChain=(AskChain *)LastChain->Next;
  }
  LastChain=workChain;
  workChain->Next=NULL;
  workChain->ContactRequest=hContact;
  LockChainDeletion=0;
  return 1;
}

HANDLE GetCurrChain()
{
  struct AskChain * workChain;
  HANDLE res=NULL;
  while (LockChainDeletion)   
  {
    SleepEx(0,TRUE);
    if (Miranda_Terminated()) return 0;
  }
  LockChainAddition=1;
  if (FirstChain)
  {
    res=FirstChain->ContactRequest;
    workChain=FirstChain->Next;
    mir_free(FirstChain);
    FirstChain=(AskChain *)workChain;
  }
  LockChainAddition=0;
  return res;
}

#define ASKPERIOD 3000
BOOL ISTREADSTARTED=0;

int AskStatusMessageThread(HWND hwnd)
{
  DWORD time;
  HANDLE h;
  HANDLE ACK=0;
  
  h=GetCurrChain(); 
  if (!h) return 0;
  ISTREADSTARTED=1;
  while (h)
  { 
    time=GetTickCount();
    if ((time-RequestTick)<ASKPERIOD)
    {
            SleepEx(ASKPERIOD-(time-RequestTick)+10,TRUE);
            if (Miranda_Terminated()) 
            {
              ISTREADSTARTED=0;
              return 0; 
            }
    }
	{
		pdisplayNameCacheEntry pdnce = GetDisplayNameCacheEntry((HANDLE)h);
		if (pdnce->ApparentMode!=ID_STATUS_OFFLINE) //don't ask if contact is always invisible (should be done with protocol)
			ACK=(HANDLE)CallContactService(h,PSS_GETAWAYMSG,0,0);
	}   
    if (!ACK)
    {
      ACKDATA ack;
      ack.hContact=h;
      ack.type=ACKTYPE_AWAYMSG;
      ack.result=ACKRESULT_FAILED;
      ClcProtoAck((WPARAM)h,(LPARAM) &ack);
    }
    RequestTick=time;
    h=GetCurrChain();
    if (h) SleepEx(ASKPERIOD,TRUE); else break;
    if (Miranda_Terminated()) 
    {
      ISTREADSTARTED=0;
      return 0; 
    }

  }
  ISTREADSTARTED=0;
  return 1;
}


void ReAskStatusMessage(HANDLE wParam)
{
  int res;
  if (!DBGetContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",0)) return;
  res=AddHandleToChain(wParam); 
  //if (res) {
		//char buf[256];
		//pdisplayNameCacheEntry pdnce = GetDisplayNameCacheEntry((HANDLE)wParam);
		//_snprintf(buf,sizeof(buf),"XXXXX Asked PSS_GETAWAYMSG for %s(%x)\n",pdnce->name,wParam);
		//TRACE(buf);
  //}
  if (!ISTREADSTARTED && res) 
  {
    TRACE("....start ask thread\n");
    forkthread(AskStatusMessageThread,0,0);
  }
  return;
}



typedef BOOL (* ExecuteOnAllContactsFuncPtr) (struct ClcContact *contact, BOOL subcontact, void *param);
BOOL ExecuteOnAllContacts(struct ClcData *dat, ExecuteOnAllContactsFuncPtr func, void *param);
BOOL ExecuteOnAllContactsOfGroup(struct ClcGroup *group, ExecuteOnAllContactsFuncPtr func, void *param);



// Get all lines of text
void Cache_GetText(struct ClcData *dat, struct ClcContact *contact)
{
	Cache_GetFirstLineText(dat, contact);
	Cache_GetSecondLineText(dat, contact);
	Cache_GetThirdLineText(dat, contact);
}


void Cache_DestroySmileyList( SortedList* p_list )
{
	if ( p_list == NULL )
		return;

	if ( p_list->items != NULL )
	{
		int i;
		for ( i = 0 ; i < p_list->realCount ; i++ )
		{
			if ( p_list->items[i] != NULL )
			{
				ClcContactTextPiece *piece = (ClcContactTextPiece *) p_list->items[i];

				if (piece->smiley)
				{
					DestroyIcon(piece->smiley);
				}

				free(piece);
			}
		}
	}

	List_Destroy( p_list );
}

// Generete the list of smileys / text to be drawn
void Cache_ReplaceSmileys(struct ClcData *dat, struct ClcContact *contact, TCHAR *text, int text_size, SortedList **plText, 
						  BOOL replace_smileys)
{
	SMADD_PARSE sp;
	int last_pos=0;

	if (!dat->text_replace_smileys || !replace_smileys || text == NULL || !ServiceExists(MS_SMILEYADD_PARSE))
	{
		Cache_DestroySmileyList(*plText);
		*plText = NULL;
		return;
	}

	// Free old list
	if (*plText != NULL)
	{
		Cache_DestroySmileyList(*plText);
		*plText = NULL;
	}

	// Call service for te first time to see if needs to be used...
	sp.cbSize = sizeof(sp);

	if (dat->text_use_protocol_smileys)
	{
		sp.Protocolname = contact->proto;
	}
	else
	{
		sp.Protocolname = "clist";
	}

	sp.str = text;
	sp.startChar = 0;
	sp.size = 0;
	CallService(MS_SMILEYADD_PARSE, 0, (LPARAM)&sp);

	if (sp.SmileyIcon == NULL || sp.size == 0)
	{
		// Did not find a simley
		return;
	}

	// Lets add smileys
	*plText = List_Create( 10, 10 );

	do
	{
		// Add text
		if (sp.startChar-last_pos > 0)
		{
			ClcContactTextPiece *piece = (ClcContactTextPiece *) malloc(sizeof(ClcContactTextPiece));

			piece->type = TEXT_PIECE_TYPE_TEXT;
			piece->start_pos = last_pos ;//sp.str - text;
			piece->len = sp.startChar-last_pos;
			List_Append(*plText, piece);
		}

		// Add smiley
		{
			BITMAP bm;
			ICONINFO icon;
			ClcContactTextPiece *piece = (ClcContactTextPiece *) malloc(sizeof(ClcContactTextPiece));

			piece->type = TEXT_PIECE_TYPE_SMILEY;
			piece->len = sp.size;
			piece->smiley = sp.SmileyIcon;

			if (GetIconInfo(piece->smiley, &icon) && GetObject(icon.hbmColor,sizeof(BITMAP),&bm))
			{
				piece->smiley_width = bm.bmWidth;
				piece->smiley_height = bm.bmHeight;
			}
			else
			{
				piece->smiley_width = 16;
				piece->smiley_height = 16;
			}

			List_Append(*plText, piece);
		}
		/*
		 *	Bokra SmileyAdd Fix:
		 */
		// Get next
		last_pos=sp.startChar+sp.size;
		CallService(MS_SMILEYADD_PARSE, 0, (LPARAM)&sp);
		
	}
	while (sp.size != 0);

	//	// Get next
	//	sp.str += sp.startChar + sp.size;
	//	sp.startChar = 0;
	//	sp.size = 0;
	//	CallService(MS_SMILEYADD_PARSE, 0, (LPARAM)&sp);
	//}
	//while (sp.SmileyIcon != NULL && sp.size != 0);

	// Add rest of text
	if (last_pos < text_size)
	{
		ClcContactTextPiece *piece = (ClcContactTextPiece *) malloc(sizeof(ClcContactTextPiece));

		piece->type = TEXT_PIECE_TYPE_TEXT;
		piece->start_pos = last_pos;
		piece->len = text_size-last_pos;

		List_Append(*plText, piece);
	}
}

// Get the text based on the settings for a especific line
void Cache_GetLineText(struct ClcContact *contact, int type, LPSTR text, int text_size, char *variable_text, BOOL xstatus_has_priority, BOOL show_status_if_no_away)
{
	text[0] = '\0';

	switch(type)
	{
		case TEXT_STATUS:
		{
      BOOL noAwayMsg=FALSE;
      if (contact->status==0)
      {
        contact->status=GetContactCachedStatus(contact->hContact);
      }
			// Hide status text if Offline  /// no offline
			if (contact->status==ID_STATUS_OFFLINE || contact->status==0) noAwayMsg=TRUE;
			// Get XStatusName
			if (!noAwayMsg && xstatus_has_priority && contact->hContact && contact->proto)
			{
				DBVARIANT dbv;
				if (!DBGetContactSetting(contact->hContact, contact->proto, "XStatusName", &dbv)) 
				{
					lstrcpynA(text, dbv.pszVal, text_size);
					DBFreeVariant(&dbv);
				}
			}

			// Get StatusMsg
			if (text[0] == '\0')
			{
        char *tmp = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)(contact->status?contact->status:ID_STATUS_OFFLINE), 0);
				lstrcpynA(text, tmp, text_size);
			}

			// Get XStatusName
			if (!noAwayMsg && !xstatus_has_priority && contact->hContact && contact->proto && text[0] == '\0')
			{
				DBVARIANT dbv;
				if (!DBGetContactSetting(contact->hContact, contact->proto, "XStatusName", &dbv)) 
				{
					lstrcpynA(text, dbv.pszVal, text_size);
					DBFreeVariant(&dbv);
				}
			}

			break;
		}
		case TEXT_NICKNAME:
		{
			if (contact->hContact && contact->proto)
			{
				DBVARIANT dbv;
				if (!DBGetContactSetting(contact->hContact, contact->proto, "Nick", &dbv)) 
				{
					lstrcpynA(text, dbv.pszVal, text_size);
					DBFreeVariant(&dbv);
				}
			}
			break;
		}
		case TEXT_STATUS_MESSAGE:
		{ 
			DBVARIANT dbv;
      BOOL noAwayMsg=FALSE;
      BOOL noXstatus=FALSE;
			// Hide status text if Offline  /// no offline		
      if (contact->status==0)
      {
        contact->status=GetContactCachedStatus(contact->hContact);
      }
      if ((contact->status==ID_STATUS_OFFLINE || contact->status==0) && DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",0)) noAwayMsg=TRUE;
      if (contact->status==ID_STATUS_OFFLINE || contact->status==0) noXstatus=TRUE;
			// Get XStatusMsg
			if (!noAwayMsg&& !noXstatus&& xstatus_has_priority && contact->hContact && contact->proto)
			{
				// Try to get XStatusMsg
				if (!DBGetContactSetting(contact->hContact, contact->proto, "XStatusMsg", &dbv)) 
				{
					//lstrcpynA(text, dbv.pszVal, text_size);
					CopySkipUnPrintableChars(text, dbv.pszVal, text_size-1);
					DBFreeVariant(&dbv);
				}
			}

			// Get StatusMsg
			if (!noAwayMsg&& contact->hContact && text[0] == '\0')
			{
				if (!DBGetContactSetting(contact->hContact, "CList", "StatusMsg", &dbv)) 
				{
					CopySkipUnPrintableChars(text, dbv.pszVal, text_size-1);
					DBFreeVariant(&dbv);
				}
			}

			// Get XStatusMsg
			if (!noAwayMsg&&!noXstatus&& !xstatus_has_priority && contact->hContact && contact->proto && text[0] == '\0')
			{
				// Try to get XStatusMsg
				if (!DBGetContactSetting(contact->hContact, contact->proto, "XStatusMsg", &dbv)) 
				{
					CopySkipUnPrintableChars(text, dbv.pszVal, text_size-1);
					DBFreeVariant(&dbv);
				}
			}
      if (show_status_if_no_away && contact->hContact && text[0] == '\0')
      {
        //get text of status if no away
        Cache_GetLineText(contact, TEXT_STATUS, text, text_size, variable_text, xstatus_has_priority, show_status_if_no_away);
      }

			break;
		}
		case TEXT_TEXT:    //TO DO: Unicode to variables
		{
			if (!ServiceExists(MS_VARS_FORMATSTRING))
			{
				lstrcpynA(text, variable_text, text_size);  //transform needed
			}
			else
			{
				char *tmp = variables_parse(variable_text, contact->szText, contact->hContact);
				lstrcpynA(text, tmp, text_size);
				variables_free(tmp);
			}
			break;
		}
	}
}

void Cache_GetFirstLineText(struct ClcData *dat, struct ClcContact *contact)
{
  TCHAR *ch;
  if (contact->szText) mir_free(contact->szText);
  ch=(TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)contact->hContact,0/*TODO UNICODE*/);
  contact->szText=mir_strdupT(ch);
	
	Cache_ReplaceSmileys(dat, contact, contact->szText, lstrlen(contact->szText)+1, &(contact->plText),
		dat->first_line_draw_smileys);
}

void Cache_GetSecondLineText(struct ClcData *dat, struct ClcContact *contact)
{
  char Text[120-MAXEXTRACOLUMNS]={0};
	Cache_GetLineText(contact, dat->second_line_type, (char*)Text, sizeof(Text), dat->second_line_text,
    dat->second_line_xstatus_has_priority,dat->second_line_show_status_if_no_away);
  if (contact->szSecondLineText) mir_free(contact->szSecondLineText);
  if (Text[0]!='\0')
    contact->szSecondLineText=mir_strdupT((TCHAR*)Text);
  else
    contact->szSecondLineText=NULL;
  Text[120-MAXEXTRACOLUMNS-1]='\0';
	Cache_ReplaceSmileys(dat, contact, contact->szSecondLineText, lstrlen(contact->szSecondLineText), &contact->plSecondLineText, 
    dat->second_line_draw_smileys);

}

void Cache_GetThirdLineText(struct ClcData *dat, struct ClcContact *contact)
{
  TCHAR Text[120-MAXEXTRACOLUMNS]={0};
	Cache_GetLineText(contact, dat->third_line_type,(char*)Text, sizeof(Text), dat->third_line_text,
    dat->third_line_xstatus_has_priority,dat->third_line_show_status_if_no_away);
  if (contact->szThirdLineText) mir_free(contact->szThirdLineText);
  if (Text[0]!='\0')
    contact->szThirdLineText=mir_strdupT((TCHAR*)Text);
  else
    contact->szThirdLineText=NULL;
  Text[120-MAXEXTRACOLUMNS-1]='\0';
	Cache_ReplaceSmileys(dat, contact, contact->szThirdLineText, lstrlen(contact->szThirdLineText), &contact->plThirdLineText, 
		dat->third_line_draw_smileys);
}


// If ExecuteOnAllContactsFuncPtr returns FALSE, stop loop
// Return TRUE if finished, FALSE if was stoped
BOOL ExecuteOnAllContacts(struct ClcData *dat, ExecuteOnAllContactsFuncPtr func, void *param)
{
	return ExecuteOnAllContactsOfGroup(&dat->list, func, param);
}

BOOL ExecuteOnAllContactsOfGroup(struct ClcGroup *group, ExecuteOnAllContactsFuncPtr func, void *param)
{
	int scanIndex, i;

	for(scanIndex = 0 ; scanIndex < group->contactCount ; scanIndex++)
	{
		if (group->contact[scanIndex].type == CLCIT_CONTACT)
		{
			if (!func(&group->contact[scanIndex], FALSE, param))
			{
				return FALSE;
			}

			if (group->contact[scanIndex].SubAllocated > 0)
			{
				for (i = 0 ; i < group->contact[scanIndex].SubAllocated ; i++)
				{
					if (!func(&group->contact[scanIndex].subcontacts[i], TRUE, param))
					{
						return FALSE;
					}
				}
			}
		}
		else if (group->contact[scanIndex].type == CLCIT_GROUP) 
		{
			if (!ExecuteOnAllContactsOfGroup(group->contact[scanIndex].group, func, param))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL ReduceAvatarPosition(struct ClcContact *contact, BOOL subcontact, void *param)
{
	if (contact->avatar_pos >= *((int *)param))
	{
		contact->avatar_pos--;
	}

	return TRUE;
}

int CopySkipUnPrintableChars(char *to, char * buf, DWORD size)
{
  DWORD i;
  BOOL keep=0;
  char * cp=to;
  for (i=0; i<size; i++)
  {
    if (buf[i]==0) break;
    if ((BYTE)buf[i]<32)
    {
      *cp=' ';
      if (!keep) cp++;
      keep=1;
    }
    else
    {     
      keep=0;
      *cp=buf[i];
      cp++;
    } 
  }
  *cp=0;
  return i;
}

void Cache_GetAvatar(struct ClcData *dat, struct ClcContact *contact)
{
	if (dat->use_avatar_service)
	{
		if (dat->avatars_show && !DBGetContactSettingByte(contact->hContact, "CList", "HideContactAvatar", 0))
		{
			contact->avatar_data = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)contact->hContact, 0);

			if (contact->avatar_data == NULL || contact->avatar_data->cbSize != sizeof(struct avatarCacheEntry) 
				|| contact->avatar_data->dwFlags == AVS_BITMAP_EXPIRED)
			{
				contact->avatar_data = NULL;
			}

			if (contact->avatar_data != NULL)
				contact->avatar_data->t_lastAccess = time(NULL);
		}
		else
		{
			contact->avatar_data = NULL;
		}
	}
	else
	{
		int old_pos = contact->avatar_pos;

		contact->avatar_pos = AVATAR_POS_DONT_HAVE;
		if (dat->avatars_show && !DBGetContactSettingByte(contact->hContact, "CList", "HideContactAvatar", 0))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(contact->hContact, "ContactPhoto", "File", &dbv) && dbv.type == DBVT_ASCIIZ)
			{
				HBITMAP hBmp = (HBITMAP) CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)dbv.pszVal);
				if (hBmp != NULL)
				{
					// Make bounds
					BITMAP bm;
					if (GetObject(hBmp,sizeof(BITMAP),&bm))
					{
						// Create data...
						HDC hdc; 
						HBITMAP hDrawBmp;

						// Make bounds -> keep aspect radio
						LONG width_clip;
						LONG height_clip;
						RECT rc = {0};

						// Clipping width and hieght
						width_clip = dat->avatars_size;
						height_clip = dat->avatars_size;

						if (height_clip * bm.bmWidth / bm.bmHeight <= width_clip)
						{
							width_clip = height_clip * bm.bmWidth / bm.bmHeight;
						}
						else
						{
							height_clip = width_clip * bm.bmHeight / bm.bmWidth;					
						}

						// Create objs
						hdc = CreateCompatibleDC(dat->avatar_cache.hdc); 
						hDrawBmp = CreateBitmap32(width_clip, height_clip);
						SelectObject(hdc, hDrawBmp);
						SetBkMode(hdc,TRANSPARENT);
						{
							POINT org;
							GetBrushOrgEx(hdc, &org);
							SetStretchBltMode(hdc, HALFTONE);
							SetBrushOrgEx(hdc, org.x, org.y, NULL);
						}

						rc.right = width_clip - 1;
						rc.bottom = height_clip - 1;

						// Draw bitmap             8//8
						{
							HDC dcMem = CreateCompatibleDC(hdc);
							SelectObject(dcMem, hBmp);

							StretchBlt(hdc, 0, 0, width_clip, height_clip,dcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

              DeleteDC(dcMem);
            }
            {
              RECT rtr={0};
              rtr.right=width_clip+1;
              rtr.bottom=height_clip+1;
              FillRect255Alpha(hdc,&rtr);
            }

            hDrawBmp = GetCurrentObject(hdc, OBJ_BITMAP);
            DeleteDC(hdc);

						// Add to list
						if (old_pos >= 0)
						{
							ImageArray_ChangeImage(&dat->avatar_cache, hDrawBmp, old_pos);
							contact->avatar_pos = old_pos;
						}
						else
						{
							contact->avatar_pos = ImageArray_AddImage(&dat->avatar_cache, hDrawBmp, -1);
						}

						DeleteObject(hDrawBmp);
					} // if (GetObject(hBmp,sizeof(BITMAP),&bm))
					DeleteObject(hBmp);
				} //if (hBmp != NULL)
			}
			DBFreeVariant(&dbv);
		}

		// Remove avatar if needed
		if (old_pos >= 0 && contact->avatar_pos == AVATAR_POS_DONT_HAVE)
		{
			ImageArray_RemoveImage(&dat->avatar_cache, old_pos);

			// Update all itens
			ExecuteOnAllContacts(dat, ReduceAvatarPosition, (void *)&old_pos);
		}
	}
}













/*------------
void Cache_GetStatusMessage(struct ClcContact *contact)
{
	DBVARIANT dbv;

	contact->szStatusMsg[0] = '\0';

	// Get XStatusMsg
	if (contact->hContact && contact->proto)
	{
		// Try to get XStatusMsg
		if (!DBGetContactSetting(contact->hContact, contact->proto, "XStatusMsg", &dbv)) 
		{
			lstrcpynA(contact->szStatusMsg, dbv.pszVal, sizeof(contact->szStatusMsg));
			DBFreeVariant(&dbv);
		}
		// Get StatusMsg
	  if (contact->szStatusMsg[0] == '\0')
	  {
		  if (!DBGetContactSetting(contact->hContact, "CList", "StatusMsg", &dbv)) 
		  {
  			lstrcpynA(contact->szStatusMsg, dbv.pszVal, sizeof(contact->szStatusMsg));
        {
          int i;
          for (i=0; i<sizeof(contact->szStatusMsg); i++)
          {
            if (contact->szStatusMsg[i]==0) break;
            else if ((BYTE)contact->szStatusMsg[i]<32) 
              contact->szStatusMsg[i]=(char)32;
          }
        }
	  		DBFreeVariant(&dbv);
		  }
	  }
  }
}-------------*/
