#include "memfile.h"

FILE* mem_to_file(void* buf, size_t size)
{
	FILE* fp = tmpfile();
	fwrite(buf, size, 1, fp);
	rewind(fp);
	return fp;
}
