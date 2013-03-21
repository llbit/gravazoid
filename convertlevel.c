#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int main(int argc, char **argv) {
	FILE *f;
	int nblock, btype, x, z;
	char *fname, *levelname;
	char buf[128];
	int i;

	srand(0);

	if(argc != 2) errx(1, "Usage: %s levelfile", argv[0]);
	fname = argv[1];

	levelname = fname;
	while(index(levelname, '/')) levelname = index(levelname, '/') + 1;

	f = fopen(fname, "r");
	if(!f) err(1, "%s", fname);

	if(!fgets(buf, sizeof(buf), f)) err(1, "%s: Empty file?", fname);
	if(1 != sscanf(buf, "%d", &nblock)) errx(1, "%s: No block count found", fname);
	if(nblock < 1 || nblock > 255) errx(1, "%s: Invalid block count", fname);

	printf("uint8_t data_%s[] = {\n", levelname);
	printf("\t\t0x%02x,\n", nblock);
	for(i = 0; i < nblock; i++) {
		if(!fgets(buf, sizeof(buf), f)) err(1, "%s: Unexpected EOF", fname);
		if(3 == sscanf(buf, "%d , %d , %d", &btype, &x, &z)) {
			printf("\t\t0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
				btype,
				rand() % 6,
				x,
				z);
		} else errx(1, "%s: Couldn't parse line", fname);
	}
	printf("};\n");

	return 0;
}
