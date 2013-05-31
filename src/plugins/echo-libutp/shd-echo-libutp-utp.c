/**
 * The Shadow Simulator
 *
 * Copyright (c) 2010-2012 Rob Jansen <jansen@cs.umn.edu>
 * Copyright (c) 2013 Steven Murdoch <Steven.Murdoch@cl.cam.ac.uk>
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

#include "shd-echo-libutp.h"
#include "utp.h"

static EchoClient* _echoutp_newClient(ShadowLogFunc log, in_addr_t serverIPAddress) {
	g_assert(log);

	/* create the socket and get a socket descriptor */
	gint socketd = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);
	if (socketd == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in socket");
		return NULL;
	}

	/* setup the socket address info, client has outgoing connection to server */
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = serverIPAddress;
	serverAddr.sin_port = htons(ECHO_SERVER_PORT);

	/* connect to server. we cannot block, and expect this to return EINPROGRESS */
	gint result = connect(socketd,(struct sockaddr *)  &serverAddr, sizeof(serverAddr));
	if (result == -1 && errno != EINPROGRESS) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in connect");
		return NULL;
	}

	/* create an epoll so we can wait for IO events */
	gint epolld = epoll_create(1);
	if(epolld == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_create");
		close(socketd);
		return NULL;
	}

	/* setup the events we will watch for */
	struct epoll_event ev;
	ev.events = EPOLLIN|EPOLLOUT;
	ev.data.fd = socketd;

	/* start watching out socket */
	result = epoll_ctl(epolld, EPOLL_CTL_ADD, socketd, &ev);
	if(result == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
		close(epolld);
		close(socketd);
		return NULL;
	}

	/* create our client and store our client socket */
	EchoClient* ec = g_new0(EchoClient, 1);
	ec->socketd = socketd;
	ec->epolld = epolld;
	ec->serverIP = serverIPAddress;
	ec->log = log;
	return ec;
}

static EchoServer* _echoutp_newServer(ShadowLogFunc log, in_addr_t bindIPAddress) {
	g_assert(log);

	/* create the socket and get a socket descriptor */
	gint socketd = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);
	if (socketd == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in socket");
		return NULL;
	}

	/* setup the socket address info, client has outgoing connection to server */
	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_addr.s_addr = bindIPAddress;
	bindAddr.sin_port = htons(ECHO_SERVER_PORT);

	/* bind the socket to the server port */
	gint result = bind(socketd, (struct sockaddr *) &bindAddr, sizeof(bindAddr));
	if (result == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "error in bind");
		return NULL;
	}

	/* set as server socket that will listen for clients */
	result = listen(socketd, 100);
	if (result == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "error in listen");
		return NULL;
	}

	/* create an epoll so we can wait for IO events */
	gint epolld = epoll_create(1);
	if(epolld == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_create");
		close(socketd);
		return NULL;
	}

	/* setup the events we will watch for */
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = socketd;

	/* start watching out socket */
	result = epoll_ctl(epolld, EPOLL_CTL_ADD, socketd, &ev);
	if(result == -1) {
		log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
		close(epolld);
		close(socketd);
		return NULL;
	}

	/* create our server and store our server socket */
	EchoServer* es = g_new0(EchoServer, 1);
	es->listend = socketd;
	es->epolld = epolld;
	es->log = log;
	return es;
}

