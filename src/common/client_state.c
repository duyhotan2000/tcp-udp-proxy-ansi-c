/*
 * Copyright (C) 2017 by DXO
 * $Id: client.c Sun Sep 3 00:42:37 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include "type.h"
#include "udp_client.h"
#include "tcp_client.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern int sv_global_state;

void do_next(client_ctx_t *cx)
{
    logger_info("cl_state: %d \n", cx->state);
    int new_state=0;
    logger_info("sv_state: %d \n", sv_global_state);
    int cl_state = cx->state;

    switch(sv_global_state)
    {
		case ss_parse:
			if(cl_state == s_proxy_udp)
				cl_state = s_parse_udp;
			if(cl_state == s_proxy_tcp)
				cl_state = s_parse_tcp;
			break;
		case ss_proxy:
			break;
		default:
			break;
    }

    ASSERT(cl_state != s_dead);
    switch (cl_state)
    {
        case s_parse_udp:
            new_state = parse_to_get_sc(cx);
            break;
        case s_parse_tcp:
        	new_state = tcp_parse_to_get_sc(cx);
        	break;
        case s_proxy_udp:
            new_state = udp_do_proxy(cx);
            break;
        case s_proxy_tcp:
			new_state = tcp_do_proxy(cx);
			break;
        case s_prepare_proxy_tcp:
            new_state = tcp_do_proxy_start(cx);
            break;
        default:
        	logger_info("before die=[%d]\n", cl_state);
            UNREACHABLE();
    }
    cx->state = new_state;

    if (cx->state == s_dead)
    {
    	conn_t* incoming = &cx->incoming;
    	conn_t* outgoing = &cx->outgoing;
		free(incoming->udp_send_req);
		incoming->udp_send_req = 0;
		free(outgoing->udp_send_req);
		outgoing->udp_send_req = 0;

        if (DEBUG_CHECKS)
        {
            memset(cx, -1, sizeof(*cx));
        }
        free(cx);
    }
}

