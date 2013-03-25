#ifndef ARK_MEMFILE_H
#define ARK_MEMFILE_H

#include <stdio.h>
#include <stddef.h>

FILE* mem_to_file(void* buf, size_t size);

#endif

