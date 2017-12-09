/*
 * parse_config.h
 *
 *  Created on: Sep 17, 2017
 *      Author: duy
 */

#ifndef INCL_PARSE_CONFIG_H_
#define INCL_PARSE_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <time.h>
#include <uv.h>
#include <type.h>

#define MAXBUF 1024

proxy_config_t* get_config(char *filename);
void init_udp_socket(char *info[], struct sockaddr_in *udp_client_addr);
void init_timer(char *info[256], struct tm *timeGetFile);
void init_filter(char *filter[], char *string[256]);

#endif /* INCL_PARSE_CONFIG_H_ */
