/*
 * Copyright (C) 2017 by DXO
 * $Id: util.c Sun Sep 3 00:53:53 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include <string.h>

void *xmalloc(size_t size) {
  void *ptr;

  ptr = malloc(size);
  if (ptr == NULL) {
    logger_error("out of memory, need %lu bytes", (unsigned long) size);
    exit(1);
  }

  return ptr;
}

unsigned char decodeBase96(unsigned char numInBase96)
{
    unsigned char numInBase10 = numInBase96-32;
    return numInBase10;
}

long getFileToBuffer(char *fileName, char **fileBuf)
{
    long    numbytes;
    fileName = strtok(fileName, "\n");
    logger_info("Filename %s\n",fileName);
    FILE    *infile = fopen(fileName, "r");
   // logger_info("Binary filename is %s",fileName);
    /* quit if the file does not exist */
    if(infile == NULL){
        logger_error("Binary file is not found");
        exit(1);
    }
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);

    fseek(infile, 0L, SEEK_SET);

    if(*fileBuf == 0)
    {
    	*fileBuf = (char*)malloc(numbytes* sizeof(char));
    }
    else
    {
        free(*fileBuf);
        *fileBuf = (char*)malloc(numbytes* sizeof(char));

    }

    size_t read_bytes = fread(*fileBuf, sizeof(char), numbytes, infile);
    fclose(infile);
    return read_bytes;
}

// Convert IPv4 or IPv6 sockaddr to string, DO NOT forget to free the buffer after use!
char *sockaddr_to_str(struct sockaddr_storage *addr)
{
    char *result;
    if (addr->ss_family == AF_INET) { // IPv4
        result = (char *)malloc(INET_ADDRSTRLEN);
        if (!result)
            logger_fatal("malloc() failed!");
        int n = uv_ip4_name((struct sockaddr_in*)addr, result, INET_ADDRSTRLEN);
        if (n) {
            free(result);
            result = NULL;
        }
    } else if (addr->ss_family == AF_INET6) { // IPv4
        result = (char *)malloc(INET6_ADDRSTRLEN);
        if (!result)
            logger_fatal("malloc() failed!");
        int n = uv_ip6_name((struct sockaddr_in6*)addr, result, INET6_ADDRSTRLEN);
        if (n) {
            free(result);
            result = NULL;
        }
    } else {
        result =  NULL;
    }
    return result;
}

