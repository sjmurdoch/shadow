/**
 * The Shadow Simulator
 *
 * Copyright (c) 2010-2012 Rob Jansen <jansen@cs.umn.edu>
 *
 * This file is part of Shadow.
 *
 * Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHD_HEARTBEAT_H_
#define SHD_HEARTBEAT_H_

#include "shadow.h"

typedef struct _HeartbeatEvent HeartbeatEvent;

HeartbeatEvent* heartbeat_new(Tracker* tracker);
void heartbeat_run(HeartbeatEvent* event, Node* node);
void heartbeat_free(HeartbeatEvent* event);

#endif /* SHD_HEARTBEAT_H_ */
