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
 * Author: Mahmoud Ismail<maism@kth.se>, ???, Konstantin Popov <kost@sics.se>
 * 
 */

#include "PendingEventTableListener.h"

const PendingEventField _rmnode_peCol = (WatchTableColIndex) RMN_Col_PENDING_EVENT_ID;

const WatchTableMsgField _rmnode_msgFields[8] = {
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) RMN_Msg_RMNODE_ID },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) RMN_Msg_HOST_NAME },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) RMN_Msg_COMMAND_PORT },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) RMN_Msg_HTTP_PORT },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) RMN_Msg_HEALTH_REPORT },
  { WatchTable_Int64Field,    (WatchTableMsgFieldIndex) RMN_Msg_LAST_HEALTH_REPORT_TIME },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) RMN_Msg_CURRENT_STATE },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) RMN_Msg_NODEMANAGER_VERSION },
  { WatchTable_EXCLUDED,      (WatchTableMsgFieldIndex) -1 },
};

const ListenerWatchTable rmnTable(_rmnode_table,
				  _rmnode_cols;
				  _rmnode_noCols,
				  _rmnode_events,
				  _rmnode_noEvents,
				  _rmnode_peCol,
				  _rmnode_msgFields,
				  MSG_RMNodeTable);

const unsigned int _rmnode_timeout = 10;
