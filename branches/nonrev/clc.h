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
#ifndef _CLC_H_
#define _CLC_H_
#include "dblists.h"
#include "image_array.h"


#define HCONTACT_ISGROUP    0x80000000
#define HCONTACT_ISINFO     0xFFFF0000
#define IsHContactGroup(h)  (((unsigned)(h)^HCONTACT_ISGROUP)<(HCONTACT_ISGROUP^HCONTACT_ISINFO))
#define IsHContactInfo(h)   (((unsigned)(h)&HCONTACT_ISINFO)==HCONTACT_ISINFO)
#define IsHContactContact(h) (((unsigned)(h)&HCONTACT_ISGROUP)==0)
#define MAXEXTRACOLUMNS     16
#define MAXSTATUSMSGLEN		256

#define INTM_NAMECHANGED     (WM_USER+10)
#define INTM_ICONCHANGED     (WM_USER+11)
#define INTM_GROUPCHANGED    (WM_USER+12)
#define INTM_GROUPSCHANGED   (WM_USER+13)
#define INTM_CONTACTADDED    (WM_USER+14)
#define INTM_CONTACTDELETED  (WM_USER+15)
#define INTM_HIDDENCHANGED   (WM_USER+16)
#define INTM_INVALIDATE      (WM_USER+17)
#define INTM_APPARENTMODECHANGED (WM_USER+18)
#define INTM_SETINFOTIPHOVERTIME (WM_USER+19)
#define INTM_NOTONLISTCHANGED   (WM_USER+20)
#define INTM_RELOADOPTIONS   (WM_USER+21)
#define INTM_NAMEORDERCHANGED (WM_USER+22)
#define INTM_IDLECHANGED         (WM_USER+23)
#define INTM_SCROLLBARCHANGED (WM_USER+24)
#define INTM_PROTOCHANGED (WM_USER+25)
#define INTM_STATUSMSGCHANGED	(WM_USER+26)
#define INTM_STATUSCHANGED	(WM_USER+27)
#define INTM_AVATARCHANGED	(WM_USER+28)
#define INTM_TIMEZONECHANGED	(WM_USER+29)

#define TIMERID_RENAME         10
#define TIMERID_DRAGAUTOSCROLL 11
#define TIMERID_INFOTIP        13
#define TIMERID_REBUILDAFTER   14
#define TIMERID_DELAYEDRESORTCLC   15
#define TIMERID_SUBEXPAND 21
#define TIMERID_INVALIDATE 22

struct ClcGroup;

#define CONTACTF_ONLINE    1
#define CONTACTF_INVISTO   2
#define CONTACTF_VISTO     4
#define CONTACTF_NOTONLIST 8
#define CONTACTF_CHECKED   16
#define CONTACTF_IDLE      32
//#define CONTACTF_STATUSMSG 64

#define AVATAR_POS_DONT_HAVE -1

#define TEXT_PIECE_TYPE_TEXT   0
#define TEXT_PIECE_TYPE_SMILEY 1
typedef struct 
{
	int type;
	int len;
	union
	{
		struct
		{
			int start_pos;
		};
		struct
		{
			HICON smiley;
			int smiley_width;
			int smiley_height;
		};
	};
} ClcContactTextPiece;


struct ClcContact {
	BYTE type;
	BYTE flags;
	struct ClcContact *subcontacts;
	BYTE SubAllocated;
	BYTE SubExpanded;
	BYTE isSubcontact;
	union {
		struct {
			WORD iImage;
			HANDLE hContact;
			int status;
			BOOL image_is_special;
			union
			{
				int avatar_pos;
				struct avatarCacheEntry *avatar_data;
			};
		};
		struct {
			WORD groupId;
			struct ClcGroup *group;
		};
	};
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	TCHAR *szText;//[120-MAXEXTRACOLUMNS];
	SortedList *plText;							// List of ClcContactTextPiece
	TCHAR *szSecondLineText;//[120-MAXEXTRACOLUMNS];
	SortedList *plSecondLineText;				// List of ClcContactTextPiece
	TCHAR *szThirdLineText;//[120-MAXEXTRACOLUMNS];
	SortedList *plThirdLineText;				// List of ClcContactTextPiece
	char * proto;	// MS_PROTO_GETBASEPROTO
  int iTextMaxSmileyHeight;
  int iThirdLineMaxSmileyHeight;
  int iSecondLineMaxSmileyHeight;
    DWORD timezone;
    DWORD timediff;

