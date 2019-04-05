#ifndef DQLITE_CONN_H
#define DQLITE_CONN_H

#include <raft/io_uv.h>
#include <stdint.h>
#include <stdio.h>
#include <uv.h>

#include "../include/dqlite.h"

#include "./lib/fsm.h"

#include "error.h"
#include "gateway_.h"
#include "metrics.h"
#include "config.h"
#include "request.h"
#include "logger.h"

/* The size of pre-allocated read buffer for holding the payload of incoming
 * requests. This should generally fit in a single IP packet, given typical MTU
 * sizes, and request payloads usually are short enough to fit here.
 *
 * If the request payload is larger than this amount, memory will be allocated
 * on the heap.
 **/
#define CONN__BUF_SIZE 1024

/* Serve requests from a single connected client. */
struct conn_
{
	/* public */
	struct logger *logger; /* Optional logger implementation */

	/* read-only */
	dqlite__error error; /* Last error occurred, if any */
	uint64_t protocol;   /* Protocol version */

	/* private */
	struct dqlite__metrics *metrics; /* Operational metrics */
	struct config *options;	 /* Connection state machine */
	struct dqlite__fsm fsm;		 /* Connection state machine */
	struct gateway_ gateway;	 /* Client state and request handler */
	struct request request;		 /* Incoming request */
	struct response response;	/* Response for internal failures */

	int fd;		     /* File descriptor of client stream */
	uv_loop_t *loop;     /* UV loop */
	uv_stream_t *stream; /* UV stream handle */
	uv_timer_t alive;    /* Check that the client is still alive */
	uv_buf_t buf;	/* Read buffer */

	uint64_t timestamp; /* Time at which the current request started. */
	int aborting;       /* True if we started to abort the connetion */
	int paused;	 /* True if we have paused reading from the stream */

	struct
	{
		uint64_t preamble[3];    /* Preamble buffer */
		unsigned command;	/* Command code */
		unsigned server_id;      /* Server ID of connecting server */
		uv_buf_t address;	/* Address buffer */
		struct raft *r;		 /* Raft instance */
		raft_io_uv_accept_cb cb; /* Accept callback */
		struct raft_io_uv_transport *transport;
	} raft;
};

/* Initialize a connection object */
void conn__init_(struct conn_ *c,
		 int fd,
		 struct logger *logger,
		 uv_loop_t *loop,
		 struct config *options,
		 struct dqlite__metrics *metrics);

/* Close a connection object, releasing all associated resources. */
void conn__close_(struct conn_ *c);

/* Start reading data from the client and processing requests. */
int conn__start_(struct conn_ *c);

/* Immediately close the connection with the client. */
void conn__abort(struct conn_ *c);

#endif /* DQLITE_CONN_H */