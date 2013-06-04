/*
 * The Shadow Simulator
 *
 * Copyright (c) 2010-2012 Rob Jansen <jansen@cs.umn.edu>
 * Copyright (c) 2013 Steven J. Murdoch <Steven.Murdoch@cl.cam.ac.uk>
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

#ifndef SHD_LINKPROFILER_H_
#define SHD_LINKPROFILER_H_

#include <glib.h>
#include <shd-library.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>

#define BUFFERSIZE 20000
#define LINKPROFILER_SERVER_PORT 9999
#define MAX_EVENTS 10

/**
 * Note:
 * If a module contains a function named g_module_check_init() it is called
 * automatically when the module is loaded. It is passed the GModule structure
 * and should return NULL on success or a string describing the initialization
 * error. Similarly, if a module contains a function g_module_unload() it is
 * called by GLib right before the module is unloaded.
 *
 * g_module_check_init(GModule* module)
 * g_module_unload(GModule* module)
 *
 * @param module :
 *	 the GModule corresponding to the module which has just been loaded.
 * Returns :
 *   NULL on success, or a string describing the initialization error.
 */

/**
 * Protocol modes this linkprofiler module supports.
 */
enum LinkProfilerProtocol {
	LINKPROFILERP_NONE, LINKPROFILERP_UDP,
};

/**
 *
 */
typedef struct _LinkProfilerClient LinkProfilerClient;
struct _LinkProfilerClient {
	ShadowLogFunc log;
	in_addr_t serverIP;
	gint epolld;
	gint socketd;
	gchar sendBuffer[BUFFERSIZE];
	gchar recvBuffer[BUFFERSIZE];
	gint recv_offset;
	gint sent_msg;
	gint amount_sent;
	gint is_done;
};

/**
 *
 */
typedef struct _LinkProfilerServer LinkProfilerServer;
struct _LinkProfilerServer {
	ShadowLogFunc log;
	gint epolld;
	gint listend;
	gint socketd;
	struct sockaddr_in address;
	gchar linkprofilerBuffer[BUFFERSIZE];
	gint read_offset;
	gint write_offset;
};

/**
 *
 */
typedef struct _LinkProfilerUDP LinkProfilerUDP;
struct _LinkProfilerUDP {
	ShadowLogFunc log;
	LinkProfilerClient* client;
	LinkProfilerServer* server;
};

/**
 *
 */
typedef struct _LinkProfiler LinkProfiler;
struct _LinkProfiler {
	ShadowFunctionTable shadowlibFuncs;
	enum LinkProfilerProtocol protocol;
	LinkProfilerUDP* eudp;
};

void linkprofilerplugin_new(int argc, char* argv[]);
void linkprofilerplugin_free();
void linkprofilerplugin_ready();

LinkProfilerUDP* linkprofilerudp_new(ShadowLogFunc log, int argc, char* argv[]);
void linkprofilerudp_free(LinkProfilerUDP* eudp);
void linkprofilerudp_ready(LinkProfilerUDP* eudp);

#endif /* SHD_LINKPROFILER_H_ */
