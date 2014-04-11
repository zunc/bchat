/* 
 * File:   server.h
 * Author: khoai
 *
 * Created on April 3, 2014, 2:10 PM
 */

#ifndef SERVER_H
#define	SERVER_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "connmap.h"
#include "handler.h"

#define MAXEVENTS 64

typedef struct {
	int sfd;
	uint16_t port;
	int efd;
	struct epoll_event *events;
	uint32_t ntotalcon;
} server_connection;

static int create_and_bind(uint16_t port) {
	int s;
	struct sockaddr_in serv_addr;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		log_fatal("ERROR opening socket");
	}

	bzero((char *) &serv_addr, sizeof (serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(s, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
		log_fatal("ERROR on binding");
	}
	return s;
}

static int make_socket_non_blocking(int sfd) {
	int flags, s;

	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1) {
		log_err("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s == -1) {
		log_err("fcntl");
		return -1;
	}

	return 0;
}

int conn_new(server_connection* server) {
	int s;
	struct epoll_event event;
	
	while (1) {
		struct sockaddr in_addr;
		socklen_t in_len;
		int infd;

		in_len = sizeof in_addr;
		infd = accept(server->sfd, &in_addr, &in_len);
		
		//log_info("accept: %d", infd);
		if (infd == -1) {
			//						printf("errno=%d, EAGAIN=%d, EWOULDBLOCK=%d\n", errno, EAGAIN, EWOULDBLOCK);
			if ((errno == EAGAIN) ||
					(errno == EWOULDBLOCK)) {
				/* We have processed all incoming
				 * connections. */
//				printf("processed all incoming connections.\n");
				break;
			} else {
				log_warn("accept");
				break;
			}
		}

		/* Make the incoming socket non-blocking and add it to the
		 * list of fds to monitor. */
		s = make_socket_non_blocking(infd);
		if (s == -1) {
			log_fatal("make_socket_non_blocking");
		}
		
		event.data.fd = infd;
		event.events = EPOLLIN | EPOLLET;
		s = epoll_ctl(server->efd, EPOLL_CTL_ADD, infd, &event);
		if (s == -1) {
			log_fatal("epoll_ctl");
		}
		
		// add new connection
		if (conn_callback.on_new_conn) {
			conn_callback.on_new_conn(infd);
		}
	}
}

int conn_close_by_fd(int fd){
	connection *conn = connmap_get(fd);
	if (conn_callback.on_close) {
		conn_callback.on_close(conn);
	}
}

int event_loop(server_connection* server) {
	while (1) {
		int n, i, fd, ret;
		n = epoll_wait(server->efd, server->events, MAXEVENTS, -1);
		for (i = 0; i < n; i++) {
			// debug("events on fd: %d", server->events[i].data.fd);
			fd = server->events[i].data.fd;
			if ((server->events[i].events & EPOLLERR) ||
					(server->events[i].events & EPOLLHUP) ||
					(!(server->events[i].events & EPOLLIN))) {
				conn_close_by_fd(fd);
				continue;
			} else if (server->sfd == fd) {
				conn_new(server);
				// debug("new");
			} else {
				ret = conn_read(connmap_get(fd));
				if (ret < 0) {
					// close
					conn_close_by_fd(fd);
				}
				// debug("read");
			}
		}
	}
}

int server_stop(server_connection* server) {
	free(server->events);
	close(server->sfd);
}

int server_start(uint16_t port) {
	int s;
	struct epoll_event event;
	server_connection* server;

	server = malloc(sizeof (server_connection));
	server->sfd = create_and_bind(port);
	if (server->sfd == -1) {
		log_fatal("create_and_bind");
	}

	s = make_socket_non_blocking(server->sfd);
	if (s == -1) {
		log_fatal("make_socket_non_blocking");
	}

	s = listen(server->sfd, SOMAXCONN);
	if (s == -1) {
		log_fatal("listen");
	}

	server->efd = epoll_create1(0);
	if (server->sfd == -1) {
		log_fatal("epoll_create");
	}

	event.data.fd = server->sfd;
	event.events = EPOLLIN | EPOLLET /* | EPOLLERR | EPOLLHUP | EPOLLIN*/;
	s = epoll_ctl(server->efd, EPOLL_CTL_ADD, server->sfd, &event);
	if (s == -1) {
		log_fatal("epoll_ctl");
	}

	/* Buffer where events are returned */
	server->events = calloc(MAXEVENTS, sizeof event);
	log_info("[+] bchat listening on port: %d", port);

	/* setup callback function */
	set_handler(handler_read, handler_write, handler_close, handler_new_conn);
	
	/* The event loop */
	event_loop(server);
}

#endif	/* SERVER_H */