	// For hittest
	int pos_indent;
	RECT pos_check;
	RECT pos_avatar;
	RECT pos_icon;
	RECT pos_label;
	RECT pos_contact_time;
	RECT pos_extra[MAXEXTRACOLUMNS];
};

#define GROUP_ALLOCATE_STEP  8
struct ClcGroup {
	int contactCount,allocedCount;
	struct ClcContact *contact;
	int expanded,hideOffline,groupId;
	struct ClcGroup *parent;
	int scanIndex;
	int totalMembers;
};

struct ClcFontInfo {
	HFONT hFont;
	int fontHeight,changed;
	COLORREF colour;
};

typedef struct {
	char *szProto;
	DWORD dwStatus;	
} ClcProtoStatus;

#define DRAGSTAGE_NOTMOVED  0
#define DRAGSTAGE_ACTIVE    1
#define DRAGSTAGEM_STAGE    0x00FF
#define DRAGSTAGEF_MAYBERENAME  0x8000
#define DRAGSTAGEF_OUTSIDE      0x4000


#define ITEM_AVATAR 0
#define ITEM_ICON 1
#define ITEM_TEXT 2
#define ITEM_EXTRA_ICONS 3
#define ITEM_CONTACT_TIME 4
#define NUM_ITEM_TYPE 5

#define TEXT_EMPTY -1
#define TEXT_STATUS 0
#define TEXT_NICKNAME 1
#define TEXT_STATUS_MESSAGE 2
#define TEXT_TEXT 3
#define TEXT_CONTACT_TIME 4

#define TEXT_TEXT_MAX_LENGTH 1024


struct ClcData {
	struct ClcGroup list;
	
	// Row height
	int max_row_height;
	int *row_heights;
	int row_heights_size;
	int row_heights_allocated;

	// Avatar cache
	int use_avatar_service;
	IMAGE_ARRAY_DATA avatar_cache;

	// Row
	int row_min_heigh;
	int row_border;
	BOOL row_variable_height;
	BOOL row_align_left_items_to_left;
	BOOL row_align_right_items_to_right;
	int row_items[NUM_ITEM_TYPE];
	BOOL row_hide_group_icon;
  BYTE row_align_group_mode;

	// Avatar
	BOOL avatars_show;
	BOOL avatars_draw_border;
	COLORREF avatars_border_color;
	BOOL avatars_round_corners;
	BOOL avatars_use_custom_corner_size;
	int avatars_custom_corner_size;
	BOOL avatars_ignore_size_for_row_height;
	BOOL avatars_draw_overlay;
	int avatars_overlay_type;
	int avatars_size;

	// Icon
	BOOL icon_hide_on_avatar;
	BOOL icon_draw_on_avatar_space;
	BOOL icon_ignore_size_for_row_height;

	// Contact time
	BOOL contact_time_show;
	BOOL contact_time_show_only_if_different;
	DWORD local_gmt_diff;
	DWORD local_gmt_diff_dst;

	// Text
	BOOL text_rtl;
	BOOL text_align_right;
	BOOL text_replace_smileys;
	BOOL text_resize_smileys;
	int text_smiley_height;
	BOOL text_use_protocol_smileys;
	BOOL text_ignore_size_for_row_height;

	// First line
	BOOL first_line_draw_smileys;

	// Second line
	BOOL second_line_show;
	int second_line_top_space;
	BOOL second_line_draw_smileys;
	int second_line_type;
	TCHAR second_line_text[TEXT_TEXT_MAX_LENGTH];
	BOOL second_line_xstatus_has_priority;
    BOOL second_line_show_status_if_no_away;
    BOOL second_line_use_name_and_message_for_xstatus;

	// Third line
	BOOL third_line_show;
	int third_line_top_space;
	BOOL third_line_draw_smileys;
	int third_line_type;
	TCHAR third_line_text[TEXT_TEXT_MAX_LENGTH];
	BOOL third_line_xstatus_has_priority;
    BOOL third_line_show_status_if_no_away;
    BOOL third_line_use_name_and_message_for_xstatus;


