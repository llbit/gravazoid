#include "err.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int main(int argc, char **argv) {
	FILE*		fp;
	size_t		fsize;
	char*		fname;
	char*		name;
	uint8_t*	buf;
	size_t		i;
	size_t		j;

	srand(0);

	if (argc != 3) {
		errx(1, "Usage: %s file name", argv[0]);
	}
	fname = argv[1];
	name = argv[2];

	fp = fopen(fname, "r");
	if (!fp) {
		err(1, "%s", fname);
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	rewind(fp);
	buf = malloc(fsize);
	fread(buf, fsize, 1, fp);
	fclose(fp);

	/*printf("size_t size_of_%s[] = %zd;\n", name, fsize);*/
	printf("uint8_t data_%s[] = {\n", name);
	for (i = 0; i < fsize; ) {
		printf("\t\t");
		for (j = 0; j < 8 && i+j < fsize; ++j) {
			printf("0x%02x, ", buf[i+j]);
		}
		printf("\n");
		i += j;
	}
	printf("};\n");

	free(buf);

	return 0;
}

