/*
 * Copyright (C) 2016 Hops.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* 
 * File:   DatasetTableTailer.cpp
 * Author: Mahmoud Ismail<maism@kth.se>
 * 
 */

#include "ResourceTableListener.h"

const PendingEventField _resource_peCol = Res_Col_PENDING_EVENT_ID;

const WatchTableMsgField _resource_msgFields[4] = {
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) Res_Msg_ID },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) Res_Msg_MEMORY },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) Res_Msg_VIRTUALCORES },
  { WatchTable_EXCLUDED,      (WatchTableMsgFieldIndex) -1 },
};

const ListenerWatchTable resourceTable(_resource_table,
				       _resource_cols,
				       _resource_noCols,
				       _resource_events,
				       _resource_noEvents,
				       _resource_peCol,
				       _resource_msgFields,
				       MSG_ResourceTableTailer);

const unsigned int _resource_timeout = 10;