	int yScroll;
	int selection;
	struct ClcFontInfo fontInfo[FONTID_MAX+1];
    BOOL force_in_dialog;
	int scrollTime;
	HIMAGELIST himlHighlight;
	int groupIndent;
    int subIndent;
	TCHAR szQuickSearch[128];
	int iconXSpace;
	HWND hwndRenameEdit;
	COLORREF bkColour,selBkColour,selTextColour,hotTextColour,quickSearchColour;
	int iDragItem,iInsertionMark;
	int dragStage;
	POINT ptDragStart;
	int dragAutoScrolling;
	int dragAutoScrollHeight;
	int leftMargin;
	int rightMargin;
	int insertionMarkHitHeight;
	HBITMAP hBmpBackground;
    HBITMAP hMenuBackground;
    DWORD MenuBkColor, MenuBkHiColor, MenuTextColor, MenuTextHiColor;
	int backgroundBmpUse,bkChanged,MenuBmpUse ;
	int iHotTrack;
	int gammaCorrection;
	DWORD greyoutFlags;			  //see m_clc.h
	DWORD offlineModes;
	DWORD exStyle;
	POINT ptInfoTip;
	int infoTipTimeout;
	HANDLE hInfoTipItem;
	HIMAGELIST himlExtraColumns;
	int extraColumnsCount;
	int extraColumnSpacing;
	int checkboxSize;
	int showSelAlways;
	int showIdle;
	int noVScrollbar;
	int NeedResort;
	SortedList lCLCContactsCache;
	BYTE HiLightMode;
	BYTE doubleClickExpand;
	int MetaIgnoreEmptyExtra;
    BYTE expandMeta;
    BYTE IsMetaContactsEnabled;
    CRITICAL_SECTION lockitemCS;
	time_t last_tick_time;

};

//clc.c
void ClcOptionsChanged(void);

//clcidents.c
int GetRowsPriorTo(struct ClcGroup *group,struct ClcGroup *subgroup,int contactIndex);
int FindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible, BOOL isSkipSubcontacts );
int GetRowByIndex(struct ClcData *dat,int testindex,struct ClcContact **contact,struct ClcGroup **subgroup);
HANDLE ContactToHItem(struct ClcContact *contact);
HANDLE ContactToItemHandle(struct ClcContact *contact,DWORD *nmFlags);
void ClearRowByIndexCache();

//clcitems.c
struct ClcGroup *AddGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);
void FreeGroup(struct ClcGroup *group);
int AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText);
void RebuildEntireList(HWND hwnd,struct ClcData *dat);
struct ClcGroup *RemoveItemFromGroup(HWND hwnd,struct ClcGroup *group,struct ClcContact *contact,int updateTotalCount);
void DeleteItemFromTree(HWND hwnd,HANDLE hItem);
void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void SortCLC(HWND hwnd,struct ClcData *dat,int useInsertionSort);
int GetGroupContentsCount(struct ClcGroup *group,int visibleOnly);
int GetNewSelection(struct ClcGroup *group,int selection, int direction);
void SaveStateAndRebuildList(HWND hwnd,struct ClcData *dat);

//clcmsgs.c
LRESULT ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);

//clcutils.c
void EnsureVisible(HWND hwnd,struct ClcData *dat,int iItem,int partialOk);
void RecalcScrollBar(HWND hwnd,struct ClcData *dat);
void SetGroupExpand(HWND hwnd,struct ClcData *dat,struct ClcGroup *group,int newState);
void DoSelectionDefaultAction(HWND hwnd,struct ClcData *dat);
int FindRowByText(HWND hwnd,struct ClcData *dat,const TCHAR *text,int prefixOk);
void EndRename(HWND hwnd,struct ClcData *dat,int save);
void DeleteFromContactList(HWND hwnd,struct ClcData *dat);
void BeginRenameSelection(HWND hwnd,struct ClcData *dat);
char *GetGroupCountsText(struct ClcData *dat,struct ClcContact *contact);
int HitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags);
void ScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth);
#define DROPTARGET_OUTSIDE    0
#define DROPTARGET_ONSELF     1
#define DROPTARGET_ONNOTHING  2
#define DROPTARGET_ONGROUP    3
#define DROPTARGET_ONCONTACT  4
#define DROPTARGET_INSERTION  5
#define DROPTARGET_ONMETACONTACT  6
#define DROPTARGET_ONSUBCONTACT  7


