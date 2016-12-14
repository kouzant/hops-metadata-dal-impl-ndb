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
 * File:   PendingEventTableListener.cpp
 * Author: Mahmoud Ismail<maism@kth.se>, ???, Konstantin Popov <kost@sics.se>
 * 
 */

#include "PendingEventTableListener.h"

//
const WatchTableColIndex _pendingEvents_peCol = (WatchTableColIndex) PE_Col_ID;
//
const WatchTableMsgField _pendingEvents_msgFields[5] = {
  { WatchTable_EXCLUDED,      (WatchTableMsgFieldIndex) -1 },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) PE_Msg_RMNODE_ID },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) PE_Msg_TYPE },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) PE_Msg_STATUS },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) PE_Msg_CONTAINS }
};

//
const ListenerWatchTable pendingEventTable(_pendingEvents_table,
					   _pendingEvents_cols,
					   _pendingEvents_noCols,
					   _pendingEvents_events,
					   _pendingEvents_noEvents,
					   _pendingEvents_peCol,
					   _pendingEvents_msgFields,
					   MSG_PendingEventTable);

// polling timeout;
const unsigned int _pendingEvents_timeout = 10;
