/*
 * Copyright (C) 2017 by DXO
 * $Id: main.c Sat Sep 2 18:30:04 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "server.h"
#include "config.h"

#include "type.h"

#define DEFAULT_BIND_HOST     "127.0.0.1"
#define DEFAULT_BIND_PORT     8000
#define DEFAULT_LOG_LEVEL     LOGGER_LEVEL_INFO

uv_loop_t *loop;

static void parse_opts(server_config *cf);

int main(int argc, char **argv)
{
    server_config config;
    int err;
    loop = uv_default_loop();

    memset(&config, 0, sizeof(config));
    config.bind_host = DEFAULT_BIND_HOST;
    config.bind_port = DEFAULT_BIND_PORT;
    config.log_level = DEFAULT_LOG_LEVEL;

    parse_opts(&config);

    logger_init(NULL, config.log_level | LOGGER_COLOR_ON);

    server_config udp_config;
    memset(&udp_config, 0, sizeof(udp_config));

    getConfigFromFile(&udp_config);

    udp_proxy_init(&udp_config, loop);

    err = server_run(&config, loop);
    if (err) {
      exit(1);
    }
    return 0;
}

static void parse_opts(server_config *cf)
{

}
