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
#ifndef __CACHE_FUNCS_H__
# define __CACHE_FUNCS_H__

#include "clc.h"
#include "commonprototypes.h"

void Cache_GetText(struct ClcData *dat, struct ClcContact *contact);
void Cache_GetFirstLineText(struct ClcData *dat, struct ClcContact *contact);
void Cache_GetSecondLineText(struct ClcData *dat, struct ClcContact *contact);
void Cache_GetThirdLineText(struct ClcData *dat, struct ClcContact *contact);

void Cache_GetAvatar(struct ClcData *dat, struct ClcContact *contact);

void Cache_DestroySmileyList(SortedList* p_list);

void Cache_GetTimezone(struct ClcData *dat, struct ClcContact *contact);


#endif // __CACHE_FUNCS_H__
