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

#ifndef RMNODETABLELISTENER_H
#define RMNODETABLELISTENER_H

/* 
 * File:   GenericTableListener.h
 * Author: ???, Konstantin Popov <kost@sics.se>
 *
 */

#include "GenericTableListener.h"
#include <EventMsg.h>


/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

/// the "ndb event definition" tables are inherited/reused from
/// RMNodeTableTailer.h. See PendingEventTableListener.h for further
/// comments;
extern const string _rmnode_table;
extern const int _rmnode_noCols;
extern const string _rmnode_cols[];
extern const int _rmnode_noEvents;
extern const NdbDictionary::Event::TableEvent _rmnode_events[];

typedef enum {
  RMN_Col_RMNODE_ID = 0,
  RMN_Col_HOST_NAME = 1,
  RMN_Col_COMMAND_PORT = 2,
  RMN_Col_HTTP_PORT = 3,
  RMN_Col_HEALTH_REPORT = 4,
  RMN_Col_LAST_HEALTH_REPORT_TIME = 5,
  RMN_Col_CURRENT_STATE = 6,
  RMN_Col_NODEMANAGER_VERSION = 7,
  RMN_Col_PENDING_EVENT_ID = 8,
} RMNodeTableColIndex;

typedef enum {
  RMN_Msg_RMNODE_ID = 0,
  RMN_Msg_HOST_NAME = 1,
  RMN_Msg_COMMAND_PORT = 2,
  RMN_Msg_HTTP_PORT = 3,
  RMN_Msg_HEALTH_REPORT = 4,
  RMN_Msg_LAST_HEALTH_REPORT_TIME = 5,
  RMN_Msg_CURRENT_STATE = 6,
  RMN_Msg_NODEMANAGER_VERSION = 7
} RMNodeTableMsgIndex;

extern const PendingEventField _rmnode_peCol;

extern const WatchTableMsgField _rmnode_msgFields[];

extern const ListenerWatchTable rmnTable;

extern const unsigned int _rmnode_timeout;

#endif /* RMNODETABLELISTENER_H */
