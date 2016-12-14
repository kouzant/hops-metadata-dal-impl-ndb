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
 * File:   DataflowController.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#include "DataflowController.h"

DataflowController::DataflowController()
  : consumerToBlock(false)
{
  //
  if (pthread_cond_init(&sleepC, NULL) < 0) {
    perror("pthread_cond_init");
    exit(1);
  }
  if (pthread_mutex_init(&sleepM, NULL) < 0) {
    perror("pthread_mutex_init");
    exit(1);
  }
}

DataflowController::~DataflowController()
{}
