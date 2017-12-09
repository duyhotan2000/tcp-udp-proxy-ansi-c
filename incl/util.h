#include "type.h"

void *xmalloc(size_t size);
unsigned char decodeBase96(unsigned char numInBase96);
long getFileToBuffer(char *fileName, char **fileBuf);
char *sockaddr_to_str(struct sockaddr_storage *addr);

