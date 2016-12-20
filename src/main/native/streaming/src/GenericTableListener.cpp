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
 * File:   GenericTableListener.cpp
 * Author: Mahmoud Ismail<maism@kth.se>, Konstantin Popov <kost@sics.se>
 *
 */

#include "GenericTableListener.h"

using namespace Utils;
using namespace Utils::NdbC;

//
ListenerWatchTable::ListenerWatchTable(const string mTableName,
				       const string* mColumnNames,
				       const int mNoColumns,
				       const NdbDictionary::Event::TableEvent* mWatchEvents,
				       const int mNoEvents,
				       const WatchTableColIndex pendingEventIdCol,
				       const WatchTableMsgField msgFields[],
				       const EventMsgType msgType)
  : mTableName(mTableName),
    mColumnNames(mColumnNames),
    mNoColumns(mNoColumns),
    mWatchEvents(mWatchEvents),
    mNoEvents(mNoEvents),
    pendingEventIdCol(pendingEventIdCol),
    msgFields(msgFields),
    msgType(msgType)
{}

