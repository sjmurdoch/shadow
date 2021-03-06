/**
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

#include <glib.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "shd-linkprofiler.h"

void mylog(GLogLevelFlags level, const gchar* functionName, gchar* format, ...) {
	va_list variableArguments;
	va_start(variableArguments, format);
	g_logv(G_LOG_DOMAIN, level, format, variableArguments);
	va_end(variableArguments);
}

void myccb(ShadowPluginCallbackFunc callback, gpointer data, guint millisecondsDelay) {
	;
}

gint main(gint argc, gchar *argv[]) {
	LinkProfiler linkprofilerstate;
	memset(&linkprofilerstate, 0, sizeof(LinkProfiler));

	mylog(G_LOG_LEVEL_DEBUG, __FUNCTION__, "Starting linkprofiler program");

	const char* USAGE = "linkprofiler USAGE: 'udp client serverIP', 'udp server', 'udp loopback'\n";

	/* 0 is the plugin name, 1 is the protocol */
	if(argc < 2) {
		mylog(G_LOG_LEVEL_CRITICAL, __FUNCTION__, "%s", USAGE);
		return -1;
	}

	char* protocol = argv[1];

	gboolean isError = TRUE;

	if(g_ascii_strncasecmp(protocol, "udp", 3) == 0)
	{
		linkprofilerstate.protocol = LINKPROFILERP_UDP;
		linkprofilerstate.eudp = linkprofilerudp_new(mylog, myccb, argc - 2, &argv[2]);
		isError = (linkprofilerstate.eudp == NULL) ? TRUE : FALSE;
	}

	if(isError) {
		/* unknown argument for protocol, log usage information through Shadow */
		mylog(G_LOG_LEVEL_CRITICAL, __FUNCTION__, "%s", USAGE);
	}

	LinkProfilerServer* server = linkprofilerstate.eudp ? linkprofilerstate.eudp->server : NULL;
	LinkProfilerClient* client = linkprofilerstate.eudp ? linkprofilerstate.eudp->client : NULL;

	/* do an epoll on the client/server epoll descriptors, so we know when
	 * we can wait on either of them without blocking.
	 */
	gint epolld = 0;
	if((epolld = epoll_create(1)) == -1) {
		mylog(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_create");
		return -1;
	} else {
		if(server) {
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = server->epolld;
			if(epoll_ctl(epolld, EPOLL_CTL_ADD, server->epolld, &ev) == -1) {
				mylog(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
				return -1;
			}
		}
		if(client) {
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = client->epolld;
			if(epoll_ctl(epolld, EPOLL_CTL_ADD, client->epolld, &ev) == -1) {
				mylog(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
				return -1;
			}
		}
	}

	/* main loop - when the client/server epoll fds are ready, activate them */
	while(1) {
		struct epoll_event events[10];
		int nfds = epoll_wait(epolld, events, 10, -1);
		if(nfds == -1) {
			mylog(G_LOG_LEVEL_WARNING, __FUNCTION__, "error in epoll_wait");
		}

		for(int i = 0; i < nfds; i++) {
			if(events[i].events & EPOLLIN) {
				if(linkprofilerstate.eudp) {
					linkprofilerudp_ready(linkprofilerstate.eudp);
				}
			}
		}

		if(client && client->is_done) {
			close(client->socketd);
			if(linkprofilerstate.eudp) {
				linkprofilerudp_free(linkprofilerstate.eudp);
			}
			break;
		}
	}

	return 0;
}
