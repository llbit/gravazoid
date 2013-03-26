#include "memfile.h"

FILE* mem_to_file(void* buf, size_t size)
{
#ifdef __WIN32__
	FILE* fp = tmpfile();
	fwrite(buf, size, 1, fp);
	rewind(fp);
	return fp;
#else
	return fmemopen(buf, size, "r");
#endif
}
