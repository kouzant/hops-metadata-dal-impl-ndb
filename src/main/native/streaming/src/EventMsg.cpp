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
 * File:   EventMsg.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#include <stdlib.h>
#include "EventMsg.cpp"

const size_t EventMsg::outOfMemIdx = EventMsg_OBJ_SIZE - sizeof(EventMsg);

EventMsg::EventMsg()
  : msgType((unsigned short) -1),
    pendingEventId(-1),
    nextContainerMsg((EventMsg*) -1),
    allocIdx((unsigned int) -1)
{
  // verify the sizes;
  assert(sizeof(allocIdx) == 2 && EventMsg_OBJ_SIZE <= UINT16_MAX);
}

//
void* EventMsg::operator new(size_t size)
{
  void *ptr;
  int result = posix_memalign(&ptr, CACHE_LINE, EventMsg_OBJ_SIZE);
  switch (result) {
  case 0: break;
  case EINVAL:
    LOG_ERROR("posix_memalign() wrong alignment\n");
    exit(1);
  case ENOMEM:
    LOG_ERROR("posix_memalign() insufficient memory\n");
    exit(1);
  default:
    LOG_ERROR("posix_memalign() unknown error\n");
    exit(1);
  }
  return (ptr);
}
