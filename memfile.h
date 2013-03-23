#ifndef ARK_MEMFILE_H
#define ARK_MEMFILE_H

#include <stdio.h>
#include <stddef.h>

FILE* to_memfile(const char* buf, size_t size);

#endif

