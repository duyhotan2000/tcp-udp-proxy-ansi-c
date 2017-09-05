/*
 * Copyright (C) 2017 by DXO
 * $Id: util.c Sun Sep 3 00:53:53 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include "type.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void *xmalloc(size_t size) {
  void *ptr;

  ptr = malloc(size);
  if (ptr == NULL) {
    logger_error("out of memory, need %lu bytes", (unsigned long) size);
    exit(1);
  }

  return ptr;
}

