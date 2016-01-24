
/* hiredis adapter based on tinylib  by huangsanyi */

#ifndef __HIREDIS_TINYLIB_H__
#define __HIREDIS_TINYLIB_H__
#include <stdlib.h>
#include <sys/types.h>
#include "../hiredis.h"
#include "../async.h"

#ifdef WINNT
	#include "tinylib/windows/loop.h"
#elif defined(__linux__)
	#include "tinylib/linux/loop.h"
	#include <sys/epoll.h>
	
	#define POLLIN EPOLLIN
	#define POLLOUT EPOLLOUT
#endif

typedef struct redis_events_tinylib {
    redisAsyncContext *context;
	
    loop_t *loop;
  #ifdef WINNT
	SOCKET fd;
  #elif defined(__linux__)
	int fd;
  #endif
    channel_t *channel;
} redis_events_tinylib;

static
void redis_tinylib_onevent(SOCKET fd, short event, void* userdata)
{
	redis_events_tinylib *e = (redis_events_tinylib*)userdata;
	
	if (POLLIN & event)
	{
		redisAsyncHandleRead(e->context);
	}
	
	if (POLLOUT & event)
	{
		redisAsyncHandleWrite(e->context);
	}
	
	return;
}

static void redis_enable_read_tinylib(void *privdata) {
    redis_events_tinylib *e = (redis_events_tinylib*)privdata;
	channel_setevent(e->channel, POLLIN);
	
	return;
}

static void redis_disable_read_tinylib(void *privdata) {
    redis_events_tinylib *e = (redis_events_tinylib*)privdata;
	channel_clearevent(e->channel, POLLIN);
	
	return;
}

static void redis_enable_write_tinylib(void *privdata) {
    redis_events_tinylib *e = (redis_events_tinylib*)privdata;
	channel_setevent(e->channel, POLLOUT);
	
	return;
}

static void redis_disable_write_tinylib(void *privdata) {
    redis_events_tinylib *e = (redis_events_tinylib*)privdata;
	channel_clearevent(e->channel, POLLOUT);
	
	return;
}

static void redis_cleanup_tinylib(void *privdata) {
    redis_events_tinylib *e = (redis_events_tinylib*)privdata;
	channel_detach(e->channel);
	channel_destroy(e->channel);
    free(e);
}

static int redisLibevAttach(loop_t* loop, redisAsyncContext *ac) {
    redisContext *c = &(ac->c);
    redis_events_tinylib *e;

    /* Nothing should be attached when something is already attached */
    if (ac->ev.data != NULL)
        return REDIS_ERR;

    /* Create container for context and r/w events */
    e = (redis_events_tinylib*)malloc(sizeof(*e));
    e->context = ac;

    e->loop = loop;
  #ifdef WINNT
	e->fd = (SOCKET)c->fd;
  #elif defined(__linux__)
	e->fd = c->fd;
  #endif
	e->channel = channel_new(loop, e->fd, redis_tinylib_onevent, e);
	channel_setevent(e->channel, POLLIN | POLLOUT);

    ac->ev.addRead = redis_enable_read_tinylib;
    ac->ev.delRead = redis_disable_read_tinylib;
    ac->ev.addWrite = redis_enable_write_tinylib;
    ac->ev.delWrite = redis_disable_write_tinylib;
    ac->ev.cleanup = redis_cleanup_tinylib;
    ac->ev.data = e;

    return REDIS_OK;
}

#endif