EchoUTP* echoutp_new(ShadowLogFunc log, int argc, char* argv[]) {
	g_assert(log);

	if(argc < 1) {
		return NULL;
	}

	EchoUTP* eutp = g_new0(EchoUTP, 1);
	eutp->log = log;

	gchar* mode = argv[0];
	gboolean isError = FALSE;

	if(g_ascii_strncasecmp(mode, "client", 6) == 0)
	{
		if(argc < 2) {
			isError = TRUE;
		} else {
			gchar* serverHostName = argv[1];
			struct addrinfo* serverInfo;

			if(getaddrinfo(serverHostName, NULL, NULL, &serverInfo) == -1) {
				log(G_LOG_LEVEL_WARNING, __FUNCTION__, "unable to create client: error in getaddrinfo");
				isError = TRUE;
			} else {
				in_addr_t serverIP = ((struct sockaddr_in*)(serverInfo->ai_addr))->sin_addr.s_addr;
				eutp->client = _echoutp_newClient(log, serverIP);
			}
			freeaddrinfo(serverInfo);
		}
	}
	else if (g_ascii_strncasecmp(mode, "server", 6) == 0)
	{
		char myHostName[128];

		gint result = gethostname(myHostName, 128);
		if(result == 0) {
			struct addrinfo* myInfo;

			if(getaddrinfo(myHostName, NULL, NULL, &myInfo) == -1) {
				log(G_LOG_LEVEL_WARNING, __FUNCTION__, "unable to create server: error in getaddrinfo");
				isError = TRUE;
			} else {
				in_addr_t myIP = ((struct sockaddr_in*)(myInfo->ai_addr))->sin_addr.s_addr;
				log(G_LOG_LEVEL_INFO, __FUNCTION__, "binding to %s", inet_ntoa((struct in_addr){myIP}));
				eutp->server = _echoutp_newServer(log, myIP);
			}
			freeaddrinfo(myInfo);
		} else {
			log(G_LOG_LEVEL_WARNING, __FUNCTION__, "unable to create server: error in gethostname");
			isError = TRUE;
		}
	}
	else if (g_ascii_strncasecmp(mode, "loopback", 8) == 0)
	{
		in_addr_t serverIP = htonl(INADDR_LOOPBACK);
		eutp->server = _echoutp_newServer(log, serverIP);
		eutp->client = _echoutp_newClient(log, serverIP);
	}
	else {
		isError = TRUE;
	}

	if(isError) {
		g_free(eutp);
		return NULL;
	}

	return eutp;
}

void echoutp_free(EchoUTP* eutp) {
	g_assert(eutp);

	if(eutp->client) {
		close(eutp->client->epolld);
		g_free(eutp->client);
	}

	if(eutp->server) {
		close(eutp->server->epolld);
		g_free(eutp->server);
	}

	g_free(eutp);
}

static void _echoutp_clientReadable(EchoClient* ec, gint socketd) {
	ec->log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "trying to read socket %i", socketd);

	if(!ec->is_done) {
		ssize_t b = 0;
		while(ec->amount_sent-ec->recv_offset > 0 &&
				(b = recv(socketd, ec->recvBuffer+ec->recv_offset, ec->amount_sent-ec->recv_offset, 0)) > 0) {
			ec->log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "client socket %i read %i bytes: '%s'", socketd, b, ec->recvBuffer+ec->recv_offset);
			ec->recv_offset += b;
		}

		if(ec->recv_offset >= ec->amount_sent) {
			ec->is_done = 1;
			if(memcmp(ec->sendBuffer, ec->recvBuffer, ec->amount_sent)) {
				ec->log(G_LOG_LEVEL_MESSAGE, __FUNCTION__, "inconsistent utp echo received!");
			} else {
				ec->log(G_LOG_LEVEL_MESSAGE, __FUNCTION__, "consistent utp echo received!");
			}

			if(epoll_ctl(ec->epolld, EPOLL_CTL_DEL, socketd, NULL) == -1) {
				ec->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
			}

			close(socketd);
		} else {
			ec->log(G_LOG_LEVEL_INFO, __FUNCTION__, "echo progress: %i of %i bytes", ec->recv_offset, ec->amount_sent);
		}
	}
}

static void _echoutp_serverReadable(EchoServer* es, gint socketd) {
	es->log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "trying to read socket %i", socketd);

	if(socketd == es->listend) {
		/* need to accept a connection on server listening socket,
		 * dont care about address of connector.
		 * this gives us a new socket thats connected to the client */
		gint acceptedDescriptor = 0;
		if((acceptedDescriptor = accept(es->listend, NULL, NULL)) == -1) {
			es->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "error accepting socket");
			return;
		}
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = acceptedDescriptor;
		if(epoll_ctl(es->epolld, EPOLL_CTL_ADD, acceptedDescriptor, &ev) == -1) {
			es->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
		}
	} else {
		/* read all data available */
		gint read_size = BUFFERSIZE - es->read_offset;
		if(read_size > 0) {
		    ssize_t bread = recv(socketd, es->echoBuffer + es->read_offset, read_size, 0);

			/* if we read, start listening for when we can write */
			if(bread == 0) {
				close(es->listend);
				close(socketd);
			} else if(bread > 0) {
				es->log(G_LOG_LEVEL_INFO, __FUNCTION__, "server socket %i read %i bytes", socketd, (gint)bread);
				es->read_offset += bread;
				read_size -= bread;

				struct epoll_event ev;
				ev.events = EPOLLIN|EPOLLOUT;
				ev.data.fd = socketd;
				if(epoll_ctl(es->epolld, EPOLL_CTL_MOD, socketd, &ev) == -1) {
					es->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
				}
			}
		}
	}
}

