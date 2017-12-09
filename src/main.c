/*
 * Copyright (C) 2017 by DXO
 * $Id: main.c Sat Sep 2 18:30:04 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <udp_server.h>
#include <unistd.h>
#include "type.h"
#include "parse_config.h"
#include "tcp_server.h"
#include "timer.h"

#define DEFAULT_LOG_LEVEL     LOGGER_LEVEL_INFO

uv_loop_t *loop;
int sv_global_state = 0;

int server_run(const proxy_config_t* cf);

int main(int argc, char **argv)
{
    proxy_config_t *config;

    int err;
    loop = uv_default_loop();

    if (argc != 2)
    {
        logger_info("Use default config file: ./config.conf\n");
        config = get_config("config.conf");
    }
    else
    {
        logger_info("Config file=[%s]\n", argv[1]);
        config = get_config(argv[1]);
    }

    if(!config)
    {
        printf("LOAD_CONFIG_ERR!\n");
        return 1;
    }

    logger_init(NULL, DEFAULT_LOG_LEVEL | LOGGER_COLOR_ON);

    printf("PROXY_STARTED!\n");
    fflush(stdout);

    err = server_run(config);
    if (err) {
      exit(1);
    }
    return 0;
}

int server_run(const proxy_config_t* cf)
{
	server_state_t state;
	state.config = *cf;
	state.loop = loop;
	state.state = ss_proxy;
	sv_global_state = ss_proxy;

	tcp_server_run(&state);
	udp_server_run(&state);
    timer_init(&state);

    if(uv_run(loop, UV_RUN_DEFAULT)){
        abort();
    }

	return 0;
}

