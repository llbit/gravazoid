#include "memfile.h"

FILE* to_memfile(const char* buf, size_t size)
{
	FILE* fp = tmpfile();
	fwrite(buf, size, 1, fp);
	rewind(fp);
	return fp;
}
