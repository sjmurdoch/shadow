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

#include "shd-linkprofiler.h"

/* my global structure to hold all variable, node-specific application state.
 * the name must not collide with other loaded modules globals. */
LinkProfiler linkprofilerstate;

void __shadow_plugin_init__(ShadowFunctionTable* shadowlibFuncs) {
	g_assert(shadowlibFuncs);

	/* start out with cleared state */
	memset(&linkprofilerstate, 0, sizeof(LinkProfiler));

	/* save the functions Shadow makes available to us */
	linkprofilerstate.shadowlibFuncs = *shadowlibFuncs;

	/*
	 * we 'register' our function table, telling shadow which of our functions
	 * it can use to notify our plugin
	 */
	gboolean success = linkprofilerstate.shadowlibFuncs.registerPlugin(&linkprofilerplugin_new, &linkprofilerplugin_free, &linkprofilerplugin_ready);

	/* we log through Shadow by using the log function it supplied to us */
	if(success) {
		linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_MESSAGE, __FUNCTION__,
				"successfully registered linkprofiler plug-in state");
	} else {
		linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"error registering linkprofiler plug-in state");
	}
}

void linkprofilerplugin_new(int argc, char* argv[]) {
	linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_DEBUG, __FUNCTION__,
			"linkprofilerplugin_new called");

	const char* USAGE = "linkprofiler USAGE: 'udp client serverIP', 'udp server', 'udp loopback'\n";


	/* 0 is the plugin name, 1 is the protocol */
	if(argc < 2) {
		linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_CRITICAL, __FUNCTION__, "%s", USAGE);
		return;
	}

	char* protocol = argv[1];

	gboolean isError = TRUE;

	/* check for the protocol option and create the correct application state */
	if(g_ascii_strncasecmp(protocol, "udp", 3) == 0)
	{
		linkprofilerstate.protocol = LINKPROFILERP_UDP;
		linkprofilerstate.eudp = linkprofilerudp_new(linkprofilerstate.shadowlibFuncs.log, linkprofilerstate.shadowlibFuncs.createCallback, argc - 2, &argv[2]);
		isError = (linkprofilerstate.eudp == NULL) ? TRUE : FALSE;
	}

	if(isError) {
		/* unknown argument for protocol, log usage information through Shadow */
		linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_CRITICAL, __FUNCTION__, "%s", USAGE);
	}
}

void linkprofilerplugin_free() {
	linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "linkprofilerplugin_free called");

	/* call the correct version depending on what we are running */
	switch(linkprofilerstate.protocol) {
		case LINKPROFILERP_UDP: {
			linkprofilerudp_free(linkprofilerstate.eudp);
			break;
		}

		default: {
			linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_CRITICAL, __FUNCTION__,
					"unknown protocol in linkprofilerplugin_free");
			break;
		}
	}
}

void linkprofilerplugin_ready() {
	linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "linkprofilerplugin_ready called");

	/* call the correct version depending on what we are running */
	switch(linkprofilerstate.protocol) {
		case LINKPROFILERP_UDP: {
			linkprofilerudp_ready(linkprofilerstate.eudp);
			break;
		}

		default: {
			linkprofilerstate.shadowlibFuncs.log(G_LOG_LEVEL_CRITICAL, __FUNCTION__,
					"unknown protocol in linkprofilerplugin_ready");
			break;
		}
	}
}