/* fills buffer with size random characters */
static void _echoutp_fillCharBuffer(gchar* buffer, gint size) {
	for(gint i = 0; i < size; i++) {
		gint n = rand() % 26;
		buffer[i] = 'a' + n;
	}
}

static void _echoutp_clientWritable(EchoClient* ec, gint socketd) {
	if(!ec->sent_msg) {
		ec->log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "trying to write to socket %i", socketd);

		_echoutp_fillCharBuffer(ec->sendBuffer, sizeof(ec->sendBuffer)-1);

		ssize_t b = send(socketd, ec->sendBuffer, sizeof(ec->sendBuffer), 0);
		ec->sent_msg = 1;
		ec->amount_sent += b;
		ec->log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "client socket %i wrote %i bytes: '%s'", socketd, b, ec->sendBuffer);

		if(ec->amount_sent >= sizeof(ec->sendBuffer)) {
			/* we sent everything, so stop trying to write */
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = socketd;
			if(epoll_ctl(ec->epolld, EPOLL_CTL_MOD, socketd, &ev) == -1) {
				ec->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
			}
		}
	}
}

static void _echoutp_serverWritable(EchoServer* es, gint socketd) {
	es->log(G_LOG_LEVEL_DEBUG, __FUNCTION__, "trying to write socket %i", socketd);

	/* echo it back to the client on the same sd,
	 * also taking care of data that is still hanging around from previous reads. */
	gint write_size = es->read_offset - es->write_offset;
	if(write_size > 0) {
		ssize_t bwrote = send(socketd, es->echoBuffer + es->write_offset, write_size, 0);
		if(bwrote == 0) {
			if(epoll_ctl(es->epolld, EPOLL_CTL_DEL, socketd, NULL) == -1) {
				es->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
			}
		} else if(bwrote > 0) {
			es->log(G_LOG_LEVEL_INFO, __FUNCTION__, "server socket %i wrote %i bytes", socketd, (gint)bwrote);
			es->write_offset += bwrote;
			write_size -= bwrote;
		}
	}

	if(write_size == 0) {
		/* stop trying to write */
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = socketd;
		if(epoll_ctl(es->epolld, EPOLL_CTL_MOD, socketd, &ev) == -1) {
			es->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "Error in epoll_ctl");
		}
	}
}

void echoutp_ready(EchoUTP* eutp) {
	g_assert(eutp);

	if(eutp->client) {
		struct epoll_event events[MAX_EVENTS];

		int nfds = epoll_wait(eutp->client->epolld, events, MAX_EVENTS, 0);
		if(nfds == -1) {
			eutp->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "error in epoll_wait");
		}

		for(int i = 0; i < nfds; i++) {
			if(events[i].events & EPOLLIN) {
				_echoutp_clientReadable(eutp->client, events[i].data.fd);
			}
			if(!eutp->client->is_done && (events[i].events & EPOLLOUT)) {
				_echoutp_clientWritable(eutp->client, events[i].data.fd);
			}
		}
	}

	if(eutp->server) {
		struct epoll_event events[MAX_EVENTS];

		int nfds = epoll_wait(eutp->server->epolld, events, MAX_EVENTS, 0);
		if(nfds == -1) {
			eutp->log(G_LOG_LEVEL_WARNING, __FUNCTION__, "error in epoll_wait");
		}

		for(int i = 0; i < nfds; i++) {
			if(events[i].events & EPOLLIN) {
				_echoutp_serverReadable(eutp->server, events[i].data.fd);
			}
			if(events[i].events & EPOLLOUT) {
				_echoutp_serverWritable(eutp->server, events[i].data.fd);
			}
		}

		if(eutp->server->read_offset == eutp->server->write_offset) {
			eutp->server->read_offset = 0;
			eutp->server->write_offset = 0;
		}

		/* cant close sockd to client if we havent received everything yet.
		 * keep it simple and just keep the socket open for now.
		 */
	}
}
