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
 * File:   ContainerStatusTableListener.cpp
 * Author: Mahmoud Ismail<maism@kth.se>, ???, Konstantin Popov <kost@sics.se>
 * 
 */

#include "ContainerStatusTableListener.h"

const PendingEventField _containerStatus_peCol = 
  (WatchTableColIndex) CS_Col_PENDING_EVENT_ID;

const WatchTableMsgField _containerStatus_msgFields[8] = {
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) CS_Msg_CONTAINER_ID },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) CS_Msg_RMNODE_ID },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) CS_Msg_TYPE },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) CS_Msg_STATE },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) CS_Msg_DIAGNOSTICS },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) CS_Msg_EXIT_STATUS },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) CS_Msg_UCIID },
  { WatchTable_EXCLUDED,      (WatchTableMsgFieldIndex) -1 }
};

const ListenerWatchTable containerStatusTable(_containerStatus_table,
					      _containerStatus_cols,
					      _containerStatus_noCols,
					      _containerStatus_events,
					      _containerStatus_noEvents,
					      _containerStatus_peCol,
					      _containerStatus_msgFields,
					      MSG_ContainerStatusTableTailer);

const unsigned int _containerStatus_timeout = 10;