int GetDropTargetInformation(HWND hwnd,struct ClcData *dat,POINT pt);
int ClcStatusToPf2(int status);
int IsHiddenMode(struct ClcData *dat,int status);
void HideInfoTip(HWND hwnd,struct ClcData *dat);
void NotifyNewContact(HWND hwnd,HANDLE hContact);
void LoadClcOptions(HWND hwnd,struct ClcData *dat);
void RecalculateGroupCheckboxes(HWND hwnd,struct ClcData *dat);
void SetGroupChildCheckboxes(struct ClcGroup *group,int checked);
void InvalidateItem(HWND hwnd,struct ClcData *dat,int iItem);

//clcpaint.c
void PaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint);

//clcopts.c
int ClcOptInit(WPARAM wParam,LPARAM lParam);
DWORD GetDefaultExStyle(void);
void GetFontSetting(int i,LOGFONTA *lf,COLORREF *colour);

//clistsettings.c
TCHAR* GetContactDisplayNameW( HANDLE hContact, int mode );
char* u2a( wchar_t* src );
wchar_t* a2u( char* src );

//clcfiledrop.c
void InitFileDropping(void);
void FreeFileDropping(void);
void RegisterFileDropping(HWND hwnd);
void UnregisterFileDropping(HWND hwnd);

//groups.c
TCHAR* GetGroupNameTS( int idx, DWORD* pdwFlags );
int RenameGroupT(WPARAM groupID, LPARAM newName);

int GetContactCachedStatus(HANDLE hContact);
char *GetContactCachedProtocol(HANDLE hContact);

#define CLCDEFAULT_ROWHEIGHT     17
#define CLCDEFAULT_EXSTYLE       (CLS_EX_EDITLABELS|CLS_EX_TRACKSELECT|CLS_EX_SHOWGROUPCOUNTS|CLS_EX_HIDECOUNTSWHENEMPTY|CLS_EX_TRACKSELECT|CLS_EX_NOTRANSLUCENTSEL)  //plus CLS_EX_NOSMOOTHSCROLL is got from the system
#define CLCDEFAULT_SCROLLTIME    150
#define CLCDEFAULT_GROUPINDENT   5
#define CLCDEFAULT_BKCOLOUR      GetSysColor(COLOR_3DFACE)
#define CLCDEFAULT_USEBITMAP     0
#define CLCDEFAULT_BKBMPUSE      CLB_STRETCH
#define CLCDEFAULT_OFFLINEMODES  MODEF_OFFLINE
#define CLCDEFAULT_GREYOUTFLAGS  0
#define CLCDEFAULT_FULLGREYOUTFLAGS  (MODEF_OFFLINE|PF2_INVISIBLE|GREYF_UNFOCUS)
#define CLCDEFAULT_SELBKCOLOUR   GetSysColor(COLOR_HIGHLIGHT)
#define CLCDEFAULT_TEXTCOLOUR   GetSysColor(COLOR_WINDOWTEXT)
#define CLCDEFAULT_SELTEXTCOLOUR GetSysColor(COLOR_HIGHLIGHTTEXT)
#define CLCDEFAULT_HOTTEXTCOLOUR (IsWinVer98Plus()?RGB(0,0,255):GetSysColor(COLOR_HOTLIGHT))
#define CLCDEFAULT_QUICKSEARCHCOLOUR RGB(255,255,0)
#define CLCDEFAULT_LEFTMARGIN    0
#define CLCDEFAULT_RIGHTMARGIN   2
#define CLCDEFAULT_GAMMACORRECT  1
#define CLCDEFAULT_SHOWIDLE      0

#define CLUI_SetDrawerService "CLUI/SETDRAWERSERVICE"
typedef struct {
	int cbSize;
	char *PluginName;
	char *Comments;
	char *GetDrawFuncsServiceName;

} DrawerServiceStruct,*pDrawerServiceStruct ;

#define CLUI_EXT_FUNC_PAINTCLC	1

typedef struct {
	int cbSize;
	void (*PaintClc)(HWND,struct ClcData *,HDC,RECT *,int ,ClcProtoStatus *,HIMAGELIST);

} ExternDrawer,*pExternDrawer ;

ExternDrawer SED;

#endif _CLC_H_