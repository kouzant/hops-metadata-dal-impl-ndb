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
 * File:   UpdatedContainerInfoTableListener.cpp
 * Author: Mahmoud Ismail<maism@kth.se>, ???, Konstantin Popov <kost@sics.se>
 * 
 */

#include "UpdatedContainerInfoTableListener.h"

extern const WatchTableColIndex _updatedContainerInfo_peCol = UCI_Col_PENDING_EVENT_ID;

extern const WatchTableMsgField _updatedContainerInfo_msgFields[4] = {
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) UCI_Msg_RMNODE_ID },
  { WatchTable_StringField,   (WatchTableMsgFieldIndex) UCI_Msg_CONTAINER_ID },
  { WatchTable_Int32Field,    (WatchTableMsgFieldIndex) UCI_Msg_UPDATED_CONTAINER_INFO_ID },
  { WatchTable_EXCLUDED,      (WatchTableMsgFieldIndex) -1 }
};

extern const ListenerWatchTable updatedContainerInfoTable(_updatedContainerInfo_table,
							  _updatedContainerInfo_cols,
							  _updatedContainerInfo_noCols,
							  _updatedContainerInfo_events,
							  _updatedContainerInfo_noEvents,
							  _updatedContainerInfo_peCol,
							  _updatedContainerInfo_msgFields,
							  MSG_UpdatedContainerInfoTableTailer);

const unsigned int _updatedContainerInfo_timeout = 10;
